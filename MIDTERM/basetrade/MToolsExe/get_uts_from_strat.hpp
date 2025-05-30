// =====================================================================================
//
//       Filename:  get_uts_from_strat_path.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Monday 05 January 2015 11:38:41  GMT
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <strings.h>
#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <signal.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvctrade/InitCommon/paramset.hpp"
#include "dvccode/CDef/email_utils.hpp"

#define INVALID_MAX_LOSS 9999999

using namespace HFSAT;

int getUTS(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_,
           bool use_notional_uts_ = false);
int getStirUTS(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_,
               bool use_notional_uts_ = false);
int GetUTSFromStratPath(std::string strat_file_name_, int tradingdate_, bool use_notional_uts_ = false);
int GetUTSFromStirStratPath(std::string strat_file_name, std::string shortcode_, int tradingdate_,
                            bool use_notional_uts_ = false);

void LoadParamSetVec(std::string paramfilename_, int trading_date_, std::string shortcode_,
                     std::vector<HFSAT::ParamSet> &param_set_vec_);
std::string GetRollParam(std::string _paramfile_, const int _tradingdate_);

int getUTS(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_, bool use_notional_uts_) {
  int unit_trade_size_ = 1;
  bool strat_found_ = false;
  std::vector<std::string> strats_dir_list_;
  strats_dir_list_.push_back(HFSAT::FileUtils::AppendHome("modelling/strats/" + shortcode_));
  strats_dir_list_.push_back(HFSAT::FileUtils::AppendHome("modelling/staged_strats/" + shortcode_));

  for (unsigned int strat_ind_ = 0; strat_ind_ < strats_dir_list_.size(); strat_ind_++) {
    std::string StratsDir = strats_dir_list_[strat_ind_];
    DIR *strats_directory_stream_ = opendir(StratsDir.c_str());
    if (strats_directory_stream_ == NULL) {
      std::cerr << "Error in opening strats or staged_strats directory " << StratsDir << std::endl;
      continue;
    }
    struct dirent *strats_directory_;
    while ((strats_directory_ = readdir(strats_directory_stream_)) != NULL) {
      if (strcmp(strats_directory_->d_name, ".") == 0 || strcmp(strats_directory_->d_name, "..") == 0) {
        continue;
      }
      std::string TimeperiodDir = StratsDir + "/" + strats_directory_->d_name;
      DIR *timeperiod_directory_stream_ = opendir(TimeperiodDir.c_str());
      if (timeperiod_directory_stream_ == NULL) {
        std::cerr << "Error in opening directory " << TimeperiodDir << std::endl;
        continue;
      }
      struct dirent *timeperiod_directory_;
      while ((timeperiod_directory_ = readdir(timeperiod_directory_stream_)) != NULL) {
        if (strcmp(timeperiod_directory_->d_name, strategy_filename_base_.c_str()) == 0) {
          std::string strat_file_name_ = TimeperiodDir + "/" + strategy_filename_base_;

          int t_unit_trade_size_ = GetUTSFromStratPath(strat_file_name_, tradingdate_, use_notional_uts_);
          if (t_unit_trade_size_ > 0) {
            unit_trade_size_ = t_unit_trade_size_;
            strat_found_ = true;
            break;
          }
        }
      }
      closedir(timeperiod_directory_stream_);
      if (strat_found_) {
        break;
      }
    }
    closedir(strats_directory_stream_);
    if (strat_found_) {
      break;
    }
  }
  return unit_trade_size_;
}

