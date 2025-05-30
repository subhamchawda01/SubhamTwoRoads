/**
    \file CDefCode/file_utils.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>  // for getenv
#include <sys/statvfs.h>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"

namespace HFSAT {
namespace FileUtils {

bool exists(const std::string& r_filename_) { return (access(r_filename_.c_str(), F_OK) == 0); }

bool readable(const std::string& r_filename_) { return (access(r_filename_.c_str(), R_OK) == 0); }

bool writeable(const std::string& r_filename_) { return (access(r_filename_.c_str(), W_OK) == 0); }

bool readwriteable(const std::string& r_filename_) { return (access(r_filename_.c_str(), R_OK | W_OK) == 0); }

time_t lastModified(const std::string& r_filename_) {
  struct stat ref_status;
  stat(r_filename_.c_str(), &ref_status);
  return ref_status.st_mtime;
}

// Time elapsed since the file was last updated
time_t idleTime(const std::string& r_filename_) {
  time_t m_currtime;
  time(&m_currtime);
  return m_currtime - lastModified(r_filename_);
}

bool ExistsAndReadable(const std::string& r_filename_) { return (access(r_filename_.c_str(), F_OK | R_OK) == 0); }

bool ExistsWithSize(const std::string& r_filename_, int& size) {
  std::ifstream f;
  f.open(r_filename_.c_str(), std::ios::in | std::ios::binary);
  f.seekg(0, std::ios::end);
  size = static_cast<int>(f.tellg());
  if (size <= 0) {
    return false;
  } else {
    return true;
  }
}

void MkdirEnclosing(const std::string& r_filename_) {
  char tempstr[1024] = {0};
  boost::filesystem::path r_filename_abs_ = boost::filesystem::absolute(r_filename_.c_str());
  strncpy(tempstr, r_filename_abs_.string().c_str(), 1024);
  char* dirname_ = dirname(tempstr);  // used on temporary string since dirname alters the passed char *

  boost::system::error_code ec;
  try {
    boost::filesystem::create_directories(dirname_, ec);

    if (ec) {
      HFSAT::ExitVerbose(HFSAT::kBoostFileSystemError, ec.message().c_str());
    }
  } catch (boost::filesystem::filesystem_error& exception) {
    std::cerr << exception.what() << "\n";
    HFSAT::ExitVerbose(HFSAT::kBoostFileSystemAllocationStorageFailure);
  }
}

std::string AppendHome(const std::string& r_filepath_) { return std::string(getenv("HOME")) + "/" + r_filepath_; }

void GetFilePaths(const std::string& current_directory_path_, std::vector<std::string>& rw_file_path_vec_) {
  DIR* t_dir_ = opendir(current_directory_path_.c_str());
  /// if directory invalid etc crap
  if (t_dir_ == NULL) {
    return;
  }
  //      chdir ( current_directory_path_.c_str( ) );
  struct dirent* t_entry_;
  std::vector<std::string> t_rec_dirs_;
  while (true) {
    t_entry_ = readdir(t_dir_);
    if (t_entry_ == NULL) {
      break;
    }
    if (t_entry_->d_type == DT_REG) {
      rw_file_path_vec_.push_back(current_directory_path_ + "/" + (std::string)t_entry_->d_name);
    }
    // if file type is unknown, possible when the file is not ready, use lstat to check file type
    if (t_entry_->d_type == DT_UNKNOWN) {
      struct stat sb;
      int retval = stat(t_entry_->d_name, &sb);
      if (retval == 0) {
        if (S_ISREG(sb.st_mode)) {
          rw_file_path_vec_.push_back(current_directory_path_ + "/" + (std::string)t_entry_->d_name);
        }
      }
    }
    if (t_entry_->d_type == DT_DIR && ((t_entry_->d_name)[0] != '.')) {
      t_rec_dirs_.push_back(current_directory_path_ + "/" + (std::string)t_entry_->d_name);
    }
  }
  closedir(t_dir_);

  for (auto i = 0u; i < t_rec_dirs_.size(); i++) {
    GetFilePaths(t_rec_dirs_[i], rw_file_path_vec_);
    //	  chdir ( ".." );
  }
}

void GetFileNames(std::string dir, std::vector<std::string>& file_names) {
  DIR* t_dir_ = opendir(dir.c_str());
  /// if directory invalid etc crap
  if (t_dir_ == NULL) {
    return;
  }
  chdir(dir.c_str());
  struct dirent* t_entry_;
  std::vector<std::string> t_rec_dirs_;
  while (true) {
    t_entry_ = readdir(t_dir_);
    if (t_entry_ == NULL) {
      break;
    }
    if (t_entry_->d_type == DT_REG) {
      file_names.push_back(t_entry_->d_name);
    }
    if (t_entry_->d_type == DT_DIR && ((t_entry_->d_name)[0] != '.')) {
      t_rec_dirs_.push_back(t_entry_->d_name);
    }
    // if file type is unknown, possible when the file is not ready, use lstat to check file type
    if (t_entry_->d_type == DT_UNKNOWN) {
      struct stat sb;
      int retval = stat(t_entry_->d_name, &sb);
      if (retval == 0) {
        if (S_ISREG(sb.st_mode)) {
          file_names.push_back(t_entry_->d_name);
        }
      }
    }
  }
  closedir(t_dir_);

  for (std::vector<std::string>::iterator siter = t_rec_dirs_.begin(); siter != t_rec_dirs_.end(); siter++) {
    GetFileNames((*siter).c_str(), file_names);
    chdir("..");
  }
}

unsigned long int GetAvailableSpace(const std::string& filePath) {
  struct statvfs fiData;

  // if file does not exist, look for directory
  char resolved_path[1024] = {0};
  realpath(filePath.c_str(), resolved_path);
  for (int i = strlen(resolved_path) - 1; i > 0; --i) {
    if (resolved_path[i] == '/') {
      resolved_path[i] = '\0';
      break;
    }
  }
  if ((statvfs(resolved_path, &fiData)) < 0) {
    printf("Failed to stat %s:\n", resolved_path);
    return 0;  // invalid path
  } else {
    return fiData.f_bsize * fiData.f_bfree;
  }
}

unsigned long int GetAvailableSpaceGivenDir(const std::string& filePath) {
  struct statvfs fiData;

  // if file does not exist, look for directory
  char resolved_path[1024] = {0};
  realpath(filePath.c_str(), resolved_path);
  if ((statvfs(resolved_path, &fiData)) < 0) {
    printf("Failed to stat %s:\n", resolved_path);
    return 0;  // invalid path
  } else {
    return fiData.f_bsize * fiData.f_bfree;
  }
}
}
}
