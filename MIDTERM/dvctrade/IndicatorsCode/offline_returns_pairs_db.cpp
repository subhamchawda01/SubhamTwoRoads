/**
   \file IndicatorsCode/offline_returns_lrdb.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvctrade/Indicators/offline_returns_pairs_db.hpp"

namespace HFSAT {

OfflineReturnsPairsDB::OfflineReturnsPairsDB(DebugLogger& t_dbglogger_, const Watch& r_watch_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      lrdbfile_base_dir_(std::string(BASETRADEINFODIR) + "/ReturnsInfo/"),
      returns_db_time_zone_now_(kRTZPreGMT12),
      lasttznowset_(0) {
  codes_to_lrinfo_map_vec_.resize(kRTZMAX);  // 6pm to 2am, 2am to 8am, 8am to 5pm
  // SetReturnsDBTimeZoneNow ( );
  ReloadDB();
}

void OfflineReturnsPairsDB::CollectSourceShortcodes(const std::string& dep_shc_,
                                                    std::vector<std::string>& t_shortcodes_affecting_this_indicator_) {
  for (ReturnsDBTimeZone t_time_zone_now_ = kRTZPreGMT0; t_time_zone_now_ < kRTZMAX;
       t_time_zone_now_ = ReturnsDBTimeZone(t_time_zone_now_ + 1)) {
    std::string filename_ = GetReturnsDBFullFileName(0, t_time_zone_now_);
    std::ifstream t_file_;
    t_file_.open(filename_.c_str(), std::ifstream::in);
    const int kReturnsDBLineBufferLen = 1024;
    char readline_buffer_[kReturnsDBLineBufferLen];
    bzero(readline_buffer_, kReturnsDBLineBufferLen);
    while (t_file_.good()) {
      bzero(readline_buffer_, kReturnsDBLineBufferLen);
      t_file_.getline(readline_buffer_, kReturnsDBLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kReturnsDBLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() > 4 && dep_shc_.compare(std::string(tokens_[0])) == 0)  // till 4, there are single sources
      {
        for (unsigned index_ = 2; index_ < tokens_.size() - 1; index_ += 2) {
          VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, std::string(tokens_[index_]));
        }
      }
    }
  }
}

std::vector<std::string> OfflineReturnsPairsDB::GetSourcesForShortcode(const std::string& dep_shc_) {
  std::vector<std::string> this_source_vec_;
  for (ReturnsDBTimeZone t_time_zone_now_ = kRTZPreGMT0; t_time_zone_now_ < kRTZMAX;
       t_time_zone_now_ = ReturnsDBTimeZone(t_time_zone_now_ + 1)) {
    std::string filename_ = GetReturnsDBFullFileName(0, t_time_zone_now_);
    std::ifstream t_file_;
    t_file_.open(filename_.c_str(), std::ifstream::in);
    const int kReturnsDBLineBufferLen = 1024;
    char readline_buffer_[kReturnsDBLineBufferLen];
    bzero(readline_buffer_, kReturnsDBLineBufferLen);
    while (t_file_.good()) {
      bzero(readline_buffer_, kReturnsDBLineBufferLen);
      t_file_.getline(readline_buffer_, kReturnsDBLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kReturnsDBLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() > 4)  // till 4, there are single sources
      {
        for (unsigned index_ = 2; index_ < tokens_.size() - 1; index_ += 2) {
          VectorUtils::UniqueVectorAdd(this_source_vec_, std::string(tokens_[index_]));
        }
      }
    }
  }
  return this_source_vec_;
}

std::string OfflineReturnsPairsDB::GetSourcePortfolioForShortcode(const std::string dep_shc_) {
  std::string portfolios_ = "";
  if ((((lasttznowset_ == 0) || (watch_.msecs_from_midnight() - lasttznowset_ > 600000))) &&
      watch_.msecs_from_midnight() != 0 /* hack since we dont have all files computed */) {  // every ten minutes
    SetReturnsDBTimeZoneNow();
    lasttznowset_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % 600000);
  }

  std::string filename_ = GetReturnsDBFullFileName(0, returns_db_time_zone_now_);
  std::ifstream t_file_;
  t_file_.open(filename_.c_str(), std::ifstream::in);
  const int kReturnsDBLineBufferLen = 1024;
  char readline_buffer_[kReturnsDBLineBufferLen];
  bzero(readline_buffer_, kReturnsDBLineBufferLen);
  int last_recorded_max_line_ = 0;
  while (t_file_.good()) {
    bzero(readline_buffer_, kReturnsDBLineBufferLen);
    t_file_.getline(readline_buffer_, kReturnsDBLineBufferLen);
    PerishableStringTokenizer st_(readline_buffer_, kReturnsDBLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() > 4 && dep_shc_.compare(tokens_[0]) == 0)  // till 4, there are single sources
    {
      if (last_recorded_max_line_ < int(tokens_.size())) {
        last_recorded_max_line_ = tokens_.size();
        portfolios_ = std::string(tokens_[1]);
      }
    }
  }

  return portfolios_;
}

