/**
   \file MarketAdapter/hkex_indexed_market_view_manager.hpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

   The reason I propose moving from a simpler vector based non-indexed book manager to this is
   that in the simpler implementation we were not able to maintain any history
   without doing similar work as we are doing here.
   Here we can retain many levels of data
*/
#pragma once

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_book_oputils.hpp"

namespace HFSAT {

class HKEXIndexedMarketViewManager : public BaseMarketViewManager,
                                     public HKHalfBookGlobalListener,
                                     public GlobalOrderChangeListener {
 public:
  HKEXIndexedMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                               const SecurityNameIndexer& t_sec_name_indexer_,
                               const std::vector<SecurityMarketView*>& t_security_market_view_map_)
      : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
        sec_id_to_initial_book_constructed_(t_sec_name_indexer_.NumSecurityId(), false),
        sec_id_to_initial_ticks_size_(t_sec_name_indexer_.NumSecurityId(), 255),
        sec_id_to_max_ticks_size_(t_sec_name_indexer_.NumSecurityId(), 511),
        sec_id_to_low_access_index_(t_sec_name_indexer_.NumSecurityId(), 50),
        sec_id_to_max_access_index_(t_sec_name_indexer_.NumSecurityId(), 511),
        sec_id_to_lowest_hit_int_price_(t_sec_name_indexer_.NumSecurityId(), kInvalidIntPrice),
        sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_(t_sec_name_indexer_.NumSecurityId(), 0),
        sec_id_to_highest_lift_int_price_(t_sec_name_indexer_.NumSecurityId(), kInvalidIntPrice),
        sec_id_to_accumulated_lift_size_at_highest_lift_int_price_(t_sec_name_indexer_.NumSecurityId(), 0),
        sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false) {
    // Preset the bidlevels and asklevels to (2 * tick_range_for_shortcode_ + 1) so that later we can access
    // any level ( - tick_range_for_shortcode_ to +tick_range_for_shortcode_ ) of that vector fast.
    for (auto i = 0u; i < security_market_view_map_.size(); i++) {
      SecurityMarketView* const p_this_smv_ = t_security_market_view_map_[i];
      if ((p_this_smv_ != NULL) &&
          (p_this_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_)) {
        const std::string& this_shortcode_ = p_this_smv_->shortcode();
        const HFSAT::ExchSource_t _this_exch_source_ = p_this_smv_->this_smv_exch_source_;
        const int this_security_id_ = p_this_smv_->security_id();  // should be i only.

        // customized to HKEXBookStruct
        if (_this_exch_source_ != HFSAT::kExchSourceHONGKONG) {
          continue;
        }

        unsigned int this_initial_base_index_ =
            IndexedBookOputils::GetUniqueInstance().GetInitialBaseBidIndex(this_shortcode_);
        unsigned int max_tick_range_for_this_sec_ = (2 * this_initial_base_index_ + 1);
        // saving my current limits of memory
        sec_id_to_initial_ticks_size_[this_security_id_] = this_initial_base_index_;  // number of ticks above base or
                                                                                      // below base allowed, or
                                                                                      // otherwise need to IndexRebuild
        sec_id_to_max_ticks_size_[this_security_id_] = max_tick_range_for_this_sec_;
        sec_id_to_low_access_index_[this_security_id_] = 50;
        sec_id_to_max_access_index_[this_security_id_] = max_tick_range_for_this_sec_;

        if (p_this_smv_->market_update_info_.bidlevels_.size() < max_tick_range_for_this_sec_) {
          // fill the bid-ask levels with dummy values and also increase the size of the vector.
          p_this_smv_->market_update_info_.bidlevels_.resize(
              max_tick_range_for_this_sec_,
              MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 1, watch_.tv()));
          p_this_smv_->market_update_info_.asklevels_.resize(
              max_tick_range_for_this_sec_,
              MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 1, watch_.tv()));
        }

        // since these won't be used .. only setting them once
        p_this_smv_->market_update_info_.bestbid_ordercount_ = 1;
        p_this_smv_->market_update_info_.bestask_ordercount_ = 1;
      }
    }
  }

  inline void OnHKHalfBookChange(const unsigned int t_security_id_,
                                 const HKEX_MDS::HKEXBookStruct& r_hkex_book_struct_) {
    SecurityMarketView* p_smv_ = security_market_view_map_[t_security_id_];
    if (p_smv_ == NULL) {
      return;
    }

    SecurityMarketView& this_smv_ = *p_smv_;
    if (!sec_id_to_initial_book_constructed_[t_security_id_]) {
      // check for MCH
      if ((r_hkex_book_struct_.pxs_[0] <= 1) || (r_hkex_book_struct_.pxs_[1] <= 1) ||
          (r_hkex_book_struct_.pxs_[2] <= 1) || (r_hkex_book_struct_.pxs_[3] <= 1)) {
        return;
      }

      unsigned int this_initial_base_index_ = sec_id_to_initial_ticks_size_[t_security_id_];

      int guessed_base_bid_int_price_ = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[0]);
      int guessed_base_ask_int_price_ = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[0]);
      if (r_hkex_book_struct_.side_ == HK_HALF_BOOK_SIDE_IS_BID) {
        guessed_base_ask_int_price_ = guessed_base_bid_int_price_ + 1;
      } else {
        guessed_base_bid_int_price_ = guessed_base_ask_int_price_ - 1;
      }

      // this could have been done at any time, since it is not used here
      // essentially will be used after book construction
      this_smv_.base_bid_index_ = this_initial_base_index_;
      this_smv_.base_ask_index_ = this_initial_base_index_;

      {
        this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_ =
            guessed_base_bid_int_price_;
        this_smv_.base_bid_intprice_index_adjustment_ =
            (this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_ -
             this_smv_.base_bid_index_);

        // called after setting offset variables
        for (unsigned int current_bid_index_ = 0; current_bid_index_ < this_smv_.market_update_info_.bidlevels_.size();
             current_bid_index_++) {
          this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_ =
              (int)guessed_base_bid_int_price_ - (int)(this_smv_.base_bid_index_) +
              (int)(current_bid_index_);  // we can set the int prices since that is of course the basis of this map.
#ifdef SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
          // we can set limit_price as well obviously if we can set limit_int_price_
          this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_price_ =
              this_smv_.GetDoublePx(this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_);
#endif  // SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
        }
      }

      {
        this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_ =
            guessed_base_ask_int_price_;
        this_smv_.base_ask_intprice_index_adjustment_ =
            (this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_ +
             this_smv_.base_ask_index_);  // note that this is '+' not '-' like bidside

        // called after setting offset variables
        for (unsigned int current_ask_index_ = 0; current_ask_index_ < this_smv_.market_update_info_.asklevels_.size();
             current_ask_index_++) {
          this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_ =
              (int)guessed_base_ask_int_price_ + (int)(this_smv_.base_ask_index_) -
              (int)(current_ask_index_);  // we can set the int prices since that is of course the basis of this map.
#ifdef SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
          // we can set limit_price as well obviously if we can set limit_int_price_
          this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_price_ =
              this_smv_.GetDoublePx(this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_);
#endif  // SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
        }
      }

      sec_id_to_initial_book_constructed_[t_security_id_] = true;
    }

    // now that the book is constructed we can go ahead

    // keep a flag of whether this is an l1 change
    bool l1_price_changed_in_this_update_ = false;
    bool l1_size_changed_in_this_update_ = false;
    if (r_hkex_book_struct_.side_ == HK_HALF_BOOK_SIDE_IS_BID) {
      // invalidate all same side levels "above" GetIntPx ( r_hkex_book_struct_.pxs_[0] )
      int new_toplevel_bid_int_price_ = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[0]);
      if (new_toplevel_bid_int_price_ != this_smv_.market_update_info_.bestbid_int_price_) {  // l1 price changed

        // for calling appropriate listeners
        l1_price_changed_in_this_update_ = true;

        // first check if we need to RecenterBaseIndex
        {
          int future_base_bid_index_ = (new_toplevel_bid_int_price_ - this_smv_.base_bid_intprice_index_adjustment_);
          if ((future_base_bid_index_ <= (int)sec_id_to_low_access_index_[t_security_id_]) ||
              (future_base_bid_index_ >= (int)sec_id_to_max_access_index_[t_security_id_])) {
            RecenterBaseIndex(this_smv_, t_security_id_, kTradeTypeBuy, new_toplevel_bid_int_price_);
          }
        }

        // invalidate all same side levels "above" this intprice
        if ((this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_ >
             new_toplevel_bid_int_price_) &&
            (this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_ > 0)) {
          for (int current_bid_index_ = this_smv_.base_bid_index_; current_bid_index_ >= 0; current_bid_index_--) {
            if (this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_ >
                new_toplevel_bid_int_price_) {  // invalidate this level
              this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ = 0;
            } else {
              break;
            }
          }
        }

        // invalidate all opposite side levels that cross this intprice
        if (this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_ > 0) {
          bool t_need_to_find_new_base_ask_index_ = false;
          for (int current_ask_index_ = this_smv_.base_ask_index_; current_ask_index_ > 0; current_ask_index_--) {
            if (new_toplevel_bid_int_price_ >= this_smv_.market_update_info_.asklevels_[current_ask_index_]
                                                   .limit_int_price_) {  // we should invalidate this level if needed
              if (this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ > 0) {  // valid level
                this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ = 0;
                t_need_to_find_new_base_ask_index_ = true;
              }
            } else {
              if (t_need_to_find_new_base_ask_index_) {  // still looking for next valid level
                if (this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ > 0) {  // valid level
                  this_smv_.base_ask_index_ = current_ask_index_;

                  // setting fresh bestlevel variables in SMV
                  // if masking is not sane price only then reset values to best ask from book ( if mask_ask <=
                  // new_bid_ask, invalidate mask )
                  if (this_smv_.market_update_info_.bestask_int_price_ <=
                      this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_) {
                    this_smv_.market_update_info_.bestask_int_price_ =
                        this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_;
                    this_smv_.market_update_info_.bestask_price_ =
                        this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_price_;
                    this_smv_.market_update_info_.bestask_size_ =
                        this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_;
                    this_smv_.market_update_info_.trade_update_implied_quote_ = false;
                    UpdateBestAskVariablesUsingOurOrders(t_security_id_);
                  }

                  t_need_to_find_new_base_ask_index_ = false;
                  if (this_smv_.base_ask_index_ <=
                      sec_id_to_low_access_index_[t_security_id_]) {  // check if the index has moved lo enough for this
                                                                      // to be recentered
                    RecenterBaseIndex(this_smv_, t_security_id_, kTradeTypeSell,
                                      this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_);
                  }
                  break;
                }
              } else {
                break;
              }
            }
          }
        }

      } else {
        if (r_hkex_book_struct_.demand_[0] != this_smv_.market_update_info_.bestbid_size_) {
          l1_size_changed_in_this_update_ = true;
        }
      }

      // update base_bid_index_
      this_smv_.base_bid_index_ = (new_toplevel_bid_int_price_ - this_smv_.base_bid_intprice_index_adjustment_);
      // set ip_0 to new size
      this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_ = r_hkex_book_struct_.demand_[0];

      // setting fresh bestlevel variables in SMV
      this_smv_.market_update_info_.bestbid_int_price_ =
          this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_;
      this_smv_.market_update_info_.bestbid_price_ =
          this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_price_;
      this_smv_.market_update_info_.bestbid_size_ =
          this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_;
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);

      this_smv_.market_update_info_.trade_update_implied_quote_ = false;

      // and then keep stepping index till we get to ip_4.
      // for every level that is between levels in current book, set limit_size_ to 0
      // else set limit_size_ to what is given in current book.

      // computing int prices first ... optimization, as opposed to calling GetIntPx in the for loop
      int int_pxs_[5];
      for (unsigned int vector_book_index_ = 0u; vector_book_index_ <= 4u; vector_book_index_++) {
        int_pxs_[vector_book_index_] = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[vector_book_index_]);
      }
      for (unsigned int current_bid_index_ = this_smv_.base_bid_index_ - 1, vector_book_index_ = 1u;
           (current_bid_index_ > 0) && (vector_book_index_ <= 4);) {
        // if the next level in the last snapshot of the book does not exist in the new book then delete
        if (this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_ >
            int_pxs_[vector_book_index_]) {
          this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ = 0;
          current_bid_index_--;
        } else {
          if (this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_ ==
              int_pxs_[vector_book_index_]) {
            this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ =
                r_hkex_book_struct_.demand_[vector_book_index_];
            current_bid_index_--;
            vector_book_index_++;
          } else {
            // current_bid_index_--  ;
            vector_book_index_++;
          }
        }
      }
    } else {  // ASK
      // invalidate all same side levels "above" GetIntPx ( r_hkex_book_struct_.pxs_[0] )
      int new_toplevel_ask_int_price_ = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[0]);
      if (new_toplevel_ask_int_price_ != this_smv_.market_update_info_.bestask_int_price_) {  // l1 price changed

        // for calling appropriate listeners
        l1_price_changed_in_this_update_ = true;

        // first check if we need to RecenterBaseIndex
        {
          int future_base_ask_index_ = (this_smv_.base_ask_intprice_index_adjustment_ - new_toplevel_ask_int_price_);
          if ((future_base_ask_index_ <= (int)sec_id_to_low_access_index_[t_security_id_]) ||
              (future_base_ask_index_ >= (int)sec_id_to_max_access_index_[t_security_id_])) {
            RecenterBaseIndex(this_smv_, t_security_id_, kTradeTypeSell, new_toplevel_ask_int_price_);
          }
        }

        // invalidate all same side levels "above" this intprice
        if ((this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_ <
             new_toplevel_ask_int_price_) &&
            (this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_ > 0)) {
          for (int current_ask_index_ = this_smv_.base_ask_index_; current_ask_index_ >= 0; current_ask_index_--) {
            if (this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_ <
                new_toplevel_ask_int_price_) {  // invalidate this level
              this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ = 0;
            } else {
              break;
            }
          }
        }

        // invalidate all opposite side levels that cross this intprice
        if (this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_ > 0) {
          bool t_need_to_find_new_base_bid_index_ = false;
          for (int current_bid_index_ = this_smv_.base_bid_index_; current_bid_index_ > 0; current_bid_index_--) {
            if (new_toplevel_ask_int_price_ <= this_smv_.market_update_info_.bidlevels_[current_bid_index_]
                                                   .limit_int_price_) {  // we should invalidate this level if needed
              if (this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ > 0) {  // valid level
                this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ = 0;
                t_need_to_find_new_base_bid_index_ = true;
              }
            } else {
              if (t_need_to_find_new_base_bid_index_) {  // still looking for next valid level
                if (this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_size_ > 0) {  // valid level
                  this_smv_.base_bid_index_ = current_bid_index_;

                  // setting fresh bestlevel variables in SMV
                  // reset masking values only if new book bid is less masked_bid ( hence invalidate mask )
                  if (this_smv_.market_update_info_.bestbid_int_price_ >=
                      this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_) {
                    this_smv_.market_update_info_.bestbid_int_price_ =
                        this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_int_price_;
                    this_smv_.market_update_info_.bestbid_price_ =
                        this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_price_;
                    this_smv_.market_update_info_.bestbid_size_ =
                        this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_;
                    this_smv_.market_update_info_.trade_update_implied_quote_ = false;
                    UpdateBestBidVariablesUsingOurOrders(t_security_id_);
                  }

                  t_need_to_find_new_base_bid_index_ = false;
                  if (this_smv_.base_bid_index_ <=
                      sec_id_to_low_access_index_[t_security_id_]) {  // check if the index has moved lo enough for this
                                                                      // to be recentered
                    RecenterBaseIndex(this_smv_, t_security_id_, kTradeTypeBuy,
                                      this_smv_.market_update_info_.bidlevels_[current_bid_index_].limit_int_price_);
                  }
                  break;
                }
              } else {
                break;
              }
            }
          }
        }
      } else {
        if (r_hkex_book_struct_.demand_[0] != this_smv_.market_update_info_.bestask_size_) {
          l1_size_changed_in_this_update_ = true;
        }
      }

      // update base_ask_index_
      this_smv_.base_ask_index_ = (this_smv_.base_ask_intprice_index_adjustment_ - new_toplevel_ask_int_price_);
      // set ip_0 to new size
      this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_ = r_hkex_book_struct_.demand_[0];

      // setting fresh bestlevel variables in SMV
      this_smv_.market_update_info_.bestask_int_price_ =
          this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_int_price_;
      this_smv_.market_update_info_.bestask_price_ =
          this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_price_;
      this_smv_.market_update_info_.bestask_size_ =
          this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_;
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);

      this_smv_.market_update_info_.trade_update_implied_quote_ = false;

      // and then keep stepping index till we get to ip_4.
      // for every level that is between levels in current book, set limit_size_ to 0
      // else set limit_size_ to what is given in current book.

      // computing int prices first
      int int_pxs_[5];
      for (unsigned int vector_book_index_ = 0u; vector_book_index_ <= 4u; vector_book_index_++) {
        int_pxs_[vector_book_index_] = this_smv_.GetIntPx(r_hkex_book_struct_.pxs_[vector_book_index_]);
      }
      for (int current_ask_index_ = this_smv_.base_ask_index_ - 1, vector_book_index_ = 1u;
           (current_ask_index_ >= 0) && (vector_book_index_ <= 4);) {
        // if the next level in the last snapshot of the book does not exist in the new book then delete
        if (this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_ <
            int_pxs_[vector_book_index_]) {
          this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ = 0;
          current_ask_index_--;
        } else {
          if (this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_int_price_ ==
              int_pxs_[vector_book_index_]) {
            this_smv_.market_update_info_.asklevels_[current_ask_index_].limit_size_ =
                r_hkex_book_struct_.demand_[vector_book_index_];
            current_ask_index_--;
            vector_book_index_++;
          } else {
            // current_ask_index_++;
            vector_book_index_++;
          }
        }
      }
    }

    // setting is_ready_
    if (!this_smv_.is_ready_) {
      if ((this_smv_.market_update_info_.bidlevels_[this_smv_.base_bid_index_].limit_size_ > 0) &&
          (this_smv_.market_update_info_.asklevels_[this_smv_.base_ask_index_].limit_size_ > 0)) {
        this_smv_.is_ready_ = true;
      }
    }

    if (l1_price_changed_in_this_update_ || l1_size_changed_in_this_update_) {
      this_smv_.UpdateL1Prices();
      this_smv_.NotifyL1SizeListeners();
      this_smv_.NotifyOnReadyListeners();
    } else {
      this_smv_.OnL2Update();
      this_smv_.OnL2OnlyUpdate();
      this_smv_.NotifyOnReadyListeners();
    }

    // setting these for OnTrade
    if (r_hkex_book_struct_.side_ == HK_HALF_BOOK_SIDE_IS_BID) {
      sec_id_to_lowest_hit_int_price_[t_security_id_] = 0;
      sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_[t_security_id_] = 0;
    } else {  // since we have received an ask side book update zero out masking
      sec_id_to_highest_lift_int_price_[t_security_id_] = 0;
      sec_id_to_accumulated_lift_size_at_highest_lift_int_price_[t_security_id_] = 0;
    }

    sec_id_to_prev_update_was_quote_[t_security_id_] = true;
  }

  inline void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {
    SecurityMarketView* p_smv_ = security_market_view_map_[t_security_id_];
    if (p_smv_ == NULL) {
      return;
    }

    SecurityMarketView& this_smv_ = *p_smv_;

    // not till book is ready
    if ((this_smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
        (this_smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
      return;
    }

    int t_trade_int_price_ = this_smv_.GetIntPx(t_trade_price_);

    // Guess trade type if possible
    TradeType_t t_buysell_ = kTradeTypeNoInfo;
    if (t_trade_int_price_ <= this_smv_.market_update_info_.bestbid_int_price_) {
      t_buysell_ = kTradeTypeSell;
    } else {
      if (t_trade_int_price_ >= this_smv_.market_update_info_.bestask_int_price_) {
        t_buysell_ = kTradeTypeBuy;
      }
    }

    // same as SecurityMarketView::OnTrade
    {
      if (this_smv_.trade_print_info_.computing_last_book_tdiff_) {
        if (sec_id_to_prev_update_was_quote_[t_security_id_]) {  // the difference between
                                                                 // last_book_mkt_size_weighted_price_ and
                                                                 // mkt_size_weighted_price_ is that
          // in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
          // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of
          // mkt_size_weighted_price_
          // the last time the update was a book message
          this_smv_.market_update_info_.last_book_mkt_size_weighted_price_ =
              this_smv_.market_update_info_
                  .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
        }
      }
      if (!this_smv_.market_update_info_.trade_update_implied_quote_) {
        this_smv_.market_update_info_.trade_update_implied_quote_ = true;
      }

      this_smv_.StorePreTrade();

      // set the primary variables before
      this_smv_.trade_print_info_.trade_price_ = t_trade_price_;
      this_smv_.trade_print_info_.size_traded_ = t_trade_size_;
      this_smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
      this_smv_.trade_print_info_.num_trades_++;

      this_smv_.trade_print_info_.buysell_ = t_buysell_;

      // do the conditional computations ... before the quote is adjusted for the trade
      this_smv_.SetTradeVarsForIndicatorsIfRequired();
    }

    if (t_buysell_ == kTradeTypeBuy) {  // LIFT

      // Assume that at this point
      // if ( sec_id_to_highest_lift_int_price_ [ t_security_id_ ] > 0 )
      //   { bestask_int_price_ >= sec_id_to_highest_lift_int_price_ [ t_security_id_ ] }
      // since all lower prices would have been invalidated prior to this

      // if sec_id_to_highest_lift_int_price_ [ t_security_id_ ] is valid and this new trade is deeper in the book
      if ((sec_id_to_highest_lift_int_price_[t_security_id_] == 0) ||
          (t_trade_int_price_ > sec_id_to_highest_lift_int_price_[t_security_id_])) {
        sec_id_to_highest_lift_int_price_[t_security_id_] = t_trade_int_price_;
        sec_id_to_accumulated_lift_size_at_highest_lift_int_price_[t_security_id_] = t_trade_size_;
      } else {
        if (t_trade_int_price_ == sec_id_to_highest_lift_int_price_[t_security_id_]) {
          sec_id_to_accumulated_lift_size_at_highest_lift_int_price_[t_security_id_] += t_trade_size_;
        }
        //	      else
        //		{
        //		  return ;
        //		}
      }

      // current array index of bestask_int_price_
      int toplevel_ask_index_ =
          this_smv_.base_ask_intprice_index_adjustment_ - this_smv_.market_update_info_.bestask_int_price_;

      // keep disregarding all indices that are crossed by current trade
      while ((toplevel_ask_index_ > 0) &&
             (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_int_price_ < t_trade_int_price_)) {
        toplevel_ask_index_--;
      }
      // now
      // this_smv_.market_update_info_.asklevels_ [ toplevel_ask_index_ ] . limit_int_price_ >= t_trade_int_price_ (
      // probably == )

      if (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_int_price_ == t_trade_int_price_) {
        if (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_ >
            sec_id_to_accumulated_lift_size_at_highest_lift_int_price_[t_security_id_]) {  // if after masking at this
                                                                                           // level there is still some
                                                                                           // size left

          this_smv_.market_update_info_.bestask_price_ =
              this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_price_;
          this_smv_.market_update_info_.bestask_int_price_ =
              this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_int_price_;
          this_smv_.market_update_info_.bestask_size_ =
              (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_ -
               sec_id_to_accumulated_lift_size_at_highest_lift_int_price_[t_security_id_]);
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        } else {  // this level has been masked
          toplevel_ask_index_--;

          // disregard all size=0 levels in coming up with next bestask_int_price_
          while ((toplevel_ask_index_ > 0) &&
                 (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_ <= 0)) {
            toplevel_ask_index_--;
          }

#define LIMIT_MASK_LEVELS 10
          if ((toplevel_ask_index_ == 0) ||
              (this_smv_.base_ask_intprice_index_adjustment_ - this_smv_.market_update_info_.bestask_int_price_ -
                   toplevel_ask_index_ >
               LIMIT_MASK_LEVELS)) {
            this_smv_.market_update_info_.bestask_int_price_ = t_trade_int_price_ + 1;
            this_smv_.market_update_info_.bestask_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.bestask_int_price_);
            this_smv_.market_update_info_.bestask_size_ = 1;
          } else {
#undef LIMIT_MASK_LEVELS
            if (toplevel_ask_index_ > 0) {
              this_smv_.market_update_info_.bestask_price_ =
                  this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_price_;
              this_smv_.market_update_info_.bestask_int_price_ =
                  this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_int_price_;
              this_smv_.market_update_info_.bestask_size_ =
                  this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_;
              UpdateBestAskVariablesUsingOurOrders(t_security_id_);
            }
          }
        }

      } else {  // strictly higher price so no need to worry about size

        // disregard all size=0 levels in coming up with next bestask_int_price_
        while ((toplevel_ask_index_ > 0) &&
               (this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_ <= 0)) {
          toplevel_ask_index_--;
        }
        if (toplevel_ask_index_ >
            0) {  // this_smv_.market_update_info_.asklevels_ [ toplevel_ask_index_ ] . limit_size_ > 0
          this_smv_.market_update_info_.bestask_price_ =
              this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_price_;
          this_smv_.market_update_info_.bestask_int_price_ =
              this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_int_price_;
          this_smv_.market_update_info_.bestask_size_ =
              this_smv_.market_update_info_.asklevels_[toplevel_ask_index_].limit_size_;
          UpdateBestAskVariablesUsingOurOrders(t_security_id_);
        }
      }
    }

    if (t_buysell_ == kTradeTypeSell) {  // HIT

      // Assume that at this point
      // if ( sec_id_to_lowest_hit_int_price_ [ t_security_id_ ] > 0 )
      //   { bestbid_int_price_ >= sec_id_to_lowest_hit_int_price_ [ t_security_id_ ] }
      // since all lower prices would have been invalidated prior to this

      // if sec_id_to_lowest_hit_int_price_ [ t_security_id_ ] is valid and this new trade is deeper in the book
      if ((sec_id_to_lowest_hit_int_price_[t_security_id_] == 0) ||
          (t_trade_int_price_ < sec_id_to_lowest_hit_int_price_[t_security_id_])) {
        sec_id_to_lowest_hit_int_price_[t_security_id_] = t_trade_int_price_;
        sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_[t_security_id_] = t_trade_size_;
      } else {
        if (t_trade_int_price_ == sec_id_to_lowest_hit_int_price_[t_security_id_]) {
          sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_[t_security_id_] += t_trade_size_;
        }
        //	      else
        //		{
        //		  return ;
        //		}
      }

      // current array index of bestbid_int_price_
      int toplevel_bid_index_ =
          this_smv_.market_update_info_.bestbid_int_price_ - this_smv_.base_bid_intprice_index_adjustment_;

      // keep disregarding all indices that are crossed by current trade
      while ((toplevel_bid_index_ > 0) &&
             (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_int_price_ > t_trade_int_price_)) {
        toplevel_bid_index_--;
      }
      // now
      // this_smv_.market_update_info_.bidlevels_ [ toplevel_bid_index_ ] . limit_int_price_ <= t_trade_int_price_ (
      // probably == )

      if (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_int_price_ == t_trade_int_price_) {
        if (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_ >
            sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_[t_security_id_]) {  // if after masking at this
                                                                                        // level there is still some
                                                                                        // size left

          this_smv_.market_update_info_.bestbid_price_ =
              this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_price_;
          this_smv_.market_update_info_.bestbid_int_price_ =
              this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_int_price_;
          this_smv_.market_update_info_.bestbid_size_ =
              (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_ -
               sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_[t_security_id_]);
          UpdateBestBidVariablesUsingOurOrders(t_security_id_);
        } else {  // this level has been masked
          toplevel_bid_index_--;

          // disregard all size=0 levels in coming up with next bestbid_int_price_
          while ((toplevel_bid_index_ > 0) &&
                 (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_ <= 0)) {
            toplevel_bid_index_--;
          }

#define LIMIT_MASK_LEVELS 10
          if ((toplevel_bid_index_ == 0) || (this_smv_.market_update_info_.bestbid_int_price_ -
                                                 this_smv_.base_bid_intprice_index_adjustment_ - toplevel_bid_index_ >
                                             LIMIT_MASK_LEVELS)) {
            this_smv_.market_update_info_.bestbid_int_price_ = t_trade_int_price_ - 1;
            this_smv_.market_update_info_.bestbid_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.bestbid_int_price_);
            this_smv_.market_update_info_.bestbid_size_ = 1;
          } else {
#undef LIMIT_MASK_LEVELS
            if (toplevel_bid_index_ >
                0) {  // only update the best level variables bestbid_int_price_ if toplevel_bid_index_ is valid
              this_smv_.market_update_info_.bestbid_price_ =
                  this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_price_;
              this_smv_.market_update_info_.bestbid_int_price_ =
                  this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_int_price_;
              this_smv_.market_update_info_.bestbid_size_ =
                  this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_;
              UpdateBestBidVariablesUsingOurOrders(t_security_id_);
            }
          }
        }

      } else {  // strictly higher price so no need to worry about size

        // disregard all size=0 levels in coming up with next bestbid_int_price_
        while ((toplevel_bid_index_ > 0) &&
               (this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_ <= 0)) {
          toplevel_bid_index_--;
        }
        this_smv_.market_update_info_.bestbid_price_ =
            this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_price_;
        this_smv_.market_update_info_.bestbid_int_price_ =
            this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_int_price_;
        this_smv_.market_update_info_.bestbid_size_ =
            this_smv_.market_update_info_.bidlevels_[toplevel_bid_index_].limit_size_;
        UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      }
    }

    // Same as SecurityMarketView::OnTrade
    if (this_smv_.is_ready_) {
      this_smv_.UpdateL1Prices();
      this_smv_.NotifyTradeListeners();
      this_smv_.NotifyOnReadyListeners();
    }
    sec_id_to_prev_update_was_quote_[t_security_id_] = false;
  }

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

 protected:
  std::vector<bool> sec_id_to_initial_book_constructed_;  // for initialization on receiving first valid price

  std::vector<int> sec_id_to_initial_ticks_size_;         // initial value of the center index
  std::vector<int> sec_id_to_max_ticks_size_;             // the total size of the vector
  std::vector<unsigned int> sec_id_to_low_access_index_;  // low index at which we should trigger a rebuild
  std::vector<unsigned int> sec_id_to_max_access_index_;  // high index at which we should trigger a rebuild

  // variables maintained for masking book on trades
  std::vector<int> sec_id_to_lowest_hit_int_price_;                             // the lowest int price seen on a HIT
  std::vector<int> sec_id_to_accumulated_hit_size_at_lowest_hit_int_price_;     // the accumulated trade size seen at
                                                                                // lowest int price seen on a HIT
  std::vector<int> sec_id_to_highest_lift_int_price_;                           // the highest int price seen on a LIFT
  std::vector<int> sec_id_to_accumulated_lift_size_at_highest_lift_int_price_;  // the accumulated trade size seen at
                                                                                // highest int price seen on a LIFT

  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  // this function is meant to just shift the data and hence
  // change the base_bid_intprice_index_adjustment_ and base_ask_intprice_index_adjustment_
  // and change the base_bid_index_ and base_ask_index_
  // but not to actually change anything else like not even the actual values at bidlevels_ [ base_bid_index_ ] or
  // asklevels_ [ base_ask_index_ ]
  void RecenterBaseIndex(SecurityMarketView& this_smv_, const unsigned int t_security_id_, const TradeType_t t_buysell_,
                         const int new_toplevel_int_px_) {
    int this_initial_base_index_ = sec_id_to_initial_ticks_size_[t_security_id_];

    switch (t_buysell_) {
      case HFSAT::kTradeTypeBuy: {
        int array_index_toplevel_ =
            new_toplevel_int_px_ -
            this_smv_.base_bid_intprice_index_adjustment_;  // this is what this price would have mapped to
        int diff_indices_ =
            this_initial_base_index_ -
            array_index_toplevel_;  // this_initial_base_index_ is what we want new_toplevel_int_px_ to map to

        int new_base_bid_index_ = (int)this_smv_.base_bid_index_ + (int)diff_indices_;
        if ((new_base_bid_index_ < 0) ||
            (new_base_bid_index_ >= (int)this_smv_.market_update_info_.bidlevels_.size())) {
          DBGLOG_CLASS_FUNC_LINE << " Current Base Bid Index : " << this_smv_.base_bid_index_
                                 << " new base index would have been " << new_base_bid_index_
                                 << " array_index_toplevel_ " << array_index_toplevel_ << " diff_indices_ "
                                 << diff_indices_ << DBGLOG_ENDL_FLUSH;

          if (new_base_bid_index_ < 0) {
            // Re-centering here might need handling of overlapping prices in new and old mapping
            // condition diff_indices_ has to be -ve
            for (int current_array_index_ = 0;
                 (current_array_index_ < (int)this_smv_.market_update_info_.bidlevels_.size());
                 current_array_index_++) {
              int t_source_index_ = current_array_index_ - diff_indices_;
              if (t_source_index_ >= (int)this_smv_.market_update_info_.bidlevels_.size()) {
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_ =
                    (int)(new_toplevel_int_px_ - (int)this_initial_base_index_) + (int)(current_array_index_);
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_price_ = this_smv_.GetDoublePx(
                    this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_);
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_size_ = 0;
              } else {
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_ =
                    this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_int_price_;
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_price_ =
                    this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_price_;
                this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_size_ =
                    this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_size_;
              }
            }

          } else if (new_base_bid_index_ >= (int)this_smv_.market_update_info_.bidlevels_.size()) {
            // New price is so high Recentering to this new price would exclude the level at current to go away from
            // vector
            // diff_indices_ has to be +ve , if it were -ve , base_bid_index > bidlevels_.size() which is not possible
            for (int current_array_index_ = 0;
                 (current_array_index_ < (int)this_smv_.market_update_info_.bidlevels_.size());
                 current_array_index_++) {
              this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_ =
                  (int)(new_toplevel_int_px_ - (int)this_initial_base_index_) + (int)(current_array_index_);
            }
          }
          return;
        }

        int start_array_index_ = 0;
        int step_index_ = 1;
        // if diff_indices_ > 0 we are copying from a lower index and hence we should start from the top
        if (diff_indices_ > 0) {
          start_array_index_ = this_smv_.market_update_info_.bidlevels_.size() - 1;
          step_index_ = -1;
        }

        for (int current_array_index_ = start_array_index_;
             (current_array_index_ >= 0) &&
             (current_array_index_ < (int)this_smv_.market_update_info_.bidlevels_.size());
             current_array_index_ = (int)((int)current_array_index_ + step_index_)) {
          int t_source_index_ =
              (int)current_array_index_ -
              diff_indices_;  // this is the index that we should copy from to the new location current_array_index_

          if (t_source_index_ < 0 ||
              t_source_index_ >=
                  (int)this_smv_.market_update_info_.bidlevels_.size()) {  // invalid history for the source index

            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_ =
                (int)(new_toplevel_int_px_ - (int)this_initial_base_index_) +
                (int)(current_array_index_);  // we can set the int prices since that is of course the basis of this
                                              // map.
#ifdef SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
            // we can set limit_price as well obviously if we can set limit_int_price_
            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_);
#endif  // SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_size_ = 0;
          } else {
            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_ =
                this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_int_price_;
            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_price_ =
                this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_price_;

            this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_size_ =
                this_smv_.market_update_info_.bidlevels_[t_source_index_].limit_size_;
          }
        }

        // update base_bid_index_ since whatever was at base_bid_index_ earlier now is in base_bid_index_ +
        // diff_indices_
        this_smv_.base_bid_index_ = (int)this_smv_.base_bid_index_ + (int)diff_indices_;

        // This will be the new adjustment ( to go from int_price to the array index )
        this_smv_.base_bid_intprice_index_adjustment_ = new_toplevel_int_px_ - (int)this_initial_base_index_;
      } break;
      case HFSAT::kTradeTypeSell: {
        int array_index_toplevel_ = this_smv_.base_ask_intprice_index_adjustment_ -
                                    new_toplevel_int_px_;  // this is what this price would have mapped to
        int diff_indices_ =
            this_initial_base_index_ -
            array_index_toplevel_;  // this_initial_base_index_ is what we want new_toplevel_int_px_ to map to

        int new_base_ask_index_ = (int)this_smv_.base_ask_index_ + (int)diff_indices_;
        if ((new_base_ask_index_ < 0) ||
            (new_base_ask_index_ >= (int)this_smv_.market_update_info_.asklevels_.size())) {
          DBGLOG_CLASS_FUNC_LINE << " Current Base Ask Index : " << this_smv_.base_ask_index_
                                 << " new base ask index would have been " << new_base_ask_index_
                                 << " array_index_toplevel_ " << array_index_toplevel_ << " diff_indices_ "
                                 << diff_indices_ << DBGLOG_ENDL_FLUSH;

          if (new_base_ask_index_ < 0) {
            // New price is high, so we might get overlapping levels, with new and old mapping, handle that
            for (int current_array_index_ = 0;
                 (current_array_index_ < (int)this_smv_.market_update_info_.asklevels_.size());
                 current_array_index_++) {
              int t_source_index_ = current_array_index_ - diff_indices_;
              if (t_source_index_ >= (int)this_smv_.market_update_info_.asklevels_.size()) {
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_ =
                    (int)(new_toplevel_int_px_ - (int)this_initial_base_index_) + (int)(current_array_index_);
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_price_ = this_smv_.GetDoublePx(
                    this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_);
                ;
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_size_ = 0;
              } else {
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_ =
                    this_smv_.market_update_info_.asklevels_[t_source_index_].limit_int_price_;
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_price_ =
                    this_smv_.market_update_info_.asklevels_[t_source_index_].limit_price_;
                this_smv_.market_update_info_.asklevels_[current_array_index_].limit_size_ =
                    this_smv_.market_update_info_.asklevels_[t_source_index_].limit_size_;
              }
            }
          } else if (new_base_ask_index_ >= (int)this_smv_.market_update_info_.asklevels_.size()) {
            // diff_indices_ has to be +ve , if it were -ve , base_bid_index > asklevels_.size() which is not possible
            // Here all the current levels would go outside the asklevels_vector
            for (int current_array_index_ = 0;
                 (current_array_index_ < (int)this_smv_.market_update_info_.asklevels_.size());
                 current_array_index_++) {
              this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_ =
                  (int)(new_toplevel_int_px_ - (int)this_initial_base_index_) + (int)(current_array_index_);
              this_smv_.market_update_info_.asklevels_[current_array_index_].limit_price_ = this_smv_.GetDoublePx(
                  this_smv_.market_update_info_.bidlevels_[current_array_index_].limit_int_price_);
              this_smv_.market_update_info_.asklevels_[current_array_index_].limit_size_ = 0;
            }
          }
          return;
        }

        int start_array_index_ = 0;
        int step_index_ = 1;
        // if diff_indices_ > 0 we are copying from a lower index and hence we should start from the top
        if (diff_indices_ > 0) {
          start_array_index_ = this_smv_.market_update_info_.asklevels_.size() - 1;
          step_index_ = -1;
        }

        for (int current_array_index_ = start_array_index_;
             (current_array_index_ >= 0) &&
             (current_array_index_ < (int)this_smv_.market_update_info_.asklevels_.size());
             current_array_index_ = (int)((int)current_array_index_ + step_index_)) {
          int t_source_index_ =
              (int)current_array_index_ -
              diff_indices_;  // this is the index that we should copy from to the new location current_array_index_

          if (t_source_index_ < 0 ||
              t_source_index_ >=
                  (int)this_smv_.market_update_info_.asklevels_.size()) {  // invalid history for the source index

            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_ =
                (int)(new_toplevel_int_px_ + (int)this_initial_base_index_) -
                (int)(current_array_index_);  // we can set the int prices since that is of course the basis of this
                                              // map.
#ifdef SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
            // we can set limit_price as well obviously if we can set limit_int_price_
            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_price_ =
                this_smv_.GetDoublePx(this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_);
#endif  // SET_LIMIT_PRICE_ALONG_WITH_LIMIT_INT_PRICE
            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_size_ = 0;
          } else {
            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_int_price_ =
                this_smv_.market_update_info_.asklevels_[t_source_index_].limit_int_price_;
            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_price_ =
                this_smv_.market_update_info_.asklevels_[t_source_index_].limit_price_;

            this_smv_.market_update_info_.asklevels_[current_array_index_].limit_size_ =
                this_smv_.market_update_info_.asklevels_[t_source_index_].limit_size_;
          }
        }

        // update base_ask_index_ since whatever was at base_ask_index_ earlier now is ins base_ask_index_ +
        // diff_indices_
        this_smv_.base_ask_index_ = (int)this_smv_.base_ask_index_ + (int)diff_indices_;

        // This will be the new adjustment ( to go from int_price to the array index )
        this_smv_.base_ask_intprice_index_adjustment_ =
            (new_toplevel_int_px_ + this_initial_base_index_);  // note that this is '+' not '-' like bidside

      } break;
      default: {}
    }
  }
};
}
