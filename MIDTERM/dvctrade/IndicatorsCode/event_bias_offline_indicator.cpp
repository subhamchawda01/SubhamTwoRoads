/**
    \file IndicatorsCode/event_bias_indicator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/event_bias_offline_indicator.hpp"

namespace HFSAT {

void EventBiasOfflineIndicator::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string> &_ors_source_needed_vec_,
                                                  const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

EventBiasOfflineIndicator *EventBiasOfflineIndicator::GetUniqueInstance(DebugLogger &t_dbglogger_,
                                                                        const Watch &r_watch_,
                                                                        const std::vector<const char *> &r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _event_id_ _half_life_
  if (r_tokens_.size() >= 6) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atoi(r_tokens_[4]), atof(r_tokens_[5]));
  } else {
    ExitVerbose(kExitErrorCodeGeneral,
                "insufficient inputs to EventBiasOfflineIndicator : INDICATOR _this_weight_ _indicator_string_ "
                "_event_id_ _half_life_\n");
    return NULL;  // wont reach here , just to remove warning
  }
}

EventBiasOfflineIndicator *EventBiasOfflineIndicator::GetUniqueInstance(DebugLogger &t_dbglogger_,
                                                                        const Watch &r_watch_,
                                                                        SecurityMarketView &_indep_market_view_,
                                                                        int _cat_id_, double _half_life_seconds_) {
  static std::map<std::string, EventBiasOfflineIndicator *> concise_indicator_description_map_;

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _cat_id_ << ' ' << _half_life_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new EventBiasOfflineIndicator(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _cat_id_, _half_life_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

EventBiasOfflineIndicator::EventBiasOfflineIndicator(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                     const std::string &concise_indicator_description_,
                                                     SecurityMarketView &_indep_market_view_, int _cat_id_,
                                                     double _half_life_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      event_pr_st_msecs_(30000),
      moving_sumpx_pr_(0),
      count_px_pr_(0),
      avgpx_pr_(0),
      curr_px_(0),
      pxch_pred_(0),
      is_active_(false),
      af_feeds_recv_(false),
      VAL_EPSILON(VAL_EPSILON_INT * indep_market_view_.min_price_increment()),
      DIFF_EPSILON(DIFF_EPSILON_INT * indep_market_view_.min_price_increment()) {
  DBGLOG_TIME_CLASS_FUNC << "In EventBiasOfflineIndicator contructor" << DBGLOG_ENDL_FLUSH;

  event_.cat_id_ = (uint16_t)_cat_id_;
  event_.act_margin_ = 1000 * indep_market_view_.min_price_increment();

  AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(&dbglogger_);
  catg_ = af_msgparser_.getCategoryforId(event_.cat_id_);
  if (catg_ == NULL) {
    DBGLOG_TIME_CLASS_FUNC << "No Category found for Id: " << event_.cat_id_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }

  readEventsScale();  // reads the lm-beta (scale_factor) for the event
  readMarginFile();   // reads the the maximum pxchange in reverse direction for the product and the scale for dynamic
                      // order size
  readEstimateEvents();  // reads the estimate values for the event

  /*   calc the decay factors for all durations  */
  val_decay_factor_ = MathUtils::CalcDecayFactor((int)_half_life_seconds_);
  double t_decay_val_ = 1;
  for (unsigned i = 0; i < VOLUME_DECAY_MAX_LEN; i++) {
    val_decay_vec_.push_back(t_decay_val_);
    t_decay_val_ *= val_decay_factor_;
  }
  last_viewed_msecs_ = -1;

  /*  subscribe to 100msecs timeperiod to regularly call the ProcessSignalUpdate in the unlikely event that marketupdate
   * is not called */
  watch_.subscribe_TimePeriod((TimePeriodListener *)this);

  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << kPriceTypeMidprice << std::endl;
  }

  /*   subscribe to Alphaflash feeds  */
  CombinedMDSMessagesAflashProcessor *aflash_mds_processor_ =
      HFSAT::CombinedMDSMessagesAflashProcessor::GetUniqueInstance(dbglogger_);
  aflash_mds_processor_->AddAflashListener((AflashListener *)this);
}

