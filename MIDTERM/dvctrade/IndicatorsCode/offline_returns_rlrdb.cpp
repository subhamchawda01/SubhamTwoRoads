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
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/rollover_utils.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvctrade/Indicators/offline_returns_rlrdb.hpp"

namespace HFSAT {

OfflineReturnsRetLRDB::OfflineReturnsRetLRDB(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& t_dep_shortcode_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      lrdbfile_base_dir_(std::string(BASETRADEINFODIR) + "NewRetLRDBBaseDir"),
      dep_indep_sep_('^'),
      lrdb_session_now_(0),
      dep_shortcode_(t_dep_shortcode_),
      dep_exchange_(HFSAT::ExchSourceStringForm(
          HFSAT::SecurityDefinitions::GetContractExchSource(t_dep_shortcode_, watch_.YYYYMMDD()))),
      lasttznowset_(0) {
  GetLRDBExchangeSessions();
  codes_to_lrinfo_map_vec_.resize(lrdb_exchange_sessions_.size());  // CET_900-EST_800, EST_800-CET_1900
  SetLRDBSessionNow();
  ReloadDB();
}

void OfflineReturnsRetLRDB::SetLRDBSessionNow() {
  //  static vector<std::string> lrdb_exchange_sessions_ = OfflineReturnsLRDB::GetLRDBExchangeSessions();
  for (auto i = 0u; i < lrdb_exchange_sessions_.size(); i++) {
    unsigned int j = (i - 1 + lrdb_exchange_sessions_.size()) % lrdb_exchange_sessions_.size();
    if (lrdb_exchange_sessions_[j] < lrdb_exchange_sessions_[i]) {
      if (watch_.msecs_from_midnight() <
          (int)(((double)lrdb_exchange_sessions_[i] / 100 + (double)(lrdb_exchange_sessions_[i] % 100) / 60) * 3600 *
                1000)) {
        if (watch_.msecs_from_midnight() >
            (int)(((double)lrdb_exchange_sessions_[j] / 100 + (double)(lrdb_exchange_sessions_[j] % 100) / 60) * 3600 *
                  1000)) {
          lrdb_session_now_ = lrdb_exchange_sessions_[i];
          break;
        } else {
          continue;
        }
      }
    } else {
      if (watch_.msecs_from_midnight() <
          (int)(((double)lrdb_exchange_sessions_[i] / 100 + (double)(lrdb_exchange_sessions_[i] % 100) / 60) * 3600 *
                1000)) {
        lrdb_session_now_ = lrdb_exchange_sessions_[i];
      }
    }
  }
  //  std::cout<<"LRDB session now "<<watch_.msecs_from_midnight()<<" "<<lrdb_session_now_<<std::endl;
}

void OfflineReturnsRetLRDB::GetLRDBExchangeSessions() {
  std::string lrdb_exchange_sessions_filename_ = lrdbfile_base_dir_ + "/exchange_hours_file.txt";
  std::ifstream lrdb_exchange_sessions_infile_;
  if (!FileUtils::exists(lrdb_exchange_sessions_filename_)) {
    std::cerr << "LRDB Exchange sessions file not found.. exiting" << std::endl;
    exit(EXIT_FAILURE);
  }
  lrdb_exchange_sessions_infile_.open(lrdb_exchange_sessions_filename_.c_str(), std::ifstream::in);
  if (lrdb_exchange_sessions_infile_.is_open()) {
    const int kLRDBLineBufferLen = 1024;
    char readline_buffer_[kLRDBLineBufferLen];
    bzero(readline_buffer_, kLRDBLineBufferLen);

    while (lrdb_exchange_sessions_infile_.good()) {
      bzero(readline_buffer_, kLRDBLineBufferLen);
      lrdb_exchange_sessions_infile_.getline(readline_buffer_, kLRDBLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLRDBLineBufferLen);
      std::vector<char*> tokens_;
      HFSAT::PerishableStringTokenizer::ConstStringTokenizer(readline_buffer_, ",", tokens_);

      if (tokens_.size() >= 2) {
        if (dep_exchange_.compare(tokens_[0]) == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            std::vector<char*> start_end_hhmm_vec_;
            lrdb_exchange_sessions_start_end_vec_.push_back(tokens_[i]);
            HFSAT::PerishableStringTokenizer::ConstStringTokenizer(tokens_[i], "-", start_end_hhmm_vec_);
            unsigned int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
            unsigned int num_chars_ = 0;
            while (!std::isdigit(start_end_hhmm_vec_[1][num_chars_])) {
              num_chars_++;
            }
            num_chars_--;
            unsigned int utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
                tradingdate_, HFSAT::DateTime::GetHHMMSSTime(start_end_hhmm_vec_[1] + num_chars_),
                start_end_hhmm_vec_[1]);
            utc_hhmm_ = utc_hhmm_ / 100;
            lrdb_exchange_sessions_.push_back(utc_hhmm_);
          }
        }
      }
    }
    if (lrdb_exchange_sessions_.size() == 0) {
      std::cerr << "Exchange session timing not found for :: " << dep_exchange_ << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
}

void OfflineReturnsRetLRDB::ReadDBFile(unsigned int lrdb_session_now_, std::string _lrdb_filename_) {
  if (std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) !=
      lrdb_exchange_sessions_.end()) {
    int lrdb_session_idx_ =
        std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) -
        lrdb_exchange_sessions_.begin();
    ShortcodeLRInfoMap& codes_to_lrinfo_map_ = codes_to_lrinfo_map_vec_[lrdb_session_idx_];

    std::ifstream lrdb_infile_;
    lrdb_infile_.open(_lrdb_filename_.c_str(), std::ifstream::in);
    if (lrdb_infile_.is_open()) {
      const int kLRDBLineBufferLen = 1024;
      char readline_buffer_[kLRDBLineBufferLen];
      bzero(readline_buffer_, kLRDBLineBufferLen);

      while (lrdb_infile_.good()) {
        bzero(readline_buffer_, kLRDBLineBufferLen);
        lrdb_infile_.getline(readline_buffer_, kLRDBLineBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kLRDBLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() > 2) {
          // dep_shortcode_^indep_shortcode_ lr_coeff_ lr_correlation_ start_time-end_time
          std::string dep_indep_portfolio_code_ = tokens_[0];
          LRInfo this_lrinfo_(atof(tokens_[1]), atof(tokens_[2]));  // lr_coeff_ lr_correlation_
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

void OfflineReturnsRetLRDB::ReloadDB() {
  for (auto i = 0u; i < lrdb_exchange_sessions_.size(); i++) {
    unsigned int t_lrdb_session_ = lrdb_exchange_sessions_[i];
    char* t_lrdb_session_start_end_ = lrdb_exchange_sessions_start_end_vec_[i];
    int t_intdate_ = DateTime::CalcPrevDay(watch_.YYYYMMDD());  // don't use the date that we are trading on!

    size_t read_count_ = 0;
    //    std::string most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_time_zone_);
    //    std::cout<<t_lrdb_session_ <<" "<<t_lrdb_session_start_end_<<std::endl;
    std::string most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_session_start_end_);
    if (FileUtils::exists(most_current_lrdb_filename_)) {
      //      std::cout<<"Reading lrdb file ::"<<most_current_lrdb_filename_<<std::endl;
      ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_);
      read_count_++;
    }

    for (auto i = 0u; i < 120; i++) {
      t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
      most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_session_start_end_);

      if (FileUtils::exists(most_current_lrdb_filename_)) {
        ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_);
        read_count_++;
        if (read_count_ > 5) {
          break;
        }
      }
    }

    most_current_lrdb_filename_ = GetDefaultLRDBFullFileName(t_lrdb_session_start_end_);

    if (FileUtils::exists(most_current_lrdb_filename_)) {
      ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_);
    }
    if (!FileUtils::exists(most_current_lrdb_filename_)) {
      std::cerr << "LRDB file not found" << most_current_lrdb_filename_ << std::endl;
    }
  }

