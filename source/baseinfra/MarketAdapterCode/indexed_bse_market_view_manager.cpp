/**
    \file MarketAdapter/indexed_eobi_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_bse_market_view_manager.hpp"

namespace HFSAT {

//#define INDEXED_BOOK_SIZE 1023
//#define INITIAL_BASE_INDEX 511
#define HIGH_ACCESS_INDEX 2000
#define LOW_ACCESS_INDEX 50

IndexedBSEMarketViewManager::IndexedBSEMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_mask_levels_above_(t_sec_name_indexer_.NumSecurityId(), false),
      skip_notifications_(t_sec_name_indexer_.NumSecurityId(), false),
      bid_side_exec_summary_seen_(t_sec_name_indexer_.NumSecurityId(), 0),
      ask_side_exec_summary_seen_(t_sec_name_indexer_.NumSecurityId(), 0) {


  //std::cout << "IndexedBSEMarketViewManager::IndexedBSEMarketViewManager" << std::endl;
  for (unsigned int t_ctr_ = 0; t_ctr_ < t_sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    SecurityMarketView &smv_ = *(t_security_market_view_map_[t_ctr_]);
    HFSAT::BSETradingLimit_t *t_trading_limit_ = BSESecurityDefinitions::GetTradingLimits(smv_.shortcode());
    smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_ * (1.5);
    smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_ * (0.5);
  }
 }

void IndexedBSEMarketViewManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                              const double t_price_, const uint32_t t_size_,
                                              const bool t_is_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  if(int_price_ > smv_.upper_int_price_limit_ || int_price_ < smv_.lower_int_price_limit_){
    return;
  }
/*
  std::cout << "ORDER_ADD: B/S: " << t_buysell_ << " , Px: " << t_price_ << " , Size: " << t_size_ << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
  smv_.ShowMarket();

  std::cout << "bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;
  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
  
  std::cout << "int_price_: " << int_price_ << std::endl;
*/
  if (!smv_.initial_book_constructed_) {
    //std::cout << "smv_.initial_book_construct" << std::endl;
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      //std::cout << "BUY: " << std::endl;
      bid_side_exec_summary_seen_[t_security_id_] = 0;

      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      int old_bid_index_ = (int)smv_.base_bid_index_;
/*
      std::cout << "bid_index_ = " << bid_index_ 
		<< "\nbse_bid_idx - (base_bid_index_limit_px - int_px) = "
		<< (int)smv_.base_bid_index_ << " - (" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
		<< " - " << int_price_ << " ) " 
		<< std::endl;
     
      std::cout << "LOW_ACCESS_INDEX: " << LOW_ACCESS_INDEX << " smv_.max_tick_range_: " << (int)smv_.max_tick_range_ << std::endl;
*/
      // There are 0 levels on the bid side
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
	//std::cout << "ZERO levels on bid side" << std::endl;
        if (bid_index_ < LOW_ACCESS_INDEX) {
	  //std::cout << "bid_index_ < LOW_ACCESS_INDEX :: " << bid_index_ << " " << LOW_ACCESS_INDEX << std::endl;
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
	  //std::cout << "bid_index_ >= (int)smv_.max_tick_range_ :: " << bid_index_ << " " << (int)smv_.max_tick_range_ << std::endl;
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
	  //std::cout << "AFTER RebuildIndexHighAccess: base_bid_idx :: " << bid_index_ << std::endl;
        } else {
          // bid_index_ falls within the range. So, set base_bid_index_ as bid_index_
          smv_.base_bid_index_ = bid_index_;
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ < 0) {

        // Skip the message if we don't have any info for this level
        return;
      }

// std::cout << smv_.ShowMarket() << std::endl;

      if (bid_index_ >= (int)smv_.max_tick_range_) {
	//std::cout << "bid_index_ >= (int)smv_.max_tick_range_ :: " << bid_index_ << " " << (int)smv_.max_tick_range_ 
	//	  << "\nRebuildIndexHighAccess AGAIN" << std::endl;
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        bid_index_ = smv_.base_bid_index_;
	  //std::cout << "AFTER RebuildIndexHighAccess: base_bid_idx :: " << bid_index_ << std::endl;

        l1_price_changed_[t_security_id_] = true;
      }

//  std::cout << smv_.ShowMarket() << std::endl;
/*      std::cout << "INFO at base: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl; 
      std::cout << "\n" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ 
		<< "\t" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_
		<< "\t" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
		<< "  X  " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
		<< "\t" << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_
		<< "\t" << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_
		<< std::endl << "INT_PX: " << int_price_ << std::endl; 
*/
      if (bid_index_ > (int)smv_.base_bid_index_ || 
	  (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 &&
	   int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_)) { //new changes
//        std::cout << "bid_index_ > (int)smv_.base_bid_index_ :: " << bid_index_ << " " << (int)smv_.base_bid_index_ << std::endl;   
        // Use assignment instead of increment while updating size and order count to make sure that we don't depend on
        // them being set to zero earlier.
        // But note that if this introduces holes in the book we do need the size to have been zeroed out at all invalid
        // levels
        //if(t_size_ > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_){
	smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 1;

        smv_.base_bid_index_ = bid_index_;
/*
	std::cout << "smv_.base_bid_index_, bid_index_ :: " << smv_.base_bid_index_ << " " << bid_index_ << std::endl;
  	std::cout << "int_price: " << int_price_
		  << " base_ask_index: " << smv_.base_ask_index_
		  << " ask_index_int_px: " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
		  << std::endl;
*/
        if (int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
/*
	  std::cout << "int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ :: "
		    << int_price_ << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
		    << std::endl;
*/
          if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
/*
	    std::cout << "NO SANITISATION: as smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0 :: " 
		      << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ 
		      << std::endl;
*/
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }
//  std::cout << smv_.ShowMarket() << std::endl;
	  //if(t_size_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_){ //new changes
//	  std::cout << "Sanitise ask side" << std::endl;
          // Sanitise ask side
          int index_ = smv_.base_ask_index_;
	  int size_rem = t_size_;
//	  std::cout << "smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
          for (; index_ >= 0; index_--) {
/*	
	    std::cout << "FOR Index: " << index_ 
		      << "asklevel_limit_int_price_: " << smv_.market_update_info_.asklevels_[index_].limit_int_price_
		      << "asklevels_limit_size_: " << smv_.market_update_info_.asklevels_[index_].limit_size_
		      << std::endl;
*/
            if (smv_.market_update_info_.asklevels_[index_].limit_int_price_ > int_price_ &&
                smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
	 
//	      std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
              smv_.base_ask_index_ = index_;
//	      std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
              break;
            } else {

//new change
 	      if(size_rem < smv_.market_update_info_.asklevels_[index_].limit_size_){	
	 	smv_.market_update_info_.asklevels_[index_].limit_size_ -= size_rem;
		size_rem = 0;
		smv_.base_ask_index_ = index_;
		break;
	      }
              size_rem -= smv_.market_update_info_.asklevels_[index_].limit_size_;
//
              smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
              smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
/*
	      std::cout << "AFTER FOR Index: " << index_ 
		      << "asklevel_limit_int_price_: " << smv_.market_update_info_.asklevels_[index_].limit_int_price_
		      << "asklevels_limit_size_: " << smv_.market_update_info_.asklevels_[index_].limit_size_
		      << std::endl;
*/
            }
          }

         //new changes
         if(size_rem <= 0){

// 	   std::cout << "Update BID Index: " << smv_.base_bid_index_ << std::endl;
           smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ = 0;
           smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ = 0;
 
	   int new_bid_index_ = smv_.base_bid_index_ - 1;
//	   std::cout << "BEFORE For loop: base_bid, new bid: " << smv_.base_bid_index_ << " " << new_bid_index_ << std::endl;
           for (; new_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[new_bid_index_].limit_size_ <= 0; new_bid_index_--){
             //std::cout << "New Bid: " << new_bid_index_ << std::endl;
      	   }
	/*
           if(new_bid_index_ < 0){
	     smv_.base_bid_index_ = old_bid_index_;
	   }
	*/
	   smv_.base_bid_index_ = (new_bid_index_ < 0) ? old_bid_index_ : new_bid_index_;
	 }
/*	 
      std::cout << "INFO AFTER UPDATES at base: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl; 
      std::cout << "\n" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ 
		<< "\t" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_
		<< "\t" << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
		<< "  X  " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
		<< "\t" << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_
		<< "\t" << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_
		<< std::endl << "INT_PX: " << int_price_ << std::endl; 
*/
         //
         
          //}
//  std::cout << smv_.ShowMarket() << std::endl;

          // Completely emptied the other side during sanitization
          if (index_ < 0) {
            smv_.is_ready_ = false;
//            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after sanitization "
//                                        << DBGLOG_ENDL_FLUSH;
//            DBGLOG_DUMP;
//            std::cout << " " << smv_.secname() << " Ask side empty after sanitization "
//                                        << std::endl;
            return; 
          }
//	  else{ //new change

            // Check if we need to re-centre the base_ask_index
            if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
/*
	      std::cout << "re-centre the base_ask_index as smv_.base_ask_index_ < LOW_ACCESS_INDEX: " 
	                << smv_.base_ask_index_ << " " << LOW_ACCESS_INDEX << std::endl;
	      std::cout << "RebuildIndexLowAccess" << std::endl;
*/
              RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                                    smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
            }
//   	  } //new change
          // Set best ask variables
          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
          smv_.market_update_info_.bestask_price_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_size_ =
              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;

/*
	  std::cout << "smv_.base_ask_index_: " << smv_.base_ask_index_
		    << "\n" << smv_.market_update_info_.bestask_int_price_ 
		    << " " << smv_.market_update_info_.bestask_price_ 
		    << " " << smv_.market_update_info_.bestask_size_
		    << " " << smv_.market_update_info_.bestask_ordercount_ << std::endl;
*/

          if (!smv_.price_to_yield_map_.empty()) {
	//    std::cout << "!smv_.price_to_yield_map_.empty()" << std::endl;
            smv_.hybrid_market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
            smv_.hybrid_market_update_info_.bestask_size_ =
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
          }
          // Not calling non-self update as it get's overwritten later
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;

        l1_size_changed_[t_security_id_] = true;
      } else {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
      }
