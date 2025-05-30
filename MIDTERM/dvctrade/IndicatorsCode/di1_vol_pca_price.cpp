/**
    \file IndicatorsCode/di1_vol_pca_price.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di1_vol_pca_price.hpp"

namespace HFSAT {

void DI1VolPCAPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 6u)  // doesnt mean indicator string syntax is valid .
  {
    // Adding more shortcodes here is harmless...not getting the running strip because date not available
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
  }
}

DI1VolPCAPrice* DI1VolPCAPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 7u) {
    ExitVerbose(kExitErrorCodeGeneral, "DI1VolPCAPrice needs 8 tokens");
    return NULL;
  }

  // INDICATOR 1.00 DI1CurveVolAdjustedPrice BR_DI_3 PORT VOL_FACTOR CURVE_FACTOR PRICE_TYPE HALFLIFE
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atoi(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

DI1VolPCAPrice* DI1VolPCAPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::string _port_, double _pc_index_,
                                                  PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _port_ << ' ' << _pc_index_ << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DI1VolPCAPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DI1VolPCAPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _port_, _pc_index_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DI1VolPCAPrice::DI1VolPCAPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_, const std::string _port_,
                               double _pc_index_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      price_type_t(_price_type_),
      sid_to_weights_(),
      sid_to_price_map_(),
      current_projected_price_() {
  std::vector<std::string> port_shortcodes_;
  std::vector<double> t_weights_;
  IndicatorUtil::LoadDI1EigenVector(_port_, t_weights_, r_watch_.YYYYMMDD(), _pc_index_);
  IndicatorUtil::AddDIPortfolioShortCodeVec(_port_, port_shortcodes_, r_watch_.YYYYMMDD());

  for (auto i = 0u; i < port_shortcodes_.size(); i++) {
    indep_market_view_vec_.push_back(
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(port_shortcodes_[i])));
    sid_to_price_map_[indep_market_view_vec_[i].security_id()] =
        CurveUtils::GetLastDayClosingPrice(r_watch_.YYYYMMDD(), port_shortcodes_[i]);
    sid_to_weights_[indep_market_view_vec_[i].security_id()] = t_weights_[i];
  }

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (!indep_market_view_vec_[i].subscribe_price_type(this, price_type_t)) {
      PriceType_t t_error_price_type_ = price_type_t;
      std::cerr << typeid(*this).name() << ":" << __func__ << ":" << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
}

void DI1VolPCAPrice::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  double current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  sid_to_price_map_[_security_id_] = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);

  if (!is_ready_) {
    is_ready_ = true;
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (!indep_market_view_vec_[i].is_ready_complex(2)) {
        is_ready_ = false;
        break;
      }
    }
  } else {
    current_projected_price_ +=
        sid_to_weights_[_security_id_] * (current_indep_price_ - sid_to_price_map_[_security_id_]);
    sid_to_price_map_[_security_id_] = current_indep_price_;
    indicator_value_ = current_projected_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}
}