int getStirUTS(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_, bool use_notional_uts_) {
  int unit_trade_size_ = 1;
  bool strat_found_ = 0;
  std::string stir_strats_dir_;
  stir_strats_dir_ = HFSAT::FileUtils::AppendHome("modelling/stir_strats/");

  DIR *stir_strats_directory_stream_ = opendir(stir_strats_dir_.c_str());
  struct dirent *stir_strat_dir_;
  while ((stir_strat_dir_ = readdir(stir_strats_directory_stream_)) != NULL) {
    if (strcmp(stir_strat_dir_->d_name, ".") == 0 || strcmp(stir_strat_dir_->d_name, "..") == 0) {
      continue;
    }
    std::string StratsDir = stir_strats_dir_ + "/" + stir_strat_dir_->d_name;

    DIR *strats_directory_stream_ = opendir(StratsDir.c_str());
    if (strats_directory_stream_ == NULL) {
      std::cerr << "Error in opening strats or staged_strats directory " << StratsDir << std::endl;
      continue;
    }
    struct dirent *strats_directory_;
    while ((strats_directory_ = readdir(strats_directory_stream_)) != NULL) {
      if (strcmp(strats_directory_->d_name, ".") == 0 || strcmp(strats_directory_->d_name, "..") == 0) {
        continue;
      }
      std::string TimeperiodDir = StratsDir + "/" + strats_directory_->d_name;
      DIR *timeperiod_directory_stream_ = opendir(TimeperiodDir.c_str());
      if (timeperiod_directory_stream_ == NULL) {
        //        std::cerr << "Error in opening directory " << TimeperiodDir << std::endl;
        continue;
      }
      struct dirent *timeperiod_directory_;
      while ((timeperiod_directory_ = readdir(timeperiod_directory_stream_)) != NULL) {
        if (strcmp(timeperiod_directory_->d_name, strategy_filename_base_.c_str()) == 0) {
          std::string strat_file_name_ = TimeperiodDir + "/" + strategy_filename_base_;
          int t_unit_trade_size_ =
              GetUTSFromStirStratPath(strat_file_name_, shortcode_, tradingdate_, use_notional_uts_);
          if (t_unit_trade_size_ > 0) {
            unit_trade_size_ = t_unit_trade_size_;
            strat_found_ = true;
            break;
          }
        }
      }
      closedir(timeperiod_directory_stream_);
      if (strat_found_) {
        break;
      }
    }
    closedir(strats_directory_stream_);
    if (strat_found_) {
      break;
    }
  }
  return unit_trade_size_;
}
int GetUTSFromStirStratPath(std::string strat_file_name_, std::string shortcode_, int tradingdate_,
                            bool use_notional_uts_) {
  int unit_trade_size_ = -1;
  std::ifstream strat_file_;
  std::string im_strat_ = "";
  strat_file_.open(strat_file_name_.c_str(), std::ifstream::in);
  if (strat_file_.is_open()) {
    const int kStrategyFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyFileLineBufferLen];
    bzero(readline_buffer_, kStrategyFileLineBufferLen);
    if (strat_file_.good()) {
      strat_file_.getline(readline_buffer_, kStrategyFileLineBufferLen);
      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() > 2 && std::string(tokens_[0]).compare("STRUCTURED_STRATEGYLINE") == 0) {
        im_strat_ = tokens_[1];
      }
    }
  }
  strat_file_.close();
  std::ifstream im_strat_file_;
  im_strat_file_.open(im_strat_.c_str(), std::ifstream::in);
  if (im_strat_file_.is_open()) {
    const int kStrategyFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyFileLineBufferLen];
    bzero(readline_buffer_, kStrategyFileLineBufferLen);
    while (im_strat_file_.good()) {
      im_strat_file_.getline(readline_buffer_, kStrategyFileLineBufferLen);
      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() > 3 && shortcode_.compare(tokens_[1]) == 0) {
        std::vector<HFSAT::ParamSet> param_set_vec_;
        LoadParamSetVec(tokens_[3], tradingdate_, shortcode_, param_set_vec_);
        if (param_set_vec_.size() > 0) {
          unit_trade_size_ = (use_notional_uts_) ? param_set_vec_[0].notional_uts_ : param_set_vec_[0].unit_trade_size_;
        }
        break;
      }
    }
  }
  im_strat_file_.close();
  return unit_trade_size_;
}
int GetUTSFromStratPath(std::string strat_file_name_, int tradingdate_, bool use_notional_uts_) {
  int unit_trade_size_ = -1;
  std::ifstream strat_file_;
  strat_file_.open(strat_file_name_.c_str(), std::ifstream::in);
  if (strat_file_.is_open()) {
    const int kStrategyFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyFileLineBufferLen];
    bzero(readline_buffer_, kStrategyFileLineBufferLen);
    if (strat_file_.good()) {
      strat_file_.getline(readline_buffer_, kStrategyFileLineBufferLen);
      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() > 4) {
        std::vector<HFSAT::ParamSet> param_set_vec_;
        LoadParamSetVec(tokens_[4], tradingdate_, tokens_[1], param_set_vec_);
        if (param_set_vec_.size() > 0) {
          unit_trade_size_ = (use_notional_uts_) ? param_set_vec_[0].notional_uts_ : param_set_vec_[0].unit_trade_size_;
        }
      }
    }
  }
  strat_file_.close();
  return unit_trade_size_;
}

