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
#include "dvctrade/Indicators/pcr_port_returns.hpp"

// define the folder paths for offline files
#define PORTFOLIO_COMPONENTS_FOLDER "/spare/local/tradeinfo/NSE_Files/Components/"
#define EIGEN_MARTIX_FOLDER "/spare/local/tradeinfo/NSE_Files/Eigenmatrix/"
#define BETA_FOLDER "/spare/local/tradeinfo/NSE_Files/Portbeta/"
#define STDEV_FOLDER "/spare/local/tradeinfo/NSE_Files/Stdevport/"

namespace HFSAT {

void PCRPortReturns::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
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
      std::string t_shc_ = "NSE_";
      t_shc_.append(tokens_[0]);
      t_shc_ = t_shc_ + "_FUT0";
      cons_list_.push_back(t_shc_);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, cons_list_);
}

PCRPortReturns* PCRPortReturns::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_ _shortcode_ _fractional_seconds_
  // _lookback_days_ _num_eigenvectors_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], (std::string)r_tokens_[4],
                           atof(r_tokens_[5]), atoi(r_tokens_[6]), atoi(r_tokens_[7]), atoi(r_tokens_[8]),
                           StringToPriceType_t(r_tokens_[9]));
}

PCRPortReturns* PCRPortReturns::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::string& _portfolio_, const std::string& _shortcode_,
                                                  double _fractional_seconds_, int _lookback_days_,
                                                  int _num_eigenvectors_, int _mode_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_ << ' ' << _shortcode_ << ' ' << _fractional_seconds_ << ' '
              << _lookback_days_ << ' ' << _num_eigenvectors_ << ' ' << _mode_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, PCRPortReturns*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new PCRPortReturns(t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_, _shortcode_,
                           _fractional_seconds_, _lookback_days_, _num_eigenvectors_, _mode_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

PCRPortReturns::PCRPortReturns(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_, const std::string& _portfolio_,
                               const std::string& _shortcode_, double _fractional_seconds_, int _lookback_days_,
                               int _num_eigenvectors_, int _mode_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      portfolio_(_portfolio_),
      shortcode_(_shortcode_),
      returns_duration_(_fractional_seconds_),
      lookback_days_(_lookback_days_),
      num_eigenvectors_(_num_eigenvectors_),
      mode_(_mode_),
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
void PCRPortReturns::WhyNotReady() {
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

void PCRPortReturns::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  is_ready_vec_[_security_id_] = false;
  is_ready_ = false;
  CheckForReady();
  NotifyIndicatorListeners(indicator_value_);
}

void PCRPortReturns::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }

void PCRPortReturns::CheckForReady() {
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
  // This is doing the whole matrix multiplication in one go to get the indicator value
  if (is_ready_) {
    indicator_value_ = 0;
    for (int i = 0; i < num_eigenvectors_; i++) {
      for (int j = 0; j < total_products_; j++) {
        indicator_value_ += (prev_return_vec_[j] * eigen_matrix_[j][i] * beta_vector_[i]);
        if (mode_ == 1) {
          indicator_value_ +=
              (prev_return_vec_[j] * eigen_matrix_[j][total_products_ - i - 1] * beta_vector_[total_products_ - i - 1]);
        }
      }
    }
  }
}

void PCRPortReturns::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_ret_value_) {
  // calculate and update the indicator value
  double normal_new_ret_value_ = _new_ret_value_ / stdev_vector_[_indicator_index_];
  if (is_ready_) {
    double change_in_return_ = normal_new_ret_value_ - prev_return_vec_[_indicator_index_];
    for (int i = 0; i < num_eigenvectors_; i++) {
      indicator_value_ += beta_vector_[i] * change_in_return_ * eigen_matrix_[_indicator_index_][i];
      if (mode_ == 1) {
        indicator_value_ += beta_vector_[total_products_ - i - 1] * change_in_return_ *
                            eigen_matrix_[_indicator_index_][total_products_ - i - 1];
      }
    }
    prev_return_vec_[_indicator_index_] = normal_new_ret_value_;
    NotifyIndicatorListeners(indicator_value_);
  } else {
    is_ready_vec_[_indicator_index_] = true;
    prev_return_vec_[_indicator_index_] = normal_new_ret_value_;
    CheckForReady();
  }
}

void PCRPortReturns::AddIndicatorListener() {
  for (unsigned i = 0; i < shortcode_vec_.size(); i++) {
    returns_indicator_vec_.push_back(SimpleReturns::GetUniqueInstance(
        dbglogger_, watch_, *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_vec_[i]),
        returns_duration_, kPriceTypeMidprice));
    returns_indicator_vec_[i]->add_unweighted_indicator_listener(i, this);
  }
}

