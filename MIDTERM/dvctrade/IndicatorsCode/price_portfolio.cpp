/**
    \file IndicatorsCode/pcaport_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <sstream>
#include <map>
#include <fstream>
#include <vector>
#include <string>
#include <iterator>
#include <cstdlib>
#include <map>
#include <utility>
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/index_utils.hpp"

#include "dvctrade/Indicators/price_portfolio.hpp"
#define PAIRSTRADEFILE "/spare/local/tradeinfo/PairTradeInfo/portfolio_inputs.txt"

namespace HFSAT {

void PricePortfolio::CollectShortCodes(std::vector<std::string> &r_shortcodes_affecting_this_indicator_,
                                       std::vector<std::string> &r_ors_source_needed_vec_,
                                       const std::vector<const char *> &r_tokens_) {
  std::string t_portfolio_descriptor_shortcode_ = r_tokens_[3];
  std::string line;
  std::ifstream Portfoliofile(PAIRSTRADEFILE);
  bool flag_port_found_ = false;
  if (Portfoliofile.is_open()) {
    while (getline(Portfoliofile, line)) {
      std::istringstream buf(line);
      std::istream_iterator<std::string> beg(buf), end;

      std::vector<std::string> tokens(beg, end);
      if (tokens.size() != 0) {
        if (tokens[0] == t_portfolio_descriptor_shortcode_) {
          flag_port_found_ = true;
          for (unsigned int index = 1; index <= tokens.size() / 2; index++) {
            r_shortcodes_affecting_this_indicator_.push_back(tokens[index]);
            // std :: cout<<tokens[index]<<std ::endl;
          }
          break;
        }
      }
    }
  } else {
    std::cerr << "PricePortfolio::Portfolio file not accessible" << t_portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
  }
  if (flag_port_found_ == false) {
    std::cerr << "PricePortfolio::PortfolioMissing" << t_portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
  }
}

PricePortfolio *PricePortfolio::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                  const std::vector<const char *> &r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // Will this be a problem?
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  //  std :: cout<<"Inside generic GetUniqueInstance"<<std ::endl;
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}
PricePortfolio *PricePortfolio::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                  const std::string &t_portfolio_descriptor_shortcode_,
                                                  double _fractional_seconds_, const PriceType_t &_price_type_) {
  std::ostringstream t_temp_oss;
  // std :: cout<<"Inside specific GetUniqueInstance"<<std ::endl;
  t_temp_oss << "PricePortfolio " << t_portfolio_descriptor_shortcode_ << ' ' << _fractional_seconds_ << ' '
             << PriceType_t_To_String(_price_type_);
  std::string concise_portfolio_price_description(t_temp_oss.str());

  static std::map<std::string, PricePortfolio *> concise_price_portfolio_description_map;
  if (concise_price_portfolio_description_map.find(concise_portfolio_price_description) ==
      concise_price_portfolio_description_map.end()) {
    concise_price_portfolio_description_map[concise_portfolio_price_description] =
        new PricePortfolio(t_dbglogger_, r_watch_, concise_portfolio_price_description,
                           t_portfolio_descriptor_shortcode_, _fractional_seconds_, _price_type_);
  }

  return concise_price_portfolio_description_map[concise_portfolio_price_description];
}

PricePortfolio::PricePortfolio(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                               const std::string &concise_indicator_description_,
                               const std::string &t_portfolio_descriptor_shortcode_, double _fractional_seconds_,
                               const PriceType_t &_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      portfolio_descriptor_shortcode_(t_portfolio_descriptor_shortcode_),
      price_type_(_price_type_),
      current_price_(1000),
      is_ready_(false),
      fractional_seconds_(_fractional_seconds_),
      security_id_weight_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      current_security_id_weight_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      security_id_is_ready_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), false),
      security_id_last_projected_price_map_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0),
      market_status_vec_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), kMktTradingStatusOpen),
      min_price_increment_(0.0),
      Portfoliofile(PAIRSTRADEFILE),
      avg_longterm_stdev_vec_(SecurityNameIndexer::GetUniqueInstance().NumSecurityId(), 0) {
  // std :: cout<<"Inside PricePortfolio Constructor"<<std ::endl;
  // std::cout<<"Outside File"<<std :: endl;
  bool flag_port_found_ = false;
  if (Portfoliofile.is_open()) {
    // std::cout<<"Inside file"<<std::endl;
    while (getline(Portfoliofile, line)) {
      std::istringstream buf(line);
      std::istream_iterator<std::string> beg(buf), end;

      std::vector<std::string> tokens(beg, end);
      if (tokens.size() != 0) {
        if (tokens[0] == t_portfolio_descriptor_shortcode_) {
          flag_port_found_ = true;

	  // Fairly complicated piece of code. Some example of what sort of string is expected
	  // will help.
          for (unsigned int index = 1; index <= tokens.size() / 2; index++) {
            _shortcode_vec_.push_back(tokens[index]);
            double weight_security_ = (double)atof(tokens[index + (tokens.size() / 2)].c_str());
            _port_weight_vec_.push_back(weight_security_);
          }
          break;
        }
      }
    }
  } else {
    std::cerr << "PricePortfolio::Portfoliofile not accesible" << portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, portfolio_descriptor_shortcode_.c_str());
  }
  if (flag_port_found_ == false) {
    std::cerr << "PricePortfolio::PortfolioMissing" << portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, portfolio_descriptor_shortcode_.c_str());
  }

  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(_shortcode_vec_, indep_market_view_vec_);

  for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
    if (indep_market_view_vec_[i] != NULL) {
      //  std :: cout<<"SMV of "<< _shortcode_vec_[i] << "is not null "<< std :: endl;
      indep_market_view_vec_[i]->subscribe_MktStatus(this);
    }
    _sec_id_to_index_[indep_market_view_vec_[i]->security_id()] = i;
  }
  // the following block of code is to compute the sum of ( eigen vector components / stdev )
  // while the value "sum_of_eigen_compo_by_stdev_" is being returned in the .hpp as
  // get_normalizing_factor () and used in pca_deviations_pairs_port .. it is really not used there.
  // it is needed to be multiplied there since we are dividing it here.
  // the math of the indicator is explained in the contructor of pca_deviations_pairs_port

  for (unsigned int ii = 0; ii < _shortcode_vec_.size(); ii++) {
    SecurityMarketView *p_this_indep_market_view_ = indep_market_view_vec_[ii];
    double t_thisweight_ = _port_weight_vec_[ii];
    security_id_weight_map_[p_this_indep_market_view_->security_id()] = t_thisweight_;
    current_security_id_weight_map_[p_this_indep_market_view_->security_id()] = t_thisweight_;
    security_id_last_projected_price_map_[p_this_indep_market_view_->security_id()] = 0;

    if (!p_this_indep_market_view_->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << portfolio_descriptor_shortcode_
                << " passed " << t_error_price_type_ << std::endl;
    }

    security_id_is_ready_map_[p_this_indep_market_view_->security_id()] = false;

#if EQUITY_INDICATORS_ALWAYS_READY
    security_id_is_ready_map_[p_this_indep_market_view_->security_id()] =
        IndicatorUtil::IsEquityShortcode(p_this_indep_market_view_->shortcode());
#endif
    min_price_increment_ += t_thisweight_ * (p_this_indep_market_view_->min_price_increment());
    avg_longterm_stdev_vec_[ii] =
        HFSAT::SampleDataUtil::GetAvgForPeriod(p_this_indep_market_view_->shortcode(), r_watch_.YYYYMMDD(), 60,
                                               trading_start_mfm_, trading_end_mfm_, std::string("STDEV"));
  }

  min_price_increment_ = fabs(min_price_increment_);

/*for (unsigned int ii = 1; ii < _shortcode_vec_.size(); ii++) {
  proj_ind_ = HFSAT::ProjectedPricePairs::GetUniqueInstance(t_dbglogger_, r_watch_, *indep_market_view_vec_[0],
                                                            _shortcode_vec_[ii], _fractional_seconds_, _price_type_);

  projection_indicator_vec_.push_back(proj_ind_);

  projection_indicator_vec_[ii - 1]->add_unweighted_indicator_listener(ii, this);
}*/

