/**
   \file MarketAdapter/base_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

#define ORS_TRACKER_ARRAY_SIZE 8192

#define MAX_CONF_EXEC_USECS_TO_CONSIDER 50

namespace HFSAT {

BaseMarketViewManager::BaseMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                             const SecurityNameIndexer& t_sec_name_indexer_,
                                             const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : dbglogger_(t_dbglogger_),
      watch_(t_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      security_market_view_map_(t_security_market_view_map_) {
  HFSAT::SMVUtils::SMVUtils smv_utils_ = HFSAT::SMVUtils::SMVUtils::GetUniqueInstance();
}

void BaseMarketViewManager::ShowMarket(int t_security_id_) {
  SecurityMarketView& this_smv_ = *(security_market_view_map_[t_security_id_]);
  dbglogger_ << this_smv_.ShowMarket();
  dbglogger_.CheckToFlushBuffer();
}

/**
 * Rebuild/re-centre index when base_index_ moves past the upper limit
 * Shift all levels such that base_bid_index/base_ask_index (pointing to new_int_price_) is restored to
 * INITIAL_BASE_INDEX
 * Assign correct limit_int_price_ for the rest of the levels
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param new_int_price_
 */
//This shifts the details of bid/ask level from upper index to lower index as the current price update that is received is way up the index
//which means the prices are coming at much higher side. 
//For Eg. curr base bid index has px as 100 but we are getting prices of let's say 150 then index for 150 is much higher. So we need to rebuild the Index and shift upper index details to lower
//Same thing for ask as well. 
//Eg. curr base bid index has px as 100 but we are getting prices of let's say 50 then index for 50 is much higher. So we need to rebuild the Index 
//and shift upper index details to lower
void BaseMarketViewManager::RebuildIndexHighAccess(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                   int new_int_price_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      const int offset_ =
          new_int_price_ - smv_.market_update_info_.bidlevels_[smv_.initial_tick_size_].limit_int_price_;
      if (offset_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " offset should be +ve valued but is " << offset_
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset_ << std::endl;
        std::cerr << " exiting now" << std::endl;
        exit(1);
        return;
      }

      int index_ = 0;
      for (; index_ + offset_ < (int)smv_.market_update_info_.bidlevels_.size(); index_++) {
        smv_.market_update_info_.bidlevels_[index_].limit_int_price_ =
            smv_.market_update_info_.bidlevels_[index_ + offset_].limit_int_price_;
        smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ =
            smv_.market_update_info_.bidlevels_[index_ + offset_].limit_ordercount_;
        smv_.market_update_info_.bidlevels_[index_].limit_size_ =
            smv_.market_update_info_.bidlevels_[index_ + offset_].limit_size_;
        smv_.market_update_info_.bidlevels_[index_].limit_price_ =
            smv_.market_update_info_.bidlevels_[index_ + offset_].limit_price_;
      }

      smv_.base_bid_index_ = smv_.initial_tick_size_;

      index_ = (int)smv_.market_update_info_.bidlevels_.size() - offset_;
      if (index_ < 0) {
        index_ = 0;
      }

      for (; index_ < (int)smv_.market_update_info_.bidlevels_.size(); index_++) {
        smv_.market_update_info_.bidlevels_[index_].limit_int_price_ = new_int_price_ - (smv_.base_bid_index_ - index_);
        smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
        smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
        smv_.market_update_info_.bidlevels_[index_].limit_price_ =
            smv_.GetDoublePx(smv_.market_update_info_.bidlevels_[index_].limit_int_price_);
      }
    } break;
    case kTradeTypeSell: {
      const int offset_ =
          smv_.market_update_info_.asklevels_[smv_.initial_tick_size_].limit_int_price_ - new_int_price_;
      if (offset_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " offset should be +ve valued but is " << offset_
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset_ << std::endl;
        std::cerr << " exiting now" << std::endl;
        exit(1);
        return;
      }

      int index_ = 0;
      for (; index_ + offset_ < (int)smv_.market_update_info_.asklevels_.size(); index_++) {
        smv_.market_update_info_.asklevels_[index_].limit_int_price_ =
            smv_.market_update_info_.asklevels_[index_ + offset_].limit_int_price_;
        smv_.market_update_info_.asklevels_[index_].limit_ordercount_ =
            smv_.market_update_info_.asklevels_[index_ + offset_].limit_ordercount_;
        smv_.market_update_info_.asklevels_[index_].limit_size_ =
            smv_.market_update_info_.asklevels_[index_ + offset_].limit_size_;
        smv_.market_update_info_.asklevels_[index_].limit_price_ =
            smv_.market_update_info_.asklevels_[index_ + offset_].limit_price_;
      }

      smv_.base_ask_index_ = smv_.initial_tick_size_;

      index_ = (int)smv_.market_update_info_.asklevels_.size() - offset_;
      if (index_ < 0) {
        index_ = 0;
      }

      for (; index_ < (int)smv_.market_update_info_.asklevels_.size(); index_++) {
        smv_.market_update_info_.asklevels_[index_].limit_int_price_ = new_int_price_ + (smv_.base_ask_index_ - index_);
        smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
        smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
        smv_.market_update_info_.asklevels_[index_].limit_price_ =
            smv_.GetDoublePx(smv_.market_update_info_.asklevels_[index_].limit_int_price_);
      }
    } break;
    default:
      break;
  }
}

