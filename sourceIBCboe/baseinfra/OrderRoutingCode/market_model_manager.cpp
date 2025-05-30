/**
 \file OrderRoutingCode/market_model_manager.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#include <stdlib.h>
#include <sys/dir.h>
#include <cmath>
#include <algorithm>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/OrderRouting/market_model_manager.hpp"

namespace HFSAT {

void MarketModelManager::_GetMarketModel(const std::string& _shortcode_, int _market_model_index_,
                                         MarketModel& market_model_, int tradingdate) const {
  ShortcodeMarketModelMapCiter_t _citer_;
  // for NSE we have a single entry for all products currently
  if (strncmp(_shortcode_.c_str(), "NSE_", 4) == 0) {
    _citer_ = shortcode_market_model_map_.find("NSE");
  } else if (strncmp(_shortcode_.c_str(), "BSE_", 4) == 0) {
    _citer_ = shortcode_market_model_map_.find("BSE");
  } else if (strncmp(_shortcode_.c_str(), "HK_", 3) == 0) {
    _citer_ = shortcode_market_model_map_.find("HKSTOCKS");
  }  else if (strncmp(_shortcode_.c_str(), "CBOE_", 5) == 0) {
    _citer_ = shortcode_market_model_map_.find("CBOE");
  }else {
    _citer_ = shortcode_market_model_map_.find(_shortcode_);
  }

  if (_citer_ != shortcode_market_model_map_.end()) {
// std::cout << " SHORTCOD E: " << _shortcode_ << " FOUND MODEL : " << std::endl;
    const std::vector<MarketModel>& _this_vec_ = _citer_->second;

    if ((!_this_vec_.empty()) && ((unsigned int)_market_model_index_ >= _this_vec_.size())) {
      _market_model_index_ = _this_vec_.size() - 1u;
    }
    market_model_ = _this_vec_[_market_model_index_];

// std::cout << " SHORTCOD E: " << _shortcode_ << " FOUND MODEL : " << " INDEX :"  << market_model_ << std::endl;
  } else {
    std::cerr << "Could not find market_model_info_ for : " << _shortcode_ << std::endl;
    exit(-1);
  }
}

std::string MarketModelManager::FindClosestDatedMarketModelInfoFile(int tradingdate_) {
  DIR* dir;
  struct dirent* ent;
  std::vector<int> file_dates;
  std::string market_model_info_file_name = "";
  std::string market_model_info_dir =
      FileUtils::AppendHome(std::string(BASEINFRAOFFLINECONFIGSDIR) + "MarketModelInfo/");

  // For jenkins support, we need to mention file paths used from basetrade repo relative to WORKDIR.
  const char* work_dir = getenv("WORKDIR");
  if (work_dir != nullptr) {
    const char* deps_install = getenv("DEPS_INSTALL");
    if (deps_install != nullptr) {
      market_model_info_dir = std::string(deps_install) + "/baseinfra/OfflineConfigs/MarketModelInfo/";
    }
  }
  market_model_info_file_name = market_model_info_dir + "market_model_info.txt";
// std::cout << " pathof the file : " << market_model_info_file_name << std::endl;

  if ((dir = opendir(market_model_info_dir.c_str())) != NULL) {
    /* iterating over all the files in market model info directory */
    while ((ent = readdir(dir)) != NULL) {
      /*Extracting date out of the file name*/
      std::string file_name = std::string(ent->d_name);
      std::size_t first_occurence = file_name.find_first_of("0123456789");

      if (first_occurence != std::string::npos) {
        std::string date = file_name.substr(first_occurence, 8);
        if (std::atoi(date.c_str()) != 0) file_dates.push_back(std::atoi(date.c_str()));
      }
    }
    closedir(dir);
  } else {
    /* could not open directory */
    perror("");
    std::cerr << "Could not find market_model_info_dir" << std::endl;
    ;
  }

  sort(file_dates.begin(), file_dates.end());
  for (auto it = file_dates.rbegin(); it != file_dates.rend(); ++it) {
    if (*it <= tradingdate_) {
      /* market_model_files should be in he following format : market_model_info_YYYYMMDD.txt*/
      market_model_info_file_name = market_model_info_dir + "market_model_info_" + std::to_string(*it) + ".txt";
      break;
    }
  }
  return market_model_info_file_name;
}