//  std::cout << smv_.ShowMarket() << std::endl;
    } break;
    case kTradeTypeSell: {
//      std::cout << "SELL: " << std::endl;
//  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
      ask_side_exec_summary_seen_[t_security_id_] = 0;

      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      int old_ask_index_ = (int)smv_.base_ask_index_;

//      std::cout << "smv_.base_ask_index_, ask_index_ old :: " << (int)smv_.base_ask_index_ << " , "  
//		<< ask_index_ << " , " << old_ask_index_ << std::endl;
      // There are 0 levels on the ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
//	std::cout << "0 levels on ask side: " << std::endl;
        if (ask_index_ < LOW_ACCESS_INDEX) {
//	  std::cout << "ask_index_ < LOW_ACCESS_INDEX :: " << ask_index_ << " , " << LOW_ACCESS_INDEX << std::endl;
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          ask_index_ = smv_.base_ask_index_;
//	  std::cout << "AFTER RebuildIndexLowAccess ask_indx: " << ask_index_ 
//		    << " bid_indx: " << smv_.base_bid_index_ << std::endl;
        } else if (ask_index_ >= (int)smv_.max_tick_range_) {
//	  std::cout << "else if ask_index_ >= (int)smv_.max_tick_range_ :: " << ask_index_ << " , " << (int)smv_.max_tick_range_ 
//		    << "\nRebuildIndexHighAccess:" << std::endl;
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
          ask_index_ = smv_.base_ask_index_;
//	  std::cout << "AFTER RebuildIndexHighAccess ask_indx: " << ask_index_ << std::endl;
        } else {
          // ask_index_ falls within the range. So, set base_ask_index_ as ask_index_
//	      std::cout << "*BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
          smv_.base_ask_index_ = ask_index_;
//	      std::cout << "*AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ < 0) {
        // Skip the message if we don't have info for this level
        return;
      }
//  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
//      smv_.ShowMarket();

      if (ask_index_ >= (int)smv_.max_tick_range_) {
//	std::cout << "if ask_index_ >= (int)smv_.max_tick_range_ :: " << ask_index_ << " , " << (int)smv_.max_tick_range_ 
//		  << "\nRebuildIndexHighAccess: " << std::endl;
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        ask_index_ = smv_.base_ask_index_;

//	std::cout << "AFTER RebuildIndexHighAccess ask_indx: " << ask_index_ << std::endl;
        l1_price_changed_[t_security_id_] = true;
      }
/*
      std::cout << "Ask_idx, base_ask_idx :: " << ask_index_ << " , " << (int)smv_.base_ask_index_ 
		<< " base_bid_idx :: " << smv_.base_bid_index_  
		<< " bid_px: " << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
		<< std::endl;
*/
//      smv_.ShowMarket();
      if (ask_index_ > (int)smv_.base_ask_index_ ||
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
	   int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_)) { //new changes
//	std::cout << "ask_index_ > (int)smv_.base_ask_index_ :: "
//		  << ask_index_ << " > " << (int)smv_.base_ask_index_ << std::endl;
        // Use assignment instead of increment while updating size and order count to make sure that they are set to
        // zero earlier.
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 1;

//	      std::cout << "**BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
        smv_.base_ask_index_ = ask_index_;
//	      std::cout << "**AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;

        if (int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          // Sanitise bid side
          int index_ = smv_.base_bid_index_;

          int size_rem = t_size_; //new changes
          for (; index_ >= 0; index_--) {
            if (smv_.market_update_info_.bidlevels_[index_].limit_int_price_ < int_price_ &&
                smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
              smv_.base_bid_index_ = index_;
              break;
            } else {
//new changes
	      if(size_rem < smv_.market_update_info_.bidlevels_[index_].limit_size_){
                smv_.market_update_info_.bidlevels_[index_].limit_size_ -= size_rem;
                size_rem = 0;
                smv_.base_bid_index_ = index_;
                break;
              }
              size_rem -= smv_.market_update_info_.bidlevels_[index_].limit_size_;

//
              smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
              smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
            }
          }
//new changes
         if(size_rem <= 0){
//           std::cout << "Update ASK Index: " << smv_.base_ask_index_ << std::endl;
           smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ = 0;
           smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ = 0;

           int new_ask_index_ = smv_.base_ask_index_ - 1;
           for (; new_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[new_ask_index_].limit_size_ <= 0; new_ask_index_--)
            ;

	    smv_.base_ask_index_ = (new_ask_index_ < 0) ? old_ask_index_ : new_ask_index_;
            //smv_.base_ask_index_ = new_ask_index_;
         }


//
          // Completely emptied the other side during sanitization
          if (index_ < 0) {
            smv_.is_ready_ = false;
//            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization "
//                                        << DBGLOG_ENDL_FLUSH;
//            DBGLOG_DUMP;
//	    std::cout << "RETURN: " << smv_.secname() << " Bid side empty after sanitization" << std::endl;
            return;
          }
//	  else{ //new changes

            // Check if we need to re-centre the base_bid_index_
            if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                                    smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
            }
//	  } //new changes
          // Set best bid variables
          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
          smv_.market_update_info_.bestbid_price_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
          smv_.market_update_info_.bestbid_size_ =
              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ =
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestbid_ordercount_ =
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
            smv_.hybrid_market_update_info_.bestbid_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
            smv_.hybrid_market_update_info_.bestbid_size_ =
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
          }
          // Not calling non-self update best as it gets overwritten later
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
//        std::cout << "***else if (ask_index_ == (int)smv_.base_ask_index_) " << std::endl;
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;

        l1_size_changed_[t_security_id_] = true;
      } else {
//	std::cout << "ELSE****" << std::endl;
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;
      }
    } break;
    default:
      break;
  }
  //std::cout << "AFTER*****" << std::endl;
  //std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