void EventBiasOfflineIndicator::ProcessSignalUpdate(const int num_pages_to_add_) {
  if (is_active_) {
    int secs_passed_since_ = (int)((watch_.msecs_from_midnight() - event_.event_mfm_) / 1000);
    /*   it has been long time since the event, back to inactive mode */
    if (secs_passed_since_ >= VAL_DECAY_MAX_LEN) {
      if (last_viewed_msecs_ != -1) {
        indicator_value_ = 0;
        last_viewed_msecs_ = -1;
        NotifyIndicatorListeners(indicator_value_);
      }
      return;
    } else {
      /*   either the first time here OR new updates exist */
      if (last_viewed_msecs_ == -1 || (fabs(indicator_value_) >= VAL_EPSILON && num_pages_to_add_ > 0)) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Curr_Price: " << curr_px_ << ", Prior_Price: " << avgpx_pr_
                                 << ", Predicted_PriceChange " << pxch_pred_ << DBGLOG_ENDL_FLUSH;
        }
        double pxch_act_ = curr_px_ - avgpx_pr_;  // pricechange_actual
        // double pxch_rem_ = pxch_pred_ - pxch_act_;  // price_expected - price_observed_yet

        double t_indicator_value_ = 0;
        /*   if both pxch_act_ & pxch_pred_ are in same direction
         *  OR they are in opp direction but the magnitude of pxchnage_actual is small */
        if (((pxch_act_ > 0 && pxch_pred_ > 0) || (pxch_act_ < 0 && pxch_pred_ < 0)) ||
            fabs(pxch_act_) <= event_.act_margin_) {
          t_indicator_value_ = pxch_pred_ * val_decay_vec_[secs_passed_since_];
        }
        if (fabs(t_indicator_value_) > 0 && fabs(t_indicator_value_) < VAL_EPSILON) {
          t_indicator_value_ = 0;
        }
        /*  If indicator value differs from the previous by a significant amount */
        if (fabs(t_indicator_value_ - indicator_value_) > DIFF_EPSILON) {
          indicator_value_ = t_indicator_value_;
          last_viewed_msecs_ = watch_.msecs_from_midnight();
          NotifyIndicatorListeners(indicator_value_);
        }
      }
    }
  }
}

/*   Called every 100msecs */
void EventBiasOfflineIndicator::OnTimePeriodUpdate(const int num_pages_to_add_) {
  ProcessSignalUpdate(num_pages_to_add_);
}

void EventBiasOfflineIndicator::OnMarketUpdate(const unsigned int _security_id_,
                                               const MarketUpdateInfo &_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      pxch_pred_ = 0;
      last_viewed_msecs_ = -1;
      ProcessSignalUpdate(1);
    }
  }
  curr_px_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_);
  int msecs_to_event_ = event_.event_mfm_ - watch_.msecs_from_midnight();
  /*   if event is yet to occur then update the prior price computation */
  if (msecs_to_event_ >= 0 && !is_active_) {
    if (msecs_to_event_ < event_pr_st_msecs_) {
      moving_sumpx_pr_ += curr_px_;
      count_px_pr_++;
    } else {
      moving_sumpx_pr_ = curr_px_;
      count_px_pr_ = 1;
    }
  }
}

