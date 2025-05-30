#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <list>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define PERIODIC_L1NORM_FOLDER "/spare/local/L1Norms"
#define NUM_MAX_DAYS_TO_LOOK 50

namespace HFSAT {
inline float LoadMeanL1Norm(int _YYYYMMDD, const std::string& symbol, int num_trading_days_) {
  std::string _l1norm_symbol_avg_filename = "";
  int this_YYYYMMDD = HFSAT::DateTime::CalcPrevDay(_YYYYMMDD);
  {
    std::ostringstream t_temp_oss;
    t_temp_oss << PERIODIC_L1NORM_FOLDER << "/" << this_YYYYMMDD << "/" << symbol << "_l1norm_value";
    _l1norm_symbol_avg_filename = t_temp_oss.str();
  }
  int total_days_checked_ = 0;
  int total_days_found_ = 0;
  int num_days_look_back_max_ = NUM_MAX_DAYS_TO_LOOK;
  float sum_l1_norm_ = 0;
  while (total_days_checked_ < num_days_look_back_max_ && total_days_found_ < num_trading_days_) {
    if (HFSAT::FileUtils::exists(_l1norm_symbol_avg_filename)) {
      std::ifstream l1norm_symbol_file_stream;
      l1norm_symbol_file_stream.open(_l1norm_symbol_avg_filename.c_str(), std::ifstream::in);
      if (l1norm_symbol_file_stream.is_open()) {
        const int BUFFER_LEN = 1024;
        char readline_buffer[BUFFER_LEN];
        if (l1norm_symbol_file_stream.good()) {
          bzero(readline_buffer, BUFFER_LEN);
          l1norm_symbol_file_stream.getline(readline_buffer, BUFFER_LEN);
          HFSAT::PerishableStringTokenizer st_(readline_buffer, BUFFER_LEN);
          const std::vector<const char*>& tokens_ = st_.GetTokens();
          if (tokens_.size() == 2) {
            float l1_norm_ = atof(tokens_[0]);
            if (l1_norm_ > 0) {
              sum_l1_norm_ += l1_norm_;
              total_days_found_++;
            }
          }
        }
      }
      l1norm_symbol_file_stream.close();
    }
    total_days_checked_++;
    this_YYYYMMDD = HFSAT::DateTime::CalcPrevDay(this_YYYYMMDD);
    std::ostringstream t_temp_oss;

    t_temp_oss << PERIODIC_L1NORM_FOLDER << "/" << this_YYYYMMDD << "/" << symbol << "_l1norm_value";
    _l1norm_symbol_avg_filename = t_temp_oss.str();
  }
  if (total_days_found_ > 0) {
    return sum_l1_norm_ / total_days_found_;
  } else {
    ExitVerbose(kExitErrorCodeGeneral, "did not find positive number of days");
  }
  return 1.0;
}

inline float LoadMeanL1NormPort(int _YYYYMMDD, const std::string& portfolio_descriptor_shortcode_,
                                int num_trading_days_) {
  std::vector<std::string> shortcode_vec_;
  IndicatorUtil::GetPortfolioShortCodeVec(portfolio_descriptor_shortcode_, shortcode_vec_);
  double weighted_l1_norm_ = 0;
  const EigenConstituentsVec& eigen_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(portfolio_descriptor_shortcode_);
  const std::vector<double>& stdev_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(portfolio_descriptor_shortcode_);

  if (eigen_constituent_vec.empty()) {
    std::cerr << "PCAPortPrice::EigenConstituentsVec " << portfolio_descriptor_shortcode_
              << "does not have even single eigenvectors/values computed " << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, portfolio_descriptor_shortcode_.c_str());
  }

  if (stdev_constituent_vec.size() != shortcode_vec_.size()) {
    std::cerr << "PCAPortPrice::Stdevs computed for " << stdev_constituent_vec.size() << " but portfolio size is "
              << shortcode_vec_.size() << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, portfolio_descriptor_shortcode_.c_str());
  }

  if (!eigen_constituent_vec.empty()) {
    if (eigen_constituent_vec[0].eigenvec_components_.size() != shortcode_vec_.size()) {
      std::cerr << "PCAPortPrice:: PCA EIgenvector components size"
                << eigen_constituent_vec[0].eigenvec_components_.size() << " but portfolio size is "
                << shortcode_vec_.size() << std::endl;
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, portfolio_descriptor_shortcode_.c_str());
    }
  }
  double sum_of_eigen_compo_by_stdev_ = 0;

  for (unsigned int t_shortcode_vec_index_ = 0; t_shortcode_vec_index_ < shortcode_vec_.size();
       t_shortcode_vec_index_++) {
    if (stdev_constituent_vec[t_shortcode_vec_index_] > 0) {
      sum_of_eigen_compo_by_stdev_ += (eigen_constituent_vec[0].eigenvec_components_[t_shortcode_vec_index_] /
                                       stdev_constituent_vec[t_shortcode_vec_index_]);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, "stdev <= 0 in PCAPortPrice");
    }
  }

  for (unsigned int ii = 0u; ii < shortcode_vec_.size(); ii++) {
    double t_thisweight_ = (eigen_constituent_vec[0].eigenvec_components_[ii] / stdev_constituent_vec[ii]);
    t_thisweight_ /= sum_of_eigen_compo_by_stdev_;
    weighted_l1_norm_ += t_thisweight_ * LoadMeanL1Norm(_YYYYMMDD, shortcode_vec_[ii], num_trading_days_);
  }
  return weighted_l1_norm_;
}
}
