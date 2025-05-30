/**
    \file IndicatorsCode/portfolio_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <sstream>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/portfolio_price.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/portfolio_constituent_manager.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

namespace HFSAT {

void PortfolioPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       const std::string& t_portfolio_descriptor_shortcode_) {
  IndicatorUtil::AddPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, _shortcodes_affecting_this_indicator_);
}

PortfolioPrice* PortfolioPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::string& t_portfolio_descriptor_shortcode_,
                                                  const PriceType_t& _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "PortfolioPrice " << t_portfolio_descriptor_shortcode_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_portfolio_price_description_(t_temp_oss_.str());

  static std::map<std::string, PortfolioPrice*> concise_portfolio_price_description_map_;
  if (concise_portfolio_price_description_map_.find(concise_portfolio_price_description_) ==
      concise_portfolio_price_description_map_.end()) {
    concise_portfolio_price_description_map_[concise_portfolio_price_description_] =
        new PortfolioPrice(t_dbglogger_, r_watch_, t_portfolio_descriptor_shortcode_, _price_type_);
  }
  return concise_portfolio_price_description_map_[concise_portfolio_price_description_];
}

PortfolioPrice::PortfolioPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& t_portfolio_descriptor_shortcode_, const PriceType_t& _price_type_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      portfolio_descriptor_shortcode_(t_portfolio_descriptor_shortcode_),
      price_type_(_price_type_),
      current_price_(1000),  // using a large enough value just to have non negative values till it is ready
      is_ready_(false),
      security_id_weight_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      security_id_is_ready_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), false),
      security_id_last_price_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      min_price_increment_(0) {
  // The weights here are typically expected to be inversely proportional to the stdeviation of the product.
  // Consider the following ways of coming up with weights
  // (i) Some index described in terms of number of contracts of the futures that makes up the index.
  //     Suppose this index like GSCI or sth is really important, then these number of contracts,
  //     scaled to make the standard deviation of the resulting price series equal to
  //     either the average of the pool or the dependant, in case the t_portfolio_descriptor_shortcode_ is
  //     like "USBFUT2ZF"
  // (ii) In the absence of an external index, perhaps the weights could be inversely proportional to
  //      the standard deviation of the products in the portfolio
  // (iii) An option is to compute the PCA of the normalized price change series of the products in the
  //       portfolio. Bring back the resulting eigenvector to price space by dividing by standard deviations.
  //       Scale it back to a vector ( ALPHA ) that makes standard deviation of the price change series
  //       [ = ALPHA * orig price change matrix ] equal to either the average of the pool or the dependant,
  //       in case the t_portfolio_descriptor_shortcode_ is like "USBFUT2ZF".

  const PortfolioConstituentVec& portfolio_constituent_vec_ =
      (PortfolioConstituentManager::GetUniqueInstance()).GetPortfolioConstituentVec(t_portfolio_descriptor_shortcode_);
  std::vector<std::string> shortcode_vec_;
  PortfolioConstituentUtil::GetShortcodes(portfolio_constituent_vec_, shortcode_vec_);

  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);

  // std::cout << "PortfolioPrice:";

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    SecurityMarketView* p_this_indep_market_view_ = indep_market_view_vec_[i];
    double t_thisweight_ = portfolio_constituent_vec_[i].weight_;
    if (t_thisweight_ != 0) {
      if (!p_this_indep_market_view_->subscribe_price_type(this, price_type_))  // subscribe to this SMV
      {
        PriceType_t t_error_price_type_ = price_type_;
        std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' '
                  << t_portfolio_descriptor_shortcode_ << " passed " << t_error_price_type_ << std::endl;
      }
      security_id_weight_map_[p_this_indep_market_view_->security_id()] =
          t_thisweight_;  // store this weight to be able to calculate the price difference in updates to the security's
                          // price
      security_id_is_ready_map_[p_this_indep_market_view_->security_id()] = false;
#if EQUITY_INDICATORS_ALWAYS_READY
      security_id_is_ready_map_[p_this_indep_market_view_->security_id()] =
          IndicatorUtil::IsEquityShortcode(p_this_indep_market_view_->shortcode());
#endif
      min_price_increment_ += t_thisweight_ * (p_this_indep_market_view_->min_price_increment());

      // std::cout << " [ " << p_this_indep_market_view_->shortcode () << " : " << security_id_weight_map_[
      // p_this_indep_market_view_->security_id ( )] << " ]";
    }
  }

#if EQUITY_INDICATORS_ALWAYS_READY
  is_ready_ = AreAllReady();
  if (is_ready_) {
    current_price_ = 0;
  }
#endif
  // std::cout << std::endl;
}

void PortfolioPrice::WhyNotReady() {
  if (!is_ready_) {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i] != NULL) {
        const unsigned int t_security_id_ = indep_market_view_vec_[i]->security_id();
        if ((security_id_weight_map_[t_security_id_] != 0) && (security_id_is_ready_map_[t_security_id_] != true)) {
          DBGLOG_TIME_CLASS << "For nonzero weight security " << indep_market_view_vec_[i]->secname()
                            << " is_ready_complex = " << indep_market_view_vec_[i]->is_ready_complex(2)
                            << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        }
      }
    }
  }
}

void PortfolioPrice::OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& _market_update_info_) {
  // the change in this price_type_ in this update
  // perhaps faster is such changes are computed in marketupdateinfo and indicators directly use the precomputed value
  double this_smv_price_change_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_) -
                                  security_id_last_price_map_[t_security_id_];
  security_id_last_price_map_[t_security_id_] = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  current_price_ += this_smv_price_change_ * security_id_weight_map_[t_security_id_];

  if (!is_ready_) {
    security_id_is_ready_map_[t_security_id_] = true;
    is_ready_ = AreAllReady();
    if (is_ready_) {
      current_price_ = 0;
      for (auto i = 0u; i < security_id_last_price_map_.size(); i++) {
        current_price_ += security_id_last_price_map_[t_security_id_] * security_id_weight_map_[t_security_id_];
      }
    }
  } else {
    for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
      price_change_listener_vec_[i]->OnPortfolioPriceChange(current_price_);
    }
  }

  // DBGLOG_TIME_CLASS << current_price_ << DBGLOG_ENDL_FLUSH ;
  // DBGLOG_DUMP;
}

void PortfolioPrice::AddPriceChangeListener(PortfolioPriceChangeListener* _portfolio_price_change_listener_) {
  VectorUtils::UniqueVectorAdd(price_change_listener_vec_, _portfolio_price_change_listener_);
}

// portfolio price -> sum(weights*last available price)
void PortfolioPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  //    if ( indep_market_view_.security_id() == _security_id_ )
  // {
  //	data_interrupted_ = true ;
  //	indicator_value_ = 0 ;
  //	NotifyIndicatorListeners ( indicator_value_ ) ;
  // }
}

void PortfolioPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  //    if ( indep_market_view_.security_id() == _security_id_ )
  //{
  //  data_interrupted_ = false ;
  //}
}

bool PortfolioPrice::AreAllReady() {
  for (auto i = 0u; i < security_id_is_ready_map_.size(); i++) {
    if ((security_id_weight_map_[i] != 0) && (security_id_is_ready_map_[i] != true)) {
      return false;
    }
  }
  return true;
}
}