std::string PCRPortReturns::GetLatestFile(const std::string& path_) {
  int date_ = (int)watch_.YYYYMMDD();
  // std::cout << "todays date = " << date_ << "\n";
  int previous_date_ = date_;
  std::string complete_path_ = "";
  for (int i = 0; i < 90; i++) {
    int current_date_ = previous_date_;
    // std::cout << current_date_ << "\n";
    complete_path_ = path_ + "/" + std::to_string(current_date_) + "_vec";
    if (DoesFileExist(complete_path_)) {
      file_extraction_day_ = current_date_;
      break;
    }
    previous_date_ = HFSAT::DateTime::CalcPrevDay(current_date_);
  }
  return complete_path_;
}

void PCRPortReturns::Initialize() {
  // read component file
  std::ifstream shortcode_list_file;
  std::string shortcode_list_file_location_ = PORTFOLIO_COMPONENTS_FOLDER + portfolio_;
  DBGLOG_TIME_CLASS_FUNC_LINE << " Constituent list file picked: " << shortcode_list_file_location_
                              << DBGLOG_ENDL_FLUSH;
  shortcode_list_file.open(shortcode_list_file_location_.c_str(), std::ifstream::in);

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
      if (tokens_.size() != 1) {
        continue;
      }
      std::string t_shc_ = "NSE_";
      t_shc_.append(tokens_[0]);
      t_shc_ = t_shc_ + "_FUT0";
      shortcode_vec_.push_back(t_shc_);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }
  total_products_ = shortcode_vec_.size();
  indep_market_view_vec_.resize(total_products_, NULL);
  for (int i = 0; i < total_products_; i++) {
    indep_market_view_vec_[security_id_] =
        (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcode_vec_[i]));
  }

  // read eigen matrix, this is done in 2 steps
  // Step1 = get appropriate file, we want the latest file present for the day or before the day of trading.
  // step2 = read the file and store the eigen matrix
  // Step1:
  std::string ev_folder_path_ = EIGEN_MARTIX_FOLDER + portfolio_ + "/" + std::to_string(lookback_days_) + "";
  std::string ev_latest_file_location_ = GetLatestFile(ev_folder_path_);

  // Step2:
  std::ifstream ev_latest_file_;
  DBGLOG_TIME_CLASS_FUNC_LINE << "Eigen matrix picked from the file: " << ev_latest_file_location_ << DBGLOG_ENDL_FLUSH;
  ev_latest_file_.open(ev_latest_file_location_.c_str(), std::ifstream::in);
  if (ev_latest_file_.is_open()) {
    while (ev_latest_file_.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      ev_latest_file_.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }

      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() != unsigned(total_products_)) {
        continue;
      }

      std::vector<double> temp_;
      for (int i = 0; i < total_products_; i++) temp_.push_back(atof(tokens_[i]));
      eigen_matrix_.push_back(temp_);
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }

  // read beta values
  std::string short_shortcode_ = shortcode_.substr(4, shortcode_.length() - 9);
  std::string beta_latest_file_location_ = BETA_FOLDER + portfolio_ + "/" + std::to_string(lookback_days_) + "/" +
                                           std::to_string(file_extraction_day_) + "/" + short_shortcode_ + "/" +
                                           std::to_string(total_products_);

  std::ifstream beta_latest_file_;
  DBGLOG_TIME_CLASS_FUNC_LINE << "Betas picked from the file: " << beta_latest_file_location_ << DBGLOG_ENDL_FLUSH;
  beta_latest_file_.open(beta_latest_file_location_.c_str(), std::ifstream::in);
  if (beta_latest_file_.is_open()) {
    while (beta_latest_file_.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      beta_latest_file_.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }

      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() != 1) {
        continue;
      }
      beta_vector_.push_back(atof(tokens_[0]));
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }

  // read stdev values
  std::string stdev_latest_file_location_ = STDEV_FOLDER + portfolio_ + "/" + std::to_string(lookback_days_) + "/" +
                                            std::to_string(file_extraction_day_) + "/stdev";

  std::ifstream stdev_latest_file_;
  DBGLOG_TIME_CLASS_FUNC_LINE << "Stdevs picked from the file: " << stdev_latest_file_location_ << DBGLOG_ENDL_FLUSH;
  stdev_latest_file_.open(stdev_latest_file_location_.c_str(), std::ifstream::in);
  if (stdev_latest_file_.is_open()) {
    while (stdev_latest_file_.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      stdev_latest_file_.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }

      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() != unsigned(1 + total_products_)) {
        continue;
      }
      if (std::string(tokens_[0]) == "STDEV") {
        for (int i = 1; i <= total_products_; i++) stdev_vector_.push_back(atof(tokens_[i]));
      }
      bzero(readline_buffer_, kL1AvgBufferLen);
    }
  }

  // fill vectors with zero
  prev_return_vec_.resize(total_products_, 0);
  is_ready_vec_.resize(total_products_, false);
  indicator_value_ = 0;
}
}
