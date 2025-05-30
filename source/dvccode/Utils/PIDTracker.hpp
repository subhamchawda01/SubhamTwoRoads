/**
   file PIDTracker.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

/* @Info : This Class provides a static method to obtain the pid of any process given the process name.
 *
 *
 *
 * @NOTES :
 *
 *
 * @Date  : October 24, 2011
 *
 * @Remarks on Modifications  :
 *
 * 	 replaced all C type file operations to CPP type.
 * 	 getPid now returns a vector of pids
 *
 */

#ifndef PID_TRACKER_BY_NAME_H_
#define PID_TRACKER_BY_NAME_H_

#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>  // for opendir(), readdir(), closedir()
#include <sys/stat.h>   // for stat()

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fstream>
#include <vector>

#define PROC_DIRECTORY "/proc/"
#define CASE_SENSITIVE true
#define CASE_INSENSITIVE false

class PIDTracker {
 public:
  static bool isNumeric(const char* procSubDir);
  static std::vector<int> getPIDByName(const char* processName);
};

// to find the numeric directories in the /proc
inline bool PIDTracker::isNumeric(const char* procSubDir) {
  for (; *procSubDir; procSubDir++)
    if (*procSubDir < '0' || *procSubDir > '9') return false;

  return true;
}

// does the real work for finding the pid by iterating through the entire /proc directory with a search string
inline std::vector<int> PIDTracker::getPIDByName(const char* processName) {
  struct dirent* de_DirEntity_ = NULL;
  struct dirent* sub_DirEntity_ = NULL;

  DIR* dir_proc_ = NULL;
  DIR* dir_task_ = NULL;

  std::ifstream in_;

  std::string fileName_;
  std::string processName_;
  std::string taskDir_;

  size_t found_;

  std::vector<int> pidList;

  dir_proc_ = opendir(PROC_DIRECTORY);

  if (dir_proc_ == NULL) {
    perror("Couldn't open the " PROC_DIRECTORY " directory");
    return pidList;
  }

  // Loop while not NULL
  while ((de_DirEntity_ = readdir(dir_proc_))) {
    if (de_DirEntity_->d_type == DT_DIR) {
      if (isNumeric(de_DirEntity_->d_name)) {
        fileName_ =
            fileName_ + PROC_DIRECTORY + de_DirEntity_->d_name + "/cmdline";  // this file has process name stored in it

        in_.open(fileName_.c_str(), std::ifstream::in);

        std::getline(in_, processName_);

        found_ = processName_.find(processName);

        taskDir_.clear();

        if (found_ != std::string::npos) {
          taskDir_ = taskDir_ + PROC_DIRECTORY + de_DirEntity_->d_name + "/task/";

          dir_task_ = opendir(taskDir_.c_str());

          if (dir_task_ == NULL) {
            // since we are matching entire string this is most likely due to the process itself
            // which wants to allocate cpu to PID. with one of the arguments as processname
            // and hence it could not open taskdir
            //		perror("Couldn't open the task directory") ;
            //		return pidList;
            in_.close();
            fileName_.clear();
            continue;
          }

          while ((sub_DirEntity_ = readdir(dir_task_))) {
            if (PIDTracker::isNumeric(sub_DirEntity_->d_name)) {
              pidList.push_back(atoi(sub_DirEntity_->d_name));
            }
          }

          closedir(dir_task_);
        }

        in_.close();

        fileName_.clear();
      }
    }
  }

  closedir(dir_proc_);

  return pidList;
}

#endif