  PatchLRDB();
}

void OfflineReturnsRetLRDB::PatchLRDB() {
  std::vector<std::string> _allkeys_;
  for (auto i = 0u; i < lrdb_exchange_sessions_.size(); i++) {
    for (ShortcodeLRInfoMapIter codes_to_lrinfo_map_iter_ = codes_to_lrinfo_map_vec_[i].begin();
         codes_to_lrinfo_map_iter_ != codes_to_lrinfo_map_vec_[i].end(); codes_to_lrinfo_map_iter_++) {
      std::string _key_ = (*codes_to_lrinfo_map_iter_).first;
      _allkeys_.push_back(_key_);
    }
  }
  for (auto i = 0u; i < _allkeys_.size(); i++) {
    for (unsigned int j = 0; j < lrdb_exchange_sessions_.size(); j++) {
      if (codes_to_lrinfo_map_vec_[j].find(_allkeys_[i]) == codes_to_lrinfo_map_vec_[j].end()) {
        unsigned int k = j + 1;
        k = k % lrdb_exchange_sessions_.size();
        while (codes_to_lrinfo_map_vec_[k].find(_allkeys_[i]) == codes_to_lrinfo_map_vec_[k].end() && k != j) {
          k = k + 1;
          k = k % lrdb_exchange_sessions_.size();
        }
        codes_to_lrinfo_map_vec_[j][_allkeys_[i]] = codes_to_lrinfo_map_vec_[k][_allkeys_[i]];
      }
    }
  }
}