/**
 * Rebuild/re-centre index when base_index_ moves below the lower limit
 * Shift all levels such that base_bid_index/base_ask_index (pointing to new_int_price_) is restored to
 * INITIAL_BASE_INDEX
 * Assign correct limit_int_price_ for the rest of the levels
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param new_int_price_
 */

//This shifts the details of bid/ask level from lower index to upper index as the current price update that is received is way down the index
//which means the prices are coming at much lower side. 
//For Eg. curr base bid index has px as 100 but we are getting prices of let's say 50 then index for 50 is much lower. So we need to rebuild the Index and shift lower index details to upper
//Same thing for ask as well. 
//Eg. curr base bid index has px as 100 but we are getting prices of let's say 150 then index for 150 is much lower. So we need to rebuild the Index 
//and shift lower index details to upper
void BaseMarketViewManager::RebuildIndexLowAccess(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                  int new_int_price_) {
  //std::cout << "BaseMarketViewManager::RebuildIndexLowAccess:: px: " << new_int_price_ << std::endl;
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      //std::cout << "BUY" << std::endl;
      smv_.bid_access_bitmask_ = BIT_RESET_ALL;

      int offset_ = smv_.market_update_info_.bidlevels_[smv_.initial_tick_size_].limit_int_price_ - new_int_price_;
      if (offset_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " offset should be +ve valued but is " << offset_
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset_ << std::endl;
        std::cerr << " exiting now" << std::endl;
        exit(1);
        return;
      }

      for (int index_ = smv_.market_update_info_.bidlevels_.size() - 1; index_ >= offset_; index_--) {
        smv_.market_update_info_.bidlevels_[index_].limit_int_price_ =
            smv_.market_update_info_.bidlevels_[index_ - offset_].limit_int_price_;
        smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ =
            smv_.market_update_info_.bidlevels_[index_ - offset_].limit_ordercount_;
        smv_.market_update_info_.bidlevels_[index_].limit_size_ =
            smv_.market_update_info_.bidlevels_[index_ - offset_].limit_size_;
        smv_.market_update_info_.bidlevels_[index_].limit_price_ =
            smv_.market_update_info_.bidlevels_[index_ - offset_].limit_price_;
      }

      smv_.base_bid_index_ = smv_.initial_tick_size_;

      // Offset can be quit huge, restrict size/price/ordercount resetting to the size of the array
      offset_ = std::min(offset_, (int)smv_.market_update_info_.bidlevels_.size());

      for (int index_ = 0; index_ < offset_; index_++) {
        smv_.market_update_info_.bidlevels_[index_].limit_int_price_ = new_int_price_ - (smv_.base_bid_index_ - index_);
        smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
        smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
        smv_.market_update_info_.bidlevels_[index_].limit_price_ =
            smv_.GetDoublePx(smv_.market_update_info_.bidlevels_[index_].limit_int_price_);
      }
    } break;
    case kTradeTypeSell: {
      //std::cout << "SELL" << std::endl;
      smv_.ask_access_bitmask_ = BIT_RESET_ALL;

      int offset_ = new_int_price_ - smv_.market_update_info_.asklevels_[smv_.initial_tick_size_].limit_int_price_;
      //std::cout << "offset_ = new_int_price_ - ask_lvl[initial_tick].limit_px :: "
	//	<< offset_ << " = " << new_int_price_ << " - " << smv_.market_update_info_.asklevels_[smv_.initial_tick_size_].limit_int_price_ << std::endl;
      if (offset_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " offset should be +ve valued but is " << offset_
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset_ << std::endl;
        std::cerr << " exiting now" << std::endl;
        exit(1);
        return;
      }


      //std::cout << "FOR LOOP: index= " << smv_.market_update_info_.asklevels_.size() - 1
	//	<< " till : " << offset_ << std::endl;
      for (int index_ = smv_.market_update_info_.asklevels_.size() - 1; index_ >= offset_; index_--) {
        
/*
        std::cout << "for index: " << index_ << std::endl;
	
        std::cout << "update idx " << index_ << " with index: " << index_ - offset_ << " = " << index_ << " - " << offset_ 
		  << "\npx, lmit_px sz ord_cnt :: " << smv_.market_update_info_.asklevels_[index_ - offset_].limit_price_ 
		  << " " << smv_.market_update_info_.asklevels_[index_ - offset_].limit_int_price_ 
		  << " " << smv_.market_update_info_.asklevels_[index_ - offset_].limit_size_ 
		  << " " << smv_.market_update_info_.asklevels_[index_ - offset_].limit_ordercount_ << std::endl;
*/
        smv_.market_update_info_.asklevels_[index_].limit_int_price_ =
            smv_.market_update_info_.asklevels_[index_ - offset_].limit_int_price_;
        smv_.market_update_info_.asklevels_[index_].limit_ordercount_ =
            smv_.market_update_info_.asklevels_[index_ - offset_].limit_ordercount_;
        smv_.market_update_info_.asklevels_[index_].limit_size_ =
            smv_.market_update_info_.asklevels_[index_ - offset_].limit_size_;
        smv_.market_update_info_.asklevels_[index_].limit_price_ =
            smv_.market_update_info_.asklevels_[index_ - offset_].limit_price_;
      }

      //std::cout << "AFTER FOR LOOP: smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;
      smv_.base_ask_index_ = smv_.initial_tick_size_;
      //std::cout << "UPDATED       : smv_.base_ask_index_: " << smv_.base_ask_index_ << std::endl;

      // Offset can be quit huge, restrict size/price/ordercount resetting to the size of the array
      offset_ = std::min(offset_, (int)smv_.market_update_info_.asklevels_.size());
      
      //std::cout << "OFFSET: " << offset_ << std::endl;
      for (int index_ = 0; index_ < offset_; index_++) {
        smv_.market_update_info_.asklevels_[index_].limit_int_price_ = new_int_price_ + (smv_.base_ask_index_ - index_);
        smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
        smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
        smv_.market_update_info_.asklevels_[index_].limit_price_ =
            smv_.GetDoublePx(smv_.market_update_info_.asklevels_[index_].limit_int_price_);
      }
    } break;
    default:
      break;
  }
}

