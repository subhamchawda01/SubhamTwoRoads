/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <iomanip>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/rollover_utils.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define BASETRADEINFODIR "/spare/local/tradeinfo/"

struct LRInfo {
  double lr_coeff_;
  double lr_correlation_;

  LRInfo() : lr_coeff_(0.0), lr_correlation_(0.0) {}

  LRInfo(const double _a_, const double _b_) : lr_coeff_(_a_), lr_correlation_(_b_) {}
};

namespace HFSAT {
// typedef enum { kLRTZPreGMT4, kLRTZPreGMT8, kLRTZPreGMT14, kLRTZPreGMT23, kLRTZMAX } LRDBTimeZone;

typedef std::map<std::string, LRInfo> ShortcodeLRInfoMap;
typedef std::map<std::string, LRInfo>::iterator ShortcodeLRInfoMapIter;
typedef std::vector<ShortcodeLRInfoMap> ShortcodeLRInfoMapVec;

class OfflineReturnsLRDB {
 public:
  //  DebugLogger& dbglogger_;
  //  const Watch& watch_;
  std::string& dep_shortcode_;
  std::string& dep_exchange_;
  int yyyymmdd_;
  char dep_indep_sep_;
  std::string lrdbfile_base_dir_;
  std::vector<unsigned int> lrdb_exchange_sessions_;
  std::vector<char*> lrdb_exchange_sessions_start_end_vec_;

  ShortcodeLRInfoMapVec codes_to_lrinfo_map_vec_;

 public:
  OfflineReturnsLRDB(std::string& _dep_shc_, std::string& _dep_exchange_, int _intdate_,
                     std::string _lrdbfile_base_dir_);
  ~OfflineReturnsLRDB() {}
  bool LRCoeffPresent(const std::string& t_dep_shortcode_, const std::string& t_indep_portfolio_shortcode_,
                      unsigned int lrdb_session_now_) const;
  LRInfo GetLRCoeff(std::string _dep_shortcode_, std::string _indep_portfolio_shortcode_,
                    unsigned int lrdb_session_now_);
  void ReadDBFile(unsigned int lrdb_session_now_, std::string _lrdb_filename_, std::string dep_, std::string indep_);
  void ReloadDB(std::string dep_, std::string indep_);
  void PatchLRDB();
  void SetLRDBSessionNow();
  void GetLRDBExchangeSessions();
  std::string GetLRDBFullFileName(int t_intdate_, const char* t_lrdb_session_start_end_);
  std::string GetDefaultLRDBFullFileName(const char* t_lrdb_session_start_end_);
};

OfflineReturnsLRDB::OfflineReturnsLRDB(std::string& _dep_shc_, std::string& _dep_exchange_, int _intdate_,
                                       std::string _lrdbfile_base_dir_)
    : dep_shortcode_(_dep_shc_),
      dep_exchange_(_dep_exchange_),
      yyyymmdd_(_intdate_),
      dep_indep_sep_('^'),
      lrdbfile_base_dir_(_lrdbfile_base_dir_) {
  GetLRDBExchangeSessions();
  codes_to_lrinfo_map_vec_.resize(lrdb_exchange_sessions_.size());
  //      ReloadDB();
}

void OfflineReturnsLRDB::GetLRDBExchangeSessions() {
  std::string lrdb_exchange_sessions_filename_ = lrdbfile_base_dir_ + "/exchange_hours_file.txt";
  std::ifstream lrdb_exchange_sessions_infile_;

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
            // num_chars_--;
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

void OfflineReturnsLRDB::ReadDBFile(unsigned int lrdb_session_now_, std::string _lrdb_filename_, std::string t_dep_,
                                    std::string t_indep_) {
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
          std::string key = std::string(t_dep_) + '^' + std::string(t_indep_);
          auto res = std::mismatch(key.begin(), key.end(), dep_indep_portfolio_code_.begin());
          if (res.first == key.end()) {
            LRInfo this_lrinfo_(atof(tokens_[1]), atof(tokens_[2]));  // lr_coeff_ lr_correlation_
            if (codes_to_lrinfo_map_.find(dep_indep_portfolio_code_) ==
                codes_to_lrinfo_map_.end()) {  // do not overwrite if already read
              codes_to_lrinfo_map_[dep_indep_portfolio_code_] = this_lrinfo_;
            }
          }
        }
      }
      lrdb_infile_.close();
    }
  }
}

