/**
   \file Tools/search_exec.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string>
#include <dirent.h>

std::string FindExecPath(const std::vector<std::string>& dir_to_search_, const std::string& exec_name_) {
  struct dirent* directory_entry_ = NULL;
  for (unsigned i = 0; i < dir_to_search_.size(); i++) {
    DIR* dir_ = opendir(dir_to_search_[i].c_str());

    while (dir_) {
      directory_entry_ = readdir(dir_);
      if (!directory_entry_) {
        break;
      }
      if (directory_entry_->d_type == DT_DIR) {
        continue;
      }
      if (exec_name_.compare(directory_entry_->d_name) == 0) {
        return dir_to_search_[i] + directory_entry_->d_name;
      }
    }
  }
  return "";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "usage: <exec_basename>\n";
    return 0;
  }

  std::string exec_name_ = std::string(argv[1]);
  std::string home_ = getenv("HOME");
  std::string user_name = getenv("USER");
  std::vector<std::string> dir_to_search_;
  dir_to_search_.clear();
  dir_to_search_.push_back("/home/dvctrader/LiveExec/");
  dir_to_search_.push_back(home_ + "/basetrade_install/");

  std::vector<std::string> sub_dir_name_;
  sub_dir_name_.clear();
  // std::string path = std::string(home) + std::string(user_name);
  if (exec_name_.find(".pl") != std::string::npos) {
    sub_dir_name_.push_back("scripts/");
    sub_dir_name_.push_back("ModelScripts/");
  } else if (exec_name_.find(".sh") != std::string::npos) {
    sub_dir_name_.push_back("scripts/");
    sub_dir_name_.push_back("ModelScripts/");
  } else if (exec_name_.find(".py") != std::string::npos) {
    sub_dir_name_.push_back("scripts/");
    sub_dir_name_.push_back("ModelScripts/");
  } else {
    sub_dir_name_.push_back("bin/");
  }

  std::vector<std::string> sub_dir_to_search_;
  sub_dir_to_search_.clear();

  for (unsigned j = 0; j < dir_to_search_.size(); j++) {
    for (unsigned i = 0; i < sub_dir_name_.size(); i++) {
      sub_dir_to_search_.push_back(dir_to_search_[j] + sub_dir_name_[i]);
    }
  }

  std::cout << FindExecPath(sub_dir_to_search_, exec_name_);

  return 0;
}
