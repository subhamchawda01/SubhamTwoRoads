/**
    \file IndicatorsCode/diff_tdavg_tpx_basepx_lvl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_tdavg_tpx_basepx_lvl.hpp"

namespace HFSAT {

void DiffTDAvgTPxBasepxLvl::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DiffTDAvgTPxBasepxLvl* DiffTDAvgTPxBasepxLvl::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 6) {
    if (r_tokens_.size() < 5) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  " DiffTDAvgTPxBasepxlvl Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                  "_indicator_string_ _indep_market_view_ _fractional_seconds_ price_type_");
    } else {
      t_dbglogger_ << "DiffTDAvgTPxBasepxlvl Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                      "_indicator_string_ _indep_market_view_ _fractional_seconds_ price_type_"
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
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), t_price_type_);
}

DiffTDAvgTPxBasepxLvl* DiffTDAvgTPxBasepxLvl::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                SecurityMarketView& _indep_market_view_,
                                                                double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << " "
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffTDAvgTPxBasepxLvl*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DiffTDAvgTPxBasepxLvl(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                  _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffTDAvgTPxBasepxLvl::DiffTDAvgTPxBasepxLvl(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                             PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_))) {
  if (!_indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  time_decayed_trade_info_manager_.compute_lvlsumpx();
  time_decayed_trade_info_manager_.compute_lvlsumcoeffs();
  _indep_market_view_.subscribe_tradeprints(this);
}

void DiffTDAvgTPxBasepxLvl::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& cr_market_update_info_) {
#define MIN_SIGNIFICANT_SUM_COEFFS 0.50
  // need to compare against a low value since otherwise there would be weird values as the denominator recedes in value
  if (time_decayed_trade_info_manager_.lvlsumcoeffs_ > MIN_SIGNIFICANT_SUM_COEFFS) {
    if (time_decayed_trade_info_manager_.lvlsumcoeffs_ <
        1) {  // if the trades are pretty old then make this indicator weaker
      indicator_value_ = (time_decayed_trade_info_manager_.lvlsumpx_ -
                          (time_decayed_trade_info_manager_.lvlsumcoeffs_ *
                           SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_)));
    } else {
      indicator_value_ = (time_decayed_trade_info_manager_.lvlsumpx_ / time_decayed_trade_info_manager_.lvlsumcoeffs_) -
                         SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }
  } else {
    indicator_value_ = 0;
  }

  if (std::isnan(indicator_value_)) {
    indicator_value_ = 0;
    // std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    // exit ( 1 );
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void DiffTDAvgTPxBasepxLvl::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                         const MarketUpdateInfo& cr_market_update_info_) {
  // need to compare against a low value since otherwise there would be weird values as the denominator recedes in value
  if (time_decayed_trade_info_manager_.lvlsumcoeffs_ > MIN_SIGNIFICANT_SUM_COEFFS) {
    if (time_decayed_trade_info_manager_.lvlsumcoeffs_ <
        1) {  // if the trades are pretty old then make this indicator weaker
      indicator_value_ = (time_decayed_trade_info_manager_.lvlsumpx_ -
                          (time_decayed_trade_info_manager_.lvlsumcoeffs_ *
                           SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_)));
    } else {
      indicator_value_ = (time_decayed_trade_info_manager_.lvlsumpx_ / time_decayed_trade_info_manager_.lvlsumcoeffs_) -
                         SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }
  } else {
    indicator_value_ = 0;
  }

  if (std::isnan(indicator_value_)) {
    indicator_value_ = 0;
    // std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    // exit ( 1 );
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}
#undef MIN_SIGNIFICANT_SUM_COEFFS

void DiffTDAvgTPxBasepxLvl::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void DiffTDAvgTPxBasepxLvl::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    time_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}
}
