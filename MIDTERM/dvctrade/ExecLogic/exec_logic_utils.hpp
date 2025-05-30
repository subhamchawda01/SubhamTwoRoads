/**
   \file ExecLogic/exec_logic_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */
#ifndef BASE_EXEC_LOGIC_EXEC_LOGIC_UTILS_HPP
#define BASE_EXEC_LOGIC_EXEC_LOGIC_UTILS_HPP

#include <iostream>
#include <string>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/Indicators/returns_based_projected_price.hpp"
#include "dvctrade/Indicators/online_ratio_projected_price.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

#define DEFAULT_MIN_MAX_L1_SIZE_FILE "/spare/local/tradeinfo/OfflineInfo/l1_size_file.txt"
#define DEFAULT_MIN_MAX_L1_SIZE_FILE_AS "/spare/local/tradeinfo/OfflineInfo/l1_size_file_as.txt"
#define DEFAULT_MIN_MAX_L1_SIZE_FILE_EU "/spare/local/tradeinfo/OfflineInfo/l1_size_file_eu.txt"
#define DEFAULT_MIN_MAX_L1_SIZE_FILE_US "/spare/local/tradeinfo/OfflineInfo/l1_size_file_us.txt"
#define DEFAULT_L1_SIZE_PERCENTILE_FILE "/spare/local/tradeinfo/OfflineInfo/l1_size_bound_file.txt"

#define DEFAULT_MIN_MAX_L1_ORDER_FILE "/spare/local/tradeinfo/OfflineInfo/l1_order_file.txt"
#define DEFAULT_MIN_MAX_L1_ORDER_FILE_AS "/spare/local/tradeinfo/OfflineInfo/l1_order_file_as.txt"
#define DEFAULT_MIN_MAX_L1_ORDER_FILE_EU "/spare/local/tradeinfo/OfflineInfo/l1_order_file_eu.txt"
#define DEFAULT_MIN_MAX_L1_ORDER_FILE_US "/spare/local/tradeinfo/OfflineInfo/l1_order_file_us.txt"
#define DEFAULT_L1_ORDER_PERCENTILE_FILE "/spare/local/tradeinfo/OfflineInfo/l1_order_bound_file.txt"

#define AS_THROTTLE_FILE "/spare/local/tradeinfo/OfflineInfo/throttle_limit_as.txt"
#define EU_THROTTLE_FILE "/spare/local/tradeinfo/OfflineInfo/throttle_limit_eu.txt"
#define US_THROTTLE_FILE "/spare/local/tradeinfo/OfflineInfo/throttle_limit_us.txt"
#define DEFAULT_THROTTLE_FILE "/spare/local/tradeinfo/OfflineInfo/throttle_limit_default.txt"

#define CANCELLATION_MODEL_FILE "/spare/local/tradeinfo/CancellationModel/"

#define PRE_AS_START_TIME_MFM (43 * 1800 * 1000)  // UTC_2130, AST_1630
#define EU_START_TIME_MFM (11 * 1800 * 1000)      // UTC_530
#define US_START_TIME_MFM (23 * 1800 * 1000)      // UTC_1130