//  smv_.ShowMarket();
  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    //std::cout << "L1_PX or L1_SZ Change: " << std::endl;
    if (t_buysell_ == kTradeTypeBuy) {
      smv_.market_update_info_.bestbid_int_price_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
      smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
      smv_.market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      if (!smv_.price_to_yield_map_.empty()) {
        smv_.hybrid_market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.hybrid_market_update_info_.bestbid_price_ =
            smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
        smv_.hybrid_market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
        smv_.hybrid_market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      }
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      if (!smv_.price_to_yield_map_.empty()) {
        smv_.hybrid_market_update_info_.bestask_int_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
        smv_.hybrid_market_update_info_.bestask_price_ =
            smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
        smv_.hybrid_market_update_info_.bestask_size_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
        smv_.hybrid_market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      }
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);
//      std::cout << "AFTER UpdateBestAskVariablesUsingOurOrders: " 
//		<< smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
//      smv_.ShowMarket();
    }

    smv_.UpdateL1Prices();
  }

//  std::cout << "AFTER smv_.UpdateL1Prices: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
//  smv_.ShowMarket();
  if (!smv_.is_ready_) {
//    std::cout << "!smv_.is_ready_" << std::endl;
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
//      std::cout << "smv_.is_ready_ = true" << std::endl;
      smv_.is_ready_ = true;
    }
  }
//  std::cout << "AFTER is ready: " << std::endl;
//  smv_.ShowMarket();
  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  // Skip notifying listeners if the message is intermediate
  if (!smv_.is_ready_ || t_is_intermediate_) {
    return;
  }

  if (smv_.pl_change_listeners_present_) {
//    std::cout << "smv_.pl_change_listeners_present_" << std::endl;
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        (int)smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(
          t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ - t_size_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ - 1,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_, t_is_intermediate_,
          (int)smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ > (int)t_size_ ? 'C' : 'N');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               (int)smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(
          t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ - t_size_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ - 1,
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_, t_is_intermediate_,
          (int)smv_.market_update_info_.asklevels_[ask_index_].limit_size_ > (int)t_size_ ? 'C' : 'N');
    }
  }

  //std::cout << smv_.ShowMarket() << std::endl;
//  smv_.ShowMarket();
  // Notify relevant listeners about the update
  if (l1_price_changed_[t_security_id_]) {
    smv_.NotifyL1PriceListeners();
    smv_.NotifyOnReadyListeners();

    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  }

  else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();
    smv_.NotifyOnReadyListeners();

    l1_size_changed_[t_security_id_] = false;
  }

  else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void IndexedBSEMarketViewManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const double t_price_, const int t_size_, const bool t_delete_order_,
                                               const bool t_is_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  if(smv_.GetIntPx(t_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_price_) < smv_.lower_int_price_limit_){
    return;
  }
/*
  std::cout << "ORDER_DELETE: B/S: " << t_buysell_ << " , Px: " << t_price_ << " , Size: " << t_size_ 
  	    << " , Delete_order: " << t_delete_order_ << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
*/
  if (t_size_ == 0) {
    return;
  }
/*
  std::cout << "bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;

  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
*/
  int int_price_ = smv_.GetIntPx(t_price_);
