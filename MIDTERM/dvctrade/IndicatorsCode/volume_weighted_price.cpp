/**
    \file IndicatorsCode/volume_weighted_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "dvctrade/Indicators/volume_weighted_price.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

namespace HFSAT {

void VolumeWeightedPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

VolumeWeightedPrice* VolumeWeightedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
							    const std::vector<const char*>& r_tokens_, 
							    PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight VolumeWeightedPrice _sec_market_view_ _half_life_trades_ ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), _basepx_pxtype_);
}

VolumeWeightedPrice* VolumeWeightedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
							    SecurityMarketView& _sec_market_view_,
							    int t_halflife_trades_,
							    PriceType_t t_basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << t_halflife_trades_ << ' '
	      << PriceType_t_To_String(t_basepx_pxtype_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] =
        new VolumeWeightedPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _sec_market_view_, t_halflife_trades_);
  }
  return dynamic_cast<VolumeWeightedPrice*>(global_concise_indicator_description_map_[concise_indicator_description_]);
}

VolumeWeightedPrice::VolumeWeightedPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
					 const std::string& concise_indicator_description_,
					 SecurityMarketView& _sec_market_view_,
					 int t_halflife_trades_)
  : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
    sec_market_view_(_sec_market_view_),
    trade_decayed_trade_info_manager_(*(TradeDecayedTradeInfoManager::GetUniqueInstance(
											t_dbglogger_, r_watch_, _sec_market_view_, t_halflife_trades_))) {

  trade_decayed_trade_info_manager_.compute_sumpxsz();
  trade_decayed_trade_info_manager_.compute_sumsz();
  // we like to decay s(i) * p(i) and s(i) seperately, code will be easier
  sec_market_view_.subscribe_tradeprints(this);
}

void VolumeWeightedPrice::WhyNotReady() {
  if (!is_ready_) {
    if (!(sec_market_view_.is_ready_complex2(1))) {
      DBGLOG_TIME_CLASS << sec_market_view_.secname() << " is_ready_complex2 = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void VolumeWeightedPrice::OnTradePrint(const unsigned int _security_id_,
				       const TradePrintInfo& _trade_print_info_,
				       const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (sec_market_view_.is_ready_complex(2) ||
	(NSESecurityDefinitions::IsOption(sec_market_view_.shortcode()) && sec_market_view_.is_ready_complex2(1))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = trade_decayed_trade_info_manager_.sumpxsz_ / trade_decayed_trade_info_manager_.sumsz_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

  void VolumeWeightedPrice::InitializeValues() { indicator_value_ = sec_market_view_.mid_price(); }

// market_interrupt_listener interface
void VolumeWeightedPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
    data_interrupted_ = true;
}

void VolumeWeightedPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
