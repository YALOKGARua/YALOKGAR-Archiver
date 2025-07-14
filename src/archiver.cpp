#include "archiver.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <zstd.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace fs = std::filesystem;

namespace {
struct FileEntry {
  std::string relative_path;
  uint64_t original_size;
  uint64_t compressed_size;
  uint64_t offset;
};

void write_uint64(std::ofstream& out, uint64_t value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void write_string(std::ofstream& out, const std::string& str) {
  uint64_t len = str.size();
  write_uint64(out, len);
  out.write(str.data(), len);
}

void print_progress(size_t current, size_t total) {
  int percent = int(100.0 * current / total);
  std::cout << "\rProgress: " << percent << "%" << std::flush;
  if (current == total) std::cout << std::endl;
}

bool aes_encrypt(const std::vector<char>& in, std::vector<char>& out, const std::string& password, std::vector<unsigned char>& iv) {
  out.clear();
  iv.resize(16);
  if (!RAND_bytes(iv.data(), 16)) return false;
  unsigned char key[32];
  if (!PKCS5_PBKDF2_HMAC_SHA1(password.c_str(), password.size(), iv.data(), 16, 10000, 32, key)) return false;
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv.data()) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  out.resize(in.size() + 32);
  int outlen1 = 0, outlen2 = 0;
  if (EVP_EncryptUpdate(ctx, (unsigned char*)out.data(), &outlen1, (const unsigned char*)in.data(), in.size()) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  if (EVP_EncryptFinal_ex(ctx, (unsigned char*)out.data() + outlen1, &outlen2) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  out.resize(outlen1 + outlen2);
  EVP_CIPHER_CTX_free(ctx);
  return true;
}

bool aes_decrypt(const std::vector<char>& in, std::vector<char>& out, const std::string& password, const std::vector<unsigned char>& iv) {
  out.clear();
  unsigned char key[32];
  if (!PKCS5_PBKDF2_HMAC_SHA1(password.c_str(), password.size(), iv.data(), 16, 10000, 32, key)) return false;
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv.data()) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  out.resize(in.size());
  int outlen1 = 0, outlen2 = 0;
  if (EVP_DecryptUpdate(ctx, (unsigned char*)out.data(), &outlen1, (const unsigned char*)in.data(), in.size()) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  if (EVP_DecryptFinal_ex(ctx, (unsigned char*)out.data() + outlen1, &outlen2) != 1) { EVP_CIPHER_CTX_free(ctx); return false; }
  out.resize(outlen1 + outlen2);
  EVP_CIPHER_CTX_free(ctx);
  return true;
}
}

bool pack_folder(const std::string& folder, const std::string& archive, const std::string& password) {
  std::vector<fs::path> file_paths;
  for (auto& p : fs::recursive_directory_iterator(folder)) {
    if (p.is_regular_file()) file_paths.push_back(p.path());
  }
  size_t n = file_paths.size();
  std::vector<FileEntry> files(n);
  std::vector<std::vector<char>> compressed_data(n);
  std::atomic<uint64_t> offset(0);
  std::mutex offset_mutex;
  std::atomic<size_t> progress(0);
  auto worker = [&](size_t i) {
    const auto& path = file_paths[i];
    std::ifstream in(path, std::ios::binary);
    if (!in) return;
    in.seekg(0, std::ios::end);
    uint64_t size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> data(size);
    in.read(data.data(), size);
    size_t max_dst = ZSTD_compressBound(size);
    std::vector<char> cdata(max_dst);
    size_t csize = ZSTD_compress(cdata.data(), max_dst, data.data(), size, 3);
    if (ZSTD_isError(csize)) return;
    cdata.resize(csize);
    std::string rel = fs::relative(path, folder).string();
    uint64_t my_offset;
    {
      std::lock_guard<std::mutex> lock(offset_mutex);
      my_offset = offset;
      offset += csize;
    }
    files[i] = {rel, size, csize, my_offset};
    compressed_data[i] = std::move(cdata);
    print_progress(++progress, n);
  };
  size_t thread_count = std::thread::hardware_concurrency();
  if (thread_count == 0) thread_count = 4;
  std::vector<std::thread> threads;
  size_t batch = (n + thread_count - 1) / thread_count;
  for (size_t t = 0; t < thread_count; ++t) {
    threads.emplace_back([&, t]() {
      for (size_t i = t * batch; i < std::min((t + 1) * batch, n); ++i) {
        worker(i);
      }
    });
  }
  for (auto& th : threads) th.join();
  std::vector<char> archive_data;
  {
    std::ostringstream oss(std::ios::binary);
    oss.write("YAL1", 4);
    write_uint64(oss, files.size());
    for (const auto& f : files) {
      write_string(oss, f.relative_path);
      write_uint64(oss, f.original_size);
      write_uint64(oss, f.compressed_size);
      write_uint64(oss, f.offset);
    }
    for (const auto& c : compressed_data) {
      oss.write(c.data(), c.size());
    }
    std::string s = oss.str();
    archive_data.assign(s.begin(), s.end());
  }
  if (!password.empty()) {
    std::vector<char> encrypted;
    std::vector<unsigned char> iv;
    if (!aes_encrypt(archive_data, encrypted, password, iv)) return false;
    std::ofstream out(archive, std::ios::binary);
    if (!out) return false;
    out.write("YALC", 4);
    out.write((char*)iv.data(), 16);
    out.write(encrypted.data(), encrypted.size());
    return true;
  } else {
    std::ofstream out(archive, std::ios::binary);
    if (!out) return false;
    out.write(archive_data.data(), archive_data.size());
    return true;
  }
}

bool unpack_archive(const std::string& archive, const std::string& folder, const std::string& password) {
  std::ifstream in(archive, std::ios::binary);
  if (!in) return false;
  char magic[4];
  in.read(magic, 4);
  std::vector<char> archive_data;
  if (std::string(magic, 4) == "YALC") {
    std::vector<unsigned char> iv(16);
    in.read((char*)iv.data(), 16);
    std::vector<char> encrypted((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (password.empty()) return false;
    if (!aes_decrypt(encrypted, archive_data, password, iv)) return false;
    in = std::ifstream();
    in.rdbuf()->pubsetbuf(archive_data.data(), archive_data.size());
    in.clear();
    in.seekg(0);
    in.read(magic, 4);
  } else if (std::string(magic, 4) != "YAL1") {
    return false;
  }
  uint64_t count = 0;
  in.read(reinterpret_cast<char*>(&count), sizeof(count));
  struct Entry { std::string path; uint64_t osize, csize, offset; };
  std::vector<Entry> files(count);
  for (auto& f : files) {
    uint64_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    f.path.resize(len);
    in.read(f.path.data(), len);
    in.read(reinterpret_cast<char*>(&f.osize), sizeof(f.osize));
    in.read(reinterpret_cast<char*>(&f.csize), sizeof(f.csize));
    in.read(reinterpret_cast<char*>(&f.offset), sizeof(f.offset));
  }
  std::streampos data_start = in.tellg();
  std::atomic<size_t> progress(0);
  auto worker = [&](size_t i) {
    in.seekg(data_start + std::streamoff(files[i].offset));
    std::vector<char> cdata(files[i].csize);
    in.read(cdata.data(), files[i].csize);
    std::vector<char> data(files[i].osize);
    size_t res = ZSTD_decompress(data.data(), files[i].osize, cdata.data(), files[i].csize);
    if (ZSTD_isError(res) || res != files[i].osize) return;
    fs::path out_path = fs::path(folder) / files[i].path;
    fs::create_directories(out_path.parent_path());
    std::ofstream out(out_path, std::ios::binary);
    if (!out) return;
    out.write(data.data(), files[i].osize);
    print_progress(++progress, files.size());
  };
  size_t thread_count = std::thread::hardware_concurrency();
  if (thread_count == 0) thread_count = 4;
  std::vector<std::thread> threads;
  size_t batch = (files.size() + thread_count - 1) / thread_count;
  for (size_t t = 0; t < thread_count; ++t) {
    threads.emplace_back([&, t]() {
      for (size_t i = t * batch; i < std::min((t + 1) * batch, files.size()); ++i) {
        worker(i);
      }
    });
  }
  for (auto& th : threads) th.join();
  return true;
} 