#if EQUITY_INDICATORS_ALWAYS_READY
  is_ready_ = AreAllReady();
  if (is_ready_) {
    current_price_ = 0;
  }
#endif
}

void PricePortfolio::WhyNotReady() {
  // std :: cout<<"Inside WhynotReady"<<std ::endl;
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

void PricePortfolio::InitializeValues() {
  // std :: cout<<"Inside Initialize values"<<std ::endl;
  indicator_value_ = 0;
}
void PricePortfolio::OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_) {
  // 1 we want to use model file with which has less/no indep's indicators
  // 2 we want to use model file with decent/high indep's indicators
  // std :: cout<<"Inside OnIndicator Update"<<std ::endl;

  /*unsigned int t_security_id_index_ = indep_market_view_vec_[_indicator_index_]->security_id();
  unsigned int t_security_id_0_ = indep_market_view_vec_[0]->security_id();
  security_id_last_projected_price_map_[t_security_id_index_] =
      security_id_last_projected_price_map_[t_security_id_0_] + _new_value_;
  if (!is_ready_) {
    security_id_is_ready_map_[t_security_id_index_] = indep_market_view_vec_[_indicator_index_]->is_ready();
    is_ready_ = AreAllReady();
    if (!is_ready_) {
      return;
    }
  }

  indicator_value_ = 0;
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    unsigned int current_security_id_ = indep_market_view_vec_[i]->security_id();
    indicator_value_ +=
        security_id_last_projected_price_map_[current_security_id_] * security_id_weight_map_[current_security_id_];
  }
  // if(abs(security_id_last_projected_price_map_[t_security_id_0_]) < 0.1  )
  // return;

  NotifyIndicatorListeners(indicator_value_);*/
  return;
}

