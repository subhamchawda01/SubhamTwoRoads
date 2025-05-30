/**
   \file IndicatorsCode/portfolio_constituent_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvctrade/Indicators/portfolio_constituent_manager.hpp"

namespace HFSAT {

inline std::string ToString(int t_intdate_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << t_intdate_;
  return t_temp_oss_.str();
}

void PortfolioConstituentManager::LoadPortfolioInfoFile() {
  std::string _portfolio_info_filename_prefix_ =
      std::string(BASETRADEINFODIR) + "PortfolioInfo/portfolio_constituent_info_";
  std::string _portfolio_info_filename_ = _portfolio_info_filename_prefix_ + "default.txt";

  std::map<std::string, unsigned int> date_first_seen_on_;

  int t_intdate_ = DateTime::GetCurrentIsoDateLocal();  // watch_.YYYYMMDD ( ) ;
  _portfolio_info_filename_ = _portfolio_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  for (auto i = 0u; i < 30; i++) {
    if (FileUtils::exists(_portfolio_info_filename_)) {
      LoadFromFile(_portfolio_info_filename_, date_first_seen_on_, t_intdate_);
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
    _portfolio_info_filename_ = _portfolio_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  }

  _portfolio_info_filename_ = _portfolio_info_filename_prefix_ + "default.txt";
  if (!FileUtils::exists(_portfolio_info_filename_)) {
    std::cerr << "PortfolioConstituentManager::LoadPortfolioInfoFile even " << _portfolio_info_filename_
              << " does not exist" << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }
  LoadFromFile(_portfolio_info_filename_, date_first_seen_on_, 0);
}

void PortfolioConstituentManager::LoadFromFile(const std::string& _portfolio_info_filename_,
                                               std::map<std::string, unsigned int>& date_first_seen_on_,
                                               const unsigned int t_intdate_) {
  std::ifstream portfolio_description_file_;
  portfolio_description_file_.open(_portfolio_info_filename_.c_str(), std::ifstream::in);
  if (portfolio_description_file_.is_open()) {
    const int kPortfolioDescriptionLen = 10240;
    char readline_buffer_[kPortfolioDescriptionLen];
    bzero(readline_buffer_, kPortfolioDescriptionLen);

    while (portfolio_description_file_.good()) {
      bzero(readline_buffer_, kPortfolioDescriptionLen);
      portfolio_description_file_.getline(readline_buffer_, kPortfolioDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kPortfolioDescriptionLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if ((tokens_.size() > 2) && (tokens_[0][0] != '#')) {  // at least one element in the portfolio
        std::string _this_portfolio_descriptor_shortcode_ = tokens_[0];

        if ((date_first_seen_on_.find(_this_portfolio_descriptor_shortcode_) == date_first_seen_on_.end()) ||
            (date_first_seen_on_[_this_portfolio_descriptor_shortcode_] <= t_intdate_)) {
          PortfolioConstituentVec _this_portfolio_constituent_vec_;

          for (unsigned int i = 1; i < tokens_.size(); i++) {
            PortfolioConstituent_t this_pc_;
            this_pc_.shortcode_ = tokens_[i];
            i++;
            if (i < tokens_.size()) {
              this_pc_.weight_ = atof(tokens_[i]);
            } else {
              ExitVerbose(kPortfolioConstituentManagerMissingArgs);
              this_pc_.weight_ = 1.00;
            }
            _this_portfolio_constituent_vec_.push_back(this_pc_);
          }

          // this ensures that we take the values in the last line if there are multiple lines corresponding to the same
          // shortcode
          shortcode_constituentvec_map_[_this_portfolio_descriptor_shortcode_] = _this_portfolio_constituent_vec_;
          date_first_seen_on_[_this_portfolio_descriptor_shortcode_] = t_intdate_;
        }
      }
    }

    portfolio_description_file_.close();
  }
}
}