namespace HFSAT {

namespace ExecLogicUtils {

inline std::string getCancellationModel(std::string shortcode, int trading_date,
                                 unsigned int trading_start, unsigned int trading_end) {
  std::string cancellation_file = "";
  std::string cancellation_file_prefix_ = CANCELLATION_MODEL_FILE + shortcode + "/model_" + shortcode;

  int t_intdate_ = HFSAT::DateTime::CalcPrevDay(trading_date);

  for (unsigned int ii = 0; ii < 20; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << t_intdate_;
    std::string date = t_temp_oss_.str();
    cancellation_file = cancellation_file_prefix_ + "_" + date + "";
    if (HFSAT::FileUtils::exists(cancellation_file)) {  // load from every file that exists
      return cancellation_file;
    }
    t_intdate_ = HFSAT::DateTime::CalcPrevDay(t_intdate_);
  }

  cancellation_file = cancellation_file_prefix_ + "DEFAULT.txt";

  if (!HFSAT::FileUtils::exists(cancellation_file)) {
    return "";
  }

  return cancellation_file;
}

inline std::string GetSecondaryShortcode(const std::string &r_dep_shortcode_) {
  // eurex
  if (!r_dep_shortcode_.compare("HHI_0")) {
    return std::string("HSI_0");
  }
  if (!r_dep_shortcode_.compare("HSI_0")) {
    return std::string("HHI_0");
  }
  if (!r_dep_shortcode_.compare("NK_0")) {
    return std::string("NKM_0");
  }
  if (!r_dep_shortcode_.compare("NKM_0")) {
    return std::string("NK_0");
  }
  if (!r_dep_shortcode_.compare("BR_WIN_0")) {
    return std::string("BR_IND_0");
  }
  if (!r_dep_shortcode_.compare("BR_IND_0")) {
    return std::string("BR_WIN_0");
  }
  if (!r_dep_shortcode_.compare("Si_0")) {
    return std::string("USD000UTSTOM");
  }
  if (!r_dep_shortcode_.compare("USD000UTSTOM")) {
    return std::string("Si_0");
  }
  return r_dep_shortcode_;
}

inline std::string GetIndepShortcodeForPricePair(const std::string &r_dep_shortcode_) {
  if (!r_dep_shortcode_.compare("HHI_0")) {
    return std::string("HSI_0");
  }
  if (!r_dep_shortcode_.compare("MHI_0")) {
    return std::string("HSI_0");
  }
  if (!r_dep_shortcode_.compare("MCH_0")) {
    return std::string("HHI_0");
  }
  if (!r_dep_shortcode_.compare("HSI_0")) {
    return std::string("MHI_0");
  }

  // BMF
  if (!r_dep_shortcode_.compare("BR_WIN_0")) {
    return std::string("BR_IND_0");
  }
  if (!r_dep_shortcode_.compare("BR_IND_0")) {
    return std::string("BR_WIN_0");
  }
  if (!r_dep_shortcode_.compare("BR_WDO_0")) {
    return std::string("BR_DOL_0");
  }
  if (!r_dep_shortcode_.compare("BR_DOL_0")) {
    return std::string("BR_WDO_0");
  }
  if (!r_dep_shortcode_.compare("BR_ISP_0")) {
    return std::string("ES_0");
  }

  // OSE
  if (!r_dep_shortcode_.compare("NKMF_0")) {
    return std::string("NKM_0");
  }
  if (!r_dep_shortcode_.compare("NK_0")) {
    return std::string("NKM_0");
  }
  if (!r_dep_shortcode_.compare("NKM_0")) {
    return std::string("NK_0");
  }
  if (!r_dep_shortcode_.compare("TOPIX_0")) {
    return std::string("NK_0");
  }

  // RTS-MICEX
  if (!r_dep_shortcode_.compare("Si_0")) {
    return std::string("USD000UTSTOM");
  }

  if (!r_dep_shortcode_.compare("USD000UTSTOM")) {
    return std::string("Si_0");
  }

  if (!r_dep_shortcode_.compare("USD000000TOD")) {
    return std::string("USD000UTSTOM");
  }

  if (!r_dep_shortcode_.compare("NKD_0")) {
    return std::string("NIY_0");
  }

  // CME
  if (!r_dep_shortcode_.compare("NIY_0")) {
    return std::string("NKM_0");
  }

  // EURIBORS
  if (r_dep_shortcode_.find("FEU3_") != std::string::npos) {
    std::string partner_ = r_dep_shortcode_;
    return (partner_.replace(0, 4, "LFI"));  // string copy !?
  }

  return r_dep_shortcode_;
}

inline bool IsProjectedPriceRequired(const std::string &r_dep_shortcode_) {
  if (r_dep_shortcode_.compare("BR_WIN_0") == 0 || r_dep_shortcode_.compare("BR_IND_0") == 0 ||
      r_dep_shortcode_.compare("MHI_0") == 0 || r_dep_shortcode_.compare("MCH_0") == 0 ||
      r_dep_shortcode_.compare("NIY_0") == 0 || r_dep_shortcode_.compare("NKM_0") == 0) {
    return false;
  } else if (r_dep_shortcode_.compare("TOPIX_0") == 0 || r_dep_shortcode_.compare("NKD_0") == 0 ||
             r_dep_shortcode_.compare("NKMF_0") == 0 || r_dep_shortcode_.compare("HHI_0") == 0 ||
             r_dep_shortcode_.compare("HSI_0") == 0) {
    return true;
  }
  return true;
}

inline CommonIndicator *GetProjectedBidPrice(DebugLogger &_dbglogger_, const Watch &_watch_,
                                             SecurityMarketView &_dep_market_view_,
                                             SecurityMarketView &_indep_market_view_, double duration_) {
  if (_dep_market_view_.shortcode().compare("NKMF_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  if (_dep_market_view_.shortcode().compare("NKD_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  if (_dep_market_view_.shortcode().compare("NIY_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  if (_dep_market_view_.shortcode().compare("HHI_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  if (_dep_market_view_.shortcode().compare("TOPIX_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  if (_dep_market_view_.shortcode().compare("Si_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeBidPrice);
  }

  return NULL;
}

inline CommonIndicator *GetProjectedAskPrice(DebugLogger &_dbglogger_, const Watch &_watch_,
                                             SecurityMarketView &_dep_market_view_,
                                             SecurityMarketView &_indep_market_view_, double duration_) {
  if (_dep_market_view_.shortcode().compare("NKMF_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }
  if (_dep_market_view_.shortcode().compare("NKD_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }

  if (_dep_market_view_.shortcode().compare("NIY_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }

  if (_dep_market_view_.shortcode().compare("HHI_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }

  if (_dep_market_view_.shortcode().compare("TOPIX_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }

  if (_dep_market_view_.shortcode().compare("Si_0") == 0) {
    return OnlineRatioProjectedPrice::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_, _indep_market_view_,
                                                        duration_, kPriceTypeAskPrice);
  }

  return NULL;
}

inline int GetSamplingIntervalForPnlSeries(std::string shortcode_) {
  int sampling_msecs_from_midnight_ = 15 * 60 * 1000;  // 15 minutes by default

  // To add different intervals for different shortcodes, please add them as below
  /*
      if ( shortcode_.compare() == "SHC" )
        {
          sampling_msecs_from_midnight_ = 15*60*1000;
        }
   */
  return sampling_msecs_from_midnight_;
}

inline bool ShcPresentInL1File(std::string filename, std::string shortcode) {
  std::ifstream t_l1_size_infile_;
  t_l1_size_infile_.open(filename.c_str(), std::ifstream::in);
  bool wts_found_ = false;
  if (t_l1_size_infile_.is_open()) {
    const int kOFFLINEMIXLineBufferLen = 1024;
    char readline_buffer_[kOFFLINEMIXLineBufferLen];
    bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);

    while (t_l1_size_infile_.good()) {
      bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);
      t_l1_size_infile_.getline(readline_buffer_, kOFFLINEMIXLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kOFFLINEMIXLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= 1) {
        std::string dep_ = tokens_[0];
        if (shortcode.compare(dep_) == 0) {
          wts_found_ = true;
          break;
        }
      }
    }
    t_l1_size_infile_.close();
  }
  return wts_found_;
}

inline void GetL1SizePercentile(double &lower_, double &upper_, const std::string _shortcode_, bool is_size_ = true) {
  std::ifstream t_l1_size_bound_infile_;
  if (is_size_)
    t_l1_size_bound_infile_.open(std::string(DEFAULT_L1_SIZE_PERCENTILE_FILE).c_str(), std::ifstream::in);
  else
    t_l1_size_bound_infile_.open(std::string(DEFAULT_L1_ORDER_PERCENTILE_FILE).c_str(), std::ifstream::in);
  if (t_l1_size_bound_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_l1_size_bound_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_l1_size_bound_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 3) {
        std::string file_shortcode_ = tokens_[0];
        if ((_shortcode_.compare(file_shortcode_) == 0)) {
          lower_ = atof(tokens_[1]);
          upper_ = atof(tokens_[2]);
          break;
        }
      }
    }
  }
  t_l1_size_bound_infile_.close();
}

inline void GetLowerUpperBound(double &lower_, double &upper_, const std::string _shortcode_, std::string filename,
                               bool &t_use_static_value_) {
  std::ifstream t_l1_size_infile_;
  t_l1_size_infile_.open(filename.c_str(), std::ifstream::in);
  if (t_l1_size_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_l1_size_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_l1_size_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 3) {
        std::string file_shortcode_ = tokens_[0];
        if ((_shortcode_.compare(file_shortcode_) == 0)) {
          lower_ = atof(tokens_[1]);
          upper_ = atof(tokens_[2]);
          t_use_static_value_ = true;
          break;
        }
      }
    }
  }
  t_l1_size_infile_.close();
}

inline void GetL1SizeBound(const std::string _shortcode_, double &t_l1_size_lower_bound_,
                           double &t_l1_size_upper_bound_, const Watch &watch_, const int trading_start_utc_mfm_,
                           const int trading_end_utc_mfm_, DebugLogger &dbglogger_) {
  double t_l1_size_lower_percentile_ = 0.3;
  double t_l1_size_upper_percentile_ = 0.7;
  GetL1SizePercentile(t_l1_size_lower_percentile_, t_l1_size_upper_percentile_, _shortcode_);

  t_l1_size_lower_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
      t_l1_size_lower_percentile_);
  t_l1_size_upper_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
      t_l1_size_upper_percentile_);
  DBGLOG_TIME << __func__ << " Avg " << t_l1_size_lower_percentile_ << "::" << t_l1_size_lower_bound_ << " "
              << t_l1_size_upper_percentile_ << "::" << t_l1_size_upper_bound_ << DBGLOG_ENDL_FLUSH;

  // Override the t_l1_size_lower_bound_ and t_l1_size_upper_bound_ if values are present in static file
  bool static_file = false;
  GetLowerUpperBound(t_l1_size_lower_bound_, t_l1_size_upper_bound_, _shortcode_,
                     std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE), static_file);
}

inline void GetL1SizeBound(const std::string _shortcode_, double &t_l1_size_lower_bound_,
                           double &t_l1_size_upper_bound_, const Watch &watch_, const int trading_start_utc_mfm_,
                           const int trading_end_utc_mfm_, DebugLogger &dbglogger_, double &t_l1_size_lower_percentile_,
                           double &t_l1_size_upper_percentile_, bool &t_use_static_value_) {
  if (t_l1_size_lower_percentile_ == 0 && t_l1_size_upper_percentile_ == 0) {
    t_l1_size_lower_percentile_ = 0.3;
    t_l1_size_upper_percentile_ = 0.7;
    GetL1SizePercentile(t_l1_size_lower_percentile_, t_l1_size_upper_percentile_, _shortcode_);

    // Override the t_l1_size_lower_bound_ and t_l1_size_upper_bound_ if values are present in static file
    std::string filename = std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE);
    if ((trading_start_utc_mfm_ < EU_START_TIME_MFM || trading_start_utc_mfm_ > PRE_AS_START_TIME_MFM) &&
        ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_AS), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_AS);
    } else if (trading_start_utc_mfm_ < US_START_TIME_MFM &&
               ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_EU), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_EU);
    } else if (ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_US), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_SIZE_FILE_US);
    }
    GetLowerUpperBound(t_l1_size_lower_bound_, t_l1_size_upper_bound_, _shortcode_, filename, t_use_static_value_);
  }

  if (!t_use_static_value_) {
    t_l1_size_lower_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
        _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
        t_l1_size_lower_percentile_);
    t_l1_size_upper_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
        _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
        t_l1_size_upper_percentile_);
    DBGLOG_TIME << __func__ << " Avg " << t_l1_size_lower_percentile_ << "::" << t_l1_size_lower_bound_ << " "
                << t_l1_size_upper_percentile_ << "::" << t_l1_size_upper_bound_ << DBGLOG_ENDL_FLUSH;
  }
}