/**
 * Rescale the vectors in the book
 * @param security_id
 * @param buysell
 * @param int_price
 */
void BaseMarketViewManager::ReScaleBook(const uint32_t security_id, const TradeType_t buysell, int int_price) {
  // Called when there's  order at very low price level in the book
  SecurityMarketView& smv = *(security_market_view_map_[security_id]);
  DBGLOG_TIME_CLASS_FUNC << " RebuildBook for " << smv.shortcode() << " " << smv.secname() << " " << security_id
                         << DBGLOG_ENDL_FLUSH;

  int new_tick_range = smv.initial_tick_size_;
  int offset = smv.market_update_info_.bidlevels_[smv.initial_tick_size_].limit_int_price_ - int_price;
  bool is_ask_there = false, is_bid_there = false;
  switch (buysell) {
    case kTradeTypeBuy: {
      offset = smv.market_update_info_.bidlevels_[smv.initial_tick_size_].limit_int_price_ - int_price;
      is_bid_there = true;
      if (smv.market_update_info_.asklevels_[smv.base_ask_index_].limit_size_ > 0) {
        is_ask_there = true;
      }
      // Since resize, adds to existing vector, we can do this
    } break;
    case kTradeTypeSell: {
      offset = smv.market_update_info_.asklevels_[smv.initial_tick_size_].limit_int_price_ - int_price;
      is_ask_there = true;
      if (smv.market_update_info_.bidlevels_[smv.base_bid_index_].limit_size_ > 0) {
        is_bid_there = true;
      }
    } break;
    default: { break; }
  }

  int scale_factor = ((std::abs(offset) / smv.initial_tick_size_) + 3);  // 1 to cover offset, 1 extra margin
  smv.initial_tick_size_ *= scale_factor;
  smv.initial_tick_size_ = std::min(smv.initial_tick_size_, smv.max_allowed_tick_range_);
  new_tick_range = 2 * smv.initial_tick_size_ + 1;
  smv.max_tick_range_ = new_tick_range;
  smv.market_update_info_.bidlevels_.resize(
      new_tick_range, MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));
  if (is_bid_there) {
    // Copy the existing data
    for (int i = smv.base_bid_index_; i >= 0; i--) {
      smv.market_update_info_.bidlevels_[smv.initial_tick_size_ - smv.base_bid_index_ + i].limit_int_price_ =
          smv.market_update_info_.bidlevels_[i].limit_int_price_;
      smv.market_update_info_.bidlevels_[smv.initial_tick_size_ - smv.base_bid_index_ + i].limit_price_ =
          smv.market_update_info_.bidlevels_[i].limit_price_;
      smv.market_update_info_.bidlevels_[smv.initial_tick_size_ - smv.base_bid_index_ + i].limit_ordercount_ =
          smv.market_update_info_.bidlevels_[i].limit_ordercount_;
      smv.market_update_info_.bidlevels_[smv.initial_tick_size_ - smv.base_bid_index_ + i].limit_size_ =
          smv.market_update_info_.bidlevels_[i].limit_size_;
    }

    // fill existing vector on same side
    int base_int_price = smv.market_update_info_.bidlevels_[smv.initial_tick_size_].limit_int_price_;
    for (int i = std::max(0, (int)smv.initial_tick_size_ - (int)smv.base_bid_index_ - 1); i >= 0; i--) {
      smv.market_update_info_.bidlevels_[i].limit_int_price_ = base_int_price - (smv.initial_tick_size_ - i);
      smv.market_update_info_.bidlevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_size_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.bidlevels_[i].limit_int_price_);
    }
    // fill other side of base
    for (int i = smv.initial_tick_size_ + 1; i < (int)smv.market_update_info_.bidlevels_.size(); i++) {
      smv.market_update_info_.bidlevels_[i].limit_int_price_ = base_int_price - (smv.initial_tick_size_ - i);
      smv.market_update_info_.bidlevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_size_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.bidlevels_[i].limit_int_price_);
    }

  } else {
    int base_int_price = smv.market_update_info_.bidlevels_[smv.initial_tick_size_].limit_int_price_ - 1;
    for (int i = 0; i < (int)smv.market_update_info_.bidlevels_.size(); i++) {
      smv.market_update_info_.bidlevels_[i].limit_int_price_ = base_int_price - (smv.initial_tick_size_ - i);
      smv.market_update_info_.bidlevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_size_ = 0;
      smv.market_update_info_.bidlevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.bidlevels_[i].limit_int_price_);
    }
  }
  smv.base_bid_index_ = smv.initial_tick_size_;

  smv.market_update_info_.asklevels_.resize(
      new_tick_range, MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));
  if (is_ask_there) {
    for (int i = smv.base_ask_index_; i >= 0; i--) {
      smv.market_update_info_.asklevels_[smv.initial_tick_size_ - smv.base_ask_index_ + i].limit_int_price_ =
          smv.market_update_info_.asklevels_[i].limit_int_price_;
      smv.market_update_info_.asklevels_[smv.initial_tick_size_ - smv.base_ask_index_ + i].limit_price_ =
          smv.market_update_info_.asklevels_[i].limit_price_;
      smv.market_update_info_.asklevels_[smv.initial_tick_size_ - smv.base_ask_index_ + i].limit_ordercount_ =
          smv.market_update_info_.asklevels_[i].limit_ordercount_;
      smv.market_update_info_.asklevels_[smv.initial_tick_size_ - smv.base_ask_index_ + i].limit_size_ =
          smv.market_update_info_.asklevels_[i].limit_size_;
    }

    int base_int_price = smv.market_update_info_.asklevels_[smv.initial_tick_size_].limit_int_price_;
    for (int i = std::max(0, (int)smv.initial_tick_size_ - (int)smv.base_ask_index_ - 1); i >= 0; i--) {
      smv.market_update_info_.asklevels_[i].limit_int_price_ = base_int_price + (smv.initial_tick_size_ - i);
      smv.market_update_info_.asklevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.asklevels_[i].limit_size_ = 0;
      smv.market_update_info_.asklevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.asklevels_[i].limit_int_price_);
    }

    for (int i = smv.initial_tick_size_ + 1; i < (int)smv.market_update_info_.asklevels_.size(); i++) {
      smv.market_update_info_.asklevels_[i].limit_int_price_ = base_int_price + (smv.initial_tick_size_ - i);
      smv.market_update_info_.asklevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.asklevels_[i].limit_size_ = 0;
      smv.market_update_info_.asklevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.asklevels_[i].limit_int_price_);
    }
  } else {
    int base_int_price = smv.market_update_info_.bidlevels_[smv.initial_tick_size_].limit_int_price_ + 1;
    for (int i = 0; i < (int)smv.market_update_info_.asklevels_.size(); i++) {
      smv.market_update_info_.asklevels_[i].limit_int_price_ = base_int_price + (smv.initial_tick_size_ - i);
      smv.market_update_info_.asklevels_[i].limit_ordercount_ = 0;
      smv.market_update_info_.asklevels_[i].limit_size_ = 0;
      smv.market_update_info_.asklevels_[i].limit_price_ =
          smv.GetDoublePx(smv.market_update_info_.asklevels_[i].limit_int_price_);
    }
  }
  smv.base_ask_index_ = smv.initial_tick_size_;

  smv.int_price_bid_book_.resize(new_tick_range, 0);
  smv.int_price_ask_book_.resize(new_tick_range, 0);

  smv.int_price_bid_skip_delete_.resize(new_tick_range, false);
  smv.int_price_ask_skip_delete_.resize(new_tick_range, false);
}

