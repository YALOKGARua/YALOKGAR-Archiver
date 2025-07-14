#include "archiver.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "usage: archiver pack <folder> <archive.yal> [--password <pass>]\n";
    std::cout << "       archiver unpack <archive.yal> <output_folder> [--password <pass>]\n";
    return 1;
  }
  std::string command = argv[1];
  std::string password;
  for (int i = 4; i < argc; ++i) {
    if (std::string(argv[i]) == "--password" && i + 1 < argc) {
      password = argv[i + 1];
      break;
    }
  }
  if (command == "pack") {
    std::string folder = argv[2];
    std::string archive = argv[3];
    return pack_folder(folder, archive, password) ? 0 : 2;
  }
  if (command == "unpack") {
    std::string archive = argv[2];
    std::string folder = argv[3];
    return unpack_archive(archive, folder, password) ? 0 : 3;
  }
  std::cout << "unknown command\n";
  return 1;
} 