//  std::cout << "int_price: " << int_price_ << std::endl;

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_side_exec_summary_seen_[t_security_id_] = 0;

      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (bid_index_ < 0) {
        return;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Error case
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;
        }

        l1_size_changed_[t_security_id_] = true;

        if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ <= 0) {
          // Set both size and order count to 0
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;

          // L1 has changed
          int index_ = bid_index_ - 1;
          for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
            ;

          if (index_ >= 0) {
            smv_.base_bid_index_ = index_;

            // Check if we need to re-centre the index
            if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
            }
          } else {
            // There are 0 bid levels in the book right now. So, change the is_ready_ to false.
            smv_.is_ready_ = false;
          }

          l1_price_changed_[t_security_id_] = true;
        }
      } else {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;
        }

        if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;
        }

        // We need to mask the levels above this index
        else if (sec_id_to_mask_levels_above_[t_security_id_]) {
          for (int index_ = (int)smv_.base_bid_index_; index_ > bid_index_; index_--) {
            smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
          }
          smv_.base_bid_index_ = bid_index_;

          if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0) {
            // L1 has changed
            int index_ = bid_index_ - 1;
            for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
              ;

            if (index_ >= 0) {
              smv_.base_bid_index_ = index_;
            } else {
              smv_.is_ready_ = false;
            }
          }

          // Check if we need to re-centre the index
          if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
          }

          l1_price_changed_[t_security_id_] = true;
        }
      }
    } break;
    case kTradeTypeSell: {
      ask_side_exec_summary_seen_[t_security_id_] = 0;

      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0) {
        return;
      }

      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Error case
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;
        }

        l1_size_changed_[t_security_id_] = true;

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          // Set both size and order count to 0
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;

          // L1 has changed
          int index_ = ask_index_ - 1;
          for (; (index_ >= 0) && (smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0); index_--)
            ;

          if (index_ >= 0) {
	//      std::cout << "****BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
            smv_.base_ask_index_ = index_;
	//      std::cout << "****AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;

            // Check if we need to re-centre the index
            if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
            }
          } else {
            // There are 0 ask levels in the book right now. So, change the is_ready to false.
            smv_.is_ready_ = false;
          }

          l1_price_changed_[t_security_id_] = true;
        }
      } else {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;
        }

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;
        }

        // We need to mask the levels above this index
        else if (sec_id_to_mask_levels_above_[t_security_id_]) {
          for (int index_ = (int)smv_.base_ask_index_; index_ > ask_index_; index_--) {
            smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
          }
	  //    std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
          smv_.base_ask_index_ = ask_index_;
	  //    std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;

          if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0) {
            // L1 has changed
            int index_ = ask_index_ - 1;
            for (; index_ >= 0 && smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0; index_--)
              ;

            if (index_ >= 0) {
	  //    std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
              smv_.base_ask_index_ = index_;
	  //    std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
            } else {
              smv_.is_ready_ = false;
            }
          }

          // Check if we need to re-centre the index
          if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
          }

          l1_price_changed_[t_security_id_] = true;
        }
      }
    } break;
    default:
      break;
  }

  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    if (t_buysell_ == kTradeTypeBuy) {
      smv_.market_update_info_.bestbid_int_price_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
      smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
      smv_.market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      if (!smv_.price_to_yield_map_.empty()) {
        smv_.hybrid_market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.hybrid_market_update_info_.bestbid_price_ =
            smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
        smv_.hybrid_market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
        smv_.hybrid_market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      }
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      if (!smv_.price_to_yield_map_.empty()) {
        smv_.hybrid_market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.hybrid_market_update_info_.bestbid_price_ =
            smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
        smv_.hybrid_market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
        smv_.hybrid_market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      }
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    }

    smv_.UpdateL1Prices();
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  // Skip notifying listeners if either the message is intermediate or smv is not in ready state
  // Also, skip if the message order exec message. Because this will undo the effect of exec_summary messages.
  if (!smv_.is_ready_ || t_is_intermediate_ || skip_notifications_[t_security_id_]) {
    return;
  }

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        (int)smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(
          t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ + t_size_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ + 1,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_, t_is_intermediate_,
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ == 0 ? 'D' : 'C');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               (int)smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(
          t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ + t_size_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ + 1,
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_, t_is_intermediate_,
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ == 0 ? 'D' : 'C');
    }
  }

  //std::cout << smv_.ShowMarket() << std::endl;
//  smv_.ShowMarket();
  // Notify relevant listeners about the update
  if (l1_price_changed_[t_security_id_]) {
    smv_.NotifyL1PriceListeners();
    smv_.NotifyOnReadyListeners();

    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  }

  else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();
    smv_.NotifyOnReadyListeners();

    l1_size_changed_[t_security_id_] = false;
  }

  else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();
    smv_.NotifyOnReadyListeners();
  }
}

// Handling Order Modify message:
void IndexedBSEMarketViewManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const double t_price_, const uint32_t t_size_,
                                                 const double t_prev_price_, const uint32_t t_prev_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  if(smv_.GetIntPx(t_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_price_) < smv_.lower_int_price_limit_ ||
     smv_.GetIntPx(t_prev_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_prev_price_) < smv_.lower_int_price_limit_){

//    std::cout << "IndexedBSEMarketViewManager::OnOrderModify: Prices are OOB, prev_px px :: "
//	      << t_prev_price_ << " " << t_price_ << std::endl;
    return;
  }
/*
  std::cout << "ORDER_MODIFY: B/S: " << t_buysell_ << " , Px: " << t_price_ << " , Size: " << t_size_ 
	    << " , prev_px: " << t_prev_price_ << " , prev_sz: " << t_prev_size_ << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;

  std::cout << "bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;
  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
*/
  if (smv_.DblPxCompare(t_prev_price_, t_price_)) {
    // 1. If the prices are same, simply change the size
    OnOrderDelete(t_security_id_, t_buysell_, t_prev_price_, (int)t_prev_size_ - (int)t_size_, false, false);
  } else {
    // since prices are different
    // delete the previous order and add a new one while keeping intermediate flag in delete on so that we don't notify
    // listeners upon delete message
    OnOrderDelete(t_security_id_, t_buysell_, t_prev_price_, t_prev_size_, true, true);
//    std::cout << "After Deletion operation " << std::endl;    
    //smv_.ShowMarket();
    OnOrderAdd(t_security_id_, t_buysell_, t_price_, t_size_, false);
  }
  //std::cout << smv_.ShowMarket() << std::endl;
//  smv_.ShowMarket();
}

void IndexedBSEMarketViewManager::OnOrderMassDelete(const uint32_t t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
/*
  std::cout << "OnOrderMassDelete: bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;
  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
*/
  smv_.EmptyBook();
  //std::cout << smv_.ShowMarket() << std::endl;
//  smv_.ShowMarket();
}

void IndexedBSEMarketViewManager::OnPartialOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                           const double t_traded_price_,
                                                           const uint32_t t_traded_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  if(smv_.GetIntPx(t_traded_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_traded_price_) < smv_.lower_int_price_limit_){
    return;
  }
//  std::cout << "OnPartialOrderExecution: B/S: " << t_buysell_ << " , Trd_Px: " << t_traded_price_ << " , Trd_Size: " << t_traded_size_ << std::endl;
//  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (bid_side_exec_summary_seen_[t_security_id_] > 0) {
        // bid_side_exec_summary_seen_[ t_security_id_ ] -= t_traded_size_;
        // Reset to 0 on partial exec
        bid_side_exec_summary_seen_[t_security_id_] = 0;