/**
 * Build index based on the int_price_: assign limit_int_price_, limit_ordercount_ and limit_size_ for all levels
 * @param t_security_id_
 * @param t_buysell_
 * @param int_price_
 */

//This builds Index for both bid and ask level and assign values w.r.t. what price it has received initially.
//This is just the initial one time call just to fill details in each level.

//Note: Bid side level prices are in ascending order w.r.t. index value. 
//Eg. 0th index will have lowest price and Topmost index will have highest price.
//
//Ask side level prices are in descending order w.r.t. index value. 
//Eg. 0th index will have highest price and Topmost index will have lowest price.
void BaseMarketViewManager::BuildIndex(const uint32_t t_security_id_, const TradeType_t t_buysell_, int int_price_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // This is the first packet processed for this security. Log the timestamp, so as to use it in sim.
  DBGLOG_TIME_CLASS_FUNC << " IndexBuild for " << smv_.shortcode() << " " << smv_.secname() << " " << t_security_id_
                         << DBGLOG_ENDL_FLUSH;

  smv_.bid_access_bitmask_ = smv_.ask_access_bitmask_ = BIT_RESET_ALL;

  int int_bid_price_ = (t_buysell_ == kTradeTypeBuy) ? int_price_ : int_price_ - 1;
  int int_ask_price_ = int_bid_price_ + 1;

  smv_.base_bid_index_ = smv_.initial_tick_size_;
  smv_.base_ask_index_ = smv_.initial_tick_size_;

  for (int index_ = 0; index_ < (int)smv_.market_update_info_.bidlevels_.size(); index_++) {
    smv_.market_update_info_.bidlevels_[index_].limit_int_price_ = int_bid_price_ - (smv_.base_bid_index_ - index_);
    smv_.market_update_info_.asklevels_[index_].limit_int_price_ = int_ask_price_ + (smv_.base_ask_index_ - index_);

    smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
    smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;

    smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
    smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;

    smv_.market_update_info_.bidlevels_[index_].limit_price_ =
        smv_.GetDoublePx(smv_.market_update_info_.bidlevels_[index_].limit_int_price_);
    smv_.market_update_info_.asklevels_[index_].limit_price_ =
        smv_.GetDoublePx(smv_.market_update_info_.asklevels_[index_].limit_int_price_);
  }

  smv_.initial_book_constructed_ = true;
}

