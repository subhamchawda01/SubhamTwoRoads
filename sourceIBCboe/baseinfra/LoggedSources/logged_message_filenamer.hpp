/**
    \file MDSMessages/logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
//#include "dvccode/CDef/s3_calls.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

#define FILE_NOT_FOUND_LOGDIR "/media/ephemeral0/404"
#define PRIMARY_FILE_NOT_FOUND_LOGDIR "/media/ephemeral0/primary_404"

namespace HFSAT {

#define NUM_DISKS 16  // Number of disks across which data is distributed

/// Returns the name of the data file where BMF MDS has logged data
/// Typically it will be of the format /NAS1/data/BMFLoggedData/BMF/2011/05/01/DOLM11_20110501
class LoggedMessageFileNamer {
 private:
  static bool skip_primary_location_;

 private:
  inline static unsigned long get_disk_num(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++) != 0) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return (hash % NUM_DISKS);
  }

  inline static std::string get_physical_path(const char *str) {
    unsigned long disk_num = get_disk_num(str);  // get disk number where this file would be present
    std::stringstream ss;
    ss << "/media/shared/ephemeral" << disk_num << "/s3_cache" << str;
    return ss.str();
  }

  /// Checks if orig file or gz equivalent exists
  inline static bool test(std::string &file, unsigned int yyyymmdd, std::string &error_string_) {
    std::string file_gz = file + ".gz";
    char hostname[64];
    hostname[63] = '\0';
    gethostname(hostname, 63);

    // First check if the file is present in shared folder (for ec2 workers and Jenkins)
    const char *work_dir = getenv("WORKDIR");  // Only true for Jenkins server
    if ((strncmp(hostname, "ip-10-0-1", 9) == 0 && false /*HFSAT::FileUtils::ExistsAndReadable(IS_HS1_PRESENT)  */) ||
        (work_dir != nullptr)) {
      std::string file_hs1_path = get_physical_path(file.c_str());
      if (HFSAT::FileUtils::ExistsAndReadable(file_hs1_path)) {
        file = file_hs1_path;
        return true;
      }
      file_hs1_path = get_physical_path(file_gz.c_str());
      if (HFSAT::FileUtils::ExistsAndReadable(file_hs1_path)) {
        file = file_hs1_path;
        return true;
      }
    }

    // Check for alternative paths now
    if (HFSAT::FileUtils::ExistsAndReadable(file_gz)) {
      file += ".gz";
      return true;
    }
    if (HFSAT::FileUtils::ExistsAndReadable(file)) {
      return true;
    }

    // that's it for NY servers!
    if (strncmp(hostname, "sdv", 3) == 0 || strncmp(hostname, "SDV", 3) == 0) {
      return false;  // for SDV servers
    }

    //    // Check in S3 cache, and download if required
    //    if (CheckForFileOnAllS3Cache(file_gz)) {
    //      file = file_gz;
    //      return true;
    //    }
    //
    //    // FOR EC2 serves and possibly local machines
    //    // Download from S3 only for dvctrader (not for other users)
    //    if (std::string(getenv("HOME")) == "/home/dvctrader") {
    //      HFSAT::GetS3File(file_gz, yyyymmdd,
    //                       error_string_);  // so far we want to avoid wasteful calls. only check for gz file on S3.
    //
    //      if (HFSAT::FileUtils::ExistsAndReadable(file_gz)) {
    //        file += ".gz";
    //        return true;
    //      }
    //
    //      if (CheckForFileOnAllS3Cache(file_gz)) {
    //        file = file_gz;
    //        return true;
    //      }
    //    }

    return false;
  }

  inline static bool IsFileAvailableInCache(std::string &file) {
    file += ".gz";
    return false;
    //    return (HFSAT::CheckForFileOnAllS3Cache(file));
  }

  inline static std::string consturctPath(std::string base_dir, TradingLocation_t &rw_trading_location_,
                                          std::string yyyy, std::string mm, std::string dd,
                                          const char *_exchange_symbol_, std::string _preevent_YYYYMMDD_,
                                          bool is_minute_bar = false) {
    static std::ostringstream t_temp_oss_;
    t_temp_oss_.str("");
    if (!is_minute_bar) {
      t_temp_oss_ << base_dir << TradingLocationUtils::GetTradingLocationName(rw_trading_location_) << "/" << yyyy
                  << "/" << mm << "/" << dd << "/" << _exchange_symbol_ << "_" << _preevent_YYYYMMDD_;
    } else {
      t_temp_oss_ << base_dir << "COMMON/" << yyyy << "/" << mm << "/" << dd << "/MB_" << _exchange_symbol_ << "_"
                  << _preevent_YYYYMMDD_;
    }
    return t_temp_oss_.str();
  }

  static void ReportErrorOnS3(std::string &file_not_found_file_path_, std::string &error_string_) {
    int error_code_;
    if (error_string_.length() == 0) {
      error_code_ = 4;
      error_string_ = "4 EMPTY ERROR STRING\n";
    } else {
      error_code_ = int(error_string_[0] - '0');
    }

    if (!HFSAT::FileUtils::ExistsAndReadable(file_not_found_file_path_)) {
      // create empty file
      FileUtils::MkdirEnclosing(file_not_found_file_path_.c_str());
      std::ofstream outfile(file_not_found_file_path_.c_str());
      outfile.close();
    }

    // now error file exists
    if (error_code_ >= 1 && error_code_ <= 4) {
      bool error_code_already_present_ = false;
      std::ifstream infile_(file_not_found_file_path_.c_str(), std::ifstream::in);
      const int kFileLineBufferLen = 4096;

      char readline_buffer_[kFileLineBufferLen];
      bzero(readline_buffer_, kFileLineBufferLen);

      while (infile_.good()) {
        bzero(readline_buffer_, kFileLineBufferLen);
        infile_.getline(readline_buffer_, kFileLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kFileLineBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() >= 1) {
          if (error_code_ == atoi(tokens_[0])) {
            error_code_already_present_ = true;
            break;
          }
        }
      }
      infile_.close();

      if (!error_code_already_present_) {
        std::ofstream outfile(file_not_found_file_path_.c_str(), std::ofstream::app);
        outfile << error_string_;
        outfile.close();
      }
    }
  }

 public:
  /// Returns the name of the file with binary data of that day.
  /// Searches at the location specified, otherwise in the order specified , searches other locations and
  static std::string GetName(std::string baseLogDir, const char *_exchange_symbol_,
                             const unsigned int _preevent_YYYYMMDD_, TradingLocation_t &rw_trading_location_,
                             const TradingLocation_t primary_location_, bool is_minute_bar = false) {
    std::string _preevent_YYYYMMDD_str_ = "";
    {
      std::stringstream ss;
      ss << _preevent_YYYYMMDD_;
      _preevent_YYYYMMDD_str_ = ss.str();
    }

    const char *t_exchange_symbol_ =
        HFSAT::HybridSecurityManager::IsHybridExch(std::string(_exchange_symbol_)) == false
            ? _exchange_symbol_
            : (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(_exchange_symbol_))).c_str();
    std::string _preevent_YYYY_str_ = _preevent_YYYYMMDD_str_.substr(0, 4);
    std::string _preevent_MM_str_ = _preevent_YYYYMMDD_str_.substr(4, 2);
    std::string _preevent_DD_str_ = _preevent_YYYYMMDD_str_.substr(6, 2);

    std::string retval_error = "";
    {
      std::stringstream ss;
      ss << "NO_FILE_AVAILABLE_" << t_exchange_symbol_ << "_" << _preevent_YYYYMMDD_str_ << "_"
         << TradingLocationUtils::GetTradingLocationName(rw_trading_location_) << "_"
         << TradingLocationUtils::GetTradingLocationName(primary_location_);  ///< error condition signal
      retval_error = ss.str();
    }

    // Only Search For Trading And Primary
    std::vector<TradingLocation_t> all_locs;
    all_locs.push_back(rw_trading_location_);
    // The fileNamer will not fall back & search the primary location in case it doesn't find the file in the trading
    // location
    if (!skip_primary_location_) {
      all_locs.push_back(primary_location_);
    }

    std::string return_value_ = "";

    // First Check For Local Machines and AWS default mount point
    for (int i = 0; i < (int)all_locs.size(); ++i) {  // for location specified
      rw_trading_location_ = all_locs[i];
      std::string retval = consturctPath(baseLogDir, rw_trading_location_, _preevent_YYYY_str_, _preevent_MM_str_,
                                         _preevent_DD_str_, t_exchange_symbol_, _preevent_YYYYMMDD_str_, is_minute_bar);

      std::string error_string_;
      if (test(retval, _preevent_YYYYMMDD_, error_string_)) {
        return retval;
      }

      char hostname[64];
      hostname[63] = '\0';
      gethostname(hostname, 63);
      unsigned int date_current = (unsigned int)HFSAT::DateTime::GetCurrentIsoDateLocal();
      if (strncmp(hostname, "ip-10-0-", 8) == 0 &&
          int(_preevent_YYYYMMDD_) >= HFSAT::DateTime::CalcPrevWeekDay(2, date_current)) {
        std::string file_not_found_file_path_ = "";
        if (i == 0) {
          file_not_found_file_path_ = FILE_NOT_FOUND_LOGDIR + retval;
        } else {
          file_not_found_file_path_ = PRIMARY_FILE_NOT_FOUND_LOGDIR + retval;
        }
        // ReportErrorOnS3(file_not_found_file_path_, error_string_);
      }
    }

    // Now check only AWS machine, by now files would have been copied to aws proper path
    std::vector<std::string> all_aws_disk_vec_;
    // LoadAllDiskPath(all_aws_disk_vec_);

    for (int i = 0; i < (int)all_locs.size(); ++i) {  // for location specified

      rw_trading_location_ = all_locs[i];

      //      for (unsigned int disk_counter_ = 0; disk_counter_ < all_aws_disk_vec_.size(); disk_counter_++) {
      //        std::string retval = consturctPath(
      //            (std::string(all_aws_disk_vec_[disk_counter_]) + std::string(AWS_CACHE_PREFIX) +
      //            std::string(baseLogDir)),
      //            rw_trading_location_, _preevent_YYYY_str_, _preevent_MM_str_, _preevent_DD_str_, t_exchange_symbol_,
      //            _preevent_YYYYMMDD_str_);
      //
      //        std::string retvalgz = retval + ".gz";
      //        if (CheckForFileOnAllS3Cache(retvalgz)) {
      //          return retvalgz;
      //        }
      //      }
    }
    //
    rw_trading_location_ = kTLocMAX;  // signaling error or file not found
    return retval_error;
  }

  inline static void SetSkipPrimaryLoc(bool skip_primary_loc) { skip_primary_location_ = skip_primary_loc; }
};
}

#endif  // BASE_MDSMESSAGES_BMF_LOGGED_MESSAGE_FILENAMER_H
