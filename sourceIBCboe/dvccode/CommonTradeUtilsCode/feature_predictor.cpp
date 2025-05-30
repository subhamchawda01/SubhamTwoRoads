#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvccode/CommonTradeUtils/feature_predictor.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

FeaturePredictor::FeaturePredictor(std::string modelfilename, int _date_, const bool _live_trading_) {
  modelfilename_ = modelfilename;
  live_trading_ = _live_trading_;
  date_ = _date_;
  start_time_ = "";
  end_time_ = "";
  prev_session_start_time_ = "";
  prev_session_end_time_ = "";
  start_mfm_ = 0;
  end_mfm_ = 0;
  prev_session_start_mfm_ = 0;
  prev_session_end_mfm_ = 0;
}
void FeaturePredictor::InitializeModel() {
  std::ifstream model_infile_;
  model_infile_.open(modelfilename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }

        if (strcmp(tokens_[0], "TARGET") == 0) {
          shortcode_ = tokens_[1];
          target_feature_ = tokens_[2];
        } else if (strcmp(tokens_[0], "START_TIME") == 0) {
          start_time_ = tokens_[1];
        } else if (strcmp(tokens_[0], "END_TIME") == 0)
          end_time_ = tokens_[1];
        else if (strcmp(tokens_[0], "PREV_SESS_START_TIME") == 0)
          prev_session_start_time_ = tokens_[1];
        else if (strcmp(tokens_[0], "PREV_SESS_END_TIME") == 0)
          prev_session_end_time_ = tokens_[1];
        else if (strcmp(tokens_[1], "INTERCEPT") ==
                 0) {  // || strcmp(tokens_[1], "EVENTSTDEV") == 0 || strcmp(tokens_[1], "EVENTVOL") == 0){
          std::vector<std::string> feature_info_;
          feature_info_.push_back(std::string(tokens_[1]));
          feature_weight_map_[feature_info_] = atof(tokens_[0]);
        } else {
          std::vector<std::string> feature_info_;
          feature_info_.push_back(std::string(tokens_[1]));
          feature_info_.push_back(std::string(tokens_[2]));
          feature_info_.push_back(std::string(tokens_[3]));
          feature_weight_map_[feature_info_] = atof(tokens_[0]);
        }
      }
    }
  }
  model_infile_.close();
  start_mfm_ = GetMsecsFromMidnightFromHHMMSS(
      DateTime::GetUTCHHMMSSFromTZHHMMSS(date_, DateTime::GetHHMMSSTime(start_time_.c_str() + 4), start_time_.c_str()));
  end_mfm_ = GetMsecsFromMidnightFromHHMMSS(
      DateTime::GetUTCHHMMSSFromTZHHMMSS(date_, DateTime::GetHHMMSSTime(end_time_.c_str() + 4), end_time_.c_str()));
  if (strcmp(prev_session_start_time_.c_str(), "") != 0)
    prev_session_start_mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
        date_, DateTime::GetHHMMSSTime(prev_session_start_time_.c_str() + 4), prev_session_start_time_.c_str()));
  else
    prev_session_start_mfm_ = 0;
  if (strcmp(prev_session_end_time_.c_str(), "") != 0)
    prev_session_end_mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
        date_, DateTime::GetHHMMSSTime(prev_session_end_time_.c_str() + 4), prev_session_end_time_.c_str()));
  else
    prev_session_end_mfm_ = 0;
}

double FeaturePredictor::GetModelPrediction() {
  double res = 0.0;
  for (std::map<std::vector<std::string>, double>::iterator it = feature_weight_map_.begin();
       it != feature_weight_map_.end(); it++) {
    if (strcmp((it->first)[0].c_str(), "INTERCEPT") == 0) {
      res += it->second;
      continue;
    }
    std::string feature_granularity = (it->first)[0];
    std::string feature_shortcode = (it->first)[1];
    std::string feature_ = (it->first)[2];
    int next_date_ = HFSAT::HolidayManagerUtils::GetNextBusinessDayForProduct(shortcode_, date_);
    if (strcmp(feature_granularity.c_str(), "LASTDAY") == 0) {
      double last_day_feat =
          SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 1, start_mfm_, end_mfm_, feature_);
      double last_20_days_feat =
          SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 20, start_mfm_, end_mfm_, feature_);
      res += it->second * (last_day_feat / last_20_days_feat);
    } else if (strcmp(feature_granularity.c_str(), "LAST5DAYS") == 0) {
      double last_5days_feat =
          SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 5, start_mfm_, end_mfm_, feature_);
      double last_20_days_feat =
          SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 20, start_mfm_, end_mfm_, feature_);
      res += it->second * (last_5days_feat / last_20_days_feat);
    } else if (strcmp(feature_granularity.c_str(), "PREVSESSION_0") == 0) {
      double last_20_days_feat = SampleDataUtil::GetAvgForPeriod(
          feature_shortcode, next_date_, 20, prev_session_start_mfm_, prev_session_end_mfm_, feature_);
      if (live_trading_) setenv("LIVE_TRADING_SAMPLEDATA", "1", 1);
      double prev_session_feat = SampleDataUtil::GetAvgForPeriod(
          feature_shortcode, next_date_, 1, prev_session_start_mfm_, prev_session_end_mfm_, feature_);
      res += it->second * (prev_session_feat / last_20_days_feat);
      if (live_trading_) unsetenv("LIVE_TRADING_SAMPLEDATA");
    } else if (strcmp(feature_granularity.c_str(), "PREVSESSION_1") == 0) {
      double last_20_days_feat = SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 20, prev_session_start_mfm_,
                                                                 prev_session_end_mfm_, feature_);
      if (live_trading_) setenv("LIVE_TRADING_SAMPLEDATA", "1", 1);
      double prev_session_feat = SampleDataUtil::GetAvgForPeriod(feature_shortcode, date_, 1, prev_session_start_mfm_,
                                                                 prev_session_end_mfm_, feature_);
      res += it->second * (prev_session_feat / last_20_days_feat);
      if (live_trading_) unsetenv("LIVE_TRADING_SAMPLEDATA");
    }
  }
  return res;
}
}