/**
 * This function in base_market_view_manager doesn't get called anywhere, whoever feels like calling it, please
 * implement the tests
 *
 *
 * This function deletes the levels if we receive a sub-best execution
 *
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param r_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param _client_position_
 * @param _global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 */
void BaseMarketViewManager::OrderExecuted(const int t_server_assigned_client_id_,
                                          const int _client_assigned_order_sequence_,
                                          const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                          const double _price_, const TradeType_t r_buysell_,
                                          const int _size_remaining_, const int _size_executed_,
                                          const int _client_position_, const int _global_position_,
                                          const int r_int_price_, const int32_t server_assigned_message_sequence,
                                          const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  SecurityMarketView& smv_ = *(security_market_view_map_[_security_id_]);

  // Not supporting non-indexed books for now
  if (!smv_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    return;
  }

  if (r_buysell_ == kTradeTypeBuy) {
    int t_base_bid_index_ = (int)smv_.base_bid_index_;

    // Sub best execution - delete levels
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ > r_int_price_) {
      // added 1 in end because the function called below reduces it by atleast 1
      t_base_bid_index_ =
          smv_.base_bid_index_ -
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - r_int_price_ - 1);
      t_base_bid_index_ = smv_.IndexedBookGetNextNonEmptyBidMapIndex(t_base_bid_index_);
    } else if (smv_.market_update_info().bidlevels_[smv_.base_bid_index_].limit_int_price_ == r_int_price_) {
      // Prices are same, adding it for CME for now
      // Check
    }

    if (t_base_bid_index_ > 0 && t_base_bid_index_ < (int)smv_.base_bid_index_ &&
        // Skip updating best prices, if best bid price is already <= the new price we want to assign
        smv_.market_update_info_.bestbid_int_price_ >
            smv_.market_update_info_.bidlevels_[t_base_bid_index_].limit_int_price_) {
      UpdateBestBidVariables(smv_.security_id(), t_base_bid_index_);

      smv_.UpdateL1Prices();
      smv_.NotifyL1PriceListeners();
      smv_.NotifyOnReadyListeners();
    }
  }

  // for SELL - analogous
  else {
    int t_base_ask_index_ = (int)smv_.base_ask_index_;

    // Sub best execution - delete levels
    if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ < r_int_price_) {
      t_base_ask_index_ =
          smv_.base_ask_index_ -
          (r_int_price_ - smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - 1);
      t_base_ask_index_ = smv_.IndexedBookGetNextNonEmptyAskMapIndex(t_base_ask_index_);
    }

    if (t_base_ask_index_ > 0 && (int)smv_.base_ask_index_ > t_base_ask_index_ &&
        // Skip updating best price, if best ask price is already >= new price we want to assign
        smv_.market_update_info_.bestask_int_price_ <
            smv_.market_update_info_.asklevels_[t_base_ask_index_].limit_int_price_) {
      UpdateBestAskVariables(smv_.security_id(), t_base_ask_index_);

      smv_.UpdateL1Prices();
      smv_.NotifyL1PriceListeners();
      smv_.NotifyOnReadyListeners();
    }
  }
}