inline void GetL1OrderBound(const std::string _shortcode_, double &t_l1_order_lower_bound_,
                            double &t_l1_order_upper_bound_, const Watch &watch_, const int trading_start_utc_mfm_,
                            const int trading_end_utc_mfm_, DebugLogger &dbglogger_) {
  double t_l1_order_lower_percentile_ = 0.3;
  double t_l1_order_upper_percentile_ = 0.7;
  GetL1SizePercentile(t_l1_order_lower_percentile_, t_l1_order_upper_percentile_, _shortcode_, false);
  t_l1_order_lower_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("ORDSZ"),
      t_l1_order_lower_percentile_);
  t_l1_order_upper_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("ORDSZ"),
      t_l1_order_upper_percentile_);
  DBGLOG_TIME << __func__ << " Avg " << t_l1_order_lower_percentile_ << "::" << t_l1_order_lower_bound_ << " "
              << t_l1_order_upper_percentile_ << "::" << t_l1_order_upper_bound_ << DBGLOG_ENDL_FLUSH;

  // Override the t_l1_order_lower_bound_ and t_l1_order_upper_bound_ if values are present in static file
  bool static_file = false;
  GetLowerUpperBound(t_l1_order_lower_bound_, t_l1_order_upper_bound_, _shortcode_,
                     std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE), static_file);
}