int GetUTSFromParamPath(const std::string &param_file_path, const std::string &shortcode, int tradingdate,
                        bool use_notional_uts) {
  int unit_trade_size = -1;
  std::vector<HFSAT::ParamSet> param_set_vec;
  LoadParamSetVec(param_file_path, tradingdate, shortcode, param_set_vec);
  if (param_set_vec.size() > 0) {
    unit_trade_size = (use_notional_uts) ? param_set_vec[0].notional_uts_ : param_set_vec[0].unit_trade_size_;
  }
  return unit_trade_size;
}

void LoadParamSetVec(std::string paramfilename_, int trading_date_, std::string shortcode_,
                     std::vector<HFSAT::ParamSet> &param_set_vec_) {
  std::ifstream paramlistfile_;
  std::string paramfile_ = GetRollParam(paramfilename_, trading_date_);
  paramlistfile_.open(paramfile_);
  bool paramset_file_list_read_ = false;
  param_set_vec_.clear();
  if (paramlistfile_.is_open()) {
    const int kParamfileListBufflerLen = 1024;
    char readlinebuffer_[kParamfileListBufflerLen];
    bzero(readlinebuffer_, kParamfileListBufflerLen);
    while (paramlistfile_.good()) {
      bzero(readlinebuffer_, kParamfileListBufflerLen);
      paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
      std::string this_line_ = std::string(readlinebuffer_);
      PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() < 1) {
        continue;
      }

      if ((strcmp(tokens_[0], "PARAMFILELIST") == 0) && tokens_.size() > 1) {
        std::string paramfilename_ = tokens_[1];

        param_set_vec_.emplace_back(paramfilename_, trading_date_, shortcode_);

        paramset_file_list_read_ = true;
      } else if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && !paramset_file_list_read_) {
        paramlistfile_.close();

        param_set_vec_.emplace_back(paramfilename_, trading_date_, shortcode_);

        break;
      } else if ((strcmp(tokens_[0], "INDICATOR") == 0) && paramset_file_list_read_) {
        // regime_indicator_string_ = this_line_ ;
      }
    }
  }
  paramlistfile_.close();
}

std::string GetRollParam(std::string _paramfile_, const int _tradingdate_) {
  std::string paramfile_ = _paramfile_;
  if (paramfile_.substr(0, 5).compare("ROLL:") == 0) {
    std::string paramlist_name_ = paramfile_.substr(5);
    std::ifstream paramlist_;
    paramlist_.open(paramlist_name_.c_str(), std::ifstream::in);
    if (paramlist_.is_open()) {
      const int buffer_len_ = 1024;
      char buffer_[buffer_len_];
      bzero(buffer_, buffer_len_);
      while (paramlist_.good()) {
        bzero(buffer_, buffer_len_);
        paramlist_.getline(buffer_, buffer_len_);
        PerishableStringTokenizer st_(buffer_, buffer_len_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() >= 3) {
          if (strncmp(tokens_[0], "#", 1) == 0) {
            continue;
          }
          if (strcmp(tokens_[0], "DEFAULT") == 0 || strcmp(tokens_[1], "DEFAULT") == 0) {
            paramfile_ = tokens_[2];
            continue;
          }
          int start_date_ = DateTime::GetIsoDateFromString(std::string(tokens_[0]));
          int end_date_ = DateTime::GetIsoDateFromString(std::string(tokens_[1]));
          if (_tradingdate_ >= start_date_ && _tradingdate_ <= end_date_) {
            paramfile_ = tokens_[2];
            break;
          }
        }
      }
    }
    paramlist_.close();
  }
  return paramfile_;
}