bool OfflineReturnsRetLRDB::LRCoeffPresent(const std::string& t_dep_shortcode_,
                                           const std::string& t_indep_portfolio_shortcode_) const {
  std::string shc_sname = RollOverUtils::GetNearestMajorExpiry(t_dep_shortcode_);

  std::string _key_ = shc_sname + dep_indep_sep_ + t_indep_portfolio_shortcode_;
  int lrdb_session_idx_ = std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) -
                          lrdb_exchange_sessions_.begin();
  if (codes_to_lrinfo_map_vec_[lrdb_session_idx_].find(_key_) != codes_to_lrinfo_map_vec_[lrdb_session_idx_].end()) {
    return true;
  } else {
    return false;
  }
}

LRInfo OfflineReturnsRetLRDB::GetLRCoeff(std::string t_dep_shortcode_, std::string t_indep_portfolio_shortcode_) {
  if (!t_dep_shortcode_.compare(t_indep_portfolio_shortcode_))  // In case of same shortcodes return 1 as we don't have
                                                                // these values in LRDB files
    return LRInfo(1.0, 1.0);
  std::string shc_sname = RollOverUtils::GetNearestMajorExpiry(t_dep_shortcode_);

  if ((lasttznowset_ == 0) || (watch_.msecs_from_midnight() - lasttznowset_ > 600000)) {  // every ten minutes
    SetLRDBSessionNow();
    lasttznowset_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % 600000);
  }
  std::string _key_ = shc_sname + dep_indep_sep_ + t_indep_portfolio_shortcode_;
  int lrdb_session_idx_ = std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) -
                          lrdb_exchange_sessions_.begin();
  if (codes_to_lrinfo_map_vec_[lrdb_session_idx_].find(_key_) != codes_to_lrinfo_map_vec_[lrdb_session_idx_].end()) {
    return codes_to_lrinfo_map_vec_[lrdb_session_idx_][_key_];
  } else {
    return LRInfo(0.0, 0.0);
  }
}

std::string OfflineReturnsRetLRDB::GetLRDBFullFileName(int t_intdate_, const char* t_lrdb_session_start_end_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << lrdbfile_base_dir_ << "/" << t_intdate_ << "_" << dep_exchange_ << "_" << t_lrdb_session_start_end_
              << ".txt";
  std::string retval = t_temp_oss_.str();
  if (!FileUtils::exists(retval)) {  // 300 second version
    int time_duration_of_changes_ = 300;
    std::ostringstream t_temp_s_oss_;
    t_temp_s_oss_ << lrdbfile_base_dir_ << "/" << t_intdate_ << "_" << time_duration_of_changes_ << "_timed_"
                  << dep_exchange_ << "_" << t_lrdb_session_start_end_ << ".txt";
    retval = t_temp_s_oss_.str();
  }
  //  std::cout<<"Checking file "<<retval<<std::endl;
  return retval;
}

std::string OfflineReturnsRetLRDB::GetDefaultLRDBFullFileName(const char* t_lrdb_session_start_end_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << lrdbfile_base_dir_ << "/"
              << "DEFAULT_300_timed_" << dep_exchange_ << "_" << t_lrdb_session_start_end_ << ".txt";
  std::string retval = t_temp_oss_.str();
  //  std::cout<<"Checking file "<<retval<<std::endl;
  return retval;
}
}
