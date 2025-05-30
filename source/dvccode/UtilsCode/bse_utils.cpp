#include "dvccode/Utils/bse_utils.hpp"

#include <fstream>

namespace BSE_UTILS {

const std::string GetFilePath(const std::string& directory, const std::string& filename) {
  std::string filepath;
  if (directory.substr(directory.size() - 1, 1) == "/") {
    filepath = directory + filename;
  } else {
    filepath = directory + "/" + filename;
  }
  return filepath;
}

bool BackupFile(const std::string& source_filepath, const std::string& dest_filepath) {
  bool backup_success = false;
  std::ifstream infile(source_filepath, std::ifstream::in);
  std::ofstream outfile(dest_filepath, std::ofstream::out);

  if (infile.is_open() && outfile.is_open()) {
    backup_success = true;
    std::string line;
    while (infile.good()) {
      getline(infile, line);
      if (line.empty()) {
        break;
      }
      outfile << line << "\n";
    }
  }
  if (infile.is_open()) {
    infile.close();
  }
  if (outfile.is_open()) {
    outfile.close();
  }
  return backup_success;
}
}