//  smv_.ShowMarket();
        return;
      }
    } break;
    case kTradeTypeSell: {
      if (ask_side_exec_summary_seen_[t_security_id_] > 0) {
        // ask_side_exec_summary_seen_[ t_security_id_ ] -= t_traded_size_;
        // Reset to 0 on partial exec
        ask_side_exec_summary_seen_[t_security_id_] = 0;
//  smv_.ShowMarket();
        return;
      }
    } break;
    default:
      break;
  }

  // Specify that the OnOrderDelete should mask the levels above this level
  sec_id_to_mask_levels_above_[t_security_id_] = true;

  // Delete order while keeping the corresponding order count unchanged
  OnOrderDelete(t_security_id_, t_buysell_, t_traded_price_, t_traded_size_, false, false);

  sec_id_to_mask_levels_above_[t_security_id_] = false;
  //std::cout << smv_.ShowMarket() << std::endl;
//  smv_.ShowMarket();
}

void IndexedBSEMarketViewManager::OnFullOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                        const double t_traded_price_, const uint32_t t_traded_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  if(smv_.GetIntPx(t_traded_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_traded_price_) < smv_.lower_int_price_limit_){
    return;
  }
/*
  std::cout << "OnFullOrderExecution: B/S: " << t_buysell_ << " , Px: " << t_traded_price_ << " , Size: " << t_traded_size_ << std::endl;

  std::cout << "bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;
  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
*/
  if (!smv_.initial_book_constructed_) {
    return;
  }

/*
  int int_price_ = smv_.GetIntPx(t_traded_price_);
  switch (t_buysell_) {
    case kTradeTypeBuy: {
      std::cout << "BUY " << t_buysell_ << " bid_side_exec_summary_seen_: " << bid_side_exec_summary_seen_[t_security_id_] << std::endl;
      if (bid_side_exec_summary_seen_[t_security_id_] > 0) {
        bid_side_exec_summary_seen_[t_security_id_] -= t_traded_size_;

        if (int_price_ == smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_--;

          if (smv_.pl_change_listeners_present_) {
            smv_.NotifyOnPLChangeListeners(
                t_security_id_, smv_.market_update_info_, t_buysell_, 0, int_price_, 0,
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_,
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_,
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ + 1,
                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_, true, 'C');
          }

          if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ <= 0) {
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ = 0;
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ = 0;

            int next_bid_index_ = smv_.base_bid_index_ - 1;
            for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
                 next_bid_index_--)
              ;

            if (next_bid_index_ < 0) {
	      std::cout << "next_ask_index_ < 0\n" ;
  	      std::cout << smv_.ShowMarket() << std::endl;
              return;
            }

            smv_.base_bid_index_ = next_bid_index_;

            if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
            }

            l1_price_changed_[t_security_id_] = true;
          }
        }
	std::cout << "Before return: " << std::endl;
  	std::cout << smv_.ShowMarket() << std::endl;
        return;
      }
    } break;
    case kTradeTypeSell: {
      std::cout << "SELL " << t_buysell_ << " ask_side_exec_summary_seen_: " << ask_side_exec_summary_seen_[t_security_id_] << std::endl;
      if (ask_side_exec_summary_seen_[t_security_id_] > 0) {
        ask_side_exec_summary_seen_[t_security_id_] -= t_traded_size_;
        
        std::cout << "after ask_side_exec_summary_seen_: " << ask_side_exec_summary_seen_[t_security_id_] 
		  << "\nint_px, index_int_px :: " << int_price_ << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ << std::endl;

        if (int_price_ == smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {

          std::cout << "int_price_ == index_int_px" << std::endl;
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_--;

          if (smv_.pl_change_listeners_present_) {
            smv_.NotifyOnPLChangeListeners(
                t_security_id_, smv_.market_update_info_, t_buysell_, 0, int_price_, 0,
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_,
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_,
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ + 1,
                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_, true, 'C');
          }

          if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ <= 0) {
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ = 0;
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ = 0;

            int next_ask_index_ = smv_.base_ask_index_ - 1;
	    std::cout << "next_ask_index_: " << next_ask_index_ << std::endl;
            for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
                 next_ask_index_--)
              ;
	    std::cout << "AFTER FOR loop: next_ask_index_.limit_size_: " 
		      << smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ 
		      << " px: " << smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_ 
	   	      << " ordercount_: " << smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_ <<  std::endl;
	    std::cout << "next_ask_index_: " << next_ask_index_ << std::endl;
            if (next_ask_index_ < 0) {
	      std::cout << "next_ask_index_ < 0\n" ;
  	      std::cout << smv_.ShowMarket() << std::endl;
              return;
            }

	      std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
            smv_.base_ask_index_ = next_ask_index_;
	      std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
      	    std::cout << "smv_.base_ask_index_, LOW_ACCESS_INDEX :: "
		      << smv_.base_ask_index_ << " , " << LOW_ACCESS_INDEX << std::endl;

            if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
	      std::cout << "smv_.base_ask_index_ < LOW_ACCESS_INDEX :: " << smv_.base_ask_index_ << " " << LOW_ACCESS_INDEX 
			<< "\nRebuildIndexLowAccess"<< std::endl;
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
            }

            l1_price_changed_[t_security_id_] = true;
          }
        }
	std::cout << "Before return: " << std::endl;
  	std::cout << smv_.ShowMarket() << std::endl;
        return;
      }
    } break;
    default:
      break;
  }
*/
  // Specify that the OnOrderDelete should mask the levels above this level
  sec_id_to_mask_levels_above_[t_security_id_] = true;
  skip_notifications_[t_security_id_] = true;

  // Delete order and decrease the corresponding order count
  OnOrderDelete(t_security_id_, t_buysell_, t_traded_price_, t_traded_size_, true, false);

  skip_notifications_[t_security_id_] = false;
  sec_id_to_mask_levels_above_[t_security_id_] = false;
  //std::cout << smv_.ShowMarket() << std::endl;
  //smv_.ShowMarket();
}

