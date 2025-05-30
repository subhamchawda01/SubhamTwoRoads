/**
    \file Indicators/implied_price_spread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_IMPLIED_PRICE_SPREAD_HPP
#define BASE_INDICATORS_IMPLIED_PRICE_SPREAD_HPP

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

class ImpliedPriceSpread : public CommonIndicator {
 public:
  typedef enum { LEG1_MINUS_LEG2 = 1, LEG2_MINUS_LEG1, MAX_SPREAD_MATH_TYPE } SpreadMathType_t;

  /// Returns whether the spread contract has L1_L2 or L2_L1
  static SpreadMathType_t GetIndicatorType(const std::string& dep_shortcode_, const std::string& indep_1_shortcode_,
                                           const std::string& indep_2_shortcode_) {
    if (!dep_shortcode_.compare(0, 2, "SP"))  // This should work for BAX / LFI / LFL
    {
      if ((!dep_shortcode_.compare(3, 3, "LFL")) || (!dep_shortcode_.compare(3, 3, "LFI")) ||
          (!dep_shortcode_.compare(3, 3, "BAX"))) {
        // not checking the correctness of spread and legs
        // just figuring out the correct formula to use based on numbers
        char indep_1_num = indep_1_shortcode_[4];  // BAX_0
        char indep_2_num = indep_2_shortcode_[4];  // BAX_1
        char leg_1_num = dep_shortcode_[6];        // SP_BAX0_BAX1
        char leg_2_num = dep_shortcode_[11];

        if ((indep_1_num == leg_1_num) && (indep_2_num == leg_2_num)) {
          return (LEG1_MINUS_LEG2);
        }

        if ((indep_2_num == leg_1_num) && (indep_1_num == leg_2_num)) {
          return (LEG2_MINUS_LEG1);
        }
        return (MAX_SPREAD_MATH_TYPE);  // error
      }
      return (MAX_SPREAD_MATH_TYPE);  // error
    }
    return (MAX_SPREAD_MATH_TYPE);  // error
  }

 protected:
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& leg1_market_view_;
  const SecurityMarketView& leg2_market_view_;

  const SpreadMathType_t spread_math_type_;

  const unsigned int dep_security_id_;
  const unsigned int leg1_security_id_;
  const unsigned int leg2_security_id_;

  double prev_value_dep_;   // dependent is spread
  double prev_value_leg1_;  // indep1
  double prev_value_leg2_;  // indep2
  bool dep_ready_;
  bool leg1_ready_;
  bool leg2_ready_;

  double implied_dep_price_;

 public:
  static ImpliedPriceSpread* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                               const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
    if (r_tokens_.size() < 6u) {
      ExitVerbose(kExitErrorCodeGeneral, "ImpliedPriceSpread needs 6 tokens");
      return NULL;
    }

    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);

    SpreadMathType_t spread_math_type_ =
        HFSAT::ImpliedPriceSpread::GetIndicatorType(r_tokens_[3], r_tokens_[4], r_tokens_[5]);
    if (spread_math_type_ == MAX_SPREAD_MATH_TYPE) {
      ExitVerbose(kExitErrorCodeGeneral, "SpreadMathType invalid in ImpliedPriceSpread");
      return NULL;
    }

    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])), spread_math_type_);
  }

  static ImpliedPriceSpread* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                               SecurityMarketView& t_dep_market_view_,
                                               SecurityMarketView& t_leg1_market_view_,
                                               SecurityMarketView& t_leg2_market_view_,
                                               SpreadMathType_t spread_math_type_) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_leg1_market_view_.secname() << ' '
                << t_leg2_market_view_.secname();
    std::string concise_indicator_description_(t_temp_oss_.str());

    static std::map<std::string, ImpliedPriceSpread*> concise_indicator_description_map_;

    if (concise_indicator_description_map_.find(concise_indicator_description_) ==
        concise_indicator_description_map_.end()) {
      concise_indicator_description_map_[concise_indicator_description_] =
          new ImpliedPriceSpread(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                                 t_leg1_market_view_, t_leg2_market_view_, spread_math_type_);
    }
    return concise_indicator_description_map_[concise_indicator_description_];
  }

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_) {
    if (r_tokens_.size() >= 6u) {
      VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
      VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
      VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
    }
  }

  static std::string VarName() { return "ImpliedPriceSpread"; }

  ~ImpliedPriceSpread() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if (_market_update_info_.mkt_size_weighted_price_ <= (kInvalidPrice + 0.5)) return;

    // update prices irrespective of anything
    if (_security_id_ == dep_security_id_) {
      prev_value_dep_ = _market_update_info_.mkt_size_weighted_price_;
      dep_ready_ = true;
    } else if (_security_id_ == leg1_security_id_) {
      prev_value_leg1_ = _market_update_info_.mkt_size_weighted_price_;
      leg1_ready_ = true;
    } else if (_security_id_ == leg2_security_id_) {
      prev_value_leg2_ = _market_update_info_.mkt_size_weighted_price_;
      leg2_ready_ = true;
    }

    if (!is_ready_) {
      if (dep_ready_ && leg1_ready_ && leg2_ready_) {
        is_ready_ = true;

        if (spread_math_type_ == LEG1_MINUS_LEG2)
          implied_dep_price_ = (prev_value_leg1_ - prev_value_leg2_);
        else
          implied_dep_price_ = (prev_value_leg2_ - prev_value_leg1_);

        indicator_value_ = implied_dep_price_ - prev_value_dep_;
        NotifyIndicatorListeners(indicator_value_);
      }
    } else {
      // Indicator value = implied_dep_price - prev_value_dep_
      if (spread_math_type_ == LEG1_MINUS_LEG2)
        implied_dep_price_ = (prev_value_leg1_ - prev_value_leg2_);
      else
        implied_dep_price_ = (prev_value_leg2_ - prev_value_leg1_);

      indicator_value_ = implied_dep_price_ - prev_value_dep_;

      NotifyIndicatorListeners(indicator_value_);
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}

  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}

  void WhyNotReady() {
    if (!is_ready_) {
      if (!dep_ready_) {
        DBGLOG_TIME_CLASS << dep_market_view_.shortcode() << " " << dep_market_view_.secname() << " not ready"
                          << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      if (!leg1_ready_) {
        DBGLOG_TIME_CLASS << leg1_market_view_.shortcode() << " " << leg1_market_view_.secname() << " not ready"
                          << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      if (!leg2_ready_) {
        DBGLOG_TIME_CLASS << leg2_market_view_.shortcode() << " " << leg2_market_view_.secname() << " not ready"
                          << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }

 protected:
  ImpliedPriceSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                     const std::string& concise_indicator_description_, SecurityMarketView& t_dep_market_view_,
                     SecurityMarketView& t_leg1_market_view_, SecurityMarketView& t_leg2_market_view_,
                     SpreadMathType_t r_spread_math_type_)
      : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
        dep_market_view_(t_dep_market_view_),
        leg1_market_view_(t_leg1_market_view_),
        leg2_market_view_(t_leg2_market_view_),
        spread_math_type_(r_spread_math_type_),
        dep_security_id_(t_dep_market_view_.security_id()),
        leg1_security_id_(t_leg1_market_view_.security_id()),
        leg2_security_id_(t_leg2_market_view_.security_id()),
        prev_value_dep_(0),
        prev_value_leg1_(0),
        prev_value_leg2_(0),
        dep_ready_(false),
        leg1_ready_(false),
        leg2_ready_(false),
        implied_dep_price_(kInvalidPrice) {
    if (!t_dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
    if (!t_leg1_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
    if (!t_leg2_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
  }
};
}

#endif /* BASE_INDICATORS_IMPLIED_PRICE_SPREAD_HPP */
