#ifndef _INDICATOR_BOOK_DELTA_H
#define _INDICATOR_BOOK_DELTA_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"
#include "tradeengine/Indicator/BasePrice.hpp"

class BookDelta : public BasePrice {
  int num_levels_;
  double decay_factor_;
  double skew_factor_;
  double average_spread_;
  double level_decay_factor_;
  double bid_val_;
  double ask_val_;
  bool CalculateBookDelta();
  bool CalculateBidDelta();
  bool CalculateAskDelta();

  class PriceSize {
   public:
    double price_;
    int price_int_;
    int size_;
    PriceSize() : price_(0), price_int_(0), size_(0) {}
  };

  std::vector<PriceSize> bid_pxsz_vec_;
  std::vector<PriceSize> ask_pxsz_vec_;
  std::vector<PriceSize> temp_store_;

 public:
  BookDelta(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_, double weight_,
            int num_levels_, double decay_factor, double skew_factor, double average_spread, double level_decay_factor);
  virtual ~BookDelta();

  virtual void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);

  virtual void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                            const HFSAT::MarketUpdateInfo& _market_update_info_);
};

#endif  //_INDICATOR_BOOK_DELTA_H
