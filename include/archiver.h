#pragma once
#include <string>
bool pack_folder(const std::string& folder, const std::string& archive, const std::string& password);
bool unpack_archive(const std::string& archive, const std::string& folder, const std::string& password); 