/**
    \file MarketAdapter/indexed_eobi_price_level_market_view_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

  class FpgaEobiPriceLevelMarketViewManager : public PriceLevelGlobalListener, public BaseMarketViewManager, public GlobalOrderChangeListener
  {
  private:
    int book_depth_;
    // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
    std::vector < bool > l1_price_changed_;
    std::vector < bool > l1_size_changed_;
    std::vector < bool > sec_id_to_prev_update_was_quote_ ; // for setting of a variable in ontrade

    //For removing intermediate messages
    std::vector < TradeType_t > last_trade_side_;
    std::vector < int > expected_bestask_int_price_;
    std::vector < int > expected_bestask_size_;
    std::vector < int > expected_bestbid_int_price_;
    std::vector < int > expected_bestbid_size_;

  public:

    FpgaEobiPriceLevelMarketViewManager (DebugLogger & t_dbglogger_, const Watch & t_watch_, const SecurityNameIndexer & t_sec_name_indexer_, const std::vector < SecurityMarketView * > & t_security_market_view_map_ ) ;

    void OnPriceLevelNew ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_, const double t_price_, const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_ ) ;
    void OnPriceLevelChange ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_ , const double t_price_, const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_ ) ;
    void OnPriceLevelDelete ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_ , const double t_price_, const bool t_is_intermediate_message_ ) ;
    void OnTrade ( const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_, const TradeType_t t_buysell_ ) ;

    inline void OnGlobalOrderChange ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_int_price_ )
    {
    }

    void OnPriceLevelDeleteFrom ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_min_level_deleted_, const bool t_is_intermediate_message_ )
    {
    }

    void OnPriceLevelDeleteThrough ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_max_level_deleted_, const bool t_is_intermediate_message_ )
    {
    }

    void OnPriceLevelOverlay ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_overlayed_ , const double t_price_, const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_ )
    {
    }
    inline void printBook( const unsigned int t_security_id_ ) {
      return;
      SecurityMarketView & smv_ = * ( security_market_view_map_[ t_security_id_ ] );
      for ( int t_level_ = 0; t_level_ < 5; t_level_++ )
        {
          printf ( "%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n",
              smv_.market_update_info_.bidlevels_[ t_level_ ].limit_int_price_level_,
              smv_.market_update_info_.bidlevels_[ t_level_ ].limit_size_, smv_.market_update_info_.bidlevels_[ t_level_ ].limit_ordercount_,
              smv_.market_update_info_.bidlevels_[ t_level_ ].limit_price_, smv_.market_update_info_.bidlevels_[ t_level_ ].limit_int_price_,
              smv_.market_update_info_.asklevels_[ t_level_ ].limit_int_price_, smv_.market_update_info_.asklevels_[ t_level_ ].limit_price_,
              smv_.market_update_info_.asklevels_[ t_level_ ].limit_ordercount_, smv_.market_update_info_.asklevels_[ t_level_ ].limit_size_,
              smv_.market_update_info_.asklevels_[ t_level_ ].limit_int_price_level_ );
        }
    }

  };
}