/**
 *
 * @param t_server_assigned_client_id
 * @param t_client_assigned_order_sequence
 * @param t_server_assigned_order_sequence
 * @param t_security_id
 * @param t_price
 * @param r_buysell
 * @param size_remaining
 * @param t_size_executed
 * @param client_position
 * @param global_position
 * @param r_int_price
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseMarketViewManager::OrderSequenced(const int t_server_assigned_client_id,
                                           const int t_client_assigned_order_sequence,
                                           const int t_server_assigned_order_sequence, const unsigned int t_security_id,
                                           const double t_price, const TradeType_t r_buysell, const int size_remaining,
                                           const int t_size_executed, const int client_position,
                                           const int global_position, const int r_int_price,
                                           const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}
/**
 * Update best-bids and best asks based on our orders ( exclude/include )
 * @param t_security_id_
 */
void BaseMarketViewManager::UpdateBestBidVariablesUsingOurOrders(const unsigned int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (smv_.remove_self_orders_from_book_ && smv_.p_prom_order_manager_) {
    BestBidAskInfo ors_bid_ask_info_ = smv_.p_prom_order_manager_->ors_best_bid_ask();

    // Change the best variables only if the prices match.
    // If the ors price is higher, the current best price is already taking care of our orders.
    // If the current best price is higher, we may not want to change our best price.
    if (ors_bid_ask_info_.best_bid_int_price_ == smv_.market_update_info_.bestbid_int_price_) {
      int index = smv_.GetBidIndex(smv_.market_update_info_.bestbid_int_price_);
      if (ors_bid_ask_info_.best_bid_size_ >= smv_.market_update_info_.bidlevels_[index].limit_size_) {
        // If the ors size is at least the best bid size
        // Find the next best price
        int bid_index_ =
            smv_.base_bid_index_ - (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ -
                                    smv_.market_update_info_.bestbid_int_price_);
        int next_bid_index_ = bid_index_ - 1;
        for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
             next_bid_index_--)
          ;

        if (next_bid_index_ >= 0) {
          UpdateBestBidVariables(smv_.security_id(), next_bid_index_);
        } else {
          // If the next level doesn't exist, then don't change the best variables. (This case should rarely occur.)
        }
      } else {
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[index].limit_ordercount_ > ors_bid_ask_info_.best_bid_orders_
                ? smv_.market_update_info_.bidlevels_[index].limit_ordercount_ - ors_bid_ask_info_.best_bid_orders_
                : 1;
        smv_.market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[index].limit_size_ - ors_bid_ask_info_.best_bid_size_;
      }
    }
  }
}

/**
 * Same as bid
 * @param t_security_id_
 */