inline void GetL1OrderBound(const std::string _shortcode_, double &t_l1_order_lower_bound_,
                            double &t_l1_order_upper_bound_, const Watch &watch_, const int trading_start_utc_mfm_,
                            const int trading_end_utc_mfm_, DebugLogger &dbglogger_,
                            double &t_l1_order_lower_percentile_, double &t_l1_order_upper_percentile_,
                            bool &t_use_static_value_) {
  if (t_l1_order_lower_percentile_ == 0 && t_l1_order_upper_percentile_ == 0) {
    t_l1_order_lower_percentile_ = 0.3;
    t_l1_order_upper_percentile_ = 0.7;
    GetL1SizePercentile(t_l1_order_lower_percentile_, t_l1_order_upper_percentile_, _shortcode_, false);

    // Override the t_l1_order_lower_bound_ and t_l1_order_upper_bound_ if values are present in static file
    std::string filename = std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE);
    if ((trading_start_utc_mfm_ < EU_START_TIME_MFM || trading_start_utc_mfm_ > PRE_AS_START_TIME_MFM) &&
        ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_AS), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_AS);
    } else if (trading_start_utc_mfm_ < US_START_TIME_MFM &&
               ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_EU), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_EU);
    } else if (ShcPresentInL1File(std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_US), _shortcode_)) {
      filename = std::string(DEFAULT_MIN_MAX_L1_ORDER_FILE_US);
    }
    GetLowerUpperBound(t_l1_order_lower_bound_, t_l1_order_upper_bound_, _shortcode_, filename, t_use_static_value_);
  }

  if (!t_use_static_value_) {
    t_l1_order_lower_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
        _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("ORDSZ"),
        t_l1_order_lower_percentile_);
    t_l1_order_upper_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
        _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("ORDSZ"),
        t_l1_order_upper_percentile_);
    DBGLOG_TIME << __func__ << " Avg " << t_l1_order_lower_percentile_ << "::" << t_l1_order_lower_bound_ << " "
                << t_l1_order_upper_percentile_ << "::" << t_l1_order_upper_bound_ << DBGLOG_ENDL_FLUSH;
  }
}

