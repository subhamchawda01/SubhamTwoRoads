/**
   \file IndicatorsCode/implied_price_mps.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <boost/ref.hpp>

#include "dvctrade/Indicators/implied_price_mps.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

/* Calculate implied prices of whole family based on the
   current prices of futures, spreads and butterflies

   Ax=b
   B_ij=A_ij / w_i
   By=b
   y_i=x_i * w_i
*/
namespace HFSAT {
ImpliedPriceMPS* ImpliedPriceMPS::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {  // r_tokens_[3] = dep
  // r_tokens_[4] = IN_PORTFOLIO // not a real portfolio, just a family/group of products
  // r_tokens_[5] = px_type
  std::vector<std::string> product_family_;
  product_family_.clear();
  ImpliedPriceMPS::CollectShortCodes(product_family_, product_family_, r_tokens_);
  std::vector<boost::reference_wrapper<SecurityMarketView> > market_view_vect_;
  market_view_vect_.clear();
  for (auto i = 0u; i < product_family_.size(); ++i) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(product_family_[i]);
    market_view_vect_.push_back(
        boost::ref(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(product_family_[i]))));
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_, market_view_vect_, r_tokens_[3], r_tokens_[4],
                           StringToPriceType_t(r_tokens_[5]));
}

ImpliedPriceMPS* ImpliedPriceMPS::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_,
    std::vector<boost::reference_wrapper<SecurityMarketView> > market_view_vect_, std::string t_dep_shortcode_,
    std::string t_dep_family_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_shortcode_ << ' ' << t_dep_family_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ImpliedPriceMPS*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ImpliedPriceMPS(t_dbglogger_, r_watch_, concise_indicator_description_, market_view_vect_, t_dep_shortcode_,
                            t_dep_family_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

void ImpliedPriceMPS::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  ProductFamilyManager::GetProductFamilyShortcodes(r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

void ImpliedPriceMPS::WhyNotReady() {
  if (!is_ready_) {
    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (!is_ready_vec_[i]) {
        DBGLOG_TIME_CLASS << unwrap_ref(dep_market_view_vec_[i]).secname() << " not ready\n " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }
}

void ImpliedPriceMPS::PrintDebugInfo() const {
  std::cerr << "ImpliedPriceMPS::PrintDebugInfo ( ) :: " << mp_price_vec_[dep_index] << " "
            << prev_value_vec_[dep_index] << "\n";
  for (auto i = 0u; i < mp_price_vec_.size(); ++i) std::cerr << mp_price_vec_[i] << " ";
  std::cerr << "\n";
  for (auto i = 0u; i < prev_value_vec_.size(); ++i) std::cerr << prev_value_vec_[i] << " ";
  std::cerr << "\n";
}

// do matrix multiplication on getting ready... then always use column multiplication with changed price.
void ImpliedPriceMPS::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_market_update_info_.mkt_size_weighted_price_ <= (kInvalidPrice + 0.5)) return;
  // update prices irrespective of anything

  if (!is_ready_) {
    for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
      if (boost::unwrap_ref(dep_market_view_vec_[i]).security_id() == _security_id_) {
        prev_value_vec_[i] = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
        is_ready_vec_[i] = true;
        break;
      }
    }

    if (AreAllReady()) {
      is_ready_ = true;
      MatrixMultVec();
      indicator_value_ = mp_price_vec_[dep_index] - prev_value_vec_[dep_index];
      NotifyIndicatorListeners(indicator_value_);
    }

    if (PseudoReady()) {  // outrights ready, force strategies to get ready using implied prices
      is_ready_ = true;
      MatrixMultVec();
      indicator_value_ = mp_price_vec_[dep_index] - prev_value_vec_[dep_index];
      // PrintDebugInfo ( );
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    double price_change = 0.0;
    unsigned int index_change = 0;
    for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
      if (boost::unwrap_ref(dep_market_view_vec_[i]).security_id() == _security_id_) {
        price_change = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_) - prev_value_vec_[i];
        index_change = i;
        prev_value_vec_[i] = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
        break;
      }
    }
    // Optimization: instead of multiplying matrix with vector again,
    // multiply the corresponding column with the change in price of a particular product
    ColumnMultVal(index_change, price_change);
    indicator_value_ = mp_price_vec_[dep_index] - prev_value_vec_[dep_index];
    // PrintDebugInfo ( );
    NotifyIndicatorListeners(indicator_value_);
  }
}