void IndexedBSEMarketViewManager::OnExecutionSummary(const uint32_t t_security_id_, TradeType_t t_buysell_,
                                                      const double t_trade_price_, const uint32_t t_trade_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  if(smv_.GetIntPx(t_trade_price_) > smv_.upper_int_price_limit_ || smv_.GetIntPx(t_trade_price_) < smv_.lower_int_price_limit_){
    return;
  }
/*
  std::cout << "OnExecutionSummary: B/S: " << t_buysell_ << " , Px: " << t_trade_price_ << " , Size: " << t_trade_size_ << std::endl;

  smv_.ShowMarket();
  std::cout << "bid_base_index, ask_base_index:: " << smv_.base_bid_index_ << " " << smv_.base_ask_index_ << std::endl;
  std::cout << "***bid_base_int_px, ask_base_int_px :: " 
	    << smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_
	    << " " << smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_
	    << std::endl;
  std::cout << "CURR BEST_LEVEL: " << smv_.market_update_info_.bestbid_int_price_ << " X " << smv_.market_update_info_.bestask_int_price_ << std::endl;
*/
  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

//  std::cout << "t_trade_int_price_: " << t_trade_int_price_ << std::endl;
  // same as SecurityMarketView::OnTrade
  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_[t_security_id_]) {  // the difference between
                                                             // last_book_mkt_size_weighted_price_ and
                                                             // mkt_size_weighted_price_ is that
      // in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
      // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of mkt_size_weighted_price_
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ =
          smv_.market_update_info_
              .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
    }
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  smv_.StorePreTrade();

  // Masking code
  switch (t_buysell_) {
    case kTradeTypeBuy:  // Aggressive Buy - hence, try masking the ASK levels
    {

      //std::cout << "BUY Aggressive" << std::endl;
      ask_side_exec_summary_seen_[t_security_id_] = t_trade_size_;
      //std::cout << "ask_side_exec_summary_seen_: " << ask_side_exec_summary_seen_[t_security_id_] << std::endl;

      // Calculate level index for the trade price
      int trade_index_ =
          (int)smv_.base_ask_index_ +
          (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - t_trade_int_price_);
/*
      std::cout << "trade_index_ = ask_idx + (ask_idx_int_px - int_px :: "
		<< trade_index_ << " = " << (int)smv_.base_ask_index_ << " + ( "
		<< smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ 
		<< " - " << t_trade_int_price_ << " ) " << std::endl;
*/
      if (trade_index_ > (int)smv_.base_ask_index_ || trade_index_ < 0) {
        // Possible reasons:
        // 1. An aggressive buy that resulted in synthetic match
        // 2. Error case resulted from wrong book because of either wrong logic in book or missing packets
      }

      else if (trade_index_ == (int)smv_.base_ask_index_) {
 //       std::cout << "trade_index_ == (int)smv_.base_ask_index_ :: " << trade_index_  << " == " << (int)smv_.base_ask_index_ << std::endl;
        // If the trade size is big enough to clear this level
        if ((int)t_trade_size_ >= (int)smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_) {
          // This level is cleared now. So, find the next level
          int next_ask_index_ = smv_.base_ask_index_ - 1;
          for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
               next_ask_index_--)
            ;

          // If the next_ask_index_ is a valid one, update the best variables with that level
          if (next_ask_index_ >= 0) { //&& trade_index_ - next_ask_index_ <= 10) {
            smv_.market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
            smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
            smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_;
            smv_.market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
            if (!smv_.price_to_yield_map_.empty()) {
              smv_.hybrid_market_update_info_.bestask_int_price_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
              smv_.hybrid_market_update_info_.bestask_size_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
              smv_.hybrid_market_update_info_.bestask_price_ =
                  smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
              smv_.hybrid_market_update_info_.bestask_ordercount_ =
                  smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
            }
          } else {
            smv_.market_update_info_.bestask_int_price_ = t_trade_int_price_ + 1;
            smv_.market_update_info_.bestask_size_ = 1;
            smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(smv_.market_update_info_.bestask_int_price_);
            smv_.market_update_info_.bestask_ordercount_ = 1;
            if (!smv_.price_to_yield_map_.empty()) {
              smv_.hybrid_market_update_info_.bestask_int_price_ = t_trade_int_price_ + 1;
              smv_.hybrid_market_update_info_.bestask_size_ = 1;
              smv_.hybrid_market_update_info_.bestask_price_ = smv_.price_to_yield_map_[t_trade_int_price_ + 1];
              smv_.hybrid_market_update_info_.bestask_ordercount_ = 1;
            }
          }

          // Set base_ask_index_ size and order count to 0
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_ = 0;

	//      std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
          smv_.base_ask_index_ = next_ask_index_;
	//      std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
        }

        // Otherwise, update the best variables with the current level
        else {
          // Update the book here
          smv_.market_update_info_.asklevels_[trade_index_].limit_size_ -= (int)t_trade_size_;

          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[trade_index_].limit_int_price_;
          smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[trade_index_].limit_size_;
          smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[trade_index_].limit_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestask_size_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
            smv_.hybrid_market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_;
          }
        }

        UpdateBestAskVariablesUsingOurOrders(t_security_id_);

        smv_.UpdateL1Prices();

        if (smv_.pl_change_listeners_present_) {
          bool level_cleared_ = ((int)smv_.base_ask_index_ != trade_index_);
          smv_.NotifyOnPLChangeListeners(
              t_security_id_, smv_.market_update_info_, kTradeTypeSell, 0, t_trade_int_price_, 0,
              smv_.market_update_info_.asklevels_[trade_index_].limit_size_ + t_trade_size_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_,
              level_cleared_ ? 0 : smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_,
              smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
        }

        // set the primary variables
        smv_.trade_print_info_.trade_price_ = t_trade_price_;
        smv_.trade_print_info_.size_traded_ = t_trade_size_;
        smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
        smv_.trade_print_info_.buysell_ = t_buysell_;

        smv_.SetTradeVarsForIndicatorsIfRequired();
      }

      else {
        if (smv_.market_update_info_.asklevels_[trade_index_].limit_size_ <= 0) {
          for (; trade_index_ >= 0 && smv_.market_update_info_.asklevels_[trade_index_].limit_size_ <= 0;
               trade_index_--)
            ;

          if (trade_index_ < 0) {
            return;
          }
        }

        int size_adjustment_ = 0;

        for (int index_ = smv_.base_ask_index_; index_ > trade_index_; index_--) {
          if (smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
            size_adjustment_ += smv_.market_update_info_.asklevels_[index_].limit_size_;
          }
        }

        int size_deduction_ = (int)t_trade_size_ - size_adjustment_;
        if(size_deduction_ < 0) size_deduction_ = (int)t_trade_size_;
        int new_base_ask_index_ = 0;
/*
	std::cout << "size_adjustment_, trd_sz, siz_dec: " << size_adjustment_ << " , " << (int)t_trade_size_
		  << " , " << size_deduction_ 
		  << "\nTrd_idx: " << trade_index_ << " px: " << smv_.market_update_info_.asklevels_[trade_index_].limit_price_
                    << " sz: " << smv_.market_update_info_.asklevels_[trade_index_].limit_size_ << std::endl;
*/
//  smv_.ShowMarket();

        if (size_deduction_ >= smv_.market_update_info_.asklevels_[trade_index_].limit_size_) {
//empty ask level
//	  std::cout << "Empty at idx: " << trade_index_ << " px: " << smv_.market_update_info_.asklevels_[trade_index_].limit_price_
//		    << " sz: " << smv_.market_update_info_.asklevels_[trade_index_].limit_size_ << std::endl;
	  smv_.market_update_info_.asklevels_[trade_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_ = 0;
//
          int next_ask_index_ = trade_index_ - 1;
          for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
               next_ask_index_--)
            ;

          if (next_ask_index_ < 0) {
            return;
          }
/*
	  std::cout << "NexT_ask_idx: " << next_ask_index_ << " px: " 
		    << smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_ << " sz: "
		    << smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_
		    << std::endl;
*/		
          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
          smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[next_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[next_ask_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestask_size_ =
                smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
            smv_.hybrid_market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_;
          }
          new_base_ask_index_ = next_ask_index_;
        } else {

//	  std::cout << "ELSE PART SIZE Deducted : " << size_deduction_ << " idx: " << trade_index_ << " px: " << smv_.market_update_info_.asklevels_[trade_index_].limit_price_
//		    << " sz: " << smv_.market_update_info_.asklevels_[trade_index_].limit_size_ << std::endl;
          smv_.market_update_info_.asklevels_[trade_index_].limit_size_ -= size_deduction_;
//	  std::cout << "ELSE PART UPDATE idx: " << trade_index_ << " px: " << smv_.market_update_info_.asklevels_[trade_index_].limit_price_
//		    << " sz: " << smv_.market_update_info_.asklevels_[trade_index_].limit_size_ << std::endl;

          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[trade_index_].limit_int_price_;
          smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[trade_index_].limit_size_;
          smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[trade_index_].limit_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestask_int_price_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestask_size_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestask_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestask_int_price_];
            smv_.hybrid_market_update_info_.bestask_ordercount_ =
                smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_;
          }
          new_base_ask_index_ = trade_index_;
        }
        UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();

//  smv_.ShowMarket();
        for (int index_ = smv_.base_ask_index_; index_ > trade_index_; index_--) {
          if (smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
            // set the primary variables
            smv_.trade_print_info_.trade_price_ = smv_.market_update_info_.asklevels_[index_].limit_price_;
            smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.asklevels_[index_].limit_size_;
            smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.asklevels_[index_].limit_int_price_;
            smv_.trade_print_info_.buysell_ = t_buysell_;

            smv_.SetTradeVarsForIndicatorsIfRequired();

            if (smv_.is_ready_) {
              smv_.NotifyTradeListeners();
            }

            if (smv_.pl_change_listeners_present_) {
              smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, kTradeTypeSell, 0,
                                             smv_.market_update_info_.asklevels_[index_].limit_price_, 0,
                                             smv_.market_update_info_.asklevels_[index_].limit_size_, 0,
                                             smv_.market_update_info_.asklevels_[index_].limit_ordercount_, 0, true,
                                             'D');
            }
/*
	    std::cout << "deleting Ask px: " << smv_.market_update_info_.asklevels_[index_].limit_price_ 
		      << " int_px: " << smv_.market_update_info_.asklevels_[index_].limit_int_price_
		      << " sz: " << smv_.market_update_info_.asklevels_[index_].limit_size_
		      << " ord_cnt: " << smv_.market_update_info_.asklevels_[index_].limit_ordercount_
		      << std::endl;
*/
	    smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
	    smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
          }
        }

