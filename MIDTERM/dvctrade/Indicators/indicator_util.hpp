/**
   \file Indicators/indicator_util.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_INDICATOR_UTIL_H
#define BASE_INDICATORS_INDICATOR_UTIL_H

#include <string>
#include <vector>
#include <stdlib.h>

#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/portfolio_constituent_manager.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

#define MIN_MSEC_HISTORY_INDICATORS 1  // setting it closer to rtt values
//#define PORTFOLIO_RETURNS_FILE "/home/diwakar/portfolios/portfolio_data.txt"
#define PORTFOLIO_RETURNS_FILE "/spare/local/tradeinfo/ReturnsInfo/portfolio_data.txt"

#define STDEV_RATIO_CAP 4.00

namespace HFSAT {

/// Set of utilities to work with PortfolioConstituentUtil and PortfolioConstituentManager
/// Mostly called from SimpleTrendPort and SimpleTrendCombo sort of indicators
namespace IndicatorUtil {

inline void GetLastTwoArgsFromIndicatorTokens(int start_index, const std::vector<const char *> indicator_tokens,
                                              double &first_arg, PriceType_t &price_type) {
  if ((int)indicator_tokens.size() > start_index) {
    if (std::string(indicator_tokens[start_index]).compare("#") == 0) {
      // INDICATOR 1.0 MultMktPrice FESX_0 5 0.8 # index 6
    } else {
      if ((int)indicator_tokens.size() > start_index + 1) {
        if (std::string(indicator_tokens[start_index + 1]).compare("#") == 0) {
          // INDICATOR 1.0 MultMktPrice FESX_0 5 0.8 MidPrice # index 6
          price_type = StringToPriceType_t(indicator_tokens[start_index]);
        } else {
          // INDICATOR 1.0 MultMktPrice FESX_0 5 0.8 300 MidPrice # index 6
          first_arg = atof(indicator_tokens[start_index]);
          price_type = StringToPriceType_t(indicator_tokens[start_index + 1]);
        }
      } else {
        price_type = StringToPriceType_t(indicator_tokens[start_index]);
      }
    }
  }
}

inline void AddPortfolioShortCodeVec(const std::string &_portfolio_descriptor_shortcode_,
                                     std::vector<std::string> &_shortcode_vec_) {
  PcaWeightsManager::GetUniqueInstance().AddPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, _shortcode_vec_);
}

inline void GetNormalizedL1EventsForPort(const std::string &_portfolio_descriptor_shortcode_,
                                         unsigned int &indep_num_events_, const double _fractional_seconds_,
                                         const int _tradingdate_) {
  std::vector<std::string> _shortcode_vec_;
  PcaWeightsManager::GetUniqueInstance().AddPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, _shortcode_vec_);

  double port_avg_l1_events_per_sec_ = 0;

  for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
    port_avg_l1_events_per_sec_ +=
        HFSAT::SampleDataUtil::GetAvgForPeriod(_shortcode_vec_[i], _tradingdate_, 60, std::string("L1EVPerSec"));
  }

  indep_num_events_ = (unsigned int)std::max(1.00, port_avg_l1_events_per_sec_ * _fractional_seconds_);
}

inline void GetNormalizedL1EventsForPort(const std::string &_portfolio_descriptor_shortcode_,
                                         std::string _dep_shortcode_, const unsigned int dep_num_events_,
                                         unsigned int &indep_num_events_, const int _tradingdate_) {
  std::vector<std::string> _shortcode_vec_;
  PcaWeightsManager::GetUniqueInstance().AddPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, _shortcode_vec_);

  double dep_avg_l1_events_per_sec_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(_dep_shortcode_, _tradingdate_, 60, std::string("L1EVPerSec"));

  double port_avg_l1_events_per_sec_ = 0;

  for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
    port_avg_l1_events_per_sec_ +=
        HFSAT::SampleDataUtil::GetAvgForPeriod(_shortcode_vec_[i], _tradingdate_, 60, std::string("L1EVPerSec"));
  }

  indep_num_events_ = 1;

  if (dep_avg_l1_events_per_sec_ > 0.001) {
    indep_num_events_ =
        (unsigned int)std::max(1.00, port_avg_l1_events_per_sec_ * dep_num_events_ / dep_avg_l1_events_per_sec_);
  }
}

inline void GetNormalizedL1EventsForIndep(const std::string _indep_shortcode_, std::string _dep_shortcode_,
                                          const unsigned int dep_num_events_, unsigned int &indep_num_events_,
                                          const int _tradingdate_) {
  double dep_avg_l1_events_per_sec_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(_dep_shortcode_, _tradingdate_, 60, std::string("L1EVPerSec"));

  double indep_avg_l1_events_per_sec_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(_indep_shortcode_, _tradingdate_, 60, std::string("L1EVPerSec"));

  indep_num_events_ = 1;

  if (dep_avg_l1_events_per_sec_ > 0.001) {
    indep_num_events_ =
        (unsigned int)std::max(1.00, indep_avg_l1_events_per_sec_ * dep_num_events_ / dep_avg_l1_events_per_sec_);
  }
}

inline void GetOfflineReturnsRatio(std::string dep_shortcode_, std::string indep_shortcode_,
                                   double &offline_returns_ratio_) {
  std::ifstream t_offline_returns_ratio_infile_;
  bool offline_returns_ratio_found_ = false;
  t_offline_returns_ratio_infile_.open("/spare/local/tradeinfo/OfflineInfo/offline_returns_ratio_file.txt",
                                       std::ifstream::in);
  if (t_offline_returns_ratio_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_offline_returns_ratio_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_offline_returns_ratio_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 3) {
        std::string file_dep_shortcode_ = tokens_[0];
        std::string file_indep_shortcode_ = tokens_[1];
        if ((dep_shortcode_.compare(file_dep_shortcode_) == 0) &&
            (indep_shortcode_.compare(file_indep_shortcode_) == 0)) {
          offline_returns_ratio_ =
              std::max(0.0, atof(tokens_[2]));  // AA - can't think of a case where this would need to -ve
          offline_returns_ratio_found_ = true;
        }
      }
    }
  }
  if (!offline_returns_ratio_found_) {
    offline_returns_ratio_ = 1.0;
  }
}

inline void LoadDI1EigenVector(const std::string &_portfolio_descriptor_shortcode_, std::vector<double> _weights_,
                               int _date_, int pc_index_) {
  std::ifstream t_di1_pca_infile_;
  std::string t_di1_pca_infilename_;

  int this_YYYYMMDD_ = DateTime::CalcPrevDay(_date_);  // Start yesterday.

  for (unsigned int ii = 0; ii < 40; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/PCAInfo/pca_eigen." << this_YYYYMMDD_;
    t_di1_pca_infilename_ = t_temp_oss_.str();

    int t_di1_pca_file_size_ = 0;

    if (FileUtils::ExistsWithSize(t_di1_pca_infilename_, t_di1_pca_file_size_)) {
      if (t_di1_pca_file_size_ > 0) break;
    } else {
      this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
  }

  int t_di1_pca_file_size_ = 0;

  if (!FileUtils::ExistsWithSize(t_di1_pca_infilename_, t_di1_pca_file_size_)) {  // All attempts failed
  }

  t_di1_pca_infile_.open(t_di1_pca_infilename_.c_str());

  if (t_di1_pca_infile_.is_open()) {
    const int kL1AvgBufferLen = 4096;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);

    while (t_di1_pca_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_di1_pca_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      std::string str_pc_index_ = "PC1";
      if (pc_index_ == 2) {
        str_pc_index_ = "PC2";
      }

      if (tokens_.size() > 2 && (strcmp(tokens_[0], _portfolio_descriptor_shortcode_.c_str()) == 0) &&
          (strcmp(tokens_[1], str_pc_index_.c_str()) == 0)) {
        for (unsigned int i = 2; i < tokens_.size(); i++) {
          _weights_.push_back(atof(tokens_[i]));
        }
      }
    }
  }
}

inline void AddDIPortfolioShortCodeVec(const std::string &_portfolio_descriptor_shortcode_,
                                       std::vector<std::string> &_shortcode_vec_, int _date_) {
  std::ifstream t_di1_pca_infile_;
  std::string t_di1_pca_infilename_;

  if (_date_ < 0) {
    t_di1_pca_infilename_ = "/spare/local/tradeinfo/StructureInfo/PCAInfo/pca_eigen_default";
  } else {
    int this_YYYYMMDD_ = DateTime::CalcPrevDay(_date_);  // Start yesterday.

    for (unsigned int ii = 0; ii < 40; ii++) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/PCAInfo/pca_eigen." << this_YYYYMMDD_;
      t_di1_pca_infilename_ = t_temp_oss_.str();

      int t_di1_pca_file_size_ = 0;

      if (FileUtils::ExistsWithSize(t_di1_pca_infilename_, t_di1_pca_file_size_)) {
        if (t_di1_pca_file_size_ > 0) break;
      } else {
        this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
      }
    }

    int t_di1_pca_file_size_ = 0;

    if (!FileUtils::ExistsWithSize(t_di1_pca_infilename_, t_di1_pca_file_size_)) {  // All attempts failed
      t_di1_pca_infilename_ = "/spare/local/tradeinfo/StructureInfo/PCAInfo/pca_eigen_default";
    }
  }

  t_di1_pca_infile_.open(t_di1_pca_infilename_.c_str());

  if (t_di1_pca_infile_.is_open()) {
    const int kL1AvgBufferLen = 4096;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);

    while (t_di1_pca_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_di1_pca_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() > 2 && strcmp(tokens_[0], _portfolio_descriptor_shortcode_.c_str()) == 0) {
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          _shortcode_vec_.push_back(std::string(tokens_[i]));
        }
      }
    }
  }
}

inline double GetMinPriceIncrementForPricePortfolio(const std::string &portfolio_shortcode, int tradingdate) {
  double min_price_increment = 0.0;
  std::string line;
  std::ifstream Portfoliofile(PAIRSTRADEFILE);
  bool flag_port_found = false;
  if (Portfoliofile.is_open()) {
    while (getline(Portfoliofile, line)) {
      std::istringstream buf(line);
      std::istream_iterator<std::string> beg(buf), end;

      std::vector<std::string> tokens(beg, end);
      if (tokens[0] == portfolio_shortcode) {
        flag_port_found = true;
        int half = tokens.size() / 2;
        for (unsigned int index = 1; index <= tokens.size() / 2; index++) {
          min_price_increment += atof(tokens[index + half].c_str()) *
                                 SecurityDefinitions::GetContractMinPriceIncrement(tokens[index], tradingdate);
        }
        break;
      }
    }
  } else {
    std::cerr << "PricePortfolio::Portfolio file not accessible" << portfolio_shortcode << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, portfolio_shortcode.c_str());
  }

  if (!flag_port_found) {
    min_price_increment = 1.0;
  }
  return min_price_increment;
}

inline void CollectShortcodeOrPortfolio(std::vector<std::string> &shortcodes_affecting_this_indicator,
                                        std::vector<std::string> &ors_source_needed_vec,
                                        const std::vector<const char *> &r_tokens) {
  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit((std::string)r_tokens[3])) {
    std::string t_portfolio_descriptor_shortcode_ = (std::string)r_tokens[3];
    std::string line;
    std::ifstream Portfoliofile(PAIRSTRADEFILE);
    bool flag_port_found_ = false;
    if (Portfoliofile.is_open()) {
      while (getline(Portfoliofile, line)) {
        std::istringstream buf(line);
        std::istream_iterator<std::string> beg(buf), end;

        std::vector<std::string> tokens(beg, end);
        if (tokens.size() != 0) {
          if (tokens[0] == t_portfolio_descriptor_shortcode_) {
            flag_port_found_ = true;
            for (unsigned int index = 1; index <= tokens.size() / 2; index++) {
              shortcodes_affecting_this_indicator.push_back(tokens[index]);
            }
            break;
          }
        }
      }
    } else {
      std::cerr << "PricePortfolio::Portfolio file not accessible" << t_portfolio_descriptor_shortcode_ << std::endl;
      ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
    }
    if (flag_port_found_ == false) {
      std::cerr << "PricePortfolio::PortfolioMissing" << t_portfolio_descriptor_shortcode_ << std::endl;
      ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
    }
  } else {
    VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, (std::string)r_tokens[3]);
  }
}

inline bool IsPortfolioShortcode(const std::string &_portfolio_descriptor_shortcode_) {
  return (PcaWeightsManager::GetUniqueInstance()).IsPortfolioShortcode(_portfolio_descriptor_shortcode_);
}

inline void GetPortfolioShortCodeVec(const std::string &_portfolio_descriptor_shortcode_,
                                     std::vector<std::string> &_shortcode_vec_) {
  PcaWeightsManager::GetUniqueInstance().GetPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, _shortcode_vec_);
}

inline void GetPortfolioSMVVec(const std::string &_portfolio_descriptor_shortcode_,
                               std::vector<SecurityMarketView *> &_indep_market_view_vec_) {
  std::vector<std::string> shortcode_vec_;
  GetPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, shortcode_vec_);
  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, _indep_market_view_vec_);
}

inline bool IsEquityShortcode(const std::string &t_shortcode_) {
  return (SecurityDefinitions::GetContractExchSource(t_shortcode_, 0) == kExchSourceBMFEQ);
}

inline bool IsEquityPortfolio(const std::string &t_portfolio_descriptor_shortcode_) {
  bool is_equity_ = true;
  std::vector<std::string> shortcode_vec_;

  PcaWeightsManager::GetUniqueInstance().GetPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, shortcode_vec_);

  for (auto i = 0u; i < shortcode_vec_.size(); ++i) {
    if (!IsEquityShortcode(shortcode_vec_[i])) {
      is_equity_ = false;
      break;
    }
  }

  return is_equity_;
}

inline bool IndicatorHasExch(const std::string &_ind_str_, ExchSource_t _exch_) {
  const size_t t_len_ = strlen(_ind_str_.c_str()) + 1;
  char temp_str_[t_len_];
  strncpy(temp_str_, _ind_str_.c_str(), t_len_);
  temp_str_[t_len_] = '\0';
  PerishableStringTokenizer st_(temp_str_, t_len_);
  std::vector<const char *> tokens_ = st_.GetTokens();
  int t_date_ = DateTime::GetCurrentIsoDateUTC();
  for (unsigned int i = 3; i < tokens_.size(); i++) {
    if (tokens_[i][0] == '#') {
      break;
    }
    std::string t_shc_ = std::string(tokens_[i]);
    std::vector<std::string> t_shc_vec_;
    if (IsPortfolioShortcode(t_shc_)) {
      GetPortfolioShortCodeVec(t_shc_, t_shc_vec_);
    } else {
      t_shc_vec_.push_back(t_shc_);
    }
    for (unsigned int j = 0; j < t_shc_vec_.size(); j++) {
      if (SecurityDefinitions::CheckIfContractSpecExists(t_shc_vec_[j], t_date_) &&
          SecurityDefinitions::GetContractExchSource(t_shc_vec_[j], t_date_) == _exch_) {
        return true;
      }
    }
  }
  return false;
}
}
}

#endif  // BASE_INDICATORS_INDICATOR_UTIL_H
