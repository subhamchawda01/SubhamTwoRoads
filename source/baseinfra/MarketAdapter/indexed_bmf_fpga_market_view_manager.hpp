/**
   \file MarketAdapter/indexed_bmf_fpga_market_view_manager.hpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

#define BMF_FPGA_BOOK_DEBUG 0

namespace HFSAT {

struct BestLevelInfo {
  int best_bid_int_px;
  int best_ask_int_px;
  int bid_size;
  int ask_size;
  int bid_orders;
  int ask_orders;

  BestLevelInfo() {
    best_bid_int_px = kInvalidIntPrice;
    best_ask_int_px = kInvalidIntPrice;

    bid_size = ask_size = bid_orders = ask_orders = 0;
  }

  void Set(int bid_px, int ask_px, int bid_sz, int ask_sz, int bid_ords, int ask_ords) {
    best_bid_int_px = bid_px;
    best_ask_int_px = ask_px;
    bid_size = bid_sz;
    ask_size = ask_sz;
    bid_orders = bid_ords;
    ask_orders = ask_ords;
  }
};

class IndexedBMFFpgaMarketViewManager : public BaseMarketViewManager, public BMFFPGAFullBookGlobalListener {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;
  std::vector<MktStatus_t> sec_id_to_curr_status_;
  std::vector<BestLevelInfo> sec_id_prev_best_lvl_info_;
  BookManagerErrorCode_t UpdateBook(const unsigned int security_id, FPGA_MDS::BMFFPGABookDeltaStruct* full_book,
                                    SecurityMarketView& smv, bool& l1_px_changed, bool& l1_sz_changed);
  bool NotificationAllowed(const SecurityMarketView& smv, bool is_intermediate, const unsigned int security_id);

  bool IsMarketUpdateSame(const SecurityMarketView& smv_, BestLevelInfo& prev_best_level);

 public:
  IndexedBMFFpgaMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                  SecurityNameIndexer& t_sec_name_indexer_,
                                  const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  ~IndexedBMFFpgaMarketViewManager() {}

  void OnFullBookChange(const unsigned int security_id, FPGA_MDS::BMFFPGABookDeltaStruct* full_book,
                        bool is_intermediate, bool is_mkt_closed);
  void OnTrade(const unsigned int security_id, const double trade_price, const int trade_size, TradeType_t buy_sell,
               bool is_mkt_closed);
  void OnMarketStatusUpdate(const unsigned int security_id, const MktStatus_t mkt_status);
};
}
