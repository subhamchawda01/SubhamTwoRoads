#pragma once
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>

#include "dvccode/CDef/s3_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/sem_utils.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

//namespace HFSAT {
//
//#define AWS_BUCKET "s3dvc"
//#define AWS_CACHE_PREFIX "/s3_cache"
//#define AWS_FILE_NOT_FOUND_PREFIX "/s3_404"
//#define EC2_CACHE_FETCH_SCRIPT "/home/dvctrader/basetrade_install/AwsScripts/fetch_file_from_ec2_cache.sh"
//#define EC2_CACHE_CLEANER_SCRIPT "/home/dvctrader/basetrade_install/AwsScripts/clean_files_from_ec2_cache.sh"
//#define IS_HS1_PRESENT "/home/dvctrader/.hs1_present"
//
//inline unsigned int hashString(const std::string& s) {
//  unsigned int hash = 0;
//  unsigned int offset = 'a' - 1;
//  for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
//    hash = hash << 1 | (*it - offset);
//  }
//  return hash;
//}
//
//inline void LoadAllDiskPath(std::vector<std::string>& all_disk_path_) {
//  all_disk_path_.push_back("/media/ephemeral0");
//  all_disk_path_.push_back("/media/ephemeral1");
//  all_disk_path_.push_back("/media/ephemeral2");
//  all_disk_path_.push_back("/media/ephemeral3");
//}
//
//inline std::string GetFreeDiskPathToDownload(const std::vector<std::string>& all_disk_path_) {
//  int max_space_index_ = 0;
//  unsigned long int max_space_ = 0;
//
//  for (unsigned int disk_counter_ = 0; disk_counter_ < all_disk_path_.size(); disk_counter_++) {
//    unsigned long int space_left = HFSAT::FileUtils::GetAvailableSpaceGivenDir(all_disk_path_[disk_counter_]);
//
//    if (max_space_ < space_left) {
//      max_space_ = space_left;
//      max_space_index_ = disk_counter_;
//    }
//  }
//  if (max_space_ < 80 * 1024 * 1024 * 1024L) {  // Start cleaning old files is available space is < 80 GB
//    // Start download from EC2 cache
//    system(std::string(std::string(EC2_CACHE_CLEANER_SCRIPT) + " > /dev/null 2>&1 ").c_str());
//  }
//
//  return all_disk_path_[max_space_index_];
//}
//
//inline bool FileExistInAnyDisk404(const std::vector<std::string>& all_disk_path_, std::string path) {
//  for (unsigned int disk_counter_ = 0; disk_counter_ < all_disk_path_.size(); disk_counter_++) {
//    std::string absent_file_path = all_disk_path_[disk_counter_] + AWS_FILE_NOT_FOUND_PREFIX + path;
//
//    // If Any of the Disk has a file in the 404, return
//    if (FileUtils::exists(absent_file_path))
//      return true;  // Do not try to fetch from S3 is a file is found in not-found directory
//  }
//
//  return false;
//}
//
//inline bool FileExistInAnyDiskCache(std::vector<std::string>& all_disk_path_, std::string& path) {
//  for (unsigned int disk_counter_ = 0; disk_counter_ < all_disk_path_.size(); disk_counter_++) {
//    std::string absent_file_path = all_disk_path_[disk_counter_] + AWS_CACHE_PREFIX + path;
//
//    // If Any of the Disk has a file in the 404, return
//    if (FileUtils::exists(absent_file_path))
//      return true;  // Do not try to fetch from S3 is a file is found in not-found directory
//  }
//
//  return false;
//}
//
//inline bool CheckForFileOnAllS3Cache(std::string& file) {
//  std::vector<std::string> all_disk_path_;
//  LoadAllDiskPath(all_disk_path_);
//
//  for (unsigned int disk_counter_ = 0; disk_counter_ < all_disk_path_.size(); disk_counter_++) {
//    std::string new_file_name_ = all_disk_path_[disk_counter_] + AWS_CACHE_PREFIX + file;
//
//    // If Any of the Disk has a file in the 404, return
//    if (FileUtils::exists(new_file_name_)) {
//      file = new_file_name_;
//      return true;
//    }
//  }
//
//  return false;
//}
//
//// Fetches file from EC2 cache (from CC2 workers): only  to be used on C3 workers
//inline void GetEC2File(std::string path) {
//  char hostname[64];
//  hostname[63] = '\0';
//  gethostname(hostname, 63);
//  if (strncmp(hostname, "ip-10-0-1", 9) != 0) return;
//  if (path.substr(0, 4) != "/NAS") {
//    std::cerr << "path must begin with /NAS\n";
//    exit(1);
//  }
//
//  std::vector<std::string> all_disk_path_;
//  LoadAllDiskPath(all_disk_path_);
//
//  if (FileExistInAnyDisk404(all_disk_path_, path)) {
//    return;  // Return if Already Marked as 404 in any disk
//  }
//
//  std::string partialDownload = std::string(GetFreeDiskPathToDownload(all_disk_path_)) + AWS_CACHE_PREFIX +
//                                std::string(path) + std::string(".part");
//  FileUtils::MkdirEnclosing(partialDownload);
//  std::string path2Rename = partialDownload.substr(0, partialDownload.find(".part"));
//
//  // Locking code
//  unsigned int hashOfPath = 43 + (hashString(path) % 20);
//  int semId = semget((key_t)hashOfPath, 1, IPC_CREAT | 0666);
//  bool enable_lock = true;
//  if (semId == -1) {
//    enable_lock = false;
//    std::cerr << " could not create semaphore for key " << hashOfPath << "\n";
//  }
//  struct timespec timeout;
//  timeout.tv_sec = 60;  // 10 mins at max
//  timeout.tv_nsec = 0;
//  if (enable_lock) HFSAT::SemUtils::timed_wait_and_lock(semId, &timeout);
//  if (enable_lock && errno == EAGAIN) {  // Some Process Has Acquired Lock for quite some time
//    std::cerr << " AWS Semaphore Timed Out : Must Release Semaphore Manually : " << semId
//              << " Key : " << ((key_t)hashOfPath) << "\n";
//  }
//
//  // Start download from EC2 cache
//  system(std::string(std::string(EC2_CACHE_FETCH_SCRIPT) + " " + path + " " + partialDownload + " > /dev/null 2>&1 ")
//             .c_str());
//
//  // Rename downloaded file
//  if (0 != rename(partialDownload.c_str(), path2Rename.c_str())) {
//    remove(partialDownload.c_str());
//
//    // The file probably doesn't exist in EC2 cache => has to be downloaded from S3
//    /*std::cerr << "error renaming file , partial download " << partialDownload
//        << " --> Pathname :  " << path2Rename << " Pathname.c_str : "
//        << path2Rename.c_str()  << " Download Name : " << partialDownload
//        << " .part pos : " << partialDownload.find ( ".part" )
//        << " Substr : " << partialDownload.substr( 0, partialDownload.find ( ".part" ) )
//        << " errorno: " << strerror(errno) << "\n" ;*/
//  } else if (!FileExistInAnyDiskCache(all_disk_path_, path)) {
//    std::cerr << "2 Renaming done properly, still file does not exist\n";
//  }
//
//  if (enable_lock) HFSAT::SemUtils::signal_and_unlock(semId);
//  remove(partialDownload.c_str());
//}
//
//inline void GetS3File(std::string path, unsigned int yyyymmdd, std::string& error_string_) {
//  error_string_ = "";
//  if (path.substr(0, 4) != "/NAS") {
//    std::cerr << "path must begin with /NAS\n";
//    exit(1);
//  }
//
//  S3Utils& s3instance = S3Utils::GetUniqueInstance();
//  s3instance.aws_init();
//  int rc = s3instance.aws_read_config("sample");
//  if (rc) {
//    std::cerr << "Could not find a credential in the config file";
//    std::cerr << "Make sure your ~/.awsAuth file is correct";
//    exit(1);
//  }
//
//  std::vector<std::string> all_disk_path_;
//  LoadAllDiskPath(all_disk_path_);
//
//  if (FileExistInAnyDisk404(all_disk_path_, path)) {
//    return;  // Return if Already Marked as 404 in any disk
//  }
//
//  unsigned int date_current = (unsigned int)HFSAT::DateTime::GetCurrentIsoDateLocal();
//
//  if (date_current < yyyymmdd) return;  // we don't have this data on S3 yet. no need to check
//
//  std::string absent_file_path = std::string("/media/ephemeral0") + AWS_FILE_NOT_FOUND_PREFIX + std::string(path);
//  std::string partialDownload = std::string(GetFreeDiskPathToDownload(all_disk_path_)) + AWS_CACHE_PREFIX +
//                                std::string(path) + std::string(".part");
//
//  std::string dir_path_ = partialDownload.substr(0, partialDownload.find_last_of("/"));
//  //    mkdir ( dir_path_.c_str(), S_IRWXU | S_IRWXO | S_IRWXG ) ;
//  FileUtils::MkdirEnclosing(partialDownload);
//
//  s3instance.s3_set_host("s3.amazonaws.com");
//  s3instance.s3_set_bucket(AWS_BUCKET);
//
//  FileUtils::MkdirEnclosing(path);
//  unsigned int hashOfPath = 43 + (hashString(path) % 20);
//  int semId = semget((key_t)hashOfPath, 1, IPC_CREAT | 0666);
//  bool enable_lock = true;
//  if (semId == -1) {
//    enable_lock = false;
//    std::cerr << " could not create semaphore for key " << hashOfPath << "\n";
//  }
//
//  struct timespec timeout;
//  timeout.tv_sec = 60;  // 10 mins at max
//  timeout.tv_nsec = 0;
//
//  if (enable_lock) HFSAT::SemUtils::timed_wait_and_lock(semId, &timeout);
//
//  if (enable_lock && errno == EAGAIN) {  // Some Process Has Acquired Lock for quite some time
//
//    std::cerr << " AWS Semahore Timed Out : Must Release Semaphore Manually : " << semId
//              << " Key : " << ((key_t)hashOfPath) << "\n";
//    //      if ( enable_lock ) HFSAT::SemUtils::signal_and_unlock(semId);
//
//    // Recreate
//    //      if ( enable_lock )  HFSAT::SemUtils::timed_wait_and_lock( semId, &timeout );
//  }
//
//  if (!FileExistInAnyDiskCache(all_disk_path_, path) && !FileExistInAnyDisk404(all_disk_path_, path)) {
//    FILE* downloadFileHandle = fopen(partialDownload.c_str(), "w");
//
//    if (downloadFileHandle != NULL) {
//      int retVal = s3instance.s3_download(&path[1], downloadFileHandle);  // remove leading "/" from path
//      fclose(downloadFileHandle);
//
//      char real_path[1024];
//      realpath(partialDownload.c_str(), real_path);
//
//      std::ostringstream error_oss_;
//
//      if (retVal == 200) {
//        // file found
//        std::string path2Rename = partialDownload.substr(0, partialDownload.find(".part"));
//        FileUtils::MkdirEnclosing(path2Rename.c_str());
//
//        if (0 != rename(real_path, path2Rename.c_str())) {
//          remove(partialDownload.c_str());
//
//          error_oss_ << "1 error renaming file , partial download " << partialDownload
//                     << " --> Pathname :  " << path2Rename << " Pathname.c_str : " << path2Rename.c_str()
//                     << " Download Name : " << partialDownload << " .part pos : " << partialDownload.find(".part")
//                     << " Substr : " << partialDownload.substr(0, partialDownload.find(".part"))
//                     << " Absent Name : " << absent_file_path << " errorno: " << strerror(errno) << "\n";
//          std::cerr << "error renaming file , partial download " << partialDownload
//                    << " --> Pathname :  " << path2Rename << " Pathname.c_str : " << path2Rename.c_str()
//                    << " Download Name : " << partialDownload << " .part pos : " << partialDownload.find(".part")
//                    << " Substr : " << partialDownload.substr(0, partialDownload.find(".part"))
//                    << " Absent Name : " << absent_file_path << " errorno: " << strerror(errno) << "\n";
//          remove(partialDownload.c_str());
//        } else if (!FileExistInAnyDiskCache(all_disk_path_, path)) {
//          error_oss_ << "2 Renaming done properly, still file does not exist\n";
//        }
//      } else {
//        error_oss_ << "3 retval " << retVal << " != 200\n";
//        // file not found
//        remove(partialDownload.c_str());
//        if (int(yyyymmdd) < HFSAT::DateTime::CalcPrevWeekDay(2, date_current)) {
//          // dont create 404 file for today-2 to today
//          FileUtils::MkdirEnclosing(absent_file_path.c_str());
//          std::ofstream absentfilestream(absent_file_path.c_str(), std::ofstream::app | std::ofstream::ate);
//          absentfilestream << retVal << "\n";
//          absentfilestream.close();
//        }
//      }
//      error_string_ = error_oss_.str();
//    }
//  }
//  if (enable_lock) HFSAT::SemUtils::signal_and_unlock(semId);
//  // std::cout <<" release semaphores\n";
//}
//
//inline void PutFile(std::string path) {
//  std::cerr << "Not implemented\n";
//  exit(1);
//  return;
//}
//
//inline void DeleteFile(std::string path) {
//  std::cerr << "Not implemented\n";
//  exit(1);
//  return;
//}
//}