inline void GetHighUTSBound(const std::string _shortcode_, double &t_l1_order_lower_bound_,
                            double &t_l1_order_upper_bound_, const Watch &watch_, const int trading_start_utc_mfm_,
                            const int trading_end_utc_mfm_, DebugLogger &dbglogger_) {
  std::ifstream t_l1_order_bound_infile_;
  double t_l1_order_lower_percentile_ = 0.3;
  double t_l1_order_upper_percentile_ = 0.9;
  t_l1_order_bound_infile_.open("/spare/local/tradeinfo/OfflineInfo/high_uts_l1_size_bound_file.txt",
                                std::ifstream::in);
  if (t_l1_order_bound_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_l1_order_bound_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_l1_order_bound_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() == 3) {
        std::string file_shortcode_ = tokens_[0];
        if ((_shortcode_.compare(file_shortcode_) == 0)) {
          t_l1_order_lower_percentile_ = atof(tokens_[1]);
          t_l1_order_upper_percentile_ = atof(tokens_[2]);
        }
      }
    }
  }
  t_l1_order_bound_infile_.close();
  t_l1_order_lower_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
      t_l1_order_lower_percentile_);
  t_l1_order_upper_bound_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      _shortcode_, watch_.YYYYMMDD(), 60, trading_start_utc_mfm_, trading_end_utc_mfm_, std::string("L1SZ"),
      t_l1_order_upper_percentile_);
  DBGLOG_TIME << __func__ << " Avg " << t_l1_order_lower_percentile_ << "::" << t_l1_order_lower_bound_ << " "
              << t_l1_order_upper_percentile_ << "::" << t_l1_order_upper_bound_ << DBGLOG_ENDL_FLUSH;
}

