/**
   \file ModelMath/indicator_logger.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/base_implied_price.hpp"

namespace HFSAT {

class IndicatorLogger : public IndicatorListener,
                        public SecurityMarketViewOnReadyListener,
                        public GlobalPositionChangeListener,
                        public SecurityMarketViewChangeListener,
#ifdef USING_PRETRADE_PRINTVALS
                        public SMVPreTradeListener,
#endif
                        public TimePeriodListener {
 public:

  typedef enum {
        kIndStats,
        kSampleStats,
        kPnlBasedStats,
  } DatagenStats_t; ///< enum for datagen output format for different target applications  

 protected:

  DebugLogger& dbglogger_;
  const Watch& watch_;
  EconomicEventsManager& economic_events_manager_;
  std::string model_filename_;

  /// internal variable for computation of status of model
  std::vector<bool> is_ready_vec_;
  std::vector<bool> readiness_required_vec_;
  bool is_ready_;

  std::vector<CommonIndicator*> indicator_vec_;

  SecurityMarketView* p_dep_market_view_;
  const PriceType_t dep_baseprice_type_;
  const PriceType_t dep_pred_price_type_;

  std::vector<double> prev_value_vec_;
  BulkFileWriter& bulk_file_writer_;

  // these vars control when new samples are printed
  const unsigned int msecs_to_wait_to_print_again_;
  unsigned int last_print_msecs_from_midnight_;
  const unsigned long long l1events_timeout_;
  unsigned long long last_print_l1events_;
  const unsigned int num_trades_to_wait_print_again_;
  const unsigned int num_tradesize_to_wait_print_again_;
  unsigned int last_print_num_trades_;
  unsigned int last_print_num_tradesize_;
  unsigned int num_tradesize_;

  int last_indicators_debug_print_;

  int min_msec_toprint_;
  int max_msec_toprint_;

  std::vector<EconomicZone_t> ezone_vec_;
  bool getflat_due_to_economic_times_;
  bool economic_events_allows_print_;

  const unsigned int to_print_on_economic_times_;
  bool only_print_on_economic_events_;
  bool currently_tradable_;
  EconomicZone_t ezone_traded_;

  const unsigned int sample_on_core_shortcodes_;
  const std::vector<SecurityMarketView*> p_sampling_shc_smv_vec_;
  const std::vector<double> c3_required_cutoffs_;
  std::vector<double> last_print_trigger_values_;
  std::vector<double> trigger_thresholds_;

  std::vector<unsigned int> num_tradesize_vec_;
  std::vector<std::string> indicator_name_vec_; // only used in indicator_stats

  int no_of_sampling_shortcodes_;
  unsigned int regime_mode_to_print_;

  BaseImpliedPrice* p_implied_price_indicator_;
  bool use_implied_price_;
  bool print_pbat_bias_;

 public:
  IndicatorLogger(DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
                  EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
                  SecurityMarketView* _p_dep_market_view_, PriceType_t _dep_baseprice_type_,
                  PriceType_t _dep_pred_price_type_, const std::string& _output_filename_,
                  const unsigned int t_msecs_to_wait_to_print_again_, const unsigned long long t_l1events_timeout_,
                  const unsigned int t_num_trades_to_wait_print_again_,
                  const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
                  const std::vector<SecurityMarketView*>& t_p_sampling_shc_smv_vec_,
                  const std::vector<double> t_c3_required_cutoffs_, unsigned int t_regime_mode_to_print_ = 0,
                  bool _pbat_bias_ = false)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        economic_events_manager_(r_economic_events_manager_),
        model_filename_(_model_filename_),
        is_ready_vec_(),
        readiness_required_vec_(),
        is_ready_(false),
        indicator_vec_(),
        p_dep_market_view_(_p_dep_market_view_),
        dep_baseprice_type_(_dep_baseprice_type_),
        dep_pred_price_type_(_dep_pred_price_type_),
        bulk_file_writer_(_bulk_file_writer_),
        msecs_to_wait_to_print_again_(t_msecs_to_wait_to_print_again_),
        last_print_msecs_from_midnight_(0u),
        l1events_timeout_(t_l1events_timeout_),
        last_print_l1events_(0u),
        num_trades_to_wait_print_again_(t_num_trades_to_wait_print_again_),
        num_tradesize_to_wait_print_again_(t_num_trades_to_wait_print_again_),
        last_print_num_trades_(0u),
        last_print_num_tradesize_(0u),
        num_tradesize_(0u),
        last_indicators_debug_print_(0),
        min_msec_toprint_(0),
        max_msec_toprint_(0),
        ezone_vec_(),
        getflat_due_to_economic_times_(false),
        economic_events_allows_print_(true),
        to_print_on_economic_times_(t_to_print_on_economic_times_),
        only_print_on_economic_events_(false),
        currently_tradable_(false),
        ezone_traded_(EZ_MAX),
        sample_on_core_shortcodes_(t_sample_on_core_shortcodes_),
        p_sampling_shc_smv_vec_(t_p_sampling_shc_smv_vec_),
        c3_required_cutoffs_(t_c3_required_cutoffs_),
        no_of_sampling_shortcodes_(1),  // this should be based upon the ratio of depevents/indepevnts or any other
        regime_mode_to_print_(t_regime_mode_to_print_),
        p_implied_price_indicator_(NULL),
        use_implied_price_(false),
        print_pbat_bias_(_pbat_bias_) {
    if (t_sample_on_core_shortcodes_ == 7u || t_sample_on_core_shortcodes_ == 8u ||
        t_sample_on_core_shortcodes_ == 11u || t_sample_on_core_shortcodes_ == 12u) {
      //      if (p_dep_market_view_ != NULL) {
      //        p_dep_market_view_->subscribe_tradeprints(this);
      //      }
      if (p_sampling_shc_smv_vec_.size() == 0) {
        if (p_dep_market_view_ != NULL) {
          p_dep_market_view_->subscribe_tradeprints(this);
        }
      }
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (p_sampling_shc_smv_vec_[i] != NULL) {
          p_sampling_shc_smv_vec_[i]->subscribe_tradeprints(this);
        }
        num_tradesize_vec_.push_back(0);
      }
    }

    watch_.subscribe_BigTimePeriod(this);
    bulk_file_writer_.Open(_output_filename_);

    if (p_dep_market_view_ != NULL) {
      // first arg : listener is not "this" but "NULL" .. so that this is not added as a listener .. only aim is that
      // when it fetches both sorts of prices, they should already be computed
      if (dep_baseprice_type_ != PriceType_t::kPriceTypeDUMMY) {
	if (!p_dep_market_view_->subscribe_price_type(NULL, dep_baseprice_type_)) {
	  PriceType_t t_error_price_type_ = dep_baseprice_type_;
	  std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
		    << t_error_price_type_ << std::endl;
	}
      }

      if (dep_pred_price_type_ != PriceType_t::kPriceTypeDUMMY) {
	if (!p_dep_market_view_->subscribe_price_type(NULL, dep_pred_price_type_)) {
	  PriceType_t t_error_price_type_ = dep_pred_price_type_;
	  std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
		    << t_error_price_type_ << std::endl;
	}
      }

#ifdef USING_PRETRADE_PRINTVALS
      p_dep_market_view_->SetPreTradeListener(this);
#endif

      GetEZVecForShortcode(p_dep_market_view_->shortcode(), 47700000, ezone_vec_);

      if (to_print_on_economic_times_ == 3u) {
        only_print_on_economic_events_ = true;
        economic_events_manager_.GetTradedEventsForToday();
        economic_events_manager_.SetComputeTradability(true);
      }
    }

    if (sample_on_core_shortcodes_ == 2u) {
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        trigger_thresholds_.push_back(
            no_of_sampling_shortcodes_ *
            HFSAT::SampleDataUtil::GetAvgForPeriod(p_sampling_shc_smv_vec_[i]->shortcode(), watch_.YYYYMMDD(), 60,
                                                   HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                   HFSAT::CommonIndicator::global_trading_end_mfm_, "L1EVPerSec"));
        last_print_trigger_values_.push_back(0);
      }
    }

    if (sample_on_core_shortcodes_ == 10u || sample_on_core_shortcodes_ == 12u) {
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (sample_on_core_shortcodes_ == 10u) {
          trigger_thresholds_.push_back(std::max(
              1.0, (t_msecs_to_wait_to_print_again_ * no_of_sampling_shortcodes_ *
                    HFSAT::SampleDataUtil::GetAvgForPeriod(p_sampling_shc_smv_vec_[i]->shortcode(), watch_.YYYYMMDD(),
                                                           60, HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                           HFSAT::CommonIndicator::global_trading_end_mfm_, "TRADES")) /
                       300000.0));
        } else {
          trigger_thresholds_.push_back(std::max(
              1.0, (t_msecs_to_wait_to_print_again_ * no_of_sampling_shortcodes_ *
                    HFSAT::SampleDataUtil::GetAvgForPeriod(p_sampling_shc_smv_vec_[i]->shortcode(), watch_.YYYYMMDD(),
                                                           60, HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                           HFSAT::CommonIndicator::global_trading_end_mfm_, "VOL")) /
                       300000.0));
        }
        last_print_trigger_values_.push_back(0);
      }
    }

    if (sample_on_core_shortcodes_ == 3u) {
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (!p_sampling_shc_smv_vec_[i]->subscribe_price_type(NULL, HFSAT::StringToPriceType_t("MktSizeWPrice"))) {
          PriceType_t t_error_price_type_ = HFSAT::StringToPriceType_t("MktSizeWPrice");
          std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                    << t_error_price_type_ << std::endl;
        }
        trigger_thresholds_.push_back(c3_required_cutoffs_[i] * p_sampling_shc_smv_vec_[i]->min_price_increment());
        // std::cout << p_sampling_shc_smv_vec_[i]->shortcode() << " "
        //          << c3_required_cutoffs_[i] * p_sampling_shc_smv_vec_[i]->min_price_increment() << "\n";
        last_print_trigger_values_.push_back(0.0);
      }
    }

    if (p_dep_market_view_ != NULL) {
      economic_events_manager_.AdjustSeverity(p_dep_market_view_->shortcode(), p_dep_market_view_->exch_source());
      economic_events_manager_.AllowEconomicEventsFromList(p_dep_market_view_->shortcode());
    }
    readiness_required_vec_.clear();
  }

  ~IndicatorLogger() { bulk_file_writer_.Close(); }

  void SetImpliedPriceIndicator(BaseImpliedPrice* _implied_price_type_) {
    p_implied_price_indicator_ = _implied_price_type_;
    use_implied_price_ = true;
  }

  inline const std::string& model_filename() const { return model_filename_; }

  void AddIndicator(CommonIndicator* p_this_indicator_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      p_this_indicator_->add_unweighted_indicator_listener(indicator_vec_.size(), this);
      indicator_vec_.push_back(p_this_indicator_);

      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false) {
        is_ready_vec_.push_back(false);
        // Setting readiness required true for all, as better not to print then print wrong values
        // Changes the regression weights.
        readiness_required_vec_.push_back(true);
        is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
      } else {
        is_ready_vec_.push_back(true);
        readiness_required_vec_.push_back(false);
      }
      prev_value_vec_.push_back(0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL p_this_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void AddIndicatorName(std::string t_indicator_name_){

	  indicator_name_vec_.push_back(t_indicator_name_);
  }

  std::vector<std::string> GetIndicatorNameVec(){
	  return indicator_name_vec_;
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_weight_, bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_weight_);
      indicator_vec_.push_back(p_this_indicator_);

      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false) {
        is_ready_vec_.push_back(false);
        // we have argument for this !
        readiness_required_vec_.push_back(true);
        is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
      } else {
        is_ready_vec_.push_back(true);
        readiness_required_vec_.push_back(false);
      }
      prev_value_vec_.push_back(0.0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void AddIndicator(CommonIndicator* p_this_indicator_, const double& _this_alpha_, const double& _this_beta_,
                    bool _readiness_required_) {
    if (p_this_indicator_ != NULL) {
      p_this_indicator_->add_indicator_listener(indicator_vec_.size(), this, _this_alpha_, _this_beta_);
      indicator_vec_.push_back(p_this_indicator_);

      const bool t_is_this_indicator_ready_ = p_this_indicator_->IsIndicatorReady();
      if (t_is_this_indicator_ready_ == false) {
        is_ready_vec_.push_back(false);
        // we have argument for this !
        readiness_required_vec_.push_back(true);
        is_ready_ = false;  // since is_ready_vec_ is now sure not to be false;
      } else {
        is_ready_vec_.push_back(true);
        readiness_required_vec_.push_back(false);
      }
      prev_value_vec_.push_back(0.0);
    } else {
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
        DBGLOG_TIME_CLASS_FUNC << "NULL t_indicator_" << DBGLOG_ENDL_FLUSH;
      }
      ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
    }
  }

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
        indicator_vec_[i]->SubscribeDataInterrupts(market_update_manager_);
      }
    }
  }

  void set_basepx_pxtype() {
    if (p_dep_market_view_ != NULL) {
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        indicator_vec_[i]->set_basepx_pxtype(*p_dep_market_view_, dep_baseprice_type_);
      }
    }
  }

  void set_start_end_mfm() {
    if (p_dep_market_view_ != NULL) {
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        indicator_vec_[i]->set_start_mfm(min_msec_toprint_);
        indicator_vec_[i]->set_end_mfm(max_msec_toprint_);
      }
    }
  }

  void FinishCreation() {
    if (is_ready_vec_.empty()) {
      is_ready_ = true;
      if (dbglogger_.CheckLoggingLevel(DBG_MODEL_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "No Indicators! So by default Ready!" << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (!is_ready_) {
      is_ready_vec_[_indicator_index_] = true;
      is_ready_ = AreAllReady();

      if ((!is_ready_) && (VectorUtils::CheckAllForValue(is_ready_vec_, false) == false) &&
          ((last_indicators_debug_print_ == 0) ||
           (watch_.msecs_from_midnight() >
            last_indicators_debug_print_ + 2000))) {  // some indicator is not ready but at least one other is ready
        last_indicators_debug_print_ = watch_.msecs_from_midnight();

        for (auto i = 0u; i < indicator_vec_.size(); i++) {
          if (!is_ready_vec_[i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (indicator_vec_[i]->IsIndicatorReady()) {  // if it was secretly ready
              is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady();
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready " << indicator_vec_[i]->concise_indicator_description()
                                       << DBGLOG_ENDL_FLUSH;
                // DBGLOG_DUMP ; // no dump since this is always in historical
                indicator_vec_[i]->WhyNotReady();
              }
            }
          }
        }
      }

      if (is_ready_) {
        if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
        }
      }
    }

    prev_value_vec_[_indicator_index_] = _new_value_;  // doing this irrespective of readiness of all indicators, so
    // that we always have correct indicator_vals in prev_value_vec_
  }

  // IndicatorLogger is notified after all the indicators are updated.
  // Indicators get updates from
  // (i) Portfolio which listens to SMV updates
  // (ii) SMV updates
  // (iii) GlobalPositionChange from PromOrderManager
  inline void SMVOnReady() { PrintVals(); }
  inline void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) { PrintVals(); }

#ifdef USING_PRETRADE_PRINTVALS
  inline void SMVPreTrade() { PrintVals(); }
#endif

  /// time support
  inline void setStartTM(int hhmm) { min_msec_toprint_ = GetMsecsFromMidnightFromHHMM(hhmm); }
  inline void setEndTM(int start_date_, int end_date_, int hhmm) {
    max_msec_toprint_ = GetMsecsFromMidnightFromHHMM(hhmm) + (end_date_ - start_date_) * 86400000;
  }

  inline bool PrintErrors() {
    bool return_value_ = false;
    if (!is_ready_) {
      return_value_ = true;
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        if (!is_ready_vec_[i]) {
          std::cerr << "ERROR: " << (i + 1) << " indicator not ready "
                    << indicator_vec_[i]->concise_indicator_description() << "\n";
        }
      }
    }
    return return_value_;
  }

 protected:
  inline unsigned long long l1events() const {
    return (p_dep_market_view_ != NULL) ? (p_dep_market_view_->l1events()) : (0u);
  }

  inline unsigned long long CoreShortCodesAndDepl1events() const {
    unsigned int l1all = 0u;

    // since core_shortcodes already has dep ...
    // l1all = ( p_dep_market_view_ != NULL ) ? ( p_dep_market_view_->l1events() ) : ( 0u ) ;

    for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
      if (p_sampling_shc_smv_vec_[i] != NULL) {
        l1all += p_sampling_shc_smv_vec_[i]->l1events();
      }
    }
    return l1all;
  }

  inline unsigned int num_trades() const {
    return (p_dep_market_view_ != NULL) ? (p_dep_market_view_->num_trades()) : (0u);
  }

  inline const bool ReachedTimePeriodToPrint() const {
    if (sample_on_core_shortcodes_ == 10u || sample_on_core_shortcodes_ == 12u) {
      return ((last_print_msecs_from_midnight_ == 0u) ||
              (watch_.msecs_from_midnight() > (int)(last_print_msecs_from_midnight_ + 1000000)));
    }
    return ((last_print_msecs_from_midnight_ == 0u) ||
            (watch_.msecs_from_midnight() > (int)(last_print_msecs_from_midnight_ + msecs_to_wait_to_print_again_)));
  }

  inline const bool ReachedEventPeriodToPrint() const {
    if (sample_on_core_shortcodes_ == 1u) {
      return (ReachedDepAndCoreIndepEventPeriodToPrint());
    } else if (sample_on_core_shortcodes_ == 2u) {
      return (ReachedDepOrAnyCoreIndepEventPeriodToPrint());
    } else if (sample_on_core_shortcodes_ == 3u) {
      return (ReachedTriggerThreshold());
    } else {
      return ((l1events_timeout_ > 0u) && (l1events() > last_print_l1events_ + l1events_timeout_));
    }
  }

  inline const unsigned int ReachedNumTradesToPrint() const {
    if (sample_on_core_shortcodes_ == 7u || sample_on_core_shortcodes_ == 8u || sample_on_core_shortcodes_ == 11u ||
        sample_on_core_shortcodes_ == 12u) {
      return false;
    } else if (sample_on_core_shortcodes_ == 9u) {
      // c1 based trade sampling
      unsigned int sum_trades = 0;
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (p_sampling_shc_smv_vec_[i] != NULL) {
          sum_trades += p_sampling_shc_smv_vec_[i]->num_trades();
        }
      }
      return ((num_trades_to_wait_print_again_ > 0u) &&
              sum_trades > last_print_num_trades_ + num_trades_to_wait_print_again_);
    } else if (sample_on_core_shortcodes_ == 10u) {
      // c2 based trade sampling
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (trigger_thresholds_[i] > 0u &&
            p_sampling_shc_smv_vec_[i]->num_trades() > last_print_trigger_values_[i] + trigger_thresholds_[i]) {
          return true;
        }
      }
      return false;
    } else {
      return ((num_trades_to_wait_print_again_ > 0u) &&
              (num_trades() > last_print_num_trades_ + num_trades_to_wait_print_again_));
    }
  }

  inline const unsigned int ReachedNumTradeSizeToPrint() const {
    if (sample_on_core_shortcodes_ != 7u && sample_on_core_shortcodes_ != 8u && sample_on_core_shortcodes_ != 11u &&
        sample_on_core_shortcodes_ != 12u) {
      return false;
    } else if (sample_on_core_shortcodes_ == 11u) {
      // c1 based trade sampling
      unsigned int sum_tradesize = 0;
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (p_sampling_shc_smv_vec_[i] != NULL) {
          sum_tradesize += num_tradesize_vec_[i];
        }
      }
      return ((num_tradesize_to_wait_print_again_ > 0u) &&
              sum_tradesize > last_print_num_tradesize_ + num_tradesize_to_wait_print_again_);
    } else if (sample_on_core_shortcodes_ == 12u) {
      // c2 based trade sampling
      for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
        if (trigger_thresholds_[i] > 0u &&
            num_tradesize_vec_[i] > last_print_trigger_values_[i] + trigger_thresholds_[i]) {
          return true;
        }
      }
      return false;
    } else {
      return ((num_tradesize_to_wait_print_again_ > 0u) &&
              (num_tradesize_ > last_print_num_tradesize_ + num_tradesize_to_wait_print_again_));
    }
  }

  inline const bool ReachedDepAndCoreIndepEventPeriodToPrint() const  // reached dep + core_indep event period
  {
    return ((l1events_timeout_ > 0u) && (CoreShortCodesAndDepl1events() > last_print_l1events_ + l1events_timeout_));
  }

  inline const bool ReachedTriggerThreshold() const  // reached dep + core_indep event period
  {
    for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
      //	  std::cout << " checking " << p_sampling_shc_smv_vec_[ i ] -> shortcode ( ) << " " <<
      // p_sampling_shc_smv_vec_[ i ]-> mkt_size_weighted_price ( ) << " " <<  last_print_trigger_values_[ i ] << " " <<
      // trigger_thresholds_[ i ] << "\n" ;
      if (trigger_thresholds_[i] > 0 &&
          std::fabs(p_sampling_shc_smv_vec_[i]->mkt_size_weighted_price() - last_print_trigger_values_[i]) >
              trigger_thresholds_[i]) {
        //  std::cout << " printing because of " << p_sampling_shc_smv_vec_[ i ] -> shortcode ( ) << "\n" ;
        return true;
      }
    }
    return false;
  }

  inline const bool ReachedDepOrAnyCoreIndepEventPeriodToPrint()
      const  // not a const function reached dep or core_indep event period
  {
    for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
      if (trigger_thresholds_[i] > 0u &&
          p_sampling_shc_smv_vec_[i]->l1events() > last_print_trigger_values_[i] + trigger_thresholds_[i]) {
        return true;
      }
    }
    return false;
  }

 protected:
  inline bool AreAllReady() {
    // if dependent is not ready, all not ready
    if (p_dep_market_view_ != NULL && !p_dep_market_view_->is_ready()) return false;

    // instead of just checking is ready of all indicators,
    // only checking for indicators with readiness_required_vec_ [i] == true
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false && readiness_required_vec_[i] == true) {
        return false;
      }
    }
    return true;
    // earlier was
    // return VectorUtils::CheckAllForValue ( is_ready_vec_, true ) ;
  }

  inline virtual void PrintVals() {
    if ((is_ready_) && (ReachedTimePeriodToPrint() || ReachedEventPeriodToPrint() || ReachedNumTradesToPrint() ||
                        ReachedNumTradeSizeToPrint()) &&
        (watch_.msecs_from_midnight() > min_msec_toprint_) && (watch_.msecs_from_midnight() < max_msec_toprint_) &&
        (economic_events_allows_print_) &&
        (!only_print_on_economic_events_ || (only_print_on_economic_events_ && currently_tradable_)) &&
        (regime_mode_to_print_ > 0 ? prev_value_vec_[0] == regime_mode_to_print_ : true)) {
      // std::cerr << watch_.YYYYMMDD ( ) << ' ' << watch_.msecs_from_midnight() << ' '
      // 	    << p_dep_market_view_->bestbid_int_price( ) << ' ' << p_dep_market_view_->bestask_int_price() << ' '
      // 	    << p_dep_market_view_->spread_increments( ) << ' '
      // 	    << p_dep_market_view_->bestbid_price ( ) << ' ' << p_dep_market_view_->bestask_price( ) << ' '
      // 	    << p_dep_market_view_->bestbid_size() << ' ' << p_dep_market_view_->bestbid_ordercount () << ' '
      // 	    << p_dep_market_view_->bestask_size ( ) << ' ' << p_dep_market_view_->bestask_ordercount () << ' '
      // 	    << p_dep_market_view_->mid_price() << ' ' << p_dep_market_view_->mkt_size_weighted_price ( ) << ' '
      // 	    << p_dep_market_view_->mkt_sinusoidal_price ( ) << ' ' << p_dep_market_view_->order_weighted_price (
      // ) << std::endl;

      if (sample_on_core_shortcodes_ == 1u) {
        last_print_l1events_ = CoreShortCodesAndDepl1events();
      } else if (sample_on_core_shortcodes_ == 9u) {
        last_print_num_trades_ = 0;
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          if (p_sampling_shc_smv_vec_[i] != NULL) {
            last_print_num_trades_ += p_sampling_shc_smv_vec_[i]->num_trades();
          }
        }
      } else if (sample_on_core_shortcodes_ == 11u) {
        last_print_num_tradesize_ = 0;
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          if (p_sampling_shc_smv_vec_[i] != NULL) {
            last_print_num_tradesize_ += num_tradesize_vec_[i];
          }
        }
      } else if (sample_on_core_shortcodes_ == 2u) {
        // last_print_l1events_ = l1events();
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          //		  if ( trigger_thresholds_[ i ] > 0 &&
          //    p_sampling_shc_smv_vec_[ i ]->l1events( ) > last_print_trigger_values_[ i ] + trigger_thresholds_[ i ]
          //    )  // this is tricky, should we use this check, think  independence of events ?
          { last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->l1events(); }
        }
      } else if (sample_on_core_shortcodes_ == 10u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->num_trades();
        }
      } else if (sample_on_core_shortcodes_ == 12u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          last_print_trigger_values_[i] = num_tradesize_vec_[i];
        }
      } else if (sample_on_core_shortcodes_ == 3u) {
        for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
          //		  if ( trigger_thresholds_[ i ] > 0 &&
          //   abs ( p_sampling_shc_smv_vec_[ i ]-> mkt_size_weighted_price ( ) - last_print_trigger_values_[ i ] ) >
          //   trigger_thresholds_[ i ] )
          {
            //		    std::cout << p_sampling_shc_smv_vec_[ i ] -> mkt_size_weighted_price ( ) << "\t" ;
            last_print_trigger_values_[i] = p_sampling_shc_smv_vec_[i]->mkt_size_weighted_price();
          }
        }
      } else {
        last_print_l1events_ = l1events();
        last_print_num_trades_ = num_trades();
        last_print_num_tradesize_ = num_tradesize_;
      }

      bulk_file_writer_ << watch_.msecs_from_midnight() << ' ' << l1events();

      if (!print_pbat_bias_) {
        if (use_implied_price_ && p_implied_price_indicator_ && p_implied_price_indicator_->IsIndicatorReady()) {
          bulk_file_writer_ << ' ' << p_implied_price_indicator_->GetBaseImpliedPrice(dep_pred_price_type_) << ' '
                            << p_implied_price_indicator_->GetBaseImpliedPrice(dep_baseprice_type_);
        } else if (p_dep_market_view_ != NULL) {
	  if (dep_baseprice_type_ != PriceType_t::kPriceTypeDUMMY) {
	    bulk_file_writer_ << ' ' << p_dep_market_view_->price_from_type(dep_pred_price_type_);
	  }
	  if (dep_pred_price_type_ != PriceType_t::kPriceTypeDUMMY) {
	    bulk_file_writer_ << ' ' << p_dep_market_view_->price_from_type(dep_baseprice_type_);
	  }
        }

        for (unsigned int i = (regime_mode_to_print_ > 0 ? 1 : 0); i < indicator_vec_.size(); i++) {
          bulk_file_writer_ << ' ' << prev_value_vec_[i];
        }
      } else {
        bulk_file_writer_ << ' ' << p_dep_market_view_->price_from_type(dep_baseprice_type_) << ' '
                          << p_dep_market_view_->market_update_info_.bestbid_price_ << ' '
                          << p_dep_market_view_->market_update_info_.bestask_price_;
        double t_tgt_bias_ = 0.0;
        for (unsigned int i = (regime_mode_to_print_ > 0 ? 1 : 0); i < indicator_vec_.size(); i++) {
          t_tgt_bias_ += prev_value_vec_[i];
        }
        bulk_file_writer_ << ' ' << t_tgt_bias_;
      }
      bulk_file_writer_ << '\n';
      bulk_file_writer_.CheckToFlushBuffer();

      last_print_msecs_from_midnight_ = watch_.msecs_from_midnight();
      // last_print_num_trades_ = num_trades();
      // last_print_num_tradesize_ = num_tradesize_;
    }
  }

  /**
   * Need to write because we are listening to SMVChangeListener. This is done because we want to sample based on volume
   * (cummulative trade sizes). For that we need OnTradePrint.
   * @param _security_id_
   * @param _market_update_info_
   */
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    // num_tradesize_ += _trade_print_info_.size_traded_;
    if (p_dep_market_view_->security_id() == _security_id_) {
      num_tradesize_ += _trade_print_info_.size_traded_;
    }
    for (auto i = 0u; i < p_sampling_shc_smv_vec_.size(); i++) {
      if (p_sampling_shc_smv_vec_[i]->security_id() == _security_id_) {
        num_tradesize_vec_[i] += _trade_print_info_.size_traded_;
        break;
      }
    }
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_) {
    int applicable_severity_ = 0;
    for (auto i = 0u; i < ezone_vec_.size(); i++) {
      applicable_severity_ += economic_events_manager_.GetCurrentSeverity(ezone_vec_[i]);
    }

    currently_tradable_ = false;
    if (economic_events_manager_.GetCurrentTradability()) {
      currently_tradable_ = true;
    }

    if (currently_tradable_) {
      applicable_severity_ = 0;
    }

    if (applicable_severity_ >= 2) {
      if (!getflat_due_to_economic_times_) {
        getflat_due_to_economic_times_ = true;

        if (to_print_on_economic_times_ == 0u) {
          economic_events_allows_print_ = false;
        } else {
          economic_events_allows_print_ = true;
        }
      }
    } else {
      if (getflat_due_to_economic_times_) {
        getflat_due_to_economic_times_ = false;

        if (to_print_on_economic_times_ == 2u) {
          economic_events_allows_print_ = false;
        } else {
          economic_events_allows_print_ = true;
        }
      }
    }
  }
};
}