void OfflineReturnsLRDB::ReloadDB(std::string t_dep_, std::string t_indep_) {
  for (auto i = 0u; i < lrdb_exchange_sessions_.size(); i++) {
    unsigned int t_lrdb_session_ = lrdb_exchange_sessions_[i];
    char* t_lrdb_session_start_end_ = lrdb_exchange_sessions_start_end_vec_[i];
    int t_intdate_ = DateTime::CalcPrevDay(yyyymmdd_);  // don't use the date that we are trading on!

    size_t read_count_ = 0;
    //    std::string most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_time_zone_);
    //    std::cout<<t_lrdb_session_ <<" "<<t_lrdb_session_start_end_<<std::endl;
    std::string most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_session_start_end_);
    if (FileUtils::exists(most_current_lrdb_filename_)) {
      //      std::cout<<"Reading lrdb file ::"<<most_current_lrdb_filename_<<std::endl;
      ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_, t_dep_, t_indep_);
      read_count_++;
    }

    for (auto i = 0u; i < 120; i++) {
      t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
      most_current_lrdb_filename_ = GetLRDBFullFileName(t_intdate_, t_lrdb_session_start_end_);

      if (FileUtils::exists(most_current_lrdb_filename_)) {
        ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_, t_dep_, t_indep_);
        read_count_++;
        if (read_count_ > 5) {
          break;
        }
      }
    }
   /*
    most_current_lrdb_filename_ = GetDefaultLRDBFullFileName(t_lrdb_session_start_end_);

    if (FileUtils::exists(most_current_lrdb_filename_)) {
      ReadDBFile(t_lrdb_session_, most_current_lrdb_filename_, t_dep_, t_indep_);
    }
    if (!FileUtils::exists(most_current_lrdb_filename_)) {
      std::cerr << "LRDB file not found" << most_current_lrdb_filename_ << std::endl;
    }
    */
  }

  PatchLRDB();
}

void OfflineReturnsLRDB::PatchLRDB() {
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
      //      std::cout<<"Patching for key "<<_allkeys_[i]<<std::endl;
      if (codes_to_lrinfo_map_vec_[j].find(_allkeys_[i]) == codes_to_lrinfo_map_vec_[j].end()) {
        unsigned int k = j + 1;
        k = k % lrdb_exchange_sessions_.size();
        //        std::cout<<"Key not found looking ahead  in "<<k<<std::endl;
        //        std::cout<<codes_to_lrinfo_map_vec_.size()<<" "<<k<<" " <<j<<std::endl;
        while (codes_to_lrinfo_map_vec_[k].find(_allkeys_[i]) == codes_to_lrinfo_map_vec_[k].end() && k != j) {
          k = k + 1;
          k = k % lrdb_exchange_sessions_.size();
        }
        codes_to_lrinfo_map_vec_[j][_allkeys_[i]] = codes_to_lrinfo_map_vec_[k][_allkeys_[i]];
      }
    }
  }
}

bool OfflineReturnsLRDB::LRCoeffPresent(const std::string& t_dep_shortcode_,
                                        const std::string& t_indep_portfolio_shortcode_,
                                        unsigned int lrdb_session_now_) const {
  std::string dep_sname = HFSAT::RollOverUtils::GetNearestMajorExpiry(t_dep_shortcode_);

  std::string _key_ = dep_sname + dep_indep_sep_ + t_indep_portfolio_shortcode_;
  int lrdb_session_idx_ = std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) -
                          lrdb_exchange_sessions_.begin();
  if (codes_to_lrinfo_map_vec_[lrdb_session_idx_].find(_key_) != codes_to_lrinfo_map_vec_[lrdb_session_idx_].end()) {
    return true;
  } else {
    return false;
  }
}

LRInfo OfflineReturnsLRDB::GetLRCoeff(std::string t_dep_shortcode_, std::string t_indep_portfolio_shortcode_,
                                      unsigned int lrdb_session_now_) {
  if (!t_dep_shortcode_.compare(t_indep_portfolio_shortcode_)) return LRInfo(1.0, 1.0);
  std::string dep_sname = HFSAT::RollOverUtils::GetNearestMajorExpiry(t_dep_shortcode_);

  std::string _key_ = dep_sname + dep_indep_sep_ + t_indep_portfolio_shortcode_;
  int lrdb_session_idx_ = std::find(lrdb_exchange_sessions_.begin(), lrdb_exchange_sessions_.end(), lrdb_session_now_) -
                          lrdb_exchange_sessions_.begin();
  if (codes_to_lrinfo_map_vec_[lrdb_session_idx_].find(_key_) != codes_to_lrinfo_map_vec_[lrdb_session_idx_].end()) {
    return codes_to_lrinfo_map_vec_[lrdb_session_idx_][_key_];
  } else {
    return LRInfo(0.0, 0.0);
  }
}