inline double GetMURVolBound(const std::string _shortcode_, const Watch &r_watch_, double t_fractional_seconds_,
                             const int &t_start_mfm_, const int &t_end_mfm_) {
  double trading_volume_expected_ = 0.0;
  int tradingdate_ = r_watch_.YYYYMMDD();

  std::map<int, double> utc_time_to_vol_map_;
  SampleDataUtil::GetAvgForPeriod(_shortcode_, tradingdate_, 60, t_start_mfm_, t_end_mfm_, "VOL", utc_time_to_vol_map_);

  int start_time_ = GetHHMMSSFromMsecsFromMidnight(t_start_mfm_, 1800, 0);
  int end_time_ = GetHHMMSSFromMsecsFromMidnight(t_end_mfm_, 1800, 1);
  if (t_end_mfm_ % 1800000 == 0) {
    end_time_ = GetHHMMSSFromMsecsFromMidnight(t_end_mfm_, 1800, 0);
  }

  int i = 0;
  if ((end_time_ > start_time_) && (utc_time_to_vol_map_.find(start_time_) != utc_time_to_vol_map_.end()) &&
      (utc_time_to_vol_map_.find(end_time_) != utc_time_to_vol_map_.end())) {
    for (auto it = utc_time_to_vol_map_.find(start_time_); it != utc_time_to_vol_map_.find(end_time_); it++) {
      trading_volume_expected_ += it->second;
      i++;
    }
    trading_volume_expected_ /= i;  // average expected volume for 30 mins
    trading_volume_expected_ = trading_volume_expected_ * t_fractional_seconds_ / 300;
  } else {
    for (auto it = utc_time_to_vol_map_.begin(); it != utc_time_to_vol_map_.end(); it++) {
      trading_volume_expected_ += it->second;
      i++;
    }
    trading_volume_expected_ /= i;  // average expected volume for 30 mins
    trading_volume_expected_ = trading_volume_expected_ * t_fractional_seconds_ / 300;
  }
  return trading_volume_expected_;
}

inline int GetDurationForAvgL1Size(const std::string _shortcode_) {
  if (!_shortcode_.compare("FGBS_0")) {
    return 900;
  }
  if (!_shortcode_.compare("BR_DOL_0") || !_shortcode_.compare("LFI_0") || !_shortcode_.compare("LFI_1") ||
      !_shortcode_.compare("LFI_2") || !_shortcode_.compare("LFI_3") || !_shortcode_.compare("LFI_4") ||
      !_shortcode_.compare("LFI_5") || !_shortcode_.compare("LFI_6") || !_shortcode_.compare("LFI_7") ||
      !_shortcode_.compare("LFI_8") || !_shortcode_.compare("LFI_9") || !_shortcode_.compare("LFL_4") ||
      !_shortcode_.compare("LFL_5") || !_shortcode_.compare("LFL_6") || !_shortcode_.compare("XT_0") ||
      !_shortcode_.compare("YT_0")) {
    return 600;
  }
  if (!_shortcode_.compare("LFR_0") || !_shortcode_.compare("CGB_0") || !_shortcode_.compare("SXF_0") ||
      !_shortcode_.compare("NK_0") || !_shortcode_.compare("NKM_0")) {
    return 300;
  }
  if (!_shortcode_.compare("HHI_0") || !_shortcode_.compare("HSI_0")) {
    return 150;
  }
  return 600;
}

inline int GetL1SizeDurationForHighUTS(const std::string _shortcode_) {
  if (!_shortcode_.compare("ZT_0")) {
    return 10;
  }
  return 100;
}