void BaseMarketViewManager::UpdateBestAskVariablesUsingOurOrders(const unsigned int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (smv_.remove_self_orders_from_book_ && smv_.p_prom_order_manager_) {
    BestBidAskInfo ors_bid_ask_info_ = smv_.p_prom_order_manager_->ors_best_bid_ask();

    // Change the best ask variables only if the prices match
    // if the ors price is lower, the current best ask price is already taking care of our orders.
    // if the current best price is lower, we may not want to change our best price.
    if (ors_bid_ask_info_.best_ask_int_price_ == smv_.market_update_info_.bestask_int_price_) {
      int index = smv_.GetAskIndex(smv_.market_update_info_.bestask_int_price_);
      if (ors_bid_ask_info_.best_ask_size_ >= smv_.market_update_info_.asklevels_[index].limit_size_) {
        // If the ors size is at least the best ask size
        // Find the next best price
        int ask_index_ =
            smv_.base_ask_index_ + (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ -
                                    smv_.market_update_info_.bestask_int_price_);
        int next_ask_index_ = ask_index_ - 1;
        for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
             next_ask_index_--)
          ;

        if (next_ask_index_ >= 0) {
          UpdateBestAskVariables(smv_.security_id(), next_ask_index_);

        } else {
          // if the next level doesn't exist, then don't change the best variables. (This case should rarely occur.)
        }
      } else {
        smv_.market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_[index].limit_ordercount_ > ors_bid_ask_info_.best_ask_orders_
                ? smv_.market_update_info_.asklevels_[index].limit_ordercount_ - ors_bid_ask_info_.best_ask_orders_
                : 1;
        smv_.market_update_info_.bestask_size_ =
            smv_.market_update_info_.asklevels_[index].limit_size_ - ors_bid_ask_info_.best_ask_size_;
      }
    }
  }
}

#define LOW_ACCESS_INDEX 50
/**
 * Sanitize ask side assuming bid is correct
 * @param t_security_id_
 * @return
 */
BookManagerErrorCode_t BaseMarketViewManager::SanitizeAskSide(const unsigned int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;

  // Check if we need to do sanitization
  if (int_price_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
    // Check if the ASK side is empty
    if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
      // We should not do sanitization, if the other side is not ready
      smv_.is_ready_ = false;
      return kBookManagerReturn;
    }

    // Sanitize ASK side
    int index_ = smv_.base_ask_index_;
    int last_index_ = index_;

    for (; index_ >= 0; index_--) {
      if (smv_.market_update_info_.asklevels_[index_].limit_int_price_ > int_price_ &&
          smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
        smv_.base_ask_index_ = index_;
        break;
      }
      smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
      smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
    }

    // The ask side is empty
    if (index_ < 0) {
      smv_.ask_access_bitmask_ = BIT_RESET_ALL;
      smv_.is_ready_ = false;
      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after sanitization "
                                  << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return kBookManagerReturn;
    }

    if ((smv_.base_ask_index_ - last_index_) < smv_.bitmask_size_) {
      smv_.ask_access_bitmask_ <<= (smv_.base_ask_index_ - last_index_);
    } else {
      smv_.ask_access_bitmask_ = BIT_RESET_ALL;
    }

    // Check if we need to re-align the index
    if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
    }

    // Update ASK side best variables
    UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
  }

  return kBookManagerOK;
}

/**
 *  Sanitize bid side assuming ask is correct
 * @param t_security_id_
 * @return
 */
BookManagerErrorCode_t BaseMarketViewManager::SanitizeBidSide(const unsigned int t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;

  // Check if we need to do sanitization
  if (int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
    // Check if the ASK side is empty
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
      // We should not do sanitization, if the other side is not ready
      smv_.is_ready_ = false;
      return kBookManagerReturn;
    }

    // Sanitize BID side
    int index_ = smv_.base_bid_index_;
    int last_index_ = index_;

    for (; index_ >= 0; index_--) {
      if (smv_.market_update_info_.bidlevels_[index_].limit_int_price_ < int_price_ &&
          smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
        smv_.base_bid_index_ = index_;
        break;
      }

      smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
      smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
    }

    // bid side is empty
    if (index_ < 0) {
      smv_.is_ready_ = false;
      smv_.bid_access_bitmask_ = BIT_RESET_ALL;
      DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization "
                                  << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      return kBookManagerReturn;
    }

    if ((smv_.base_bid_index_ - last_index_) < smv_.bitmask_size_) {
      smv_.bid_access_bitmask_ <<= (smv_.base_bid_index_ - last_index_);
    } else {
      smv_.bid_access_bitmask_ = BIT_RESET_ALL;
    }

    // Check if we need to re-align the center
    if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
    }

    // Update the BID side best variables
    UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
  }

  return kBookManagerOK;
}