std::string OfflineReturnsLRDB::GetLRDBFullFileName(int t_intdate_, const char* t_lrdb_session_start_end_) {
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

std::string OfflineReturnsLRDB::GetDefaultLRDBFullFileName(const char* t_lrdb_session_start_end_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << lrdbfile_base_dir_ << "/"
              << "DEFAULT_300_timed_" << dep_exchange_ << "_" << t_lrdb_session_start_end_ << ".txt";
  std::string retval = t_temp_oss_.str();
  //  std::cout<<"Checking file "<<retval<<std::endl;
  return retval;
}
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << argv[0] << " input_date shortcode\n";
    std::cout << argv[0] << " input_date shortcode indep\n";
    std::cout << argv[0] << " input_date shortcode indep returns\n";
    exit(0);
  }
  int input_date_ = atoi(argv[1]);
  std::string shc_ = std::string(argv[2]);
  std::string indep_ = "";
  std::string lrdbfile_base_dir_;
  std::string dep_exchange_;
  bool returns_lr_ = false;
  if (argc >= 4) {
    indep_ = std::string(argv[3]);
    if (indep_.compare("ALL") == 0) {
      indep_ = "";
    }
  }
  if (argc >= 5) {
    returns_lr_ = atoi(argv[4]) > 0 ? true : false;
  }

  if (returns_lr_) {
    lrdbfile_base_dir_ = std::string(BASETRADEINFODIR) + "NewRetLRDBBaseDir";
  } else {
    lrdbfile_base_dir_ = std::string(BASETRADEINFODIR) + "NewLRDBBaseDir";
  }

  bool is_nse_present_ = false;
  if (strncmp(shc_.c_str(), "NSE_", 4) == 0 || strncmp(indep_.c_str(), "NSE_", 4) == 0) {
    is_nse_present_ = true;
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }

  dep_exchange_ = HFSAT::ExchSourceStringForm(HFSAT::SecurityDefinitions::GetContractExchSource(shc_, input_date_));
  HFSAT::OfflineReturnsLRDB lrdb_obj_(shc_, dep_exchange_, input_date_, lrdbfile_base_dir_);
  lrdb_obj_.ReloadDB(shc_, indep_);
  std::vector<unsigned int> lrdb_exchange_sessions_ = lrdb_obj_.lrdb_exchange_sessions_;
  std::vector<char*> lrdb_exchange_sessions_start_end_vec_ = lrdb_obj_.lrdb_exchange_sessions_start_end_vec_;

  unsigned int t_lrdb_session_idx_ = 0;
  HFSAT::ShortcodeLRInfoMap shc_map_ = lrdb_obj_.codes_to_lrinfo_map_vec_[t_lrdb_session_idx_];
  HFSAT::ShortcodeLRInfoMapIter map_it_ = shc_map_.begin();
  std::cout << "DEP^INDEP"
            << "\t \t \t";
  for (auto i = 0u; i < lrdb_exchange_sessions_.size(); i++) {
    std::cout << lrdb_exchange_sessions_start_end_vec_[i] << "\t \t";
  }
  std::cout << std::endl;

  while (map_it_ != shc_map_.end()) {
    std::cout << std::fixed << std::setprecision(7) << map_it_->first << "\t \t \t(" << (map_it_->second).lr_coeff_
              << "," << (map_it_->second).lr_correlation_ << ")\t \t";
    for (unsigned int i = 1; i < lrdb_exchange_sessions_.size(); i++) {
      std::cout << "(" << (lrdb_obj_.codes_to_lrinfo_map_vec_[t_lrdb_session_idx_ + i])[map_it_->first].lr_coeff_ << ","
                << (lrdb_obj_.codes_to_lrinfo_map_vec_[t_lrdb_session_idx_ + i])[map_it_->first].lr_correlation_
                << ")\t \t";
    }
    std::cout << std::endl;
    map_it_++;
    // std::cout << lrdb_obj_.codes_to_lrinfo_map_vec_ [ t_lrdb_session_idx_ ][ 1 ] ;
  }
  return 0;
}
