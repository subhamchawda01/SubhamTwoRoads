/**
    \file dvccode/CDef/file_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_FILE_UTILS_H
#define BASE_CDEF_FILE_UTILS_H

#include <strings.h>

#include <sys/time.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

namespace HFSAT {

/// Set of simple functional forms to unix file status checking
/// like if the file exists, is readable or writeabel by this user
namespace FileUtils {

bool exists(const std::string& r_filename_);
bool readable(const std::string& r_filename_);
bool writeable(const std::string& r_filename_);
bool readwriteable(const std::string& r_filename_);
bool ExistsAndReadable(const std::string& r_filename_);
bool ExistsWithSize(const std::string& r_filename_, int& size);
time_t lastModified(const std::string& r_filename_);
time_t idleTime(const std::string& r_filename_);

inline bool IsFile(const std::string& r_filename_) {
  struct stat st;
  return (stat(r_filename_.c_str(), &st) == 0);
}

/// Given a filename, makes sure that the directory exists.
/// Creates it if not
void MkdirEnclosing(const std::string& r_filename_);

std::string AppendHome(const std::string& r_filepath_);

void GetFileNames(std::string dir, std::vector<std::string>& file_names);

void GetFilePaths(const std::string& current_directory_path_, std::vector<std::string>& rw_file_path_vec_);

unsigned long int GetAvailableSpace(const std::string& r_filename_);
unsigned long int GetAvailableSpaceGivenDir(const std::string& r_filename_);
}
}

#endif  // BASE_CDEF_FILE_UTILS_H
