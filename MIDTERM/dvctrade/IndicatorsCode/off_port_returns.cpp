/**
    \file IndicatorsCode/pcr_port_returns.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/off_port_returns.hpp"

// define the folder paths for offline files
#define PORTFOLIO_COMPONENTS_FOLDER "/spare/local/tradeinfo/NSE_Files/PORT/Components/"
#define BETA_FOLDER "/spare/local/tradeinfo/NSE_Files/PORT/BETA/"

namespace HFSAT {

void OffPortReturns::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  std::vector<std::string> cons_list_;
  std::ifstream shortcode_list_file;

  std::string shortcode_list_file_location_ = PORTFOLIO_COMPONENTS_FOLDER + (std::string)r_tokens_[3];
  // DBGLOG_TIME_CLASS_FUNC_LINE << " Constituent list file picked: " << shortcode_list_file_location_ <<
  // DBGLOG_ENDL_FLUSH;
  shortcode_list_file.open(shortcode_list_file_location_.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() != 1) {
        continue;
      }
      cons_list_.push_back(tokens_[0]);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, cons_list_);
}

OffPortReturns* OffPortReturns::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_ _shortcode_ _sourcealgo_ _fractional_seconds_
  // _lookback_days_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], (std::string)r_tokens_[4],
                           (std::string)r_tokens_[5], atof(r_tokens_[6]), atoi(r_tokens_[7]),
                           StringToPriceType_t(r_tokens_[8]));
}

OffPortReturns* OffPortReturns::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::string& _portfolio_, const std::string& _shortcode_,
                                                  const std::string& _sourcealgo_, double _fractional_seconds_,
                                                  int _lookback_days_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_ << ' ' << _shortcode_ << ' ' << _sourcealgo_ << ' '
              << _fractional_seconds_ << ' ' << _lookback_days_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OffPortReturns*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OffPortReturns(t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_, _shortcode_,
                           _sourcealgo_, _fractional_seconds_, _lookback_days_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OffPortReturns::OffPortReturns(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_, const std::string& _portfolio_,
                               const std::string& _shortcode_, const std::string& _sourcealgo_,
                               double _fractional_seconds_, int _lookback_days_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      portfolio_(_portfolio_),
      shortcode_(_shortcode_),
      sourcealgo_(_sourcealgo_),
      returns_duration_(_fractional_seconds_),
      lookback_days_(_lookback_days_),
      price_type_(_price_type_) {
  Initialize();
  AddIndicatorListener();

#if EQUITY_INDICATORS_ALWAYS_READY
  if (IndicatorUtil::IsEquityPortfolio(indep_portfolio_price__->shortcode())) {
    is_ready_ = true;
    InitializeValues();
  }
#endif
}
void OffPortReturns::WhyNotReady() {
  if (!is_ready_) {
    for (int i = 0; i < total_products_; i++) {
      if ((indep_market_view_vec_[i] != NULL) && !(is_ready_vec_[i])) {
        if (indep_market_view_vec_[i]->is_ready() &&
            (!(indep_market_view_vec_[i]->IsBidBookEmpty() || indep_market_view_vec_[i]->IsAskBookEmpty()))) {
          is_ready_vec_[i] = true;
        } else {
          DBGLOG_TIME_CLASS << indep_market_view_vec_[i]->secname() << " is_ready() = false " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
      }
    }
  }
}

void OffPortReturns::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  is_ready_vec_[_security_id_] = false;
  is_ready_ = false;
  CheckForReady();
  NotifyIndicatorListeners(indicator_value_);
}

void OffPortReturns::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }

void OffPortReturns::CheckForReady() {
  is_ready_ = true;
  for (int i = 0; i < total_products_; i++) {
    if (!is_ready_vec_[i]) {
      is_ready_ = false;
      // DBGLOG_TIME_CLASS << shortcode_vec_[i] << " is_ready() = false " << DBGLOG_ENDL_FLUSH;
      // DBGLOG_DUMP;
      break;
    }
  }
  // initialize the indicator value once is_ready_ is true.
  if (is_ready_) {
    indicator_value_ = 0;
    for (int j = 0; j < total_products_; j++) {
      indicator_value_ += (prev_return_vec_[j] * beta_vector_[j]);
    }
  }
}

void OffPortReturns::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_ret_value_) {
  // calculate and update the indicator value
  if (is_ready_) {
    double change_in_return_ = _new_ret_value_ - prev_return_vec_[_indicator_index_];
    indicator_value_ += beta_vector_[_indicator_index_] * change_in_return_;
    prev_return_vec_[_indicator_index_] = _new_ret_value_;
    NotifyIndicatorListeners(indicator_value_);
  } else {
    is_ready_vec_[_indicator_index_] = true;
    prev_return_vec_[_indicator_index_] = _new_ret_value_;
    CheckForReady();
  }
}

void OffPortReturns::AddIndicatorListener() {
  for (unsigned i = 0; i < shortcode_vec_.size(); i++) {
    returns_indicator_vec_.push_back(SimpleReturns::GetUniqueInstance(
        dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_vec_[i]),
        returns_duration_, kPriceTypeMidprice));
    returns_indicator_vec_[i]->add_unweighted_indicator_listener(i, this);
  }
}

std::string OffPortReturns::GetLatestFile(const std::string& path_) {
  int date_ = (int)watch_.YYYYMMDD();
  int previous_date_ = date_;
  std::string complete_path_ = "";
  for (int i = 0; i < 90; i++) {
    int current_date_ = previous_date_;
    // std::cout << current_date_ << "\n";
    complete_path_ = path_ + "/" + std::to_string(current_date_) + "/" + shortcode_;
    if (DoesFileExist(complete_path_)) {
      file_extraction_day_ = current_date_;
      break;
    }
    previous_date_ = HFSAT::DateTime::CalcPrevDay(current_date_);
  }
  return complete_path_;
}

void OffPortReturns::Initialize() {
  // read beta file, this is done in 2 steps
  // Step1 = get appropriate file, we want the latest file present for the day or before the day of trading.
  // step2 = read the file and store the betas and source

  // Step1
  std::string beta_folder_location_ =
      BETA_FOLDER + sourcealgo_ + "/" + portfolio_ + "/" + std::to_string(lookback_days_);
  std::string beta_file_location_ = GetLatestFile(beta_folder_location_);

  // Add the file readed to debuglogger
  DBGLOG_TIME_CLASS_FUNC_LINE << " Betas file picked: " << beta_file_location_ << DBGLOG_ENDL_FLUSH;

  // Step2
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(beta_file_location_.c_str(), std::ifstream::in);
  int security_id_ = 0;
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }

      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() <= 1) {
        continue;
      }
      if (std::string(tokens_[0]) == "SOURCE") {
        for (int i = 3; i < (int)tokens_.size(); i++) shortcode_vec_.push_back(tokens_[i]);
      }
      if (std::string(tokens_[0]) == "BETA") {
        for (int i = 1; i < (int)tokens_.size(); i++) beta_vector_.push_back(atof(tokens_[i]));
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }

  total_products_ = shortcode_vec_.size();
  indep_market_view_vec_.resize(total_products_, NULL);
  for (int i = 0; i < total_products_; i++) {
    indep_market_view_vec_[security_id_] =
        (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_vec_[i]));
  }

  // fill vectors with zero
  prev_return_vec_.resize(total_products_, 0);
  is_ready_vec_.resize(total_products_, false);
  indicator_value_ = 0;
}
}