inline int GetThrottleForShortcode(const std::string &shortcode, int trading_start_mfm, int trading_end_mfm) {
  // For given ShortCode-trading_time find, the default value of throttle message
  std::string file_mfm = DEFAULT_THROTTLE_FILE;
  if ((trading_start_mfm > PRE_AS_START_TIME_MFM || trading_start_mfm < EU_START_TIME_MFM)) {
    file_mfm = AS_THROTTLE_FILE;
  } else if (trading_start_mfm < US_START_TIME_MFM) {
    file_mfm = EU_THROTTLE_FILE;
  } else if (trading_start_mfm >= US_START_TIME_MFM) {
    file_mfm = US_THROTTLE_FILE;
  }

  // adding default value here for NSE/BSE
  if (strncmp(shortcode.c_str(), "NSE_", 4) == 0 || strncmp(shortcode.c_str(), "BSE_", 4) == 0) {
    return 50;
  }

  std::ifstream throttle_file;
  throttle_file.open(file_mfm.c_str(), std::ifstream::in);

  const int kL1AvgBufferLen = 1024;
  char readline_buffer_[kL1AvgBufferLen];
  if (throttle_file.is_open()) {
    bzero(readline_buffer_, kL1AvgBufferLen);
    throttle_file.getline(readline_buffer_, kL1AvgBufferLen);
  }
  while (throttle_file.good()) {
    PerishableStringTokenizer st(readline_buffer_, kL1AvgBufferLen);
    const std::vector<const char *> &tokens = st.GetTokens();
    if (tokens.size() > 0) {
      if (shortcode.compare(tokens[0]) == 0) {
        // line for this ShortCode
        int throttle_value = atoi(tokens[1]);
        return throttle_value;
      }
    }
    bzero(readline_buffer_, kL1AvgBufferLen);
    throttle_file.getline(readline_buffer_, kL1AvgBufferLen);
  }

  // Should not be here
  ExitVerbose(kExitErrorCodeGeneral, "Throttle message limit not specified, offline-file as well...");
  return -1;
}

inline void GetShortCodesForCombinedGetFlat(std::string r_dep_shortcode_,
                                            std::vector<std::string> &ors_needed_by_indicators_vec_) {
  std::string filepath = "/spare/local/tradeinfo/CombinedGetFlatInfo/shortcodes";
  std::ifstream shortcode_info;
  shortcode_info.open(filepath.c_str(), std::ifstream::in);
  const int kL1AvgBufferLen = 1024;
  char readline_buffer_[kL1AvgBufferLen];
  if (shortcode_info.is_open()) {
    bzero(readline_buffer_, kL1AvgBufferLen);
    shortcode_info.getline(readline_buffer_, kL1AvgBufferLen);
  }
  while (shortcode_info.good()) {
    PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    bool readline = false;
    if (tokens_.size() > 0) {
      for (auto i = 0u; i < tokens_.size(); i++) {
        if (tokens_[i] == r_dep_shortcode_)  // TODO won't work...please correct this
        {
          HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_, std::string(tokens_[i]));
          readline = true;
          break;
        }
      }
      if (readline) {
        for (auto i = 0u; i < tokens_.size(); i++) {
          HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_, std::string(tokens_[i]));
        }
        break;
      }
    }
    bzero(readline_buffer_, kL1AvgBufferLen);
    shortcode_info.getline(readline_buffer_, kL1AvgBufferLen);
  }
  shortcode_info.close();
}

inline bool ToCheckWorstCaseOnPlaceCancel(const std::string _shortcode_) {
  if (!_shortcode_.compare("RI_0") || !_shortcode_.compare("Si_0") || !_shortcode_.compare("ED_0") ||
      !_shortcode_.compare("GD_0") || !_shortcode_.compare("USD000UTSTOM") || !_shortcode_.compare("USD000000TOD") ||
      !_shortcode_.compare("USD000TODTOM") || !_shortcode_.compare("LFZ_0")) {
    return true;
  }
  return false;
}

inline bool IsEarnings(std::string _shortcode_, int _tradingdate_) {
  if (_shortcode_.substr(0, 4) == "NSE_") {
    std::string date_ = std::to_string(_tradingdate_);
    std::string prevdate_ = std::to_string(HFSAT::DateTime::CalcPrevWeekDay(_tradingdate_));
    std::string schshort_ = _shortcode_.substr(4, _shortcode_.length() - 9);
    std::string date_file_location_ = "/spare/local/tradeinfo/NSE_Files/Earnings/Products/" + schshort_;
    std::ifstream date_file_;
    date_file_.open(date_file_location_.c_str(), std::ifstream::in);
    if (date_file_.is_open()) {
      while (date_file_.good()) {
        const int kL1AvgBufferLen = 1024;
        char readline_buffer_[kL1AvgBufferLen];
        bzero(readline_buffer_, kL1AvgBufferLen);
        date_file_.getline(readline_buffer_, kL1AvgBufferLen);
        if (readline_buffer_[0] == '#') {
          continue;
        }
        PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) {
          continue;
        }
        if (date_ == tokens_[0] || prevdate_ == tokens_[0]) {
          return true;
          break;
        }
      }
    }
  }
  return false;
}
}
}

#endif  // BASE_EXEC_LOGIC_EXEC_LOGIC_UTILS_HPP
