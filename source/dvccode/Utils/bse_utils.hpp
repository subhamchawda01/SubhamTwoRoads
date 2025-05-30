#pragma once

#include <string>

namespace BSE_UTILS {

// also defined in CDef/file_utils.hpp
const std::string GetFilePath(const std::string& directory, const std::string& filename);

bool BackupFile(const std::string& source_filepath, const std::string& dest_filepath);
}
