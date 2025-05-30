/**
    \file MarketAdapter/indexed_eobi_price_level_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "baseinfra/MarketAdapter/fpga_eobi_price_level_market_view_manager.hpp"

namespace HFSAT {

  FpgaEobiPriceLevelMarketViewManager :: FpgaEobiPriceLevelMarketViewManager ( DebugLogger & t_dbglogger_, const Watch & t_watch_, const SecurityNameIndexer & t_sec_name_indexer_, const std::vector < SecurityMarketView * > & t_security_market_view_map_ )
    :
      BaseMarketViewManager ( t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_ ),
      book_depth_ (5),
      l1_price_changed_( t_sec_name_indexer_.NumSecurityId ( ), false ),
      l1_size_changed_ ( t_sec_name_indexer_.NumSecurityId ( ), false ),
      sec_id_to_prev_update_was_quote_ ( t_sec_name_indexer_.NumSecurityId ( ), false ),
      last_trade_side_ ( t_sec_name_indexer_.NumSecurityId ( ), kTradeTypeNoInfo ),
      expected_bestask_int_price_ ( t_sec_name_indexer_.NumSecurityId ( ), 0 ),
      expected_bestask_size_ ( t_sec_name_indexer_.NumSecurityId ( ), 0 ),
      expected_bestbid_int_price_ ( t_sec_name_indexer_.NumSecurityId ( ), 0 ),
      expected_bestbid_size_ ( t_sec_name_indexer_.NumSecurityId ( ), 0 )
  {
    // Hack required to preserve the design of indexed book managers
    for (size_t sec_id_ = 0; sec_id_ < security_market_view_map_.size(); sec_id_++)
      {
        security_market_view_map_[ sec_id_ ] -> bid_access_bitmask_ = 0xFFFF;
        security_market_view_map_[ sec_id_ ] -> ask_access_bitmask_ = 0xFFFF;
        security_market_view_map_[ sec_id_ ] -> bid_level_change_bitmask_ = 0x0000;
        security_market_view_map_[ sec_id_ ] -> ask_level_change_bitmask_ = 0x0000;
        security_market_view_map_[ sec_id_ ] -> base_bid_index_ = 0;
        security_market_view_map_[ sec_id_ ] -> base_ask_index_ = 0;
        //Initialize the Indexed Book pool
        security_market_view_map_[ sec_id_ ] -> market_update_info_.bidlevels_.resize ( book_depth_, MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() ) );
        security_market_view_map_[ sec_id_ ] -> market_update_info_.asklevels_.resize ( book_depth_, MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() ) );
      }
  }

  void FpgaEobiPriceLevelMarketViewManager :: OnPriceLevelNew ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_, const double t_price_, const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_ )
  {
    SecurityMarketView & smv_ = * ( security_market_view_map_[ t_security_id_ ] );

    int int_price_ = smv_.GetIntPx ( t_price_ );

    int bid_index_ = 0;
    int ask_index_ = 0;

    int old_size_       = 0;
    int old_ordercount_ = 0;
    int old_price_      = 0;
    if( int_price_ == 0 && t_new_size_ == 0 && t_new_ordercount_ == 0 )//End of Book update => delete this price level and those below this
      {
        switch ( t_buysell_ )
        {
          case kTradeTypeBuy:
            {
              bid_index_ = t_level_added_ - 1;
              for ( int index_ = bid_index_; index_ < book_depth_; index_++ )
                {
                  smv_.market_update_info_.bidlevels_[ index_ ] = MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() );
                }
            }break;
          case kTradeTypeSell:
            {
              ask_index_ = t_level_added_ - 1;
              for ( int index_ = ask_index_; index_ < book_depth_; index_++ )
                {
                  smv_.market_update_info_.asklevels_[ index_ ] = MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() );
                }
            }break;
          default:
            {

            }break;
        }
        smv_.l2_changed_since_last_ = true;
        //End of Book update => no need to do anything else on this
      }
    else
      {
        switch ( t_buysell_ )
        {
          case kTradeTypeBuy:
            {
              bid_index_ = t_level_added_ - 1;

              if( bid_index_ < 0 || bid_index_ >= book_depth_ )
                return;     //We are not concerned about this level

              old_size_       = smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_size_;
              old_ordercount_ = smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_ordercount_;
              old_price_      = smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_int_price_;

              //Sanitize incoming values (Required for FPGA)
              if ( int_price_ == 0 )//Price hasn't changed
                {
                  int_price_ = old_price_;
                }

              // Required to identify intermediate updates (after Execution Summary messages) in the case of FPGA
              if ( bid_index_ == 0 && last_trade_side_[t_security_id_] == kTradeTypeSell &&
                  ( (expected_bestbid_int_price_[t_security_id_] == int_price_ &&
                  ( t_new_size_ <= expected_bestbid_size_[t_security_id_] )) || ( expected_bestbid_int_price_[t_security_id_] > int_price_ )))
                {
                  last_trade_side_[t_security_id_] = kTradeTypeNoInfo;
                }

              // Update the size and order count
              smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_size_       = t_new_size_;
              smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_ordercount_ = t_new_ordercount_;
              smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_int_price_ = int_price_;
              smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_price_ = t_price_;
              smv_.market_update_info_.bidlevels_[ bid_index_ ].limit_int_price_level_ = bid_index_;

              if ( bid_index_ == 0 )     //L1 update
                {
                  if ( old_price_ != int_price_ ) l1_price_changed_[ t_security_id_ ] = true;
                  else if ( old_size_ != t_new_size_ ) l1_size_changed_[ t_security_id_ ] = true;
                }
              else
                {
                  smv_.l2_changed_since_last_ = true;
                }

              // Sanitise ASK side
              if ( int_price_ >= smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_ )
                {
                  if ( smv_.market_update_info_.asklevels_[ 0 ].limit_size_ <= 0 )
                    {
                      // We should not do sanitization, if the other side is not ready
                      smv_.is_ready_ = false;
                      break;
                    }

                  DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname ( ) << " Ask side sanitization required even while using FPGA (Weird!) " << DBGLOG_ENDL_FLUSH ;
                  DBGLOG_DUMP ;
                  std::cerr << " Ask side sanitization required even while using FPGA (Weird!) \n";

                  int index_ = 0;

                  for ( ; index_ < book_depth_; index_++ )
                    {
                      if ( !( smv_.market_update_info_.asklevels_[ index_ ].limit_int_price_ <= int_price_
                          && smv_.market_update_info_.asklevels_[ index_ ].limit_size_ > 0 ) )
                        {
                          break;
                        }
                    }

                  for ( int temp_index_ = index_ ; ( temp_index_ < book_depth_ ) && ( smv_.market_update_info_.asklevels_[ temp_index_ ].limit_size_ > 0 ); temp_index_++ )
                    {
                      smv_.market_update_info_.asklevels_[ temp_index_ - index_ ] = smv_.market_update_info_.asklevels_[ temp_index_ ];
                      smv_.market_update_info_.asklevels_[ temp_index_ ] = MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() );
                    }

                  // Completely emptied the other side during sanitization
                  if ( index_ >= book_depth_ || smv_.market_update_info_.asklevels_[ 0 ].limit_size_ <= 0 )
                    {
                      smv_.is_ready_ = false;
                      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname ( ) << " Ask side empty after sanitization " << DBGLOG_ENDL_FLUSH ;
                      DBGLOG_DUMP ;
                      return;
                    }

                  // Update best variables
                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ 0 ].limit_size_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ 0 ].limit_ordercount_;

                  l1_price_changed_[ t_security_id_ ] = true;
                }

              if ( l1_price_changed_[ t_security_id_ ] || l1_size_changed_[ t_security_id_ ] )
                {
                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ 0 ].limit_size_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ 0 ].limit_ordercount_;
                }
            }
            break;
          case kTradeTypeSell:
            {
              ask_index_ = t_level_added_ - 1;

              if( ask_index_ < 0 || ask_index_ >= book_depth_ )
                return;     //We are not concerned about this level

              old_size_       = smv_.market_update_info_.asklevels_[ ask_index_ ].limit_size_;
              old_ordercount_ = smv_.market_update_info_.asklevels_[ ask_index_ ].limit_ordercount_;
              old_price_      = smv_.market_update_info_.asklevels_[ ask_index_ ].limit_int_price_;

              //Sanitize incoming values (Required for FPGA)
              if ( int_price_ == 0 )//Price hasn't changed
                {
                  int_price_ = old_price_;
                }

              // Required to identify intermediate updates (after Execution Summary messages) in the case of FPGA
              if ( ask_index_ == 0 && last_trade_side_[t_security_id_] == kTradeTypeBuy &&
                  ( (expected_bestask_int_price_[t_security_id_] == int_price_ &&
                  ( t_new_size_ <= expected_bestask_size_[t_security_id_] )) || ( expected_bestask_int_price_[t_security_id_] < int_price_ )))
                {
                  last_trade_side_[t_security_id_] = kTradeTypeNoInfo;
                }

              // Update the size and order count
              smv_.market_update_info_.asklevels_[ ask_index_ ].limit_size_       = t_new_size_;
              smv_.market_update_info_.asklevels_[ ask_index_ ].limit_ordercount_ = t_new_ordercount_;
              smv_.market_update_info_.asklevels_[ ask_index_ ].limit_int_price_  = int_price_;
              smv_.market_update_info_.asklevels_[ ask_index_ ].limit_price_      = t_price_;
              smv_.market_update_info_.asklevels_[ ask_index_ ].limit_int_price_level_ = ask_index_;

              if ( ask_index_ == 0 )     //L1 update
                {
                  if ( old_price_ != int_price_ ) l1_price_changed_[ t_security_id_ ] = true;
                  else if ( old_size_ != t_new_size_ ) l1_size_changed_[ t_security_id_ ] = true;
                }
              else
                {
                  smv_.l2_changed_since_last_ = true;
                }

              // Sanitise BID side
              if ( int_price_ <= smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_ )
                {
                  if ( smv_.market_update_info_.bidlevels_[ 0 ].limit_size_ <= 0 )
                    {
                      // We should not do sanitization, if the other side is not ready
                      smv_.is_ready_ = false;
                      break;
                    }

                  DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname ( ) << " Bid side sanitization required even while using FPGA (Weird!) " << DBGLOG_ENDL_FLUSH ;
                  DBGLOG_DUMP ;
                  std::cerr << " Bid side sanitization required even while using FPGA (Weird!) \n";

                  int index_ = 0;

                  for ( ; index_ < book_depth_; index_++ )
                    {
                      if (!( smv_.market_update_info_.bidlevels_[ index_ ].limit_int_price_ >= int_price_
                          && smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ > 0 ))
                        {
                          break;
                        }
                    }

                  for ( int temp_index_ = index_ ; ( temp_index_ < book_depth_ ) && ( smv_.market_update_info_.bidlevels_[ temp_index_ ].limit_size_ > 0 ); temp_index_++ )
                    {
                      smv_.market_update_info_.bidlevels_[ temp_index_ - index_ ] = smv_.market_update_info_.bidlevels_[ temp_index_ ];
                      smv_.market_update_info_.bidlevels_[ temp_index_ ] = MarketUpdateInfoLevelStruct ( 0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv() );
                    }

                  // Completely emptied the other side during sanitization
                  if ( index_ >= book_depth_ || smv_.market_update_info_.bidlevels_[ 0 ].limit_size_ <= 0 )
                    {
                      smv_.is_ready_ = false;
                      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname ( ) << " Bid side empty after sanitization " << DBGLOG_ENDL_FLUSH ;
                      DBGLOG_DUMP ;
                      return;
                    }



                  // Update best variables
                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ 0 ].limit_size_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ 0 ].limit_ordercount_;

                  l1_price_changed_[ t_security_id_ ] = true;
                }

              if ( l1_price_changed_[ t_security_id_ ] || l1_size_changed_[ t_security_id_ ] )
                {
                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ 0 ].limit_size_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ 0 ].limit_ordercount_;
                }
            }
            break;
          default:
            break;
        }
      }

    if ( smv_.market_update_info_.trade_update_implied_quote_ )
      {
        smv_.market_update_info_.trade_update_implied_quote_ = false;
        smv_.trade_print_info_.num_trades_++;
      }

    if ( l1_price_changed_[ t_security_id_ ] || l1_size_changed_[ t_security_id_ ] )
      {
        smv_.UpdateL1Prices ( );
      }

    if ( ! smv_.is_ready_ )
      {
        if ( smv_.market_update_info_.bidlevels_[ 0 ].limit_size_ > 0 &&
             smv_.market_update_info_.asklevels_[ 0 ].limit_size_ > 0 &&
             smv_.ArePricesComputed ( ) )
          {
            smv_.is_ready_ = true;
          }
      }

    if ( ! smv_.is_ready_ || t_is_intermediate_message_ || ( last_trade_side_[t_security_id_] != kTradeTypeNoInfo))
      {
        return;
      }

    if ( smv_.pl_change_listeners_present_ )
      {
        if ( t_buysell_ == kTradeTypeBuy )
          {
            smv_.NotifyOnPLChangeListeners ( t_security_id_, smv_.market_update_info_, t_buysell_, t_level_added_, int_price_, smv_.GetIntPx ( smv_.market_update_info_.bestbid_price_ - t_price_ ),
              old_size_, t_new_size_, old_ordercount_, t_new_ordercount_, t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C' );
          }
        else if ( t_buysell_ == kTradeTypeSell )
          {
            smv_.NotifyOnPLChangeListeners ( t_security_id_, smv_.market_update_info_, t_buysell_, t_level_added_, int_price_, smv_.GetIntPx ( t_price_ - smv_.market_update_info_.bestask_price_  ),
              old_size_, t_new_size_, old_ordercount_, t_new_ordercount_, t_is_intermediate_message_, old_size_ == 0 ? 'N' : 'C' );
          }
      }

    if ( l1_price_changed_[ t_security_id_ ] )
      {
        smv_.NotifyL1PriceListeners ( );

        l1_price_changed_[ t_security_id_ ] = false;
        l1_size_changed_ [ t_security_id_ ] = false;
      }
    else if ( l1_size_changed_[ t_security_id_ ] )
      {
        smv_.NotifyL1SizeListeners ( );

        l1_size_changed_ [ t_security_id_ ] = false;
      }
    else
      {
        smv_.NotifyL2Listeners ( );
      }

    if ( smv_.l2_changed_since_last_ )
      {
        smv_.NotifyL2OnlyListeners ( );

        smv_.l2_changed_since_last_ = false;
      }

    smv_.NotifyOnReadyListeners ( );
    sec_id_to_prev_update_was_quote_ [ t_security_id_ ] = true ;
    printBook(t_security_id_);
  }

  void FpgaEobiPriceLevelMarketViewManager :: OnPriceLevelChange ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_ , const double t_price_, const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_ )
  {
    double price_ = t_price_;
    int order_count_ = t_new_ordercount_;
    if (! ( t_price_ == 0 && t_new_size_ == 0 && t_new_ordercount_ == 0 )) {//All are 0 => End of Book Update
      SecurityMarketView & smv_ = * ( security_market_view_map_[ t_security_id_ ] );
      //Sanitize incoming values (Required for FPGA)
      if ( t_price_ == 0 )//Price hasn't changed
        {
          if ( t_buysell_ == kTradeTypeBuy )
            price_ = smv_.market_update_info_.bidlevels_[ t_level_changed_ - 1 ].limit_price_;
          else
            price_ = smv_.market_update_info_.asklevels_[ t_level_changed_ - 1 ].limit_price_;
        }
      if ( t_new_ordercount_ == 0)//Order count hasn't changed
        {
          if ( t_buysell_ == kTradeTypeBuy )
            order_count_ = smv_.market_update_info_.bidlevels_[ t_level_changed_ - 1 ].limit_ordercount_;
          else
            order_count_ = smv_.market_update_info_.asklevels_[ t_level_changed_ - 1 ].limit_ordercount_;
        }
    }
    OnPriceLevelNew ( t_security_id_, t_buysell_, t_level_changed_, price_, t_new_size_, order_count_, t_is_intermediate_message_ );
  }

  void FpgaEobiPriceLevelMarketViewManager :: OnPriceLevelDelete ( const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_ , const double t_price_, const bool t_is_intermediate_message_ )
  {
  }

  void FpgaEobiPriceLevelMarketViewManager :: OnTrade ( const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_, const TradeType_t t_buysell_ )
  {
    //To mark intermediate BookLevelUpdates
    last_trade_side_[t_security_id_] = t_buysell_;

    SecurityMarketView & smv_ = * ( security_market_view_map_[ t_security_id_ ] );

    // not till book is ready
    if ( ( smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice ) ||
        ( smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice ) )
      {
        return;
      }

    int t_trade_int_price_ = smv_.GetIntPx ( t_trade_price_ );

    // same as SecurityMarketView::OnTrade
    if ( smv_.trade_print_info_.computing_last_book_tdiff_ )
      {
        if ( sec_id_to_prev_update_was_quote_ [ t_security_id_ ] )
          { // the difference between last_book_mkt_size_weighted_price_ and mkt_size_weighted_price_ is that
            // in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
            // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of mkt_size_weighted_price_
            // the last time the update was a book message
            smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_ ; // noting the mktpx as it was justbefore the first trade message
          }
      }

    if ( ! smv_.market_update_info_.trade_update_implied_quote_ )
      {
        smv_.market_update_info_.trade_update_implied_quote_ = true;
      }

    smv_.StorePreTrade ( );

    // Masking code
    switch ( t_buysell_ )
    {
      case kTradeTypeBuy: // Aggressive Buy - hence, try masking the ASK levels
        {
          if ( t_trade_int_price_ < smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_ )
            {
              // Possible reasons:
              // 1. An aggressive buy that resulted in synthetic match
              // 2. Error case resulted from wrong book because of either wrong logic in book or missing packets
            }

          else if ( t_trade_int_price_ == smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_ )// At best ask price
            {
              // If the trade size is big enough to clear this level
              if ( t_trade_size_ >= smv_.market_update_info_.asklevels_[ 0 ].limit_size_ )
                {
                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ 1 ].limit_int_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ 1 ].limit_size_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ 1 ].limit_price_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ 1 ].limit_ordercount_;
                }

              // Otherwise, update the best variables with the current level
              else
                {
                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ 0 ].limit_size_ - (int)t_trade_size_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ 0 ].limit_ordercount_;
                }
              expected_bestask_int_price_[t_security_id_] = smv_.market_update_info_.bestask_int_price_;
              expected_bestask_size_[t_security_id_] = smv_.market_update_info_.bestask_size_;

              smv_.UpdateL1Prices ( );

              // set the primary variables
              smv_.trade_print_info_.trade_price_     = t_trade_price_ ;
              smv_.trade_print_info_.size_traded_     = t_trade_size_ ;
              smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_ ;
              smv_.trade_print_info_.buysell_         = t_buysell_ ;

              smv_.SetTradeVarsForIndicatorsIfRequired ( );
            }

          else
            {
              if ( ( smv_.market_update_info_.asklevels_[ book_depth_ - 1 ].limit_size_ > 0 )
                  && ( t_trade_int_price_ > smv_.market_update_info_.asklevels_[ book_depth_ - 1 ].limit_int_price_) )
                return;

              int index_, size_deduction_ = (int)t_trade_size_;

              for ( index_ = 0; ( index_ < book_depth_ ) && (smv_.market_update_info_.asklevels_[ index_ ].limit_size_ > 0 )
                && (smv_.market_update_info_.asklevels_[ index_ ].limit_int_price_ < t_trade_int_price_); index_++ )
                {
                  size_deduction_ -= smv_.market_update_info_.asklevels_[ index_ ].limit_size_;
                }

              if ( ( index_ >= book_depth_ ) || ( smv_.market_update_info_.asklevels_[ index_ ].limit_size_ <= 0 ) )
                {
                  return;
                }
              if ( size_deduction_ >= smv_.market_update_info_.asklevels_[ index_ ].limit_size_ )
                {
                  int next_ask_index_ = index_ + 1;

                  if ( next_ask_index_ >= book_depth_ || ( smv_.market_update_info_.asklevels_[ next_ask_index_ ].limit_size_ <= 0 ) )
                    {
                      return;
                    }

                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ next_ask_index_ ].limit_int_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ next_ask_index_ ].limit_size_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ next_ask_index_ ].limit_price_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ next_ask_index_ ].limit_ordercount_;
                }
              else
                {
                  smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[ index_ ].limit_int_price_;
                  smv_.market_update_info_.bestask_size_      = smv_.market_update_info_.asklevels_[ index_ ].limit_size_ - size_deduction_;
                  smv_.market_update_info_.bestask_price_     = smv_.market_update_info_.asklevels_[ index_ ].limit_price_;
                  smv_.market_update_info_.bestask_ordercount_= smv_.market_update_info_.asklevels_[ index_ ].limit_ordercount_;
                }
              expected_bestask_int_price_[t_security_id_] = smv_.market_update_info_.bestask_int_price_;
              expected_bestask_size_[t_security_id_] = smv_.market_update_info_.bestask_size_;

              smv_.UpdateL1Prices ( );

              for ( index_ = 0; ( index_ < book_depth_ ) && (smv_.market_update_info_.asklevels_[ index_ ].limit_size_ > 0 )
                && (smv_.market_update_info_.asklevels_[ index_ ].limit_int_price_ < t_trade_int_price_); index_++ )
                {
                  // set the primary variables
                  smv_.trade_print_info_.trade_price_     = smv_.market_update_info_.asklevels_[ index_ ].limit_price_ ;
                  smv_.trade_print_info_.size_traded_     = smv_.market_update_info_.asklevels_[ index_ ].limit_size_ ;
                  smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.asklevels_[ index_ ].limit_int_price_ ;
                  smv_.trade_print_info_.buysell_         = t_buysell_ ;

                  smv_.SetTradeVarsForIndicatorsIfRequired ( );

                  if ( smv_.is_ready_ )
                    {
                      smv_.NotifyTradeListeners ( );
                    }
                }

              // set the primary variables
              smv_.trade_print_info_.trade_price_     = t_trade_price_ ;
              smv_.trade_print_info_.size_traded_     = std::max ( 1, size_deduction_ ) ;
              smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_ ;
              smv_.trade_print_info_.buysell_         = t_buysell_ ;

              smv_.SetTradeVarsForIndicatorsIfRequired ( );
            }
        }
        break;
      case kTradeTypeSell: // Aggressive Sell - hence, try masking the BID levels
        {
          if ( t_trade_int_price_ > smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_ )
            {
              // Possible reasons:
              // 1. An aggressive buy that resulted in synthetic match
              // 2. Error case resulted from wrong book because of either wrong logic in book or missing packets
            }

          else if ( t_trade_int_price_ == smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_ )// At best bid price
            {
              // If the trade size is big enough to clear this level
              if ( t_trade_size_ >= smv_.market_update_info_.bidlevels_[ 0 ].limit_size_ )
                {
                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ 1 ].limit_int_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ 1 ].limit_size_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ 1 ].limit_price_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ 1 ].limit_ordercount_;
                }

              // Otherwise, update the best variables with the current level
              else
                {
                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ 0 ].limit_int_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ 0 ].limit_size_ - (int)t_trade_size_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ 0 ].limit_price_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ 0 ].limit_ordercount_;
                }
              expected_bestbid_int_price_[t_security_id_] = smv_.market_update_info_.bestbid_int_price_;
              expected_bestbid_size_[t_security_id_] = smv_.market_update_info_.bestbid_size_;

              smv_.UpdateL1Prices ( );

              // set the primary variables
              smv_.trade_print_info_.trade_price_     = t_trade_price_ ;
              smv_.trade_print_info_.size_traded_     = t_trade_size_ ;
              smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_ ;
              smv_.trade_print_info_.buysell_         = t_buysell_ ;

              smv_.SetTradeVarsForIndicatorsIfRequired ( );
            }

          else
            {
              if ( ( smv_.market_update_info_.bidlevels_[ book_depth_ - 1 ].limit_size_ > 0 )
                && ( t_trade_int_price_ < smv_.market_update_info_.bidlevels_[ book_depth_ - 1 ].limit_int_price_) )
                return;

              int index_, size_deduction_ = (int)t_trade_size_;

              for ( index_ = 0; ( index_ < book_depth_ ) && (smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ > 0 )
                && (smv_.market_update_info_.bidlevels_[ index_ ].limit_int_price_ > t_trade_int_price_); index_++ )
                {
                  size_deduction_ -= smv_.market_update_info_.bidlevels_[ index_ ].limit_size_;
                }

              if ( index_ >= book_depth_  || (smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ <= 0 ) )
                {
                  return;
                }
              if ( size_deduction_ >= smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ )
                {
                  int next_bid_index_ = index_ + 1;

                  if ( next_bid_index_ >= book_depth_  || (smv_.market_update_info_.bidlevels_[ next_bid_index_ ].limit_size_ <= 0 ) )
                    {
                      return;
                    }

                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ next_bid_index_ ].limit_int_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ next_bid_index_ ].limit_size_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ next_bid_index_ ].limit_price_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ next_bid_index_ ].limit_ordercount_;
                }
              else
                {
                  smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[ index_ ].limit_int_price_;
                  smv_.market_update_info_.bestbid_size_      = smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ - size_deduction_;
                  smv_.market_update_info_.bestbid_price_     = smv_.market_update_info_.bidlevels_[ index_ ].limit_price_;
                  smv_.market_update_info_.bestbid_ordercount_= smv_.market_update_info_.bidlevels_[ index_ ].limit_ordercount_;
                }
              expected_bestbid_int_price_[t_security_id_] = smv_.market_update_info_.bestbid_int_price_;
              expected_bestbid_size_[t_security_id_] = smv_.market_update_info_.bestbid_size_;

              smv_.UpdateL1Prices ( );

              for ( index_ = 0; ( index_ < book_depth_ ) && (smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ > 0 )
                && (smv_.market_update_info_.bidlevels_[ index_ ].limit_int_price_ > t_trade_int_price_); index_++ )
                {
                  // set the primary variables
                  smv_.trade_print_info_.trade_price_     = smv_.market_update_info_.bidlevels_[ index_ ].limit_price_ ;
                  smv_.trade_print_info_.size_traded_     = smv_.market_update_info_.bidlevels_[ index_ ].limit_size_ ;
                  smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.bidlevels_[ index_ ].limit_int_price_ ;
                  smv_.trade_print_info_.buysell_         = t_buysell_ ;

                  smv_.SetTradeVarsForIndicatorsIfRequired ( );

                  if ( smv_.is_ready_ )
                    {
                      smv_.NotifyTradeListeners ( );
                    }
                }

              // set the primary variables
              smv_.trade_print_info_.trade_price_     = t_trade_price_ ;
              smv_.trade_print_info_.size_traded_     = std::max ( 1, size_deduction_ ) ;
              smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_ ;
              smv_.trade_print_info_.buysell_         = t_buysell_ ;

              smv_.SetTradeVarsForIndicatorsIfRequired ( );
            }
        }
        break;
      default:
        break;
    }

    // Same as SecurityMarketView::OnTrade
    if ( smv_.is_ready_ )
      {
        smv_.NotifyTradeListeners ( );
        smv_.NotifyOnReadyListeners ( );
      }
    sec_id_to_prev_update_was_quote_ [ t_security_id_ ] = false ;
    printBook(t_security_id_);
  }
}