void OfflineReturnsPairsDB::ReadDBFile(ReturnsDBTimeZone t_lrdb_time_zone_, std::string _lrdb_filename_) {
  if (t_lrdb_time_zone_ >= 0 && t_lrdb_time_zone_ < kRTZMAX) {
    ShortcodeReturnsInfoMap& codes_to_lrinfo_map_ = codes_to_lrinfo_map_vec_[t_lrdb_time_zone_];

    std::ifstream lrdb_infile_;
    lrdb_infile_.open(_lrdb_filename_.c_str(), std::ifstream::in);
    if (lrdb_infile_.is_open()) {
      const int kReturnsDBLineBufferLen = 1024;
      char readline_buffer_[kReturnsDBLineBufferLen];
      bzero(readline_buffer_, kReturnsDBLineBufferLen);

      while (lrdb_infile_.good()) {
        bzero(readline_buffer_, kReturnsDBLineBufferLen);
        lrdb_infile_.getline(readline_buffer_, kReturnsDBLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kReturnsDBLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() > 2) {
          // dep_shortcode_^indep_shortcode_ lr_coeff_ lr_correlation_ start_time-end_time
          std::string dep_indep_portfolio_code_ =
              std::string(tokens_[0]) + std::string(tokens_[1]);  // dep+portfolio_name
          std::vector<std::string> source_shc_vec_;
          std::vector<double> source_weight_vec_;
          for (unsigned i = 2; i < tokens_.size() - 1; i += 2) {
            source_shc_vec_.push_back(std::string(tokens_[i]));
            source_weight_vec_.push_back(atof(tokens_[i + 1]));
          }
          DBGLOG_TIME_CLASS_FUNC_LINE << " Loading info for : " << dep_indep_portfolio_code_
                                      << " zone: " << t_lrdb_time_zone_ << DBGLOG_ENDL_FLUSH;
          ReturnsInfo this_lrinfo_(source_weight_vec_, source_shc_vec_);  // lr_coeff_ lr_correlation_
          if (codes_to_lrinfo_map_.find(dep_indep_portfolio_code_) ==
              codes_to_lrinfo_map_.end()) {  // do not overwrite if already read
            codes_to_lrinfo_map_[dep_indep_portfolio_code_] = this_lrinfo_;
          }
        }
      }

      lrdb_infile_.close();
    }
  }
}

void OfflineReturnsPairsDB::ReloadDB() {
  for (ReturnsDBTimeZone t_time_zone_ = kRTZPreGMT0; t_time_zone_ < kRTZMAX;
       t_time_zone_ = ReturnsDBTimeZone(t_time_zone_ + 1)) {
    std::string filename_ = lrdbfile_base_dir_ + "/portfolio_data.txt";
    filename_ = GetReturnsDBFullFileName(watch_.YYYYMMDD(), t_time_zone_);
    DBGLOG_TIME_CLASS_FUNC_LINE << " Current timezone: " << t_time_zone_ << " file to read: " << filename_
                                << DBGLOG_ENDL_FLUSH;
    ReadDBFile(t_time_zone_, filename_);
  }
}

