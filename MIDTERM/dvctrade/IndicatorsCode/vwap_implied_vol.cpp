/**
   \file IndicatorsCode/vwap_implied_vol.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

/*
  listen to vwap of future and option ( both are decayed by num_trades )
  use option object to infer iv
  update based on vega

*/

#include "dvctrade/Indicators/vwap_implied_vol.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"


namespace HFSAT {

  void VWAPImpliedVol::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
					 std::vector<std::string>& _ors_source_needed_vec_,
					 const std::vector<const char*>& r_tokens_) {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
				 NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  }

  VWAPImpliedVol* VWAPImpliedVol::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
						    const std::vector<const char*>& r_tokens_,
						    PriceType_t _basepx_pxtype_) {
    // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_ _price_type_
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(
	 NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
    return GetUniqueInstance(t_dbglogger_, r_watch_,
			     *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
			     atoi(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
  }
  
  VWAPImpliedVol* VWAPImpliedVol::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
						    SecurityMarketView& _option_market_view_,
						    int t_halflife_num_trades_,
						    PriceType_t _price_type_) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << VarName() << ' ' << _option_market_view_.secname() << ' ' << t_halflife_num_trades_ << ' '
		<< PriceType_t_To_String(_price_type_);
    std::string concise_indicator_description_(t_temp_oss_.str());

    if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
	global_concise_indicator_description_map_.end()) {
      global_concise_indicator_description_map_[concise_indicator_description_] =
        new VWAPImpliedVol(t_dbglogger_, r_watch_, concise_indicator_description_, _option_market_view_,
			   t_halflife_num_trades_, _price_type_);
    }
    return dynamic_cast<VWAPImpliedVol*>(global_concise_indicator_description_map_[concise_indicator_description_]);
  }

  VWAPImpliedVol::VWAPImpliedVol(DebugLogger& t_dbglogger_, const Watch& r_watch_,
				 const std::string& concise_indicator_description_,
				 SecurityMarketView& _option_market_view_,
				 int _num_trades_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      option_market_view_(_option_market_view_),
      future_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(option_market_view_.shortcode())))),
		   current_options_vwap_(0.0),
		   current_fut_vwap_(0.0){


    fut_vwap_indicator_ = VolumeWeightedPrice::GetUniqueInstance(dbglogger_, watch_, future_market_view_, _num_trades_, _price_type_);
    fut_vwap_indicator_->add_unweighted_indicator_listener(1u, this);

    opt_vwap_indicator_ = VolumeWeightedPrice::GetUniqueInstance(dbglogger_, watch_, option_market_view_, _num_trades_, _price_type_);
    opt_vwap_indicator_->add_unweighted_indicator_listener(2u, this);


    option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, option_market_view_.shortcode());

  }

  void VWAPImpliedVol::WhyNotReady() {
    if (!is_ready_) {
      if (!(future_market_view_.is_ready())) {
	DBGLOG_TIME_CLASS << future_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
	DBGLOG_DUMP;
      }
      if (!(option_market_view_.is_ready_complex2(1))) {
	DBGLOG_TIME_CLASS << option_market_view_.secname() << " is_ready_complex2(1) = false " << DBGLOG_ENDL_FLUSH;
	DBGLOG_DUMP;
      }
    }
  }

  void VWAPImpliedVol::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _average_price_) {
    if (_indicator_index_ == 1u) {
      current_fut_vwap_ = _average_price_;
    } else if (_indicator_index_ == 2u) {
      current_options_vwap_ = _average_price_;      
    }

    if ( ! is_ready_ ) {
      if(current_options_vwap_ > 0 && current_fut_vwap_ > 0) {
	is_ready_ = true;
      }
    } else {
      indicator_value_ = option_->MktImpliedVol(current_fut_vwap_, current_options_vwap_);
      if (!std::isnan(indicator_value_*0)) {
	NotifyIndicatorListeners(indicator_value_);
      }
    }   
  }

  void VWAPImpliedVol::InitializeValues() {
    indicator_value_ = 0;
  }

  // market_interrupt_listener interface

  void VWAPImpliedVol::OnMarketDataInterrupted(const unsigned int _security_id_,
					       const int msecs_since_last_receive_) {
  }

  void VWAPImpliedVol::OnMarketDataResumed(const unsigned int _security_id_) {
  }
}