void MarketModelManager::LoadMarketModelInfoFile(int tradingdate_) {
  std::string _market_model_info_filename_ = MarketModelManager::FindClosestDatedMarketModelInfoFile(tradingdate_);
  MarketModelManager::LoadDrDates();
  std::ifstream market_model_info_file_;
  market_model_info_file_.open(_market_model_info_filename_.c_str(), std::ifstream::in);
  if (market_model_info_file_.is_open()) {
    const int kMarketModelInfoLineLen = 1024;
    char readline_buffer_[kMarketModelInfoLineLen];
    bzero(readline_buffer_, kMarketModelInfoLineLen);
    while (market_model_info_file_.good()) {
      bzero(readline_buffer_, kMarketModelInfoLineLen);
      market_model_info_file_.getline(readline_buffer_, kMarketModelInfoLineLen);
      PerishableStringTokenizer st_(readline_buffer_, kMarketModelInfoLineLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 4) {
        // SHORTCODE  COM_USECS  CONF_USECS  CXL_USECS  ORS_MKT_DIFF SEND_DELAY_DIFF CXL_DELAY_DIFF
        std::string _this_shortcode_ = tokens_[0];
        MarketModel _this_mm_;
        int drDateaDelay_ = MarketModelManager::CheckDateDr(_this_shortcode_,tradingdate_);
        if (drDateaDelay_ != 0 ){
          _this_mm_.com_usecs_  =  drDateaDelay_;
          _this_mm_.conf_usecs_ =  drDateaDelay_;
          _this_mm_.cxl_usecs_ = drDateaDelay_;
          _this_mm_.ors_mkt_diff_ = drDateaDelay_;
          _this_mm_.send_delay_diff = drDateaDelay_;
          _this_mm_.cxl_delay_diff = drDateaDelay_;
        }else{
          _this_mm_.com_usecs_ = atol(tokens_[1]);
          _this_mm_.conf_usecs_ = atol(tokens_[2]);
          _this_mm_.cxl_usecs_ = atol(tokens_[3]);

          if (tokens_.size() >= 5) {
            _this_mm_.ors_mkt_diff_ = atoi(tokens_[4]);
          }

          if (tokens_.size() >= 6) {
            _this_mm_.send_delay_diff = atoi(tokens_[5]);
          }

          if (tokens_.size() >= 7) {
            _this_mm_.cxl_delay_diff = atoi(tokens_[6]);
          }
        }

        shortcode_market_model_map_[_this_shortcode_].push_back(_this_mm_);
      }
    }
    market_model_info_file_.close();
  }
}
void MarketModelManager::LoadDrDates(){
  std::string dr_date_info_file_name = "";
  std::string dr_date_info_dir ="/home/pengine/prod/live_configs/";
  // For jenkins support, we need to mention file paths used from basetrade repo relative to WORKDIR.
  dr_date_info_file_name = dr_date_info_dir + "dr_dates_delay_info.txt";
  std::cout<<dr_date_info_file_name<<std::endl;
  std::ifstream dr_dates_info_file_;
  dr_dates_info_file_.open(dr_date_info_file_name.c_str(), std::ifstream::in);
  if (dr_dates_info_file_.is_open()) {
    const int kDrDateInfoLineLen = 1024;
    char readline_buffer_[kDrDateInfoLineLen];
    bzero(readline_buffer_, kDrDateInfoLineLen);

    while (dr_dates_info_file_.good()) {
      bzero(readline_buffer_, kDrDateInfoLineLen);
      dr_dates_info_file_.getline(readline_buffer_, kDrDateInfoLineLen);
      PerishableStringTokenizer st_(readline_buffer_, kDrDateInfoLineLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() == 3) {
        auto exchIt = ExchDrDatesDelay_.find(tokens_[0]);

        // If the outer key doesn't exist, create a new inner map
        if (exchIt == ExchDrDatesDelay_.end()) {
          std::map<int, int> dateToDelay;
          dateToDelay[atoi(tokens_[1])] = atoi(tokens_[2]);
          ExchDrDatesDelay_[tokens_[0]] = dateToDelay;
        } else {
            // If the outer key exists, add or update the inner key-value pair
            exchIt->second[atoi(tokens_[1])] = atoi(tokens_[2]);
        }
      }
    }
    dr_dates_info_file_.close();
  }
}
int MarketModelManager::CheckDateDr(std::string shortcode_, int tradeing_date_){
   auto exchIt = ExchDrDatesDelay_.find(shortcode_);

   if (exchIt != ExchDrDatesDelay_.end()) {
     auto dateIt = exchIt->second.find(tradeing_date_);

     if (dateIt != exchIt->second.end()) {
       return dateIt->second;
     }
   }
   return 0;
}
}
