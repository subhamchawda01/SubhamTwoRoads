/*
 * spread_market_view.hpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#ifndef SPREAD_MARKET_VIEW_HPP_
#define SPREAD_MARKET_VIEW_HPP_

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/book_interface.hpp"

namespace HFSAT {

class SpreadMarketView;

class SpreadMarketViewListener {
 public:
  virtual ~SpreadMarketViewListener() {}
  virtual void OnSpreadMarketViewUpdate(const SpreadMarketView &_spread_market_view_) = 0;
};

class SpreadMarketView : public BookInterface, public SecurityMarketViewChangeListener {
 protected:
  DebugLogger &dbglogger_;
  const Watch &watch_;
  std::string spread_shc_;
  std::vector<std::string> shortcode_vec_;
  std::vector<int> uts_vec_;
  std::vector<SecurityMarketView *> smv_vec_;
  std::map<unsigned int, int> secid_to_idx_map_;
  std::vector<bool> ready_vec_;
  bool is_ready_;

  double bestbid_price_;
  double bestask_price_;
  double bestbid_size_;
  double bestask_size_;
  double aggbid_price_;
  double aggask_price_;
  int last_bestbid_size_idx_;
  int last_bestask_size_idx_;
  std::vector<double> last_bestbid_price_;
  std::vector<double> last_bestask_price_;

  double mid_price_;
  double mkt_price_;

  int norm_factor_;

  mutable std::vector<SpreadMarketViewListener *> listeners_;

  void ComputePrices();
  void InitializeBestVars();

  void NotifyListeners();

 public:
  virtual ~SpreadMarketView() {}

  SpreadMarketView(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &_spread_shc_);

  inline std::string shortcode() const { return spread_shc_; }
  inline bool is_ready() const { return is_ready_; }

  // BookInterface
  inline double bestbid_price() const { return bestbid_price_ / norm_factor_; }
  inline double bestask_price() const { return bestask_price_ / norm_factor_; }

  inline double bestbid_size() const { return bestbid_size_; }
  inline double bestask_size() const { return bestask_size_; }

  double price_from_type(const std::string pricetype_) const;

  double price_from_type(const PriceType_t t_price_type_) const;

  void ShowBook() const;
  void PrintBook() const;

  // SecurityMarketViewChangeListener
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                    const MarketUpdateInfo &_market_update_info_);

  inline std::vector<int> GetUtsVec() const { return uts_vec_; }
  inline std::vector<std::string> GetShcVec() const { return shortcode_vec_; }

  int SumSize() const;
  bool IsBellyShc(const std::string _shortcode_) const;

  bool SubscribeSpreadMarketView(SpreadMarketViewListener *_listener_) const;
};
} /* namespace HFSAT */

#endif /* SPREAD_MARKET_VIEW_HPP_ */