/**
 *  Adjust bid index if price has gone beyond current vector sizes
 * @param t_security_id_
 * @param t_bid_int_price_
 * @param t_bid_index_
 * @return
 */
BookManagerErrorCode_t BaseMarketViewManager::AdjustBidIndex(const unsigned int t_security_id_, int t_bid_int_price_,
                                                             int& t_bid_index_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // There are 0 levels on bid side
  // if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
  if (t_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy, t_bid_int_price_);
    t_bid_index_ = smv_.base_bid_index_;
  } else if (t_bid_index_ >= (int)smv_.max_tick_range_) {
    RebuildIndexHighAccess(t_security_id_, kTradeTypeBuy, t_bid_int_price_);
    t_bid_index_ = smv_.base_bid_index_;
  }

  smv_.base_bid_index_ = t_bid_index_;
  //} else if (t_bid_index_ < 0) {
  //   return kBookManagerReturn;
  // }

  // if (t_bid_index_ >= (int)smv_.max_tick_range_) {
  //   RebuildIndexHighAccess(t_security_id_, kTradeTypeBuy, t_bid_int_price_);
  //   t_bid_index_ = smv_.base_bid_index_;
  //  }

  return kBookManagerOK;
}

/**
 *  Same as bid
 * @param t_security_id_
 * @param t_ask_int_price_
 * @param t_ask_index_
 * @return
 */

BookManagerErrorCode_t BaseMarketViewManager::AdjustAskIndex(const unsigned int t_security_id_, int t_ask_int_price_,
                                                             int& t_ask_index_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // There are 0 levels on bid side
  // if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
  if (t_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeSell, t_ask_int_price_);
    t_ask_index_ = smv_.base_ask_index_;
  } else if (t_ask_index_ >= (int)smv_.max_tick_range_) {
    RebuildIndexHighAccess(t_security_id_, kTradeTypeSell, t_ask_int_price_);
    t_ask_index_ = smv_.base_ask_index_;
  }

  smv_.base_ask_index_ = t_ask_index_;
  //} else if (t_ask_index_ < 0) {
  //  return kBookManagerReturn;
  //}

  // if (t_ask_index_ >= (int)smv_.max_tick_range_) {
  //  RebuildIndexHighAccess(t_security_id_, kTradeTypeSell, t_ask_int_price_);
  //  t_ask_index_ = smv_.base_ask_index_;
  //}

  return kBookManagerOK;
}
#undef LOW_ACCESS_INDEX

/**
 * Need to do things
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param r_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param _client_position_
 * @param _global_position_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseMarketViewManager::OrderConfirmed(const int t_server_assigned_client_id,
                                           const int t_client_assigned_order_sequence,
                                           const int t_server_assigned_order_sequence, const unsigned int t_security_id,
                                           const double t_price, const TradeType_t r_buysell,
                                           const int t_size_remaining, const int t_size_executed,
                                           const int t_client_position, const int global_position,
                                           const int r_int_price, const int32_t server_assigned_message_sequence,
                                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

/**
 * Need to do things
 * @param t_server_assigned_client_id_
 * @param _client_assigned_order_sequence_
 * @param _server_assigned_order_sequence_
 * @param _security_id_
 * @param _price_
 * @param r_buysell_
 * @param _size_remaining_
 * @param _size_executed_
 * @param r_int_price_
 * @param server_assigned_message_sequence
 * @param exchange_order_id
 * @param time_set_by_server
 */
void BaseMarketViewManager::OrderORSConfirmed(const int t_server_assigned_client_id_,
                                              const int _client_assigned_order_sequence_,
                                              const int _server_assigned_order_sequence_,
                                              const unsigned int _security_id_, const double _price_,
                                              const TradeType_t r_buysell_, const int _size_remaining_,
                                              const int _size_executed_, const int r_int_price_,
                                              const int32_t server_assigned_message_sequence,
                                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

/**
 * Rests the book
 * @param t_security_id
 */
void BaseMarketViewManager::ResetBook(unsigned int t_security_id) {
  auto smv = security_market_view_map_[t_security_id];
  std::fill(smv->market_update_info_.bidlevels_.begin(), smv->market_update_info_.bidlevels_.end(),
            MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));
  std::fill(smv->market_update_info_.asklevels_.begin(), smv->market_update_info_.asklevels_.end(),
            MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));

  smv->initial_book_constructed_ = false;
}
}
