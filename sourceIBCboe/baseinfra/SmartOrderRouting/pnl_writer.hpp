/**
    \file SmartOrderRouting/pnl_writer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

class PnlWriter {
 protected:
  //  log_buffer_ -> buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec ;
  //  log_buffer_ -> buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec ;
  //  log_buffer_ -> buffer_data_.query_trade_.trade_size_ = trade1size ;
  //  log_buffer_ -> buffer_data_.query_trade_.trade_price_ = this_trade_price_ ;
  //  log_buffer_ -> buffer_data_.query_trade_.new_position_ = 0 ;
  //  log_buffer_ -> buffer_data_.query_trade_.open_unrealized_pnl_ = 0 ;
  //  log_buffer_ -> buffer_data_.query_trade_.total_pnl_ = (int) round ( realized_pnl_ ) ;
  //  log_buffer_ -> buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size() ;
  //  log_buffer_ -> buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price() ;
  //  log_buffer_ -> buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price() ;
  //  log_buffer_ -> buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size() ;
  //  log_buffer_ -> buffer_data_.query_trade_.mult_risk_ = mult_risk_ ;
  //  log_buffer_ -> buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_ ;
  //  log_buffer_ -> buffer_data_.query_trade_.open_or_flat_ = 'F' ;
  //  log_buffer_ -> buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar ( _buysell_ ) ;
 public:
  virtual ~PnlWriter() {}
  virtual void WriteTrade(int security_id, ttime_t time, int trade_size, double trade_price, int new_position,
                          double opentrade_pnl, int total_pnl, int bestbid_size, double bestbid_price,
                          double bestask_price, int bestask_size, double mult_risk, int mult_base_pnl,
                          char open_or_flat, char trade_type) = 0;
};
}
