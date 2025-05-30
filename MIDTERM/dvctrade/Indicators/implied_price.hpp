/**
    \file Indicators/implied_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_IMPLIED_PRICE_HPP
#define BASE_INDICATORS_IMPLIED_PRICE_HPP

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

namespace HFSAT {

class ImpliedPrice : public CommonIndicator {
 public:
  typedef enum { SRC_MINUS_DEP = 1, DEP_MINUS_SRC, MAX_SPREAD_MATH_TYPE } SpreadMathType_t;

  /// Returns whether the middle spread contract has DEP_MINUS_SRC or SRC_MINUS_DEP
  static SpreadMathType_t GetIndicatorType(const std::string& dep_shotcode_, const std::string& indep_1_shortcode_,
                                           const std::string& indep_2_shortcode_) {
    if ((!dep_shotcode_.compare(0, 3, "LFL")) || (!dep_shotcode_.compare(0, 3, "LFI")) ||
        (!dep_shotcode_.compare(0, 3, "BAX"))) {
      // not checking the correctness of spread and legs
      // just figuring out the correct formula to use based on numbers
      char dep_num = dep_shotcode_[4];           // BAX_0
      char indep_2_num = indep_2_shortcode_[4];  // BAX_1
      char leg_1_num = indep_1_shortcode_[6];    // SP_BAX0_BAX1
      char leg_2_num = indep_1_shortcode_[11];

      if ((dep_num == leg_1_num) && (indep_2_num == leg_2_num)) {
        return (DEP_MINUS_SRC);
      }

      if ((dep_num == leg_2_num) && (indep_2_num == leg_1_num)) {
        return (SRC_MINUS_DEP);
      }

      return (MAX_SPREAD_MATH_TYPE);  // error
    }
    return (MAX_SPREAD_MATH_TYPE);  // error
  }

 protected:
  const SecurityMarketView& dep_market_view_;
  const SecurityMarketView& spread_market_view_;
  const SecurityMarketView& src_market_view_;

  const SpreadMathType_t spread_math_type_;

  const unsigned int dep_security_id_;
  const unsigned int spread_security_id_;
  const unsigned int src_security_id_;

  double prev_value_dep_;
  double prev_value_spread_;  // spread
  double prev_value_src_;     // other leg
  bool dep_ready_;
  bool spread_ready_;
  bool src_ready_;

  double implied_dep_price_;

 public:
  static ImpliedPrice* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
    if (r_tokens_.size() < 6u) {
      ExitVerbose(kExitErrorCodeGeneral, "ImpliedPrice needs 6 tokens");
      return NULL;
    }

    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);

    SpreadMathType_t spread_math_type_ =
        HFSAT::ImpliedPrice::GetIndicatorType(r_tokens_[3], r_tokens_[4], r_tokens_[5]);
    if (spread_math_type_ == MAX_SPREAD_MATH_TYPE) {
      ExitVerbose(kExitErrorCodeGeneral, "SpreadMathType invalid in ImpliedPrice");
      return NULL;
    }

    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])), spread_math_type_);
  }

  static ImpliedPrice* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         SecurityMarketView& t_dep_market_view_,
                                         SecurityMarketView& t_spread_market_view_,
                                         SecurityMarketView& t_src_market_view_, SpreadMathType_t spread_math_type_) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_spread_market_view_.secname() << ' '
                << t_src_market_view_.secname();
    std::string concise_indicator_description_(t_temp_oss_.str());

    static std::map<std::string, ImpliedPrice*> concise_indicator_description_map_;

    if (concise_indicator_description_map_.find(concise_indicator_description_) ==
        concise_indicator_description_map_.end()) {
      concise_indicator_description_map_[concise_indicator_description_] =
          new ImpliedPrice(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                           t_spread_market_view_, t_src_market_view_, spread_math_type_);
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

  static std::string VarName() { return "ImpliedPrice"; }

  ~ImpliedPrice() {}

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if (_market_update_info_.mkt_size_weighted_price_ <= (kInvalidPrice + 0.5)) return;

    // update prices irrespective of anything
    if (_security_id_ == dep_security_id_) {
      prev_value_dep_ = _market_update_info_.mkt_size_weighted_price_;
      dep_ready_ = true;
    } else if (_security_id_ == spread_security_id_) {
      prev_value_spread_ = _market_update_info_.mkt_size_weighted_price_;
      spread_ready_ = true;
    } else if (_security_id_ == src_security_id_) {
      prev_value_src_ = _market_update_info_.mkt_size_weighted_price_;
      src_ready_ = true;
    }

    if (!is_ready_) {
      if (dep_ready_ && src_ready_ &&
          (!spread_ready_)) {  // PseudoReady -- outrights are ready, spread is not, send indicator value = 0
        is_ready_ = true;
        spread_ready_ = true;
        if (spread_math_type_ == DEP_MINUS_SRC) {
          prev_value_spread_ = prev_value_dep_ - prev_value_src_;
        } else {
          prev_value_spread_ = prev_value_src_ - prev_value_dep_;
        }
      }
      if (dep_ready_ && spread_ready_ && src_ready_) {
        is_ready_ = true;
        indicator_value_ = 0;
        implied_dep_price_ = prev_value_dep_;
        NotifyIndicatorListeners(indicator_value_);
      }
    } else {
      // Indicator value = implied_dep_price - prev_value_dep_
      if (spread_math_type_ == DEP_MINUS_SRC)
        implied_dep_price_ = (prev_value_spread_ + prev_value_src_);
      else
        implied_dep_price_ = (prev_value_src_ - prev_value_spread_);

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
      if (!spread_ready_) {
        DBGLOG_TIME_CLASS << spread_market_view_.shortcode() << " " << spread_market_view_.secname() << " not ready"
                          << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      if (!src_ready_) {
        DBGLOG_TIME_CLASS << src_market_view_.shortcode() << " " << src_market_view_.secname() << " not ready"
                          << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }

 protected:
  ImpliedPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
               SecurityMarketView& t_dep_market_view_, SecurityMarketView& t_spread_market_view_,
               SecurityMarketView& t_src_market_view_, SpreadMathType_t r_spread_math_type_)
      : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
        dep_market_view_(t_dep_market_view_),
        spread_market_view_(t_spread_market_view_),
        src_market_view_(t_src_market_view_),
        spread_math_type_(r_spread_math_type_),
        dep_security_id_(t_dep_market_view_.security_id()),
        spread_security_id_(t_spread_market_view_.security_id()),
        src_security_id_(t_src_market_view_.security_id()),
        prev_value_dep_(0),
        prev_value_spread_(0),
        prev_value_src_(0),
        dep_ready_(false),
        spread_ready_(false),
        src_ready_(false),
        implied_dep_price_(kInvalidPrice) {
    if (!t_dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
    if (!t_spread_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
    if (!t_src_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << kPriceTypeMktSizeWPrice << std::endl;
    }
  }
};
}

#endif /* BASE_INDICATORS_IMPLIED_PRICE_HPP */
