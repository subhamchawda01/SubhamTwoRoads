/**
    \file IndicatorsCode/pcaport_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <map>
#include <sstream>

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/index_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/pcaport_price.hpp"

namespace HFSAT {

void PCAPortPrice::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                     const std::string &t_portfolio_descriptor_shortcode_) {
  IndicatorUtil::AddPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, _shortcodes_affecting_this_indicator_);
}

PCAPortPrice *PCAPortPrice::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                              const std::string &t_portfolio_descriptor_shortcode_,
                                              const PriceType_t &_price_type_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "PCAPortPrice " << t_portfolio_descriptor_shortcode_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_portfolio_price_description(t_temp_oss.str());

  static std::map<std::string, PCAPortPrice *> concise_pca_portfolio_price_description_map;
  if (concise_pca_portfolio_price_description_map.find(concise_portfolio_price_description) ==
      concise_pca_portfolio_price_description_map.end()) {
    concise_pca_portfolio_price_description_map[concise_portfolio_price_description] =
        new PCAPortPrice(t_dbglogger_, r_watch_, t_portfolio_descriptor_shortcode_, _price_type_);
  }

  return concise_pca_portfolio_price_description_map[concise_portfolio_price_description];
}

PCAPortPrice::PCAPortPrice(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                           const std::string &t_portfolio_descriptor_shortcode_, const PriceType_t &_price_type_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
      portfolio_descriptor_shortcode_(t_portfolio_descriptor_shortcode_),
      price_type_(_price_type_),
      current_price_(1000),
      is_ready_(false),
      security_id_weight_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      current_security_id_weight_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      data_interrupted_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), false),
      security_id_is_ready_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), false),
      security_id_last_price_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      market_status_vec_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), kMktTradingStatusOpen),
      min_price_increment_(0.0),
      sum_of_eigen_compo_by_stdev_(0.0) {
  std::vector<std::string> shortcode_vec_;
  IndicatorUtil::GetPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, shortcode_vec_);
  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);

  const EigenConstituentsVec &eigen_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(t_portfolio_descriptor_shortcode_);
  const std::vector<double> &stdev_constituent_vec =
      (PcaWeightsManager::GetUniqueInstance()).GetPortfolioStdevs(t_portfolio_descriptor_shortcode_);
  is_pca_ = (PcaWeightsManager::GetUniqueInstance().IsPCA(t_portfolio_descriptor_shortcode_));
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode stdev Values for port : " << t_portfolio_descriptor_shortcode_
                                << DBGLOG_ENDL_FLUSH;
    for (unsigned i = 0; i < stdev_constituent_vec.size(); i++) {
      dbglogger_ << stdev_constituent_vec[i] << " ";
    }
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

  if (eigen_constituent_vec.empty()) {
    std::cerr << "PCAPortPrice::EigenConstituentsVec " << t_portfolio_descriptor_shortcode_
              << "does not have even single eigenvectors/values computed " << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
  }
  if (stdev_constituent_vec.size() != shortcode_vec_.size()) {
    std::cerr << "PCAPortPrice::Stdevs computed for " << stdev_constituent_vec.size() << " but portfolio size is "
              << shortcode_vec_.size() << std::endl;
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
  }

  if (!eigen_constituent_vec.empty()) {
    if (eigen_constituent_vec[0].eigenvec_components_.size() != shortcode_vec_.size()) {
      std::cerr << "PCAPortPrice:: PCA EIgenvector components size"
                << eigen_constituent_vec[0].eigenvec_components_.size() << " but portfolio size is "
                << shortcode_vec_.size() << std::endl;
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, t_portfolio_descriptor_shortcode_.c_str());
    }
  }

  // the following block of code is to compute the sum of ( eigen vector components / stdev )
  // while the value "sum_of_eigen_compo_by_stdev_" is being returned in the .hpp as
  // get_normalizing_factor () and used in pca_deviations_pairs_port .. it is really not used there.
  // it is needed to be multiplied there since we are dividing it here.
  // the math of the indicator is explained in the contructor of pca_deviations_pairs_port
  sum_of_eigen_compo_by_stdev_ = 0;

  for (unsigned int t_shortcode_vec_index_ = 0; t_shortcode_vec_index_ < shortcode_vec_.size();
       t_shortcode_vec_index_++) {
    if (stdev_constituent_vec[t_shortcode_vec_index_] > 0) {
      sum_of_eigen_compo_by_stdev_ += (eigen_constituent_vec[0].eigenvec_components_[t_shortcode_vec_index_] /
                                       stdev_constituent_vec[t_shortcode_vec_index_]);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, "stdev <= 0 in PCAPortPrice");
    }
  }

  sum_of_eigen_compo_by_stdev_ =
      std::max(0.00005, fabs(sum_of_eigen_compo_by_stdev_));  // to prevent changing of sign of indicators and to avoid
                                                              // division by very small values

  security_id_to_id_vec_.resize(sec_name_indexer_.NumSecurityId(), -1);

  for (unsigned int ii = 0; ii < shortcode_vec_.size(); ii++) {
    SecurityMarketView *p_this_indep_market_view_ = indep_market_view_vec_[ii];
    double t_thisweight_ = (eigen_constituent_vec[0].eigenvec_components_[ii] / stdev_constituent_vec[ii]);

    security_id_to_id_vec_[p_this_indep_market_view_->security_id()] = ii;

    t_thisweight_ /= sum_of_eigen_compo_by_stdev_;
    if (!p_this_indep_market_view_->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << portfolio_descriptor_shortcode_
                << " passed " << t_error_price_type_ << std::endl;
    }

    if (is_pca_) {
      security_id_weight_map_[p_this_indep_market_view_->security_id()] = t_thisweight_;
      current_security_id_weight_map_[p_this_indep_market_view_->security_id()] = t_thisweight_;
    } else {
      security_id_weight_map_[p_this_indep_market_view_->security_id()] =
          eigen_constituent_vec[0].eigenvec_components_[ii];
      current_security_id_weight_map_[p_this_indep_market_view_->security_id()] =
          eigen_constituent_vec[0].eigenvec_components_[ii];
    }
    security_id_is_ready_map_[p_this_indep_market_view_->security_id()] = false;

#if EQUITY_INDICATORS_ALWAYS_READY
    security_id_is_ready_map_[p_this_indep_market_view_->security_id()] =
        IndicatorUtil::IsEquityShortcode(p_this_indep_market_view_->shortcode());
#endif
    min_price_increment_ += t_thisweight_ * (p_this_indep_market_view_->min_price_increment());
  }

  min_price_increment_ = fabs(min_price_increment_);

#if EQUITY_INDICATORS_ALWAYS_READY
  is_ready_ = AreAllReady();
  if (is_ready_) {
    current_price_ = 0;
  }

#endif
}

void PCAPortPrice::WhyNotReady() {
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

void PCAPortPrice::OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo &_market_update_info_) {
  if (data_interrupted_map_[t_security_id_]) {
    return;
  }

  // the change in this price_type_ in this update
  // perhaps faster is such changes are computed in marketupdateinfo and indicators directly use the precomputed value
  double constituent_new_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  const double this_smv_price_change_ = constituent_new_price_ - security_id_last_price_map_[t_security_id_];

  security_id_last_price_map_[t_security_id_] = constituent_new_price_;

  current_price_ += this_smv_price_change_ * current_security_id_weight_map_[t_security_id_];

  if (!is_ready_) {
    if (security_id_to_id_vec_[t_security_id_] == -1) {
      return;
    }

    if (indep_market_view_vec_[security_id_to_id_vec_[t_security_id_]]->is_ready_complex(2)) {
      security_id_is_ready_map_[t_security_id_] = true;
      is_ready_ = AreAllReady();
    }

    if (is_ready_) {
      current_price_ = 0;
      for (auto i = 0u; i < security_id_last_price_map_.size(); i++) {
        current_price_ += security_id_last_price_map_[i] * current_security_id_weight_map_[i];
      }
    } else {
      HistoricPriceManager *p_historic_price_manager_ = HistoricPriceManager::GetUniqueInstance();

      // All constituents not ready yet, use historical prices.
      for (auto i = 0u; i < security_id_last_price_map_.size(); i++) {
        if (security_id_is_ready_map_[i]) {
          continue;
        }

        security_id_last_price_map_[i] = p_historic_price_manager_->GetPriceFromType(price_type_, i);

        if (security_id_last_price_map_[i] > 0.1) {
          security_id_is_ready_map_[i] = true;
        }
      }

      // Try to get ready again.
      is_ready_ = AreAllReady();
      if (is_ready_) {
        current_price_ = 0;
        for (auto i = 0u; i < security_id_last_price_map_.size(); i++) {
          current_price_ += security_id_last_price_map_[i] * current_security_id_weight_map_[i];
        }

        DBGLOG_TIME_CLASS_FUNC_LINE << "PCAPORT got ready using historical data" << DBGLOG_ENDL_FLUSH;
      } else {
        // DBGLOG_TIME_CLASS_FUNC_LINE << "PCAPORT not ready using historical data" << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if (is_ready_) {
    for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
      price_change_listener_vec_[i]->OnPortfolioPriceChange(current_price_);
    }

    if (dbglogger_.CheckLoggingLevel(INDICATOR_PRICE_TEST)) {
      dbglogger_ << " On Portfolio Price Update @ " << watch_.tv() << " For : " << portfolio_descriptor_shortcode_
                 << " Current Price : " << current_price_ << "\n";
      dbglogger_.CheckToFlushBuffer();
    }
  }
}

void PCAPortPrice::AddPriceChangeListener(PortfolioPriceChangeListener *_portfolio_price_change_listener_) {
  VectorUtils::UniqueVectorAdd(price_change_listener_vec_, _portfolio_price_change_listener_);
}

bool PCAPortPrice::AreAllReady() {
  for (auto i = 0u; i < security_id_is_ready_map_.size(); i++) {
    if ((security_id_weight_map_[i] != 0) && (security_id_is_ready_map_[i] != true)) {
      return false;
    }
  }
  return true;
}

void PCAPortPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  bool in_port = false;
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (indep_market_view_vec_[i]->security_id() == _security_id_) {
      in_port = true;
    }
  }
  if (!in_port) {
    return;
  }

  data_interrupted_map_[_security_id_] = true;

  for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
    price_change_listener_vec_[i]->OnPortfolioPriceReset(current_price_, current_price_, 1u);
  }
}

void PCAPortPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  bool in_port = false;
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (indep_market_view_vec_[i]->security_id() == _security_id_) {
      in_port = true;
    }
  }
  if (!in_port) {
    return;
  }

  data_interrupted_map_[_security_id_] = false;

  for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
    price_change_listener_vec_[i]->OnPortfolioPriceReset(current_price_, current_price_, 2u);
  }
}

void PCAPortPrice::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  bool in_port = false;
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (indep_market_view_vec_[i]->security_id() == _security_id_) {
      in_port = true;
    }
  }
  if (!in_port) {
    return;
  }

  if (market_status_vec_[_security_id_] != _new_market_status_) {
    if (_new_market_status_ == kMktTradingStatusReserved || _new_market_status_ == kMktTradingStatusClosed) {
      double old_port_price_ = current_price_;

      const double this_smv_price_change_ = -security_id_last_price_map_[_security_id_];
      current_price_ += this_smv_price_change_ * current_security_id_weight_map_[_security_id_];
      current_security_id_weight_map_[_security_id_] = 0;
      if (is_ready_) {
        for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
          price_change_listener_vec_[i]->OnPortfolioPriceReset(current_price_, old_port_price_, 0u);
        }
      }
    } else {
      double old_port_price_ = current_price_;
      current_security_id_weight_map_[_security_id_] = security_id_weight_map_[_security_id_];
      const double this_smv_price_change_ = security_id_last_price_map_[_security_id_];
      current_price_ += this_smv_price_change_ * current_security_id_weight_map_[_security_id_];
      if (is_ready_) {
        for (auto i = 0u; i < price_change_listener_vec_.size(); i++) {
          price_change_listener_vec_[i]->OnPortfolioPriceReset(current_price_, old_port_price_, 0u);
        }
      }
    }
    market_status_vec_[_security_id_] = _new_market_status_;
  }
}
}
