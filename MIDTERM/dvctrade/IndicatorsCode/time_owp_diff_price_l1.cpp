/**
    \file IndicatorsCode/diff_price_type.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/time_owp_diff_price_l1.hpp"

namespace HFSAT {

void TimeOwpDiffPriceL1::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                           std::vector<std::string> &_ors_source_needed_vec_,
                                           const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TimeOwpDiffPriceL1 *TimeOwpDiffPriceL1::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                          const std::vector<const char *> &r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[3]));

  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 6) {
    if (r_tokens_.size() < 5) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "TimeOwpDiffPriceL1 incorrect syntax should b Indicator _this_weight_ _indicator_string_ "
                  "_idnep_market_view _fractional_seconds_ _price_type_");
    } else {
      t_dbglogger_ << "TimeOwpDiffPriceL1 incorrect syntax should b Indicator _this_weight_ _indicator_string_ "
                      "_idnep_market_view _fractional_seconds_ _price_type_"
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = _basepx_pxtype_;
    }
  } else {
    if (std::string(r_tokens_[5]).compare("#") == 0) {
      t_price_type_ = _basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(r_tokens_[5]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[3]))),
                           atof(r_tokens_[4]), t_price_type_);
}

TimeOwpDiffPriceL1 *TimeOwpDiffPriceL1::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                          SecurityMarketView &t_indep_market_view_,
                                                          double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeOwpDiffPriceL1 *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TimeOwpDiffPriceL1(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                               _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeOwpDiffPriceL1::TimeOwpDiffPriceL1(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                       const std::string &concise_indicator_description_,
                                       SecurityMarketView &t_indep_market_view_, double _fractional_seconds_,
                                       PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_indep_market_view_, _fractional_seconds_))),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      watch_(r_watch_),
      last_new_page_msecs_(0),
      page_width_msecs_(60000),
      alpha_(0.1),
      last_recorded_l1events_(0),
      current_l1events_avg_(0),
      first_time_(true),
      avg_l1size_(0),
      trade_price_avg_(0.0),
      k_factor_(0.0),
      max_significant_trade_sz_(0),
      trade_hist_msecs_((int)(1000 * _fractional_seconds_)),
      dyn_factor_(1.0),
      hist_l1_info_found_(false),
      last_level_shift_msecs_(0),
      last_bid_int_price_(0),
      last_ask_int_price_(0) {
  watch_.subscribe_BigTimePeriod(this);
  Initialize();
  if (!t_indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
  indep_market_view_.ComputeOrderWPrice();
  time_decayed_trade_info_manager_.compute_onelvlsumpxsz();
  time_decayed_trade_info_manager_.compute_onelvlsumsz();
  t_indep_market_view_.subscribe_tradeprints(this);
}

void TimeOwpDiffPriceL1::OnTimePeriodUpdate(const int num_pages_to_add_) {
  int num_pages_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);

  if (first_time_) {
    current_l1events_avg_ =
        (unsigned int)(current_l1events_avg_ + (indep_market_view_.l1events() - last_recorded_l1events_));
    last_recorded_l1events_ = indep_market_view_.l1events();
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ > page_width_msecs_) {
      first_time_ = false;
      last_new_page_msecs_ += (num_pages_ * page_width_msecs_);
    }
  } else {
    current_l1events_avg_ = (unsigned int)((1 - alpha_) * current_l1events_avg_ +
                                           (indep_market_view_.l1events() - last_recorded_l1events_));
    last_recorded_l1events_ = indep_market_view_.l1events();
  }
}

void TimeOwpDiffPriceL1::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                                      const MarketUpdateInfo &_market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void TimeOwpDiffPriceL1::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo &_market_update_info_) {
  if (!hist_l1_info_found_) {
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else {
#define MIN_SIGNIFICANT_SUM_SZ_TRADED 1.00
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2)) {
        is_ready_ = true;
        indicator_value_ = 0;
        NotifyIndicatorListeners(indicator_value_);
      }
    } else if (!data_interrupted_) {
      if (first_time_) {
        indicator_value_ = 0;

        NotifyIndicatorListeners(indicator_value_);
      } else {
        if ((last_bid_int_price_ != _market_update_info_.bestbid_int_price_) ||
            (last_ask_int_price_ != _market_update_info_.bestask_int_price_)) {
          last_level_shift_msecs_ = watch_.msecs_from_midnight();
          last_bid_int_price_ = _market_update_info_.bestbid_int_price_;
          last_ask_int_price_ = _market_update_info_.bestask_int_price_;
        }

        // need to compare against a low value since otherwise there would be weird values as the denominator recedes in
        // value
        if (time_decayed_trade_info_manager_.onelvlsumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
          trade_price_avg_ =
              (time_decayed_trade_info_manager_.onelvlsumpxsz_ / time_decayed_trade_info_manager_.onelvlsumsz_);
        } else {
          trade_price_avg_ = 0;
        }

        dyn_factor_ = (double)2 * current_l1events_avg_ /
                      (_market_update_info_.bestask_size_ + _market_update_info_.bestbid_size_);
        double l1_factor_ = std::min(1.0, 2 * (k_factor_ / dyn_factor_) / (1 + (k_factor_ / dyn_factor_)));
        if (std::isnan(l1_factor_)) {
          l1_factor_ = 1.0;
        }

        double trade_fac_ = std::max(
            0.0, std::min((time_decayed_trade_info_manager_.onelvlsumsz_ - 1) / max_significant_trade_sz_, 1.0));
        double mkt_adj_price_;

        // spread > tick
        if (_market_update_info_.spread_increments_ > 1) {
          mkt_adj_price_ = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_) / 2;

          mkt_adj_price_ = (1 - trade_fac_) * mkt_adj_price_ + trade_fac_ * trade_price_avg_;
          indicator_value_ = (SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_) +
                              l1_factor_ * (mkt_adj_price_ - SecurityMarketView::GetPriceFromType(
                                                                 kPriceTypeMidprice, _market_update_info_))) -
                             SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
        } else {
          // use OrderWPrice for 5 seconds if a level shift happens
          if (watch_.msecs_from_midnight() - last_level_shift_msecs_ < 5000) {
            mkt_adj_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeOrderWPrice, _market_update_info_);
            mkt_adj_price_ = (1 - trade_fac_) * mkt_adj_price_ + trade_fac_ * trade_price_avg_;
          } else {
            mkt_adj_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, _market_update_info_);
            mkt_adj_price_ = (1 - trade_fac_) * mkt_adj_price_ + trade_fac_ * trade_price_avg_;
          }

          indicator_value_ = (SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_) +
                              l1_factor_ * (mkt_adj_price_ - SecurityMarketView::GetPriceFromType(
                                                                 kPriceTypeMidprice, _market_update_info_))) -
                             SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
        }

        NotifyIndicatorListeners(indicator_value_);
      }
    }
  }
}

void TimeOwpDiffPriceL1::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void TimeOwpDiffPriceL1::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    time_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}

void TimeOwpDiffPriceL1::Initialize() {
  std::ifstream t_l1_book_avg_infile_;

  t_l1_book_avg_infile_.open("/spare/local/tradeinfo/OfflineInfo/l1_hist_avg.txt", std::ifstream::in);
  if (t_l1_book_avg_infile_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_l1_book_avg_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_l1_book_avg_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 6) {
        std::string dep_indep_portfolio_code_ = tokens_[0];
        int trade_half_life_msecs_ = atoi(tokens_[1]);
        if ((indep_market_view_.shortcode().compare(dep_indep_portfolio_code_) == 0) &&
            (trade_hist_msecs_ == trade_half_life_msecs_)) {
          avg_l1size_ = atof(tokens_[2]);
          page_width_msecs_ = atof(tokens_[3]);
          k_factor_ = atof(tokens_[4]) * page_width_msecs_ / (avg_l1size_ * 1000);
          max_significant_trade_sz_ = atof(tokens_[5]);
          hist_l1_info_found_ = true;
        }
      }
    }
  }
  alpha_ = (double)1000 / page_width_msecs_;
  last_recorded_l1events_ = indep_market_view_.l1events();
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}
}
