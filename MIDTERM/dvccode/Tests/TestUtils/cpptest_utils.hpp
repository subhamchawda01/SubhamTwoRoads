/**
  \file basetrade/Tests/dvctrade/MarketAdapter/cpptest_utils.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#pragma once

#include <iostream>
#include <string>
#include <string.h>

namespace HFTEST {

/**
 * Gets full path for data stored in basetrade/Tests/data
 * The path can be changed by use of environment variables, (used in Jenkins)
 * @param test_data_base_path
 * @param subdir
 * @return
 */
inline std::string GetTestDataFullPath(const std::string& test_data_base_path, const std::string repo,
                                       const std::string subdir = "") {
  // get the default directory

  std::string work_dir_str = std::string("/home/dvctrader/") + repo;

  // Temporary env variable set for jenkins

  const char* work_dir = getenv("WORKDIR");
  if (work_dir != nullptr) {
    work_dir_str = std::string(work_dir);
  } else {
    // If env is not set then use home
    const char* home_dir = getenv("HOME");
    if (home_dir != nullptr) {
      work_dir_str = std::string(home_dir) + "/" + repo;
    }
  }
  std::string exchange_filename_ = work_dir_str + "/Tests/data/" + test_data_base_path;
  if (!subdir.empty()) {
    exchange_filename_ = work_dir_str + "/Tests/data/" + subdir + "/" + test_data_base_path;
  }
  return exchange_filename_;
}

/**
 *
 * @return
 */
inline bool IsLoggingEnabled() {
  const char* env = getenv("CPPUNIT_LOGGING");
  bool logging_enabled = false;
  if (env != nullptr) logging_enabled = (strncmp(getenv("CPPUNIT_LOGGING"), "ENABLED", strlen("ENABLED")) == 0);

  return logging_enabled;
}
}
