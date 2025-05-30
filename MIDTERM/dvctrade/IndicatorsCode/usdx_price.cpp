/**
    \file IndicatorsCode/usdx_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/usdx_price.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>

namespace HFSAT {

void USDXPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                  std::vector<std::string>& _ors_source_needed_vec_,
                                  const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("6E_0"));
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("6J_0"));
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("6B_0"));
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("6C_0"));
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("SEK_0"));
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)("6S_0"));
}

USDXPrice* USDXPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                        const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _price_type_
  if (r_tokens_.size() < 4) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_, "INDICATOR weight USDXPrice_price_type_ ");
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_, StringToPriceType_t(r_tokens_[3]));
}

USDXPrice* USDXPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, USDXPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new USDXPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

USDXPrice::USDXPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                     const std::string& concise_indicator_description_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), price_type_(_price_type_) {
  const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("6E_0")));
  const_secid_index_map_[const_smv_vec_[0]->security_id()] = 0;
  const_wt_vec_.push_back(-0.576);
  const_last_price_.push_back(0.0);
  const_is_ready_.push_back(false);
  const_normal_.push_back(10000);

  const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("6J_0")));
  const_secid_index_map_[const_smv_vec_[1]->security_id()] = 1;
  const_wt_vec_.push_back(-0.136);
  const_last_price_.push_back(0.0);
  const_is_ready_.push_back(false);
  const_normal_.push_back(1000000);

  const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("6B_0")));
  const_secid_index_map_[const_smv_vec_[2]->security_id()] = 2;
  const_wt_vec_.push_back(-0.119);
  const_last_price_.push_back(0.0);
  const_is_ready_.push_back(false);
  const_normal_.push_back(10000);

  const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("6C_0")));
  const_secid_index_map_[const_smv_vec_[3]->security_id()] = 3;
  const_wt_vec_.push_back(-0.091);
  const_last_price_.push_back(0.0);
  const_is_ready_.push_back(false);
  const_normal_.push_back(10000);

  if (watch_.YYYYMMDD() > 20140225 && watch_.YYYYMMDD() < 20141119)  // missing data
  {
    const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("SEK_0")));
    const_secid_index_map_[const_smv_vec_[4]->security_id()] = 4;
    const_wt_vec_.push_back(-0.042);

    std::stringstream ss1;
    ss1 << 20140225;
    boost::gregorian::date sd1_ = boost::gregorian::from_undelimited_string(ss1.str());

    std::stringstream ss2;
    ss2 << 20141119;
    boost::gregorian::date sd2_ = boost::gregorian::from_undelimited_string(ss2.str());

    std::stringstream ss3;
    ss3 << watch_.YYYYMMDD();
    boost::gregorian::date sd3_ = boost::gregorian::from_undelimited_string(ss3.str());

    boost::gregorian::days r1 = (sd2_ - sd1_);
    boost::gregorian::days r2 = (sd3_ - sd1_);
    boost::gregorian::days r3 = (sd2_ - sd3_);

    double w1 = (r2.days() / 365.0) / (r1.days() / 365.0);
    double w2 = (r3.days() / 365.0) / (r1.days() / 365.0);
    double p1 = 15379;
    double p2 = 13536;

    const_is_ready_.push_back(true);
    const_normal_.push_back(100000);
    const_last_price_.push_back((p2 * w1 + p1 * w2) / 100000);
  } else {
    const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("SEK_0")));
    const_secid_index_map_[const_smv_vec_[4]->security_id()] = 4;
    const_wt_vec_.push_back(-0.042);
    const_last_price_.push_back(0.0);
    const_is_ready_.push_back(false);
    const_normal_.push_back(100000);
  }

  const_smv_vec_.push_back((ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("6S_0")));
  const_secid_index_map_[const_smv_vec_[5]->security_id()] = 5;
  const_wt_vec_.push_back(-0.036);
  const_last_price_.push_back(0.0);
  const_is_ready_.push_back(false);
  const_normal_.push_back(10000);

  for (auto i = 0u; i < const_smv_vec_.size(); i++) {
    if (!const_is_ready_[i]) {
      if (!const_smv_vec_[i]->subscribe_price_type(this, price_type_)) {
        PriceType_t t_error_price_type_ = _price_type_;
        std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                  << " passed " << t_error_price_type_ << std::endl;
      }
    }
  }
}

void USDXPrice::WhyNotReady() {
  if (!is_ready_) {
    for (auto i = 0u; i < const_smv_vec_.size(); i++) {
      if (!(const_is_ready_[i])) {
        DBGLOG_TIME_CLASS << const_smv_vec_[i]->secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }
}

bool USDXPrice::AllAreReady() {
  for (auto i = 0u; i < const_smv_vec_.size(); i++) {
    if (!const_is_ready_[i]) {
      return false;
    }
  }
  return true;
}

void USDXPrice::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  // clean but not efficient:
  // ( current_const_price/previous_const_price)^weight_const * previous_index_price = current_index_price. After proper
  // initalization.
  double price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  unsigned index_ = const_secid_index_map_[_security_id_];
  price_ = price_ / const_normal_[index_];
  if (!is_ready_) {
    if (const_is_ready_[index_]) {
      const_last_price_[index_] = price_;
    } else if (const_smv_vec_[index_]->is_ready_complex(2)) {
      const_last_price_[index_] = price_;
      const_is_ready_[index_] = true;
    }
    if (AllAreReady()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = indicator_value_ * std::pow((price_ / const_last_price_[index_]), const_wt_vec_[index_]);
    const_last_price_[index_] = price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void USDXPrice::InitializeValues() {
  indicator_value_ = 50.14348112;
  for (auto i = 0u; i < const_smv_vec_.size(); i++) {
    indicator_value_ = indicator_value_ * std::pow(const_last_price_[i], const_wt_vec_[i]);
  }
}

// market_interrupt_listener interface
void USDXPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  NotifyIndicatorListeners(indicator_value_);
}

void USDXPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  data_interrupted_ = false;  // all are from same exchange ?
  InitializeValues();
}
}