inline void ImpliedPriceMPS::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                          const MarketUpdateInfo& _market_update_info_) {
  for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
    if (boost::unwrap_ref(dep_market_view_vec_[i]).security_id() == _security_id_) {
      trade_volumes_vec_[i] = _trade_print_info_.num_trades_;
      break;
    }
  }

#define MIN_RATIO 0.02
#define MAX_RATIO 100.0
#define THIRTYMINUTESMSECS 1800000

  if ((last_weights_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_weights_updated_msecs_ > THIRTYMINUTESMSECS)) {
    double dep_rvm_ = (double)(trade_volumes_vec_[dep_index]);
    for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
      double indep_rvm_ = (double)(trade_volumes_vec_[i]);
      if ((dep_rvm_ > 0) && (indep_rvm_ > 0)) {
        // mp_weights_vec_[i] = sqrt ( std::max ( MIN_RATIO, std::min ( MAX_RATIO, ( indep_rvm_ / dep_rvm_ ) ) ) ) ;
        mp_weights_vec_[i] = (std::max(MIN_RATIO, std::min(MAX_RATIO, (dep_rvm_ / indep_rvm_))));

        DBGLOG_TIME << concise_indicator_description_ << " Current MPS weight: " << mp_weights_vec_[i]
                    << " Dep: " << dep_rvm_ << " Indep: " << indep_rvm_ << DBGLOG_ENDL_FLUSH;
      }
    }
    // need to re-calculate A and pseudo-inv (A) here. Also calculate x once again
    CalculatePInv();
    MatrixMultVec();

#undef MAX_RATIO
#undef MIN_RATIO

    last_weights_updated_msecs_ = watch_.msecs_from_midnight();
  }
  OnMarketUpdate(_security_id_, _market_update_info_);
}