std::vector<std::string> &EventBiasOfflineIndicator::split(const std::string &s, char delim,
                                                           std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

void EventBiasOfflineIndicator::readEventsScale() {
  std::string scale_file_ = "/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt";
  DBGLOG_TIME_CLASS_FUNC << " Reading EventScales from " << scale_file_ << DBGLOG_ENDL_FLUSH;
  std::ifstream scale_read_;
  scale_read_.open(scale_file_, std::ifstream::in);

  if (scale_read_.is_open()) {
    std::string line_;
    std::string t_shc_ = indep_market_view_.shortcode();
    const int line_length_ = 1024;
    char readline_buffer_[line_length_];

    /*  Line Format:
     *  <shc> <event_catg_id> <fid1:beta1> <fid2:beta2> .. */
    while (scale_read_.good()) {
      bzero(readline_buffer_, line_length_);
      scale_read_.getline(readline_buffer_, line_length_);
      PerishableStringTokenizer st_(readline_buffer_, line_length_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() > 2u) {
        if (t_shc_.compare(tokens_[0]) == 0 && atoi(tokens_[1]) == (int)event_.cat_id_) {
          for (unsigned indx_ = 2; indx_ < tokens_.size(); indx_++) {
            std::vector<std::string> t_tokens_;
            split(std::string(tokens_[indx_]), ':', t_tokens_);
            uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
            if (t_tokens_.size() < 2) {
              continue;
            }
            if (event_.scale_beta_.find(fid_) != event_.scale_beta_.end()) {
              DBGLOG_TIME_CLASS_FUNC << "Beta for Cat: " << event_.cat_id_ << ", " << fid_
                                     << " Already Present.. Ignoring.." << DBGLOG_ENDL_FLUSH;
              continue;
            }
            event_.scale_beta_[fid_] = stod(t_tokens_[1]);
            event_.cat_datum_.push_back(fid_);
            DBGLOG_TIME_CLASS_FUNC << " Assigned Cat: " << event_.cat_id_ << ", " << fid_ << " : "
                                   << event_.scale_beta_[fid_] << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
    if (event_.scale_beta_.empty()) {
      std::cerr << "Error: No Beta value provided for Event: " << event_.cat_id_ << std::endl;
      DBGLOG_TIME_CLASS_FUNC << "Error: No Beta value provided for Event: " << event_.cat_id_ << DBGLOG_ENDL_FLUSH;
      exit(1);
    }
  } else {
    std::cerr << " Error: Scale_File NOT opening: " << scale_file_ << std::endl;
    DBGLOG_TIME_CLASS_FUNC << " Error: Scale_File NOT opening: " << scale_file_ << DBGLOG_ENDL_FLUSH;
    exit(1);
  }
}

void EventBiasOfflineIndicator::readMarginFile() {
  std::string margin_file_ = "/spare/local/tradeinfo/Alphaflash/af_shortcode_margins.txt";
  DBGLOG_TIME_CLASS_FUNC << " Reading ProductMargins from " << margin_file_ << DBGLOG_ENDL_FLUSH;
  std::ifstream margin_read_;
  margin_read_.open(margin_file_, std::ifstream::in);

  /*  Line Format:
   *  <shc> <act_margin_> <getflat_margin_> <order_scale_> */
  if (margin_read_.is_open()) {
    std::string line_;
    std::string t_shc_ = indep_market_view_.shortcode();
    const int line_length_ = 1024;
    char readline_buffer_[line_length_];

    while (margin_read_.good()) {
      bzero(readline_buffer_, line_length_);
      margin_read_.getline(readline_buffer_, line_length_);
      PerishableStringTokenizer st_(readline_buffer_, line_length_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 0u || t_shc_.compare(tokens_[0]) != 0) {
        continue;
      }
      if (tokens_.size() > 1u) {
        event_.act_margin_ = atof(tokens_[1]) * indep_market_view_.min_price_increment();
        DBGLOG_TIME_CLASS_FUNC << " Assigned event_.act_margin_: " << event_.act_margin_ << DBGLOG_ENDL_FLUSH;
      }
      if (tokens_.size() > 2u) {
        event_.getflat_margin_ = atof(tokens_[2]) * indep_market_view_.min_price_increment();
        DBGLOG_TIME_CLASS_FUNC << " Assigned event_.getflat_margin_: " << event_.getflat_margin_ << DBGLOG_ENDL_FLUSH;
      }
      break;
    }
  }
}

void EventBiasOfflineIndicator::readEstimateEvents() {
  int date_ = watch_.YYYYMMDD();
  const int SKIP_LIMIT = 10;
  int skipped_ = 0;

  while (skipped_ < SKIP_LIMIT) {
    std::string estimates_file_ =
        std::string("/spare/local/tradeinfo/Alphaflash/Estimates/estimates_") + std::to_string(date_);
    DBGLOG_TIME_CLASS_FUNC << " Reading Estimate Data from " << estimates_file_ << DBGLOG_ENDL_FLUSH;
    std::ifstream estimates_read_;
    estimates_read_.open(estimates_file_, std::ifstream::in);
    DBGLOG_TIME_CLASS_FUNC << "Estimates File: " << estimates_file_ << DBGLOG_ENDL_FLUSH;

    bool found_ = false;
    if (estimates_read_.is_open()) {
      std::string line_;
      const int line_length_ = 1024;
      char readline_buffer_[line_length_];

      /*  Line Format:
       *  <shc> <event_catg_id> <fid1:estimate_val1> <fid2:estimate_val2> .. */
      while (estimates_read_.good()) {
        bzero(readline_buffer_, line_length_);
        estimates_read_.getline(readline_buffer_, line_length_);
        PerishableStringTokenizer st_(readline_buffer_, line_length_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() >= 3u) {
          uint16_t cat_id_t_ = (uint16_t)atoi(tokens_[0]);

          if (cat_id_t_ != event_.cat_id_) {
            continue;
          }
          found_ = true;
          event_.event_mfm_ = atoi(tokens_[1]);

          for (unsigned indx_ = 2; indx_ < tokens_.size(); indx_++) {
            std::vector<std::string> t_tokens_;
            split(std::string(tokens_[indx_]), ':', t_tokens_);
            uint8_t fid_ = (uint8_t)stoi(t_tokens_[0]);
            if (event_.scale_beta_.find(fid_) != event_.scale_beta_.end()) {
              event_.estimate_vals_[fid_] = stod(t_tokens_[1]);
              DBGLOG_TIME_CLASS_FUNC << "Estimates Value: " << event_.cat_id_ << ", " << fid_ << " : " << t_tokens_[1]
                                     << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }
    if (found_) {
      break;
    }

    skipped_++;
    date_ = DateTime::CalcPrevDay(date_);
  }

  for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
    if (event_.estimate_vals_.find(*it) == event_.estimate_vals_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "Estimate Cat_id_: " << event_.cat_id_ << ", datum_id_: " << *it
                             << " Missing.. Hence ignoring this message" << DBGLOG_ENDL_FLUSH;
      std::cerr << "Estimate Cat_id_: " << event_.cat_id_ << ", datum_id_: " << *it
                << " Missing.. Hence ignoring this message\n";
      exit(1);
    }
  }
}

/*  When An alphaflash msg arrives */
void EventBiasOfflineIndicator::onAflashMsgNew(int uid_, timeval time_, char symbol_[4], uint8_t type_,
                                               uint8_t version_, uint8_t nfields_,
                                               AFLASH_MDS::AFlashDatum fields[AFLASH_MDS::MAX_FIELDS],
                                               uint16_t category_id_) {
  if (!af_feeds_recv_) {
    DBGLOG_TIME_CLASS_FUNC << "Alphaflash Message Received in EventBiasOfflineIndicator" << DBGLOG_ENDL_FLUSH;
    af_feeds_recv_ = true;
  }

  /*  If currently, inactive and the msg belong to this event and it of Release type */
  if (!is_active_ && category_id_ == event_.cat_id_ && type_ == (uint8_t)AF_MSGSPECS::kRelease) {
    // DBGLOG_TIME_CLASS_FUNC << next_event_->ToString() << DBGLOG_ENDL_FLUSH;

    AF_MSGSPECS::AF_MsgParser &af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(&dbglogger_);
    AF_MSGSPECS::Message *msg_ = af_msgparser_.getMsgFromCatg(catg_, (short)type_);  // AF_MSGSPECS::kRelease );
    for (int i = 0; i < nfields_; i++) {
      if (event_.estimate_vals_.find(fields[i].field_id_) != event_.estimate_vals_.end() &&
          event_.scale_beta_.find(fields[i].field_id_) != event_.scale_beta_.end()) {
        AF_MSGSPECS::Field *t_field_ = af_msgparser_.getFieldFromMsg(msg_, fields[i].field_id_);
        switch (t_field_->field_type_) {
          case AF_MSGSPECS::kFloat:
          case AF_MSGSPECS::kDouble:
            event_.actual_vals_[fields[i].field_id_] = fields[i].data_.vFloat;
            break;
          case AF_MSGSPECS::kShort_value_enumeration:
          case AF_MSGSPECS::kLong:
          case AF_MSGSPECS::kShort:
          case AF_MSGSPECS::kInt:
            event_.actual_vals_[fields[i].field_id_] = fields[i].data_.vInt;
            break;
          case AF_MSGSPECS::kBoolean:
            DBGLOG_TIME_CLASS_FUNC << "Error: Boolean value not identified" << DBGLOG_ENDL_FLUSH;
            break;
          default:
            DBGLOG_TIME_CLASS_FUNC << "Error: Not identified" << DBGLOG_ENDL_FLUSH;
            break;
        }
      }
    }
    for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
      if (event_.actual_vals_.find(*it) == event_.actual_vals_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "Message Cat_id_: " << event_.cat_id_ << ", datum_id_: " << *it
                               << " Missing.. Hence ignoring this message" << DBGLOG_ENDL_FLUSH;
        return;
      }
    }

    /*  Computes the predicted price_change */
    pxch_pred_ = 0;
    for (auto it = event_.cat_datum_.begin(); it != event_.cat_datum_.end(); ++it) {
      DBGLOG_TIME_CLASS_FUNC << " pxch: " << *it << " " << event_.scale_beta_[*it] << " "
                             << (event_.actual_vals_[*it] - event_.estimate_vals_[*it]) << DBGLOG_ENDL_FLUSH;
      pxch_pred_ += event_.scale_beta_[*it] * (event_.actual_vals_[*it] - event_.estimate_vals_[*it]);
    }

    event_.mfm_received_ = watch_.msecs_from_midnight();
    avgpx_pr_ = moving_sumpx_pr_ / count_px_pr_;
    is_active_ = true;
    DBGLOG_TIME_CLASS_FUNC << "EventBiasOfflineIndicator Initials: " << event_.mfm_received_ << " " << pxch_pred_ << " "
                           << avgpx_pr_ << DBGLOG_ENDL_FLUSH;
    ProcessSignalUpdate(1);  // CALL THE LOGIC
  }
}
}
