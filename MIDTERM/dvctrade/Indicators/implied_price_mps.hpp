/**
    \file Indicators/implied_price_mps.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include <boost/ref.hpp>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/product_family_manager.hpp"
#include "dvctrade/linal/linal_util.hpp"

/* Calculate implied prices of whole family based on the
current prices of futures, spreads and butterflies

Ax=b
B_ij=A_ij / w_i
By=b
y_i=x_i * w_i
 */
namespace HFSAT {
struct future {
  char leg1;
  unsigned int index;
};

struct spread {
  char leg1;
  char leg2;
  unsigned int index;
};

class ImpliedPriceMPS : public CommonIndicator {
 protected:
  std::vector<boost::reference_wrapper<SecurityMarketView> > dep_market_view_vec_;
  std::vector<double> prev_value_vec_;           // b
  std::vector<double> mp_price_vec_;             // x
  std::vector<double> mp_weights_vec_;           // w
  std::vector<unsigned int> trade_volumes_vec_;  // useful for calculating w
  std::vector<bool> is_ready_vec_;
  std::string dep_shortcode_;
  std::string dep_family_;
  PriceType_t price_type_;
  LINAL::Matrix pib;  // Pseudo-inverse of B
  unsigned int dep_index;
  std::vector<future> future_num;
  std::vector<spread> spread_num;
  int last_weights_updated_msecs_;

 public:
  static ImpliedPriceMPS* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_);

  static ImpliedPriceMPS* GetUniqueInstance(
      DebugLogger& t_dbglogger_, const Watch& r_watch_,
      std::vector<boost::reference_wrapper<SecurityMarketView> > market_view_vect_, std::string t_dep_shortcode_,
      std::string t_dep_family_, PriceType_t _price_type_);

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static std::string VarName() { return "ImpliedPriceMPS"; }

  ~ImpliedPriceMPS() {}

  void WhyNotReady();

  void PrintDebugInfo() const;

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // do matrix multiplication on getting ready... then always use column multiplication with changed price.
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}

  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

 protected:
  ImpliedPriceMPS(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
                  std::vector<boost::reference_wrapper<SecurityMarketView> > market_view_vect_,
                  std::string& t_dep_shortcode_, std::string& t_dep_family_, PriceType_t _price_type_);

  inline bool AreAllReady();

  inline bool PseudoReady();

  // calculates A, and then calculates pseudo-inverse of A
  inline void CalculatePInv();

  inline void MatrixMultVec();

  inline void ColumnMultVal(unsigned int index_change, double price_change);

  inline void GetProductFamilyWeights();
};
}