ImpliedPriceMPS::ImpliedPriceMPS(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 std::vector<boost::reference_wrapper<SecurityMarketView> > market_view_vect_,
                                 std::string& t_dep_shortcode_, std::string& t_dep_family_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_vec_(market_view_vect_),
      prev_value_vec_(market_view_vect_.size(), 0.0),
      mp_price_vec_(market_view_vect_.size(), 0.0),
      mp_weights_vec_(market_view_vect_.size(), 0.0),
      trade_volumes_vec_(market_view_vect_.size(), 0),
      is_ready_vec_(market_view_vect_.size(), false),
      dep_shortcode_(t_dep_shortcode_),
      dep_family_(t_dep_family_),
      price_type_(_price_type_),
      pib(),
      dep_index(0),
      last_weights_updated_msecs_(0) {
  for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
    if (!boost::unwrap_ref(dep_market_view_vec_[i]).subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
    if (!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(dep_shortcode_)) {
      dep_index = i;
    }
  }
  GetProductFamilyWeights();
  CalculatePInv();
  prev_value_vec_.clear();
  prev_value_vec_.resize(pib.getColumnDimension(), 0.0);  // required since size of b is dependent on no of spreads
}

inline bool ImpliedPriceMPS::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

inline bool ImpliedPriceMPS::PseudoReady() {  // check if outrights ready, then force strategies (spreads/butterflies)
                                              // to get ready using implied prices
  bool pready = true;
  for (auto i = 0u; i < future_num.size(); ++i) pready = pready && is_ready_vec_[future_num[i].index];
  if (!pready) return (pready);

  unsigned int index_leg1, index_leg2;
  for (auto i = 0u; i < spread_num.size(); ++i) {
    if (!is_ready_vec_[spread_num[i].index]) {
      index_leg1 = 0;
      index_leg2 = 0;
      for (unsigned int j = 0; j < future_num.size(); ++j) {
        if (future_num[j].leg1 == spread_num[i].leg1) {
          index_leg1 = j;
        }
        if (future_num[j].leg1 == spread_num[i].leg2) {
          index_leg2 = j;
        }
      }

      prev_value_vec_[spread_num[i].index] = prev_value_vec_[index_leg1] - prev_value_vec_[index_leg2];
      is_ready_vec_[spread_num[i].index] = true;  // not really required, but still
    }
  }

  return (pready);
}

// calculates A, and then calculates pseudo-inverse of A
inline void ImpliedPriceMPS::CalculatePInv() {
  // calculate A of Ax = b
  // x = mp_price_vec_ -- add extra entries to this this vector also
  // b = prev_value_vec_

  future_num.clear();
  spread_num.clear();
  for (auto i = 0u; i < dep_market_view_vec_.size(); ++i) {
    if ((!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 3, "LFL")) ||
        (!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 3, "LFI")) ||
        (!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 3, "BAX"))) {
      // future
      future temp_future;
      temp_future.leg1 = unwrap_ref(dep_market_view_vec_[i]).shortcode()[4];
      temp_future.index = i;
      future_num.push_back(temp_future);
    } else if ((!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 6, "SP_LFL")) ||
               (!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 6, "SP_LFI")) ||
               (!unwrap_ref(dep_market_view_vec_[i]).shortcode().compare(0, 6, "SP_BAX"))) {
      // spread
      spread temp_spread;
      temp_spread.leg1 = unwrap_ref(dep_market_view_vec_[i]).shortcode()[6];
      temp_spread.leg2 = unwrap_ref(dep_market_view_vec_[i]).shortcode()[11];
      temp_spread.index = i;
      spread_num.push_back(temp_spread);
    } else {
      // check for butterflies and other strategies here ... also update num_rows, etc variables
    }
  }
  unsigned int num_rows = future_num.size() + 2 * spread_num.size();
  unsigned int num_cols = future_num.size() + spread_num.size();
  LINAL::Matrix A(num_rows, num_cols);  // #rows = #equations   // #cols = #products
  for (auto i = 0u; i < num_rows; ++i)
    for (unsigned int j = 0; j < num_cols; ++j) A.set(i, j, 0.0);  // initialize A with all zeroes
  for (unsigned int j = 0; j < num_cols; ++j) A.set(j, j, 1.0);    // trivial equations for knowns
  for (unsigned int i = num_cols; i < num_rows; ++i) {             // equations for spreads as difference of futures
    // prev_value_vec_.push_back ( 0.0 ) ;
    A.set(i, spread_num[i - num_cols].index, 1.0);
    for (unsigned int j = 0; j < future_num.size(); ++j) {
      if (future_num[j].leg1 == spread_num[i - num_cols].leg1) {
        A.set(i, future_num[j].index, -1.0);
      }
      if (future_num[j].leg1 == spread_num[i - num_cols].leg2) {
        A.set(i, future_num[j].index, 1.0);
      }
    }
  }
  // Change A to B inplace
  for (auto i = 0u; i < num_rows; ++i)
    for (unsigned int j = 0; j < num_cols; ++j) A.set(i, j, A.get(i, j) / mp_weights_vec_[j]);

  // calculate PInv (A)
  // pib.create ( num_cols, num_rows ) ;
  //    A.printToConsole( ) ;
  pib.copy(LINAL::getPINV(A));
  //	LINAL::getPINV(A).printToConsole( ) ;
  //	std::cout<< "\n";
  // pib.printToConsole( ) ;
}

inline void ImpliedPriceMPS::MatrixMultVec() {
  // Function to multiply pseudo-inverse of A with b to get x
  for (auto i = 0u; i < pib.getRowDimension(); ++i) {
    mp_price_vec_[i] = 0;
    for (unsigned int j = 0; j < pib.getColumnDimension(); ++j) {
      mp_price_vec_[i] += pib.get(i, j) * prev_value_vec_[j];
    }
    mp_price_vec_[i] /= mp_weights_vec_[i];  // inplace conversion from y to x
  }
}

inline void ImpliedPriceMPS::ColumnMultVal(unsigned int index_change, double price_change) {
  for (auto i = 0u; i < pib.getRowDimension(); ++i) {
    mp_price_vec_[i] += (pib.get(i, index_change) * price_change) / mp_weights_vec_[i];
    //          mp_price_vec_[i] /= mp_weights_vec_[i] ; //inplace conversion from y to x
  }
}

inline void ImpliedPriceMPS::GetProductFamilyWeights() {
  ProductFamilyManager::GetProductFamilyWeights(dep_family_, mp_weights_vec_);
}
}