bool OfflineReturnsPairsDB::ReturnsCoeffPresent(const std::string& t_dep_shortcode_,
                                                const std::string& t_indep_portfolio_shortcode_) {
  std::string _key_ = t_dep_shortcode_ + t_indep_portfolio_shortcode_;
  DBGLOG_TIME_CLASS_FUNC_LINE << " Checking for " << _key_ << " zone " << returns_db_time_zone_now_
                              << DBGLOG_ENDL_FLUSH;

  for (ReturnsDBTimeZone t_returns_db_time_zone_now_ = kRTZPreGMT0; t_returns_db_time_zone_now_ < kRTZMAX;
       t_returns_db_time_zone_now_ = ReturnsDBTimeZone(t_returns_db_time_zone_now_ + 1)) {
    if (codes_to_lrinfo_map_vec_[t_returns_db_time_zone_now_].find(_key_) !=
        codes_to_lrinfo_map_vec_[t_returns_db_time_zone_now_].end()) {
      return true;
    }
  }
  return false;
}

ReturnsInfo OfflineReturnsPairsDB::GetReturnsCoeff(std::string t_dep_shortcode_,
                                                   std::string t_indep_portfolio_shortcode_) {
  std::string _key_ = t_dep_shortcode_ + t_indep_portfolio_shortcode_;

  DBGLOG_TIME_CLASS_FUNC_LINE << " looking for " << _key_ << " zone " << returns_db_time_zone_now_ << DBGLOG_ENDL_FLUSH;
  if (codes_to_lrinfo_map_vec_[returns_db_time_zone_now_].find(_key_) !=
      codes_to_lrinfo_map_vec_[returns_db_time_zone_now_].end()) {
    return codes_to_lrinfo_map_vec_[returns_db_time_zone_now_][_key_];
  } else {
    return ReturnsInfo();
  }
}

void OfflineReturnsPairsDB::SetReturnsDBTimeZoneNow() {
  if (watch_.msecs_from_midnight() < 1 * 3600 * 1000 || watch_.msecs_from_midnight() >= 19 * 3600 * 1000) {
    returns_db_time_zone_now_ = kRTZPreGMT0;
  } else if (watch_.msecs_from_midnight() < 5 * 3600 * 1000) {
    returns_db_time_zone_now_ = kRTZPreGMT6;
  } else if (watch_.msecs_from_midnight() < 11 * 3600 * 1000) {
    returns_db_time_zone_now_ = kRTZPreGMT12;
  } else {
    returns_db_time_zone_now_ = kRTZPreGMT18;
  }

  DBGLOG_CLASS_FUNC_LINE << " Setting Timezone : " << returns_db_time_zone_now_ << " At time: " << watch_.tv()
                         << DBGLOG_ENDL_FLUSH;
  // returns_db_time_zone_now_ = kRTZPreGMT12;
}

std::string OfflineReturnsPairsDB::GetReturnsDBFullFileName(int t_intdate_, ReturnsDBTimeZone t_lrdb_time_zone_) {
  static std::string lrdbtimezonestr_[] = {"PreGMT0", "PreGMT6", "PreGMT12", "PreGMT18"};

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << BASETRADEINFODIR << "/ReturnsInfo/"
              << "portfolio_data_tr_ridge_" << lrdbtimezonestr_[t_lrdb_time_zone_] << ".txt";
  std::string retval = t_temp_oss_.str();

  return retval;
}

std::string OfflineReturnsPairsDB::GetDefaultReturnsDBFullFileName(const ReturnsDBTimeZone t_lrdb_time_zone_) {
  static std::string lrdbtimezonestr_[] = {"PreGMT0", "PreGMT6", "PreGMT12", "PreGMT18"};

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << lrdbfile_base_dir_ << "/"
              << "DEFAULT_300_timed_" << lrdbtimezonestr_[t_lrdb_time_zone_] << ".txt";
  std::string retval = t_temp_oss_.str();
  return retval;
}
}