//	      std::cout << "BEFORE Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
        smv_.base_ask_index_ = new_base_ask_index_;
//	      std::cout << "AFTER Change smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;

//  smv_.ShowMarket();
        // set the primary variables
        smv_.trade_print_info_.trade_price_ = t_trade_price_;
        smv_.trade_print_info_.size_traded_ = std::max(1, size_deduction_);
        smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
        smv_.trade_print_info_.buysell_ = t_buysell_;

        if (smv_.pl_change_listeners_present_) {
          bool level_cleared_ = ((int)smv_.base_ask_index_ != trade_index_);
          smv_.NotifyOnPLChangeListeners(
              t_security_id_, smv_.market_update_info_, kTradeTypeSell, 0, t_trade_int_price_, 0,
              smv_.market_update_info_.asklevels_[trade_index_].limit_size_ + size_deduction_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_,
              level_cleared_ ? 0 : smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_,
              smv_.market_update_info_.asklevels_[trade_index_].limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
        }

        smv_.SetTradeVarsForIndicatorsIfRequired();
      }

      if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
        RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                              smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
      }
    } break;
    case kTradeTypeSell:  // Aggressive Sell - hence, try masking the BID levels
    {
      //std::cout << "SELL" << std::endl;
      bid_side_exec_summary_seen_[t_security_id_] = t_trade_size_;

      // Calculate level index for the trade price
      int trade_index_ =
          (int)smv_.base_bid_index_ -
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - t_trade_int_price_);
/*
      std::cout << "trade_index_ = base_bid_index_ - bidlevels_.int_px - trd_int_px :: "
		<< trade_index_ << " = " << (int)smv_.base_bid_index_ << " - (" 
		<< smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ 
		<< " - " << t_trade_int_price_ << ")" << std::endl;
*/
      if (trade_index_ > (int)smv_.base_bid_index_ || trade_index_ < 0) {
        // Possible reasons:
        // 1. An aggressive sell that resulted in synthetic match
        // 2. Error case resulted from wrong book because of either wrong logic in book or missing packets
      }

      else if (trade_index_ == (int)smv_.base_bid_index_) {
//        std::cout << "else if (trade_index_ == (int)smv_.base_bid_index_) :: "
//		  << trade_index_ << " == " << (int)smv_.base_bid_index_ << std::endl;
        // If the trade size is big enough to clear this level
        if ((int)t_trade_size_ >= (int)smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
/*	
          std::cout << "t_trade_size_ >= bidlevels_.limit_size_ :: "
		    << (int)t_trade_size_ << " >= " << (int)smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ 
		    << std::endl;
*/
          // This level is cleared now. So, find the next level
          int next_bid_index_ = smv_.base_bid_index_ - 1;
/*
          std::cout << "next_bid_index_: " << next_bid_index_ 
		    << " px: " << smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_
		    << " sz: " << smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ << std::endl;
*/
          for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
               next_bid_index_--)
            ;
/*
          std::cout << "AFTER FOR LOOP\nnext_bid_index_: " << next_bid_index_ 
		    << " px: " << smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_
		    << " sz: " << smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ << std::endl;
*/
          // If the next_ask_index_ is a valid one, update the best variables with that level
          if (next_bid_index_ >= 0) { // && trade_index_ - next_bid_index_ <= 10) {
//	    std::cout << "next_bid_index_ >= 0 && trade_index_ - next_bid_index_ <= 10 :: "
//		      << trade_index_ << " - " << next_bid_index_ << " : " << trade_index_ - next_bid_index_ << std::endl;

            smv_.market_update_info_.bestbid_int_price_ =
                smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
            smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
            smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_;
            smv_.market_update_info_.bestbid_ordercount_ =
                smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
            if (!smv_.price_to_yield_map_.empty()) {
              smv_.hybrid_market_update_info_.bestbid_int_price_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
              smv_.hybrid_market_update_info_.bestbid_size_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
              smv_.hybrid_market_update_info_.bestbid_price_ =
                  smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
              smv_.hybrid_market_update_info_.bestbid_ordercount_ =
                  smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
            }
          } else {
//	    std::cout << "ELSE" << std::endl;
            smv_.market_update_info_.bestbid_int_price_ = t_trade_int_price_ - 1;
            smv_.market_update_info_.bestbid_size_ = 1;
            smv_.market_update_info_.bestbid_price_ = smv_.GetDoublePx(smv_.market_update_info_.bestbid_int_price_);
            smv_.market_update_info_.bestbid_ordercount_ = 1;
            if (!smv_.price_to_yield_map_.empty()) {
              smv_.hybrid_market_update_info_.bestbid_int_price_ = t_trade_int_price_ - 1;
              smv_.hybrid_market_update_info_.bestbid_size_ = 1;
              smv_.hybrid_market_update_info_.bestbid_price_ = smv_.price_to_yield_map_[t_trade_int_price_ - 1];
              smv_.hybrid_market_update_info_.bestbid_ordercount_ = 1;
            }
          }

          // Set base_bid_index_ size and order count to 0
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_ = 0;

          smv_.base_bid_index_ = next_bid_index_;
        }

        // Otherwise, update the best variables with the current level
        else {
	    //std::cout << "ELSE BODY" << std::endl;
          // Update the book here
          smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ -= (int)t_trade_size_;

          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[trade_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[trade_index_].limit_size_;
          smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[trade_index_].limit_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestbid_size_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestbid_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
            smv_.hybrid_market_update_info_.bestbid_ordercount_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_;
          }
        }
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();

        if (smv_.pl_change_listeners_present_) {
          bool level_cleared_ = ((int)smv_.base_bid_index_ != trade_index_);
          smv_.NotifyOnPLChangeListeners(
              t_security_id_, smv_.market_update_info_, kTradeTypeBuy, 0, t_trade_int_price_, 0,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ + t_trade_size_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_,
              level_cleared_ ? 0 : smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
        }

        // set the primary variables
        smv_.trade_print_info_.trade_price_ = t_trade_price_;
        smv_.trade_print_info_.size_traded_ = t_trade_size_;
        smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
        smv_.trade_print_info_.buysell_ = t_buysell_;

        smv_.SetTradeVarsForIndicatorsIfRequired();
      }

      else {
        if (smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ <= 0) {
          for (; trade_index_ >= 0 && smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ <= 0;
               trade_index_--)
            ;

          if (trade_index_ < 0) {
            return;
          }
        }

        int size_adjustment_ = 0;

        for (int index_ = smv_.base_bid_index_; index_ > trade_index_; index_--) {
          if (smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
            size_adjustment_ += smv_.market_update_info_.bidlevels_[index_].limit_size_;
          }
        }

        int size_deduction_ = (int)t_trade_size_ - size_adjustment_;
	if(size_deduction_ < 0) size_deduction_ = (int)t_trade_size_;
        int new_base_bid_index_ = 0;

        if (size_deduction_ >= smv_.market_update_info_.bidlevels_[trade_index_].limit_size_) {
//empty trade idx
	  smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_ = 0;

//

          int next_bid_index_ = trade_index_ - 1;
          for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
               next_bid_index_--)
            ;

          if (next_bid_index_ < 0) {
            return;
          }

          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
          smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[next_bid_index_].limit_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ =
                smv_.market_update_info_.bidlevels_[next_bid_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestbid_size_ =
                smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestbid_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
            smv_.hybrid_market_update_info_.bestbid_ordercount_ =
                smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_;
          }
          new_base_bid_index_ = next_bid_index_;
        } else {
          smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ -= size_deduction_;

          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[trade_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[trade_index_].limit_size_;
          smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[trade_index_].limit_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_;
          if (!smv_.price_to_yield_map_.empty()) {
            smv_.hybrid_market_update_info_.bestbid_int_price_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_int_price_;
            smv_.hybrid_market_update_info_.bestbid_size_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_size_;
            smv_.hybrid_market_update_info_.bestbid_price_ =
                smv_.price_to_yield_map_[smv_.hybrid_market_update_info_.bestbid_int_price_];
            smv_.hybrid_market_update_info_.bestbid_ordercount_ =
                smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_;
          }

          new_base_bid_index_ = trade_index_;
        }

        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        smv_.UpdateL1Prices();

        for (int index_ = smv_.base_bid_index_; index_ > trade_index_; index_--) {
          if (smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
            // set the primary variables
            smv_.trade_print_info_.trade_price_ = smv_.market_update_info_.bidlevels_[index_].limit_price_;
            smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.bidlevels_[index_].limit_size_;
            smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.bidlevels_[index_].limit_int_price_;
            smv_.trade_print_info_.buysell_ = t_buysell_;

            smv_.SetTradeVarsForIndicatorsIfRequired();

            if (smv_.is_ready_) {
              smv_.NotifyTradeListeners();
            }

            if (smv_.pl_change_listeners_present_) {
              smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, kTradeTypeBuy, 0,
                                             smv_.market_update_info_.bidlevels_[index_].limit_price_, 0,
                                             smv_.market_update_info_.bidlevels_[index_].limit_size_, 0,
                                             smv_.market_update_info_.bidlevels_[index_].limit_ordercount_, 0, true,
                                             'D');
            }
/*
	    std::cout << "deleting Bid px: " << smv_.market_update_info_.bidlevels_[index_].limit_price_ 
		      << " int_px: " << smv_.market_update_info_.bidlevels_[index_].limit_int_price_
		      << " sz: " << smv_.market_update_info_.bidlevels_[index_].limit_size_
		      << " ord_cnt: " << smv_.market_update_info_.bidlevels_[index_].limit_ordercount_
		      << std::endl;
*/
	    smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
	    smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
          }
        }

        smv_.base_bid_index_ = new_base_bid_index_;

        // set the primary variables
        smv_.trade_print_info_.trade_price_ = t_trade_price_;
        smv_.trade_print_info_.size_traded_ = std::max(1, size_deduction_);
        smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
        smv_.trade_print_info_.buysell_ = t_buysell_;

        if (smv_.pl_change_listeners_present_) {
          bool level_cleared_ = ((int)smv_.base_bid_index_ != trade_index_);
          smv_.NotifyOnPLChangeListeners(
              t_security_id_, smv_.market_update_info_, kTradeTypeBuy, 0, t_trade_int_price_, 0,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_ + size_deduction_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_size_,
              level_cleared_ ? 0 : smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_,
              smv_.market_update_info_.bidlevels_[trade_index_].limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
        }

        smv_.SetTradeVarsForIndicatorsIfRequired();
      }

      if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
        RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                              smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
      }
    } break;
    default:
      break;
  }

  //std::cout << smv_.ShowMarket() << std::endl;
  // Same as SecurityMarketView::OnTrade
  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
//  smv_.ShowMarket();
}

void IndexedBSEMarketViewManager::OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                       const int t_int_price_) {}

void IndexedBSEMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}
}