void PricePortfolio::OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo &_market_update_info_) {
  double new_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  double change_in_dep_price_ =
      new_dep_price_ - security_id_last_projected_price_map_[_sec_id_to_index_[t_security_id_]];
  security_id_last_projected_price_map_[_sec_id_to_index_[t_security_id_]] = new_dep_price_;
  if (!IsAnyoneReady()) {
    InitializeValues();
  }
  indicator_value_ += change_in_dep_price_ * security_id_weight_map_[t_security_id_] * avg_longterm_stdev_vec_[0] /
                      avg_longterm_stdev_vec_[_sec_id_to_index_[t_security_id_]];
  if (!is_ready_) {
    security_id_is_ready_map_[t_security_id_] = indep_market_view_vec_[_sec_id_to_index_[t_security_id_]]->is_ready();
    is_ready_ = AreAllReady();
  }
  if (!is_ready_) return;

  NotifyIndicatorListeners(indicator_value_);
}

void PricePortfolio::AddPriceChangeListener(PortfolioPriceChangeListener *_portfolio_price_change_listener_) {
  VectorUtils::UniqueVectorAdd(price_change_listener_vec_, _portfolio_price_change_listener_);
}

bool PricePortfolio::AreAllReady() {
  for (auto i = 0u; i < security_id_is_ready_map_.size(); i++) {
    if ((security_id_weight_map_[i] != 0) && (security_id_is_ready_map_[i] != true)) {
      return false;
    }
  }
  return true;
}

bool PricePortfolio::IsAnyoneReady() {
  for (auto i = 0u; i < security_id_is_ready_map_.size(); i++) {
    if ((security_id_weight_map_[i] != 0) && (security_id_is_ready_map_[i] == true)) {
      return true;
    }
  }
  return false;
}

void PricePortfolio::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  return;
}

void PricePortfolio::OnMarketDataResumed(const unsigned int _security_id_) { return; }

void PricePortfolio::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  return;
}
}
