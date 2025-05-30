// 1. henceforth ignore effects of trade masks and
// 2. later if we start handling self executions, effect of last self execution can be ingored as well
//          (note that in EUREX updates are sent to public stream before they are sent to private stream and hence
//          looking at self executions should not be useful)
void SecurityMarketView::AdjustBestAskVariablesForPOM() {
  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  return;

  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ = market_update_info_;

    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_ask_int_price_ = current_best_level_.best_ask_int_price_;
    last_best_level_.best_ask_size_ = current_best_level_.best_ask_size_;
    last_best_level_.best_ask_orders_ = current_best_level_.best_ask_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ &&
      (market_update_info_.asklevels_[base_ask_index_].limit_int_price_ == self_best_bid_ask_.best_ask_int_price_)) {
    int t_non_self_best_ask_size_ =
        (market_update_info_.asklevels_[base_ask_index_].limit_size_ - self_best_bid_ask_.best_ask_size_);
    int t_non_self_best_ask_order_count_ =
        (market_update_info_.asklevels_[base_ask_index_].limit_ordercount_ - self_best_bid_ask_.best_ask_orders_);

    if (t_non_self_best_ask_size_ <= 0) {  // if apart from us no one is at this level
      // set bestlevel to the second level
      market_update_info_.bestask_price_ = ((asklevels_size() > 1 && ask_price(1) > kInvalidPrice)
                                                ? ask_price(1)
                                                : market_update_info_.asklevels_[base_ask_index_].limit_price_);
      market_update_info_.bestask_size_ =
          ((asklevels_size() > 1 && ask_size(1) > 0) ? ask_size(1)
                                                     : market_update_info_.asklevels_[base_ask_index_].limit_size_);
      market_update_info_.bestask_int_price_ = ((asklevels_size() > 1 && ask_int_price(1) > kInvalidIntPrice)
                                                    ? ask_int_price(1)
                                                    : market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
      market_update_info_.bestask_ordercount_ =
          ((asklevels_size() > 1 && ask_order(1) > 0)
               ? ask_order(1)
               : market_update_info_.asklevels_[base_ask_index_].limit_ordercount_);
    } else {  // if there are other orders at this level aprt from us
      // assign level 0 with reduced size
      market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
      market_update_info_.bestask_size_ = t_non_self_best_ask_size_;
      market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      market_update_info_.bestask_ordercount_ = (t_non_self_best_ask_order_count_ > 0)
                                                    ? t_non_self_best_ask_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
    }
  } else {  // no need to care about self orders
    market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
    market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
    market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
    market_update_info_.bestask_ordercount_ = market_update_info_.asklevels_[base_ask_index_].limit_ordercount_;
  }

  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) &&
      market_update_info_.bidlevels_.size() >= 1 && market_update_info_.asklevels_.size() >= 1) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_
                                << " MKT : " << market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_size_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ << " - "
                                << market_update_info_.asklevels_[base_ask_index_].limit_int_price_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_size_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_ordercount_
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;
    dump_non_self_smv_on_next_update_ = false;
  }

  if (!is_ready_) {
    // DBGLOG_TIME_CLASS_FUNC_LINE << " "
    //            << market_update_info_.ToString() << " "
    //            << "ArePricesComputed " << ArePricesComputed()
    //            << DBGLOG_ENDL_FLUSH;

    if ((market_update_info_.bidlevels_.size() > 0 && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        (market_update_info_.asklevels_.size() > 0 && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  } else {
    if ((market_update_info_.bidlevels_.size() > 0 && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        (market_update_info_.asklevels_.size() > 0 && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
    } else
      is_ready_ = false;
  }
}

void SecurityMarketView::AdjustBestAskVariablesForPOMIndexedEPLMV() {
  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ = market_update_info_;

    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_ask_int_price_ = current_best_level_.best_ask_int_price_;
    last_best_level_.best_ask_size_ = current_best_level_.best_ask_size_;
    last_best_level_.best_ask_orders_ = current_best_level_.best_ask_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ &&
      (market_update_info_.asklevels_[base_ask_index_].limit_int_price_ == self_best_bid_ask_.best_ask_int_price_)) {
    int t_non_self_best_ask_size_ =
        (market_update_info_.asklevels_[base_ask_index_].limit_size_ - self_best_bid_ask_.best_ask_size_);
    int t_non_self_best_ask_order_count_ =
        (market_update_info_.asklevels_[base_ask_index_].limit_ordercount_ - self_best_bid_ask_.best_ask_orders_);

    if (t_non_self_best_ask_size_ <= 0) {  // if apart from us no one is at this level
      // set bestlevel to the second level
      market_update_info_.bestask_price_ = ((asklevels_size() > 1 && ask_price(1) > kInvalidPrice)
                                                ? ask_price(1)
                                                : market_update_info_.asklevels_[base_ask_index_].limit_price_);
      market_update_info_.bestask_size_ =
          ((asklevels_size() > 1 && ask_size(1) > 0) ? ask_size(1)
                                                     : market_update_info_.asklevels_[base_ask_index_].limit_size_);
      market_update_info_.bestask_int_price_ = ((asklevels_size() > 1 && ask_int_price(1) > kInvalidIntPrice)
                                                    ? ask_int_price(1)
                                                    : market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
      market_update_info_.bestask_ordercount_ =
          ((asklevels_size() > 1 && ask_order(1) > 0)
               ? ask_order(1)
               : market_update_info_.asklevels_[base_ask_index_].limit_ordercount_);
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_size_ =
            ((asklevels_size() > 1 && ask_size(1) > 0) ? ask_size(1)
                                                       : market_update_info_.asklevels_[base_ask_index_].limit_size_);
        hybrid_market_update_info_.bestask_int_price_ =
            ((asklevels_size() > 1 && ask_int_price(1) > kInvalidIntPrice)
                 ? ask_int_price(1)
                 : market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
        hybrid_market_update_info_.bestask_ordercount_ =
            ((asklevels_size() > 1 && ask_order(1) > 0)
                 ? ask_order(1)
                 : market_update_info_.asklevels_[base_ask_index_].limit_ordercount_);
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[hybrid_market_update_info_.bestask_int_price_];
      }
    } else {  // if there are other orders at this level aprt from us
      // assign level 0 with reduced size
      market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
      market_update_info_.bestask_size_ = t_non_self_best_ask_size_;
      market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      market_update_info_.bestask_ordercount_ = (t_non_self_best_ask_order_count_ > 0)
                                                    ? t_non_self_best_ask_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_size_ = t_non_self_best_ask_size_;
        hybrid_market_update_info_.bestask_int_price_ =
            market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
        hybrid_market_update_info_.bestask_ordercount_ = (t_non_self_best_ask_order_count_ > 0)
                                                             ? t_non_self_best_ask_order_count_
                                                             : 1;  // Transient bad cases reduce order_count to 0.
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[hybrid_market_update_info_.bestask_int_price_];
      }
    }
  } else {  // no need to care about self orders
    market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
    market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
    market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
    market_update_info_.bestask_ordercount_ = market_update_info_.asklevels_[base_ask_index_].limit_ordercount_;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
      hybrid_market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      hybrid_market_update_info_.bestask_ordercount_ =
          market_update_info_.asklevels_[base_ask_index_].limit_ordercount_;
      hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[hybrid_market_update_info_.bestask_int_price_];
    }
  }

  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) &&
      market_update_info_.bidlevels_.size() >= 1 && market_update_info_.asklevels_.size() >= 1) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_
                                << " MKT : " << market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_size_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ << " - "
                                << market_update_info_.asklevels_[base_ask_index_].limit_int_price_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_size_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_ordercount_
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;
    dump_non_self_smv_on_next_update_ = false;
  }

  if (!is_ready_) {
    // DBGLOG_TIME_CLASS_FUNC_LINE << " "
    //            << market_update_info_.ToString() << " "
    //            << "ArePricesComputed " << ArePricesComputed()
    //            << DBGLOG_ENDL_FLUSH;

    if ((market_update_info_.bidlevels_.size() > 0 && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        (market_update_info_.asklevels_.size() > 0 && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  } else {
    if ((market_update_info_.bidlevels_.size() > 0 && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        (market_update_info_.asklevels_.size() > 0 && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
    } else
      is_ready_ = false;
  }
}

/// 1. henceforth ignore effects of trade masks and
/// 2. later if we start handling self executions, effect of last self execution can be ingored as well
///          (note that in EUREX updates are sent to public stream before they are sent to private stream and hence
///          looking at self executions should not be useful)
void SecurityMarketView::AdjustBestBidVariablesForPOM() {
  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  return;

  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ = market_update_info_;

    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_bid_int_price_ = current_best_level_.best_bid_int_price_;
    last_best_level_.best_bid_size_ = current_best_level_.best_bid_size_;
    last_best_level_.best_bid_orders_ = current_best_level_.best_bid_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ &&
      (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ == self_best_bid_ask_.best_bid_int_price_)) {
    int t_non_self_best_bid_size_ =
        (market_update_info_.bidlevels_[base_bid_index_].limit_size_ - self_best_bid_ask_.best_bid_size_);
    int t_non_self_best_bid_order_count_ =
        (market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ - self_best_bid_ask_.best_bid_orders_);

    if (t_non_self_best_bid_size_ <= 0) {
      // remove level
      // TODO .. remove reference to bidlevels_size()
      market_update_info_.bestbid_price_ = ((bidlevels_size() > 1 && bid_price(1) > kInvalidPrice)
                                                ? bid_price(1)
                                                : market_update_info_.bidlevels_[base_bid_index_].limit_price_);
      market_update_info_.bestbid_size_ =
          ((bidlevels_size() > 1 && bid_size(1) > 0) ? bid_size(1)
                                                     : market_update_info_.bidlevels_[base_bid_index_].limit_size_);
      market_update_info_.bestbid_int_price_ = ((bidlevels_size() > 1 && bid_int_price(1) > kInvalidIntPrice)
                                                    ? bid_int_price(1)
                                                    : market_update_info_.bidlevels_[base_bid_index_].limit_int_price_);
      market_update_info_.bestbid_ordercount_ =
          ((bidlevels_size() > 1 && bid_order(1) > 0)
               ? bid_order(1)
               : market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_);
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_size_ =
            ((bidlevels_size() > 1 && bid_size(1) > 0) ? bid_size(1)
                                                       : market_update_info_.bidlevels_[base_bid_index_].limit_size_);
        hybrid_market_update_info_.bestbid_int_price_ =
            ((bidlevels_size() > 1 && bid_int_price(1) > kInvalidIntPrice)
                 ? bid_int_price(1)
                 : market_update_info_.bidlevels_[base_bid_index_].limit_int_price_);
        hybrid_market_update_info_.bestbid_ordercount_ =
            ((bidlevels_size() > 1 && bid_order(1) > 0)
                 ? bid_order(1)
                 : market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_);
        market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
      }
    } else {
      // assign level 0 with reduced size
      market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
      market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
      market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
      market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                    ? t_non_self_best_bid_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
        hybrid_market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
        hybrid_market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                             ? t_non_self_best_bid_order_count_
                                                             : 1;  // Transient bad cases reduce order_count to 0.
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
      }
    }
  } else  // Full mkt.
  {
    market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
    market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
    market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
    market_update_info_.bestbid_ordercount_ = market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
      hybrid_market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
      hybrid_market_update_info_.bestbid_ordercount_ =
          market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_;
      hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
    }
  }

  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) && (!IsBidBookEmpty()) &&
      (!IsAskBookEmpty())) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_
                                << " MKT : " << market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_size_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ << " - "
                                << market_update_info_.asklevels_[base_ask_index_].limit_int_price_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_size_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_ordercount_
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;

    dump_non_self_smv_on_next_update_ = false;
  }

  // assign is_ready_
  if (!is_ready_) {
    if (((!IsBidBookEmpty()) && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        ((!IsAskBookEmpty()) && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  } else {
    if (((!IsBidBookEmpty()) && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        ((!IsAskBookEmpty()) && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
    } else
      is_ready_ = false;
  }
}

void SecurityMarketView::AdjustBestBidVariablesForPOMIndexedEPLMV() {
  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  // the following block sets the variable self_best_bid_ask_
  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ =
        market_update_info_;  // copies over the variables like t_market_update_info_.bidlevels_[0].limit_int_price_
    // Note it copies over int price and not price
    // also it copies over the array values and not bestbid_int_price_

    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_bid_int_price_ = current_best_level_.best_bid_int_price_;
    last_best_level_.best_bid_size_ = current_best_level_.best_bid_size_;
    last_best_level_.best_bid_orders_ = current_best_level_.best_bid_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ &&
      (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ == self_best_bid_ask_.best_bid_int_price_)) {
    int t_non_self_best_bid_size_ =
        (market_update_info_.bidlevels_[base_bid_index_].limit_size_ - self_best_bid_ask_.best_bid_size_);
    int t_non_self_best_bid_order_count_ =
        (market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ - self_best_bid_ask_.best_bid_orders_);

    if (t_non_self_best_bid_size_ <= 0) {
      // remove level
      // TODO .. remove reference to bidlevels_size()
      market_update_info_.bestbid_price_ = ((bidlevels_size() > 1 && bid_price(1) > kInvalidPrice)
                                                ? bid_price(1)
                                                : market_update_info_.bidlevels_[base_bid_index_].limit_price_);
      market_update_info_.bestbid_size_ =
          ((bidlevels_size() > 1 && bid_size(1) > 0) ? bid_size(1)
                                                     : market_update_info_.bidlevels_[base_bid_index_].limit_size_);
      market_update_info_.bestbid_int_price_ = ((bidlevels_size() > 1 && bid_int_price(1) > kInvalidIntPrice)
                                                    ? bid_int_price(1)
                                                    : market_update_info_.bidlevels_[base_bid_index_].limit_int_price_);
      market_update_info_.bestbid_ordercount_ =
          ((bidlevels_size() > 1 && bid_order(1) > 0)
               ? bid_order(1)
               : market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_);
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_size_ =
            ((bidlevels_size() > 1 && bid_size(1) > 0) ? bid_size(1)
                                                       : market_update_info_.bidlevels_[base_bid_index_].limit_size_);
        hybrid_market_update_info_.bestbid_int_price_ =
            ((bidlevels_size() > 1 && bid_int_price(1) > kInvalidIntPrice)
                 ? bid_int_price(1)
                 : market_update_info_.bidlevels_[base_bid_index_].limit_int_price_);
        hybrid_market_update_info_.bestbid_ordercount_ =
            ((bidlevels_size() > 1 && bid_order(1) > 0)
                 ? bid_order(1)
                 : market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_);
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
      }
    } else {
      // assign level 0 with reduced size
      market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
      market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
      market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
      market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                    ? t_non_self_best_bid_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
        hybrid_market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
        hybrid_market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                             ? t_non_self_best_bid_order_count_
                                                             : 1;  // Transient bad cases reduce order_count to 0.
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
      }
    }
  } else  // Full mkt.
  {
    market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
    market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
    market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
    market_update_info_.bestbid_ordercount_ = market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
      hybrid_market_update_info_.bestbid_int_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_int_price_;
      hybrid_market_update_info_.bestbid_ordercount_ =
          market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_;
      hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
    }
  }

  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) && (!IsBidBookEmpty()) &&
      (!IsAskBookEmpty())) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_
                                << " MKT : " << market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_size_ << " "
                                << market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ << " - "
                                << market_update_info_.asklevels_[base_ask_index_].limit_int_price_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_size_ << " "
                                << market_update_info_.asklevels_[base_ask_index_].limit_ordercount_
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;

    dump_non_self_smv_on_next_update_ = false;
  }

  // assign is_ready_
  if (!is_ready_) {
    if (((!IsBidBookEmpty()) && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        ((!IsAskBookEmpty()) && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  } else {
    if (((!IsBidBookEmpty()) && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        ((!IsAskBookEmpty()) && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
    } else
      is_ready_ = false;
  }
}

void SecurityMarketView::CopyOrderLevelDepthBookOnBid(int t_level_added_, int index_to_copy_) {
  /*As this fn is called after the price level Book is Updated we check for the case that if index_to_copy is after
   * the current added level
   * then in the order book that index would correspond to index-1 in the order Book
   */

  // dbglogger_ << "Copy Book on Bid Level Added " << t_level_added_ << " Index to copy " << index_to_copy_ << "
  // Size
  // of book  " << market_update_info_.bid_level_order_depth_book_.size() <<"\n";
  // dbglogger_.DumpCurrentBuffer();
  if (index_to_copy_ > t_level_added_ - 1) {
    std::vector<MarketOrder *> new_order_vec_;
    new_order_vec_ = market_update_info_.bid_level_order_depth_book_[index_to_copy_ - 1];

    market_update_info_.bid_level_order_depth_book_.insert(
        market_update_info_.bid_level_order_depth_book_.begin() + t_level_added_ - 1, new_order_vec_);
    if (market_update_info_.bid_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
      market_update_info_.bid_level_order_depth_book_.pop_back();
    }
  } else {
    std::vector<MarketOrder *> new_order_vec_;
    new_order_vec_ = market_update_info_.bid_level_order_depth_book_[index_to_copy_];

    market_update_info_.bid_level_order_depth_book_.insert(
        market_update_info_.bid_level_order_depth_book_.begin() + t_level_added_ - 1, new_order_vec_);
    if (market_update_info_.bid_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
      market_update_info_.bid_level_order_depth_book_.pop_back();
    }
  }
}

void SecurityMarketView::CopyOrderLevelDepthBookOnAsk(int t_level_added_, int index_to_copy_) {
  /*As this fn is called after the price level Book is Updated we check for the case that if index_to_copy is after
   * the current added level
   * then in the order book that index would correspond to index-1 in the order Book
   */

  if (index_to_copy_ > t_level_added_ - 1) {
    std::vector<MarketOrder *> new_order_vec_;
    new_order_vec_ = market_update_info_.ask_level_order_depth_book_[index_to_copy_ - 1];

    market_update_info_.ask_level_order_depth_book_.insert(
        market_update_info_.ask_level_order_depth_book_.begin() + t_level_added_ - 1,
        new_order_vec_);  // complexity .. O (number of elements after t_iter_)
    if (market_update_info_.ask_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
      market_update_info_.ask_level_order_depth_book_.pop_back();
    }
  } else {
    std::vector<MarketOrder *> new_order_vec_;
    new_order_vec_ = market_update_info_.ask_level_order_depth_book_[index_to_copy_];

    market_update_info_.ask_level_order_depth_book_.insert(
        market_update_info_.ask_level_order_depth_book_.begin() + t_level_added_ - 1,
        new_order_vec_);  // complexity .. O (number of elements after t_iter_)
    if (market_update_info_.ask_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
      market_update_info_.ask_level_order_depth_book_.pop_back();
    }
  }
}

void SecurityMarketView::UpdateAskOrderLevelBook(int t_level_changed_, MarketOrder *p_market_order_,
                                                 bool trade_delta_packet_) {
  // int seed = (int)GetCpucycleCount();
  // srand(seed);
  int delta_order_count_ =
      (p_market_order_->order_count_ > 0 ? p_market_order_->order_count_ : -1 * p_market_order_->order_count_);
  int delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);

  // if(trade_delta_packet_ == true && delta_size_ != last_traded_size_)
  //   {
  //     ////dbglogger_ << "Trade Delta Mismatch \n";
  //   }

  /*Forcefully deleting from top because last message was trade and the next delta packet does not correspond to
   * that
   * trade message.
   * Delete traded size from the Top and adjust the remaining Delta by the normal Procedure that is Followed below.
   */
  if (trade_delta_packet_ == true && delta_size_ != last_traded_size_ &&
      last_traded_size_ <= market_update_info_.asklevels_[t_level_changed_ - 1].limit_size_) {
    ////dbglogger_ << "Delta Size and Trade Delta Mismatch at Ask. Delta Size = " << p_market_order_->size_<< "
    /// Delta
    /// Order Count = "<< p_market_order_->order_count_ << " Last Trade Size = " << last_traded_size_ << "\n";
    int size_to_delete_from_front_ = last_traded_size_;
    int order_count_deleted_ = 0;
    while (size_to_delete_from_front_ > 0) {
      // Deleting Entire Order Level because it got Traded
      if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ <=
          size_to_delete_from_front_) {
        order_count_deleted_ += market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_;
        size_to_delete_from_front_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin());
      } else {
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= size_to_delete_from_front_;
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ >
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]
                ->size_) {  // If order Count Becomes larger than Size then we make order count equal to the
                            // remaining
                            // Size At that Level
          order_count_deleted_ +=
              (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ -
               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
        } else {  // Dividing the Order Count Proportionately.
          int propotionate_oc_ =
              (size_to_delete_from_front_ *
               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_) /
              (size_to_delete_from_front_ +
               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ -= propotionate_oc_;
          order_count_deleted_ += propotionate_oc_;
        }
        size_to_delete_from_front_ = 0;
      }
    }
    // Recalculating Delta Order Count and Size so that it can be adjusted by the normal Procedure
    p_market_order_->order_count_ += order_count_deleted_;
    p_market_order_->size_ += last_traded_size_;
    delta_order_count_ =
        (p_market_order_->order_count_ > 0 ? p_market_order_->order_count_ : -1 * p_market_order_->order_count_);
    delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);
    trade_delta_packet_ = false;
  }
  /*In all the cases below we have two options of performing changes
   * 1. Start Deleting/Adding from top of the Order Level Depth Book
   * 2. Start Deleting/Adding from bottom of the Order Level Depth Book
   * We use a Probability to determine whether to go from Top or Bottom . In case of Trades we always go from Top .
   * The difference in adding/deleting at top or bottom is that it affects the estimate of QSA computed by Order
   * Level
   * SIM
   */

  if (p_market_order_->order_count_ > 0 &&
      p_market_order_->size_ > 0) {  // Simply inserting a new entry in the order level Book .
    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
  } else if (p_market_order_->order_count_ < 0 && p_market_order_->size_ >= 0) {
    /*Here more orders of smaller size have been deleted and lesser orders of larger sizes have come in the market .
     * In this case we independently try to adjust the values of delta size and delta ordercount in the Order Book
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (rand_front_ <= prob_modify_from_top_) {  // Starting to Modifying from Front
      int index_ = 0;
      for (index_ = 0; index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
           index_++) {
        if (delta_order_count_ <
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              delta_order_count_;
          delta_order_count_ = 0;
          break;
        } else if (delta_order_count_ >=
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_order_count_ -=
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          delta_size_ += market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          index_--;
        }
        if (delta_order_count_ == 0) break;
      }
      index_ = 0;
      // Probabilistically Trying To add the delta Size to any of the already Existing Entries
      while (index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size()) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
          break;
        } else {
          index_++;
        }
      }
      if (index_ == (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() &&
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ += delta_size_;
      }
    } else {  // Modifying from Back
      int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      for (; index_ >= 0; index_--) {
        if (delta_order_count_ <
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              delta_order_count_;
          delta_order_count_ = 0;
          break;
        } else if (delta_order_count_ >=
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_order_count_ -=
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          delta_size_ += market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
        }
        if (delta_order_count_ == 0) break;
      }
      index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;

      while (index_ >= 0) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
          break;
        } else {
          index_--;
        }
      }
      if (index_ < 0 && market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ += delta_size_;
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  } else if (p_market_order_->order_count_ > 0 && p_market_order_->size_ <= 0) {
    /*Here less orders of larger size have been deleted and more orders of smaller sizes have come in the market .
     * In this case we independently try to adjust the values of delta size and delta ordercount in the Order Book
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (rand_front_ <= prob_modify_from_top_) {
      int index_ = 0;
      while (index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size()) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ +=
              delta_order_count_;
          // Making Sure that at Each Entry in the Order Book Size >= Count
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_size_ +=
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                 market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          }
          break;
        } else {
          index_++;
        }
      }  // This is to ensure that If by applying Probability we fail in all the above cases then we modify the last
         // entry of the book .
      if (index_ == (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() &&
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ +=
            delta_order_count_;
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ >
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_) {
          delta_size_ +=
              (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ -
               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_;
        }
      }
      index_ = 0;
      for (index_ = 0; index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
           index_++) {
        if (delta_size_ <=
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
          delta_size_ = 0;
          break;
        } else if (delta_size_ >
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_size_ -= (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_size_ == 0) break;
      }
    } else {
      // Modifying the Order Book From Back
      int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      // Adjusting Order Count
      while (index_ >= 0) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ +=
              delta_order_count_;
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_size_ +=
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                 market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          }
          break;
        } else {
          index_--;
        }
      }
      // Adjusting Size
      if (index_ < 0 && market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ +=
            delta_order_count_;
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ >
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_) {
          delta_size_ +=
              (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ -
               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_;
        }
      }
      index_ = (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      ;
      for (; index_ >= 0; index_--) {
        if (delta_size_ <=
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
          delta_size_ = 0;
          break;
        } else if (delta_size_ >
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_size_ -= (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_size_ == 0) break;
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  }

  else if (p_market_order_->order_count_ == 0)  // when order count =0
  {
    ////dbglogger_ << "here in ask  order count 0 with " << p_market_order_->size_ << " \n";
    bool flag_ = false;
    if (p_market_order_->size_ > 0) {
      int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

      if (rand_front_ <= prob_modify_from_top_) {
        int index_ = 0;
        while (index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size()) {
          // Probabilistically trying to update the size of an Order Level Entry
          int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
          if (rand_ <= prob_modify_level_) {
            flag_ = true;
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ ==
                1) {  // If the size of an entry is increased then we update its time and move it down in the order
                      // level Book
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                  watch_.tv();
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              index_--;
              (*market_orders_mempool_).DeAlloc(p_market_order_);
            } else {
              // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
              // existing order size between these orders .
              int toDeduct =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              p_market_order_->order_count_ = 1;
              p_market_order_->size_ += toDeduct;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
            }

            break;
          } else {
            index_++;
          }
        }
        if (flag_ == false && (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_].size() > index_) {
          index_--;
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                watch_.tv();
            (*market_orders_mempool_).DeAlloc(p_market_order_);
            // No need to shift as it is the bottom most entry
          } else {
            // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
            // existing
            // order size between these orders .
            int toDeduct = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                           market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
            p_market_order_->order_count_ = 1;
            p_market_order_->size_ += toDeduct;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
          }
        }
      } else {  // Modifications Being Performed at the Bottom Of The Order Level Book
        int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        while (index_ >= 0) {
          // Probabilistically trying to update the size of an Order Level Entry
          int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
          if (rand_ <= prob_modify_level_) {
            flag_ = true;
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
              // If the size of an entry is increased then we update its time and move it down in the order level
              // Book
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                  watch_.tv();
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              index_--;
              (*market_orders_mempool_).DeAlloc(p_market_order_);
            } else {
              // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
              // existing order size between these orders .
              int toDeduct =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              p_market_order_->order_count_ = 1;
              p_market_order_->size_ += toDeduct;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
            }

            break;
          } else {
            index_--;
          }
        }
        // If the Probabilistic Approach fails then The last entry is updated
        if (flag_ == false && market_update_info_.ask_level_order_depth_book_[t_level_changed_].size() > 0) {
          index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                watch_.tv();
            (*market_orders_mempool_).DeAlloc(p_market_order_);
          } else {
            // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
            // existing
            // order size between these orders .
            int toDeduct = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                           market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
            p_market_order_->order_count_ = 1;
            p_market_order_->size_ += toDeduct;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
          }
        }
      }
    } else if (p_market_order_->size_ < 0) {
      int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
      if (rand_front_ <= prob_modify_from_top_ || trade_delta_packet_ == true) {
        int index_ = 0;

        if (trade_delta_packet_ == false) {
          while (index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size()) {
            /*looking for an order entry in the order depth book that can afford to completely delete this size .
             * e.g and entry with size 5 and count 2 cannot afford to delete a size 4 and order count 0
             */
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <
                (delta_size_)) {
              index_++;
              continue;
            }

            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

            if (rand_ <= prob_modify_level_) {
              ////dbglogger_<<"dELETEING FROM HERE THAT IS randome one \n";

              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;

              break;
            } else {
              index_++;
            }
          }
        }
        // we want to delete forcefully from top only if above heuristic fail or if last message was trade
        if ((index_ == (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() ||
             trade_delta_packet_ == true) &&
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          ////dbglogger_<<"dELETEING FROM HERE THAT IS non randome one \n";
          for (int i = 0; i < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); i++) {
            if (delta_size_ - (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_) >
                0) {
              delta_size_ = delta_size_ -
                            (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                             market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_;
            } else {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -= delta_size_;

              delta_size_ = 0;
              break;
            }
          }
        }
      } else  // delete from non front
      {
        int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        if (trade_delta_packet_ == false) {
          /*looking for an order entry in the order depth book that can afford to completely delete this size .
           * e.g and entry with size 5 and count 2 cannot afford to delete a size 4 and order count 0
           */
          while (index_ >= 0) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <
                (delta_size_)) {
              index_--;
              continue;
            }

            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

            if (rand_ <= prob_modify_level_) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;

              break;
            } else {
              index_--;
            }
          }
        }
        // we want to delete forcefully from Bottom only if above heuristic fails
        if ((index_ == -1) && market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          ////dbglogger_<<"dELETEING FROM HERE THAT IS non randome one \n";
          for (int i = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1; i >= 0; i--) {
            if (delta_size_ - (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                               market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_) >
                0) {
              delta_size_ = delta_size_ -
                            (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                             market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_;
            } else {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -= delta_size_;

              delta_size_ = 0;
              break;
            }
          }
        }
      }
      (*market_orders_mempool_).DeAlloc(p_market_order_);
    }
  } else if (p_market_order_->order_count_ == -1 && p_market_order_->size_ < 0) {
    int index_ = 0;
    bool deleted_ = false;
    //////dbglogger_<< " SingleDelete\n";

    /*
     * Checking performance of Naive book vs Approximate Book
     *
         int naive_delete_ = market_update_info_.asklevels_[t_level_changed_ -1].limit_size_ /
     market_update_info_.asklevels_[t_level_changed_ -1].limit_ordercount_;

         while(index_ < market_update_info_.ask_level_order_depth_book_[t_level_changed_-1].size() )
         {
         if(market_update_info_.ask_level_order_depth_book_[t_level_changed_-1][index_]->size_ == naive_delete_ &&
     market_update_info_.ask_level_order_depth_book_[t_level_changed_-1][index_]->order_count_ == 1)
         {
         //////dbglogger_<< "NaiveDeleteMatch\n";
         break;
         }
         else
         {
         index_++;
         }
         }*/

    index_ = 0;
    if (trade_delta_packet_ ==
        false) {  // trying to find an exact match if the current delta does not correspond to Trade .
      index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      while (index_ >= 0) {
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ == delta_size_ &&
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          deleted_ = true;
          //////dbglogger_<< "ApproxDeleteMatch\n";
          break;
        } else {
          index_--;
        }
      }
    }
    // we want to delete forcefully from top only if above heuristic fail or if last message was trade
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if ((rand_front_ <= prob_modify_from_top_ || trade_delta_packet_ == true) && deleted_ == false) {
      if ((deleted_ == false || trade_delta_packet_ == true) &&
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        // probabilistic approach that if any of the entries present can afford to delete this delta packet
        index_ = 0;
        if (trade_delta_packet_ == false) {
          while (index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size()) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                         1) <
                    delta_size_ ||
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <= 1) {
              index_++;
              continue;
            }
            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
            if (rand_ <= prob_modify_level_) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              delta_order_count_ = 0;
              delta_size_ = 0;
              deleted_ = true;
              break;
            } else {
              index_++;
            }
          }
        }
        /*If there was no exact match and neither of the entries can afford to delete this delta then we start
         * pruning
         * from Top ot the order level Book
         * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta
         * Order
         * Count or Delta Order Size is not equal to 0 .
         * While pruning we try to maintain that the level from which deletion is being performed should have order
         * count <= the size
         */
        if ((deleted_ == false || trade_delta_packet_ == true) &&
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          for (int index_ = 0;
               index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() &&
                   delta_size_ > 0;
               index_++) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                    delta_order_count_) {
              if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                      delta_order_count_) {
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                delta_size_ = 0;
                break;
              } else {
                // Ensuring that Order Count <= Size
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_size_ -=
                    (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                     market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_order_count_ = 0;
                break;
              }
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
              if (delta_order_count_ -
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                  delta_size_ - market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
                delta_order_count_ =
                    delta_order_count_ -
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                index_--;
              } else {
                delta_order_count_ =
                    delta_order_count_ -
                    (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                    (delta_size_ - delta_order_count_);
                delta_size_ = delta_order_count_;
              }
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              delta_order_count_ =
                  delta_order_count_ -
                  (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                  (delta_size_ - delta_order_count_);
              delta_size_ = delta_order_count_;
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                           delta_order_count_) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                  (delta_order_count_ - 1);
              delta_order_count_ = 1;
              delta_size_ =
                  delta_size_ -
                  (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            }
            if (delta_order_count_ <= 0 || delta_size_ <= 0) {
              break;
            }
          }

          // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is
          // true
          // we handle them independently
          // Also adjusting Delta OC will lead to changes in Delta Size
          if (delta_order_count_ > 0) {
            for (int index_ = 0;
                 index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); index_++) {
              if (delta_order_count_ <
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                break;
              } else if (delta_order_count_ >=
                         market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                delta_order_count_ -=
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                index_--;
              }
              if (delta_order_count_ == 0) break;
            }
          }

          if (delta_size_ != 0) {
            if (delta_size_ < 0) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ += (-1 * delta_size_);
            } else {
              for (int index_ = 0;
                   index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
                   index_++) {
                if (delta_size_ == 0) {
                  break;
                } else if (delta_size_ > 0) {
                  if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                      0) {
                    if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]
                                ->order_count_ >=
                        delta_size_) {
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                          delta_size_;
                      delta_size_ = 0;
                    } else {
                      delta_size_ -=
                          (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                           market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } else if (deleted_ == false) {
      if (deleted_ == false && market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        // probabilistic approach that if any of the entries present can afford to delete this delta packet
        index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        if (trade_delta_packet_ == false) {  //  If Trade delta Packet is true then we will prune from the top
          while (index_ >= 0) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                         1) <
                    delta_size_ ||
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <= 1) {
              index_--;
              continue;
            }
            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
            if (rand_ <= prob_modify_level_) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              delta_order_count_ = 0;
              delta_size_ = 0;

              break;
            } else {
              index_--;
            }
          }
        }

        /*If there was no exact match and neither of the entries can afford to delete this delta then we start
         * pruning
         * from Top ot the order level Book
         * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta
         * Order
         * Count or Delta Order Size is not equal to 0 .
         * While pruning we try to maintain that the level from which deletion is being performed should have order
         * count <= the size
         */
        if (index_ == -1 && market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
               index_ >= 0 && delta_size_ > 0; index_--) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                    delta_order_count_) {
              if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                      delta_order_count_) {
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                delta_size_ = 0;
                break;
              } else {
                // Ensuring that Order Count <= Size
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_size_ -=
                    (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                     market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_order_count_ = 0;
                break;
              }
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
              if (delta_order_count_ -
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                  delta_size_ - market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
                delta_order_count_ =
                    delta_order_count_ -
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              } else {
                delta_order_count_ =
                    delta_order_count_ -
                    (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                    (delta_size_ - delta_order_count_);
                delta_size_ = delta_order_count_;
              }
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              delta_order_count_ =
                  delta_order_count_ -
                  (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                  (delta_size_ - delta_order_count_);
              delta_size_ = delta_order_count_;
            } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                           delta_order_count_) {
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                  (delta_order_count_ - 1);
              delta_order_count_ = 1;
              delta_size_ =
                  delta_size_ -
                  (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            }
            if (delta_order_count_ <= 0 || delta_size_ <= 0) {
              break;
            }
          }
          // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is
          // true
          // we handle them independently
          // Also adjusting Delta OC will lead to changes in Delta Size
          if (delta_order_count_ > 0) {
            for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
                 index_ >= 0; index_--) {
              if (delta_order_count_ <
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                break;
              } else if (delta_order_count_ >=
                         market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                delta_order_count_ -=
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                // index--_;
              }
              if (delta_order_count_ == 0) break;
            }
          }

          if (delta_size_ != 0) {
            if (delta_size_ < 0) {
              market_update_info_.ask_level_order_depth_book_
                  [t_level_changed_ - 1][market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() -
                                         1]->size_ += (-1 * delta_size_);
            } else {
              for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
                   index_ >= 0; index_--) {
                if (delta_size_ == 0) {
                  break;
                } else if (delta_size_ > 0) {
                  if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                      0) {
                    if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]
                                ->order_count_ >=
                        delta_size_) {
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                          delta_size_;
                      delta_size_ = 0;
                    } else {
                      delta_size_ -=
                          (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                           market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  } else if (p_market_order_->order_count_ < 0 &&
             p_market_order_->size_ < 0)  // delta order count <-1 and delta size < 0
  {
    /* In this case we only do Pruning .
     * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta Order
     * Count or Delta Order Size is not equal to 0 .
     * While pruning we try to maintain that the level from which deletion is being performed should have order
     * count
     * <= the size
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (trade_delta_packet_ == true || rand_front_ <= prob_modify_from_top_) {
      // If last packet was trade packet corresponding to this level then modifications are performed from the top
      for (int index_ = 0; index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() &&
                               delta_size_ > 0;
           index_++) {
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                delta_order_count_) {
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                  delta_order_count_) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            delta_size_ = 0;
            break;
          } else {
            // Ensuring that Order Count <= Size
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_size_ -=
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                 market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_order_count_ = 0;
            break;
          }
        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
          if (delta_order_count_ -
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
              delta_size_ - market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_order_count_ =
                delta_order_count_ -
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            index_--;
          } else {
            // dbglogger_<< "Entering this area  3\n";
            delta_order_count_ =
                delta_order_count_ -
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                (delta_size_ - delta_order_count_);
            delta_size_ = delta_order_count_;
          }
        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          delta_order_count_ =
              delta_order_count_ -
              (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
              (delta_size_ - delta_order_count_);
          delta_size_ = delta_order_count_;
        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                       delta_order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              (delta_order_count_ - 1);
          delta_order_count_ = 1;
          delta_size_ = delta_size_ -
                        (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                         market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_order_count_ <= 0 || delta_size_ <= 0) {
          break;
        }
      }
      // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is true
      // we
      // handle them independently
      // Also adjusting Delta OC will lead to changes in Delta Size
      if (delta_order_count_ > 0) {
        for (int index_ = 0; index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size();
             index_++) {
          if (delta_order_count_ <
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            break;
          } else if (delta_order_count_ >=
                     market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            delta_order_count_ -=
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            index_--;
          }
          if (delta_order_count_ == 0) break;
        }
      }

      if (delta_size_ != 0) {
        if (delta_size_ < 0) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ += (-1 * delta_size_);
        } else {
          for (int index_ = 0;
               index_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); index_++) {
            if (delta_size_ == 0) {
              break;
            } else if (delta_size_ > 0) {
              if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                  0) {
                if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                    delta_size_) {
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                  delta_size_ = 0;
                } else {
                  delta_size_ -=
                      (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                }
              }
            }
          }
        }
      }
    } else if (trade_delta_packet_ == false) {
      for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
           index_ >= 0 && delta_size_ > 0; index_--) {
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                delta_order_count_) {
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                  delta_order_count_) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            delta_size_ = 0;
            break;
          } else {
            // Ensuring that Order Count <= Size
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_size_ -=
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                 market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_order_count_ = 0;
            break;
          }
        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
          if (delta_order_count_ -
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
              delta_size_ - market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_order_count_ =
                delta_order_count_ -
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          } else {
            delta_order_count_ =
                delta_order_count_ -
                (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                (delta_size_ - delta_order_count_);
            delta_size_ = delta_order_count_;
          }

        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          delta_order_count_ =
              delta_order_count_ -
              (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
              (delta_size_ - delta_order_count_);
          delta_size_ = delta_order_count_;
        } else if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                       delta_order_count_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              (delta_order_count_ - 1);
          delta_order_count_ = 1;
          delta_size_ = delta_size_ -
                        (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                         market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_order_count_ <= 0 || delta_size_ <= 0) {
          break;
        }
      }
      // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is true
      // we
      // handle them independently
      // Also adjusting Delta OC will lead to changes in Delta Size

      if (delta_order_count_ > 0) {
        for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1; index_ >= 0;
             index_--) {
          if (delta_order_count_ <
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            break;
          } else if (delta_order_count_ >=
                     market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            delta_order_count_ -=
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            // index_--;
          }
          if (delta_order_count_ == 0) break;
        }
      }

      if (delta_size_ != 0) {
        if (delta_size_ < 0) {
          market_update_info_.ask_level_order_depth_book_
              [t_level_changed_ - 1][market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1]
                  ->size_ += (-1 * delta_size_);
        } else {
          for (int index_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
               index_ >= 0; index_--) {
            if (delta_size_ == 0) {
              break;
            } else if (delta_size_ > 0) {
              if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                  0) {
                if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                    delta_size_) {
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                  delta_size_ = 0;
                } else {
                  delta_size_ -=
                      (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                      market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                }
              }
            }
          }
        }
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  }
  // dbglogger_<<"\n\nEND *************\n\n";
  dbglogger_.DumpCurrentBuffer();
}

void SecurityMarketView::UpdateBidOrderLevelBook(int t_level_changed_, MarketOrder *p_market_order_,
                                                 bool trade_delta_packet_) {
  // int seed = (int)GetCpucycleCount();
  // srand(seed);
  int delta_order_count_ =
      (p_market_order_->order_count_ > 0 ? p_market_order_->order_count_ : -1 * p_market_order_->order_count_);
  int delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);

  // if(trade_delta_packet_ == true && delta_size_ != last_traded_size_)
  //   {
  ////dbglogger_ << "Trade Delta Mismatch \n";
  // }

  /*Forcefully deleting from top because last message was trade and the next delta packet does not correspond to
   * that
   * trade message.
   * Delete traded size from the Top and adjust the remaining Delta by the normal Procedure that is Followed below.
   */
  if (trade_delta_packet_ == true && delta_size_ != last_traded_size_ &&
      last_traded_size_ <= market_update_info_.bidlevels_[t_level_changed_ - 1].limit_size_) {
    ////dbglogger_ << "Delta Size and Trade Delta Mismatch at Bid. Delta Size = " << p_market_order_->size_<< "
    /// Delta
    /// Order Count = "<< p_market_order_->order_count_ << " Last Trade Size = " << last_traded_size_ << "\n";
    int size_to_delete_from_front_ = last_traded_size_;
    int order_count_deleted_ = 0;
    while (size_to_delete_from_front_ > 0) {
      // Deleting Entire Order Level because it got Traded
      if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ <=
          size_to_delete_from_front_) {
        order_count_deleted_ += market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_;
        size_to_delete_from_front_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin());
      } else {
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= size_to_delete_from_front_;
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ >
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]
                ->size_) {  // If order Count Becomes larger than Size then we make order count equal to the
                            // remaining
                            // Size At that Level
          order_count_deleted_ +=
              (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ -
               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
        } else {  // Dividing the Order Count Proportionately.
          int propotionate_oc_ =
              (size_to_delete_from_front_ *
               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_) /
              (size_to_delete_from_front_ +
               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->order_count_ -= propotionate_oc_;
          order_count_deleted_ += propotionate_oc_;
        }
        size_to_delete_from_front_ = 0;
      }
    }
    // Recalculating Delta Order Count and Size so that it can be adjusted by the normal Procedure
    p_market_order_->order_count_ += order_count_deleted_;
    p_market_order_->size_ += last_traded_size_;
    delta_order_count_ =
        (p_market_order_->order_count_ > 0 ? p_market_order_->order_count_ : -1 * p_market_order_->order_count_);
    delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);
    trade_delta_packet_ = false;
  }
  /*In all the cases below we have two options of performing changes
   * 1. Start Deleting/Adding from top of the Order Level Depth Book
   * 2. Start Deleting/Adding from bottom of the Order Level Depth Book
   * We use a Probability to determine whether to go from Top or Bottom . In case of Trades we always go from Top .
   * The difference in adding/deleting at top or bottom is that it affects the estimate of QSA computed by Order
   * Level
   * SIM
   */

  if (p_market_order_->order_count_ > 0 &&
      p_market_order_->size_ > 0) {  // Simply inserting a new entry in the order level Book .
    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
  } else if (p_market_order_->order_count_ < 0 && p_market_order_->size_ >= 0) {
    /*Here more orders of smaller size have been deleted and lesser orders of larger sizes have come in the market .
     * In this case we independently try to adjust the values of delta size and delta ordercount in the Order Book
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (rand_front_ <= prob_modify_from_top_) {  // Starting to Modifying from Front
      int index_ = 0;
      for (index_ = 0; index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
           index_++) {
        if (delta_order_count_ <
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              delta_order_count_;
          delta_order_count_ = 0;
          break;
        } else if (delta_order_count_ >=
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_order_count_ -=
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          delta_size_ += market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          index_--;
        }
        if (delta_order_count_ == 0) break;
      }
      index_ = 0;
      // Probabilistically Trying To add the delta Size to any of the already Existing Entries
      while (index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size()) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
          break;
        } else {
          index_++;
        }
      }
      if (index_ == (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() &&
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ += delta_size_;
      }
    } else {  // Modifying from Back
      int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      for (; index_ >= 0; index_--) {
        if (delta_order_count_ <
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              delta_order_count_;
          delta_order_count_ = 0;
          break;
        } else if (delta_order_count_ >=
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_order_count_ -=
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          delta_size_ += market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
        }
        if (delta_order_count_ == 0) break;
      }
      index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;

      while (index_ >= 0) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
          break;
        } else {
          index_--;
        }
      }
      if (index_ < 0 && market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ += delta_size_;
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  } else if (p_market_order_->order_count_ > 0 && p_market_order_->size_ <= 0) {
    /*Here less orders of larger size have been deleted and more orders of smaller sizes have come in the market .
     * In this case we independently try to adjust the values of delta size and delta ordercount in the Order Book
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (rand_front_ <= prob_modify_from_top_) {
      int index_ = 0;
      while (index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size()) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ +=
              delta_order_count_;
          // Making Sure that at Each Entry in the Order Book Size >= Count
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_size_ +=
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                 market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          }
          break;
        } else {
          index_++;
        }
      }  // This is to ensure that If by applying Probability we fail in all the above cases then we modify the last
         // entry of the book .
      if (index_ == (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() &&
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ +=
            delta_order_count_;
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ >
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_) {
          delta_size_ +=
              (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ -
               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_;
        }
      }
      index_ = 0;
      for (index_ = 0; index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
           index_++) {
        if (delta_size_ <=
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
          delta_size_ = 0;
          break;
        } else if (delta_size_ >
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_size_ -= (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_size_ == 0) break;
      }
    } else {
      // Modifying the Order Book From Back
      int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      // Adjusting Order Count
      while (index_ >= 0) {
        int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
        if (rand_ <= prob_modify_level_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ +=
              delta_order_count_;
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_size_ +=
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                 market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
          }
          break;
        } else {
          index_--;
        }
      }
      // Adjusting Size
      if (index_ < 0 && market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ +=
            delta_order_count_;
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ >
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_) {
          delta_size_ +=
              (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_ -
               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_ - 1]->order_count_;
        }
      }
      index_ = (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      ;
      for (; index_ >= 0; index_--) {
        if (delta_size_ <=
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
          delta_size_ = 0;
          break;
        } else if (delta_size_ >
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
          delta_size_ -= (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_size_ == 0) break;
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  }

  else if (p_market_order_->order_count_ == 0)  // when order count =0
  {
    ////dbglogger_ << "here in bid  order count 0 with " << p_market_order_->size_ << " \n";
    bool flag_ = false;
    if (p_market_order_->size_ > 0) {
      int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

      if (rand_front_ <= prob_modify_from_top_) {
        int index_ = 0;
        while (index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size()) {
          // Probabilistically trying to update the size of an Order Level Entry
          int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
          if (rand_ <= prob_modify_level_) {
            flag_ = true;
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ ==
                1) {  // If the size of an entry is increased then we update its time and move it down in the order
                      // level Book
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                  watch_.tv();
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              index_--;
              (*market_orders_mempool_).DeAlloc(p_market_order_);
            } else {
              // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
              // existing order size between these orders .
              int toDeduct =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              p_market_order_->order_count_ = 1;
              p_market_order_->size_ += toDeduct;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
            }

            break;
          } else {
            index_++;
          }
        }
        if (flag_ == false && (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_].size() > index_) {
          index_--;
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                watch_.tv();
            (*market_orders_mempool_).DeAlloc(p_market_order_);
            // No need to shift as it is the bottom most entry
          } else {
            // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
            // existing
            // order size between these orders .
            int toDeduct = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                           market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
            p_market_order_->order_count_ = 1;
            p_market_order_->size_ += toDeduct;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
          }
        }
      } else {  // Modifications Being Performed at the Bottom Of The Order Level Book
        int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        while (index_ >= 0) {
          // Probabilistically trying to update the size of an Order Level Entry
          int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
          if (rand_ <= prob_modify_level_) {
            flag_ = true;
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
              // If the size of an entry is increased then we update its time and move it down in the order level
              // Book
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                  watch_.tv();
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              index_--;
              (*market_orders_mempool_).DeAlloc(p_market_order_);
            } else {
              // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
              // existing order size between these orders .
              int toDeduct =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              p_market_order_->order_count_ = 1;
              p_market_order_->size_ += toDeduct;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
            }

            break;
          } else {
            index_--;
          }
        }
        // If the Probabilistic Approach fails then The last entry is updated
        if (flag_ == false && market_update_info_.bid_level_order_depth_book_[t_level_changed_].size() > 0) {
          index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ += delta_size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_entry_time_ =
                watch_.tv();
            (*market_orders_mempool_).DeAlloc(p_market_order_);
          } else {
            // Shifting one of the orders from the chunk of orders down in the order Book. Also dividing the
            // existing
            // order size between these orders .
            int toDeduct = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ /
                           market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= toDeduct;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
            p_market_order_->order_count_ = 1;
            p_market_order_->size_ += toDeduct;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
          }
        }
      }
    } else if (p_market_order_->size_ < 0) {
      int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
      if (rand_front_ <= prob_modify_from_top_ || trade_delta_packet_ == true) {
        int index_ = 0;

        if (trade_delta_packet_ == false) {
          while (index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size()) {
            /*looking for an order entry in the order depth book that can afford to completely delete this size .
             * e.g and entry with size 5 and count 2 cannot afford to delete a size 4 and order count 0
             */
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <
                (delta_size_)) {
              index_++;
              continue;
            }

            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

            if (rand_ <= prob_modify_level_) {
              ////dbglogger_<<"dELETEING FROM HERE THAT IS randome one \n";

              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;

              break;
            } else {
              index_++;
            }
          }
        }
        // we want to delete forcefully from top only if above heuristic fail or if last message was trade
        if ((index_ == (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() ||
             trade_delta_packet_ == true) &&
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          ////dbglogger_<<"dELETEING FROM HERE THAT IS non randome one \n";
          for (int i = 0; i < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); i++) {
            if (delta_size_ - (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_) >
                0) {
              delta_size_ = delta_size_ -
                            (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                             market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_;
            } else {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -= delta_size_;

              delta_size_ = 0;
              break;
            }
          }
        }
      } else  // delete from non front
      {
        int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        if (trade_delta_packet_ == false) {
          /*looking for an order entry in the order depth book that can afford to completely delete this size .
           * e.g and entry with size 5 and count 2 cannot afford to delete a size 4 and order count 0
           */
          while (index_ >= 0) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <
                (delta_size_)) {
              index_--;
              continue;
            }

            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);

            if (rand_ <= prob_modify_level_) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;

              break;
            } else {
              index_--;
            }
          }
        }
        // we want to delete forcefully from Bottom only if above heuristic fails
        if ((index_ == -1) && market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          ////dbglogger_<<"dELETEING FROM HERE THAT IS non randome one \n";
          for (int i = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1; i >= 0; i--) {
            if (delta_size_ - (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                               market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_) >
                0) {
              delta_size_ = delta_size_ -
                            (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -
                             market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->order_count_;
            } else {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][i]->size_ -= delta_size_;

              delta_size_ = 0;
              break;
            }
          }
        }
      }
      (*market_orders_mempool_).DeAlloc(p_market_order_);
    }
  } else if (p_market_order_->order_count_ == -1 && p_market_order_->size_ < 0) {
    int index_ = 0;
    bool deleted_ = false;
    //////dbglogger_<< " SingleDelete\n";

    /*
     * Checking performance of Naive book vs Approximate Book
     *
         int naive_delete_ = market_update_info_.bidlevels_[t_level_changed_ -1].limit_size_ /
     market_update_info_.bidlevels_[t_level_changed_ -1].limit_ordercount_;

         while(index_ < market_update_info_.bid_level_order_depth_book_[t_level_changed_-1].size() )
         {
         if(market_update_info_.bid_level_order_depth_book_[t_level_changed_-1][index_]->size_ == naive_delete_ &&
     market_update_info_.bid_level_order_depth_book_[t_level_changed_-1][index_]->order_count_ == 1)
         {
         //////dbglogger_<< "NaiveDeleteMatch\n";
         break;
         }
         else
         {
         index_++;
         }
         }*/

    index_ = 0;
    if (trade_delta_packet_ ==
        false) {  // trying to find an exact match if the current delta does not correspond to Trade .
      index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
      while (index_ >= 0) {
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ == delta_size_ &&
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ == 1) {
          (*market_orders_mempool_)
              .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          deleted_ = true;
          break;
        } else {
          index_--;
        }
      }
    }
    // we want to delete forcefully from top only if above heuristic fail or if last message was trade
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if ((rand_front_ <= prob_modify_from_top_ || trade_delta_packet_ == true) && deleted_ == false) {
      if ((deleted_ == false || trade_delta_packet_ == true) &&
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        // probabilistic approach that if any of the entries present can afford to delete this delta packet
        index_ = 0;
        if (trade_delta_packet_ == false) {
          while (index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size()) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                         1) <
                    delta_size_ ||
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <= 1) {
              index_++;
              continue;
            }
            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
            if (rand_ <= prob_modify_level_) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              delta_order_count_ = 0;
              delta_size_ = 0;
              deleted_ = true;
              break;
            } else {
              index_++;
            }
          }
        }
        /*If there was no exact match and neither of the entries can afford to delete this delta then we start
         * pruning
         * from Top ot the order level Book
         * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta
         * Order
         * Count or Delta Order Size is not equal to 0 .
         * While pruning we try to maintain that the level from which deletion is being performed should have order
         * count <= the size
         */
        if ((deleted_ == false || trade_delta_packet_ == true) &&
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          for (int index_ = 0;
               index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() &&
                   delta_size_ > 0;
               index_++) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                    delta_order_count_) {
              if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                      delta_order_count_) {
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                delta_size_ = 0;
                break;
              } else {
                // Ensuring that Order Count <= Size
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_size_ -=
                    (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                     market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_order_count_ = 0;
                break;
              }
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
              if (delta_order_count_ -
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                  delta_size_ - market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
                delta_order_count_ =
                    delta_order_count_ -
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                index_--;
              } else {
                delta_order_count_ =
                    delta_order_count_ -
                    (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                    (delta_size_ - delta_order_count_);
                delta_size_ = delta_order_count_;
              }
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              delta_order_count_ =
                  delta_order_count_ -
                  (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                  (delta_size_ - delta_order_count_);
              delta_size_ = delta_order_count_;
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                           delta_order_count_) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                  (delta_order_count_ - 1);
              delta_order_count_ = 1;
              delta_size_ =
                  delta_size_ -
                  (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            }
            if (delta_order_count_ <= 0 || delta_size_ <= 0) {
              break;
            }
          }

          // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is
          // true
          // we handle them independently
          // Also adjusting Delta OC will lead to changes in Delta Size
          if (delta_order_count_ > 0) {
            for (int index_ = 0;
                 index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); index_++) {
              if (delta_order_count_ <
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                break;
              } else if (delta_order_count_ >=
                         market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                delta_order_count_ -=
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                index_--;
              }
              if (delta_order_count_ == 0) break;
            }
          }

          if (delta_size_ != 0) {
            if (delta_size_ < 0) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ += (-1 * delta_size_);
            } else {
              for (int index_ = 0;
                   index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
                   index_++) {
                if (delta_size_ == 0) {
                  break;
                } else if (delta_size_ > 0) {
                  if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                      0) {
                    if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]
                                ->order_count_ >=
                        delta_size_) {
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                          delta_size_;
                      delta_size_ = 0;
                    } else {
                      delta_size_ -=
                          (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                           market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } else if (deleted_ == false) {
      if (deleted_ == false && market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
        // probabilistic approach that if any of the entries present can afford to delete this delta packet
        index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
        if (trade_delta_packet_ == false) {  //  If Trade delta Packet is true then we will prune from the top
          while (index_ >= 0) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                         1) <
                    delta_size_ ||
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <= 1) {
              index_--;
              continue;
            }
            int rand_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
            if (rand_ <= prob_modify_level_) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -= 1;
              delta_order_count_ = 0;
              delta_size_ = 0;

              break;
            } else {
              index_--;
            }
          }
        }

        /*If there was no exact match and neither of the entries can afford to delete this delta then we start
         * pruning
         * from Top ot the order level Book
         * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta
         * Order
         * Count or Delta Order Size is not equal to 0 .
         * While pruning we try to maintain that the level from which deletion is being performed should have order
         * count <= the size
         */
        if (index_ == -1 && market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() > 0) {
          for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
               index_ >= 0 && delta_size_ > 0; index_--) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                    delta_order_count_) {
              if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                      delta_order_count_) {
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                delta_size_ = 0;
                break;
              } else {
                // Ensuring that Order Count <= Size
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_size_ -=
                    (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                     market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_order_count_ = 0;
                break;
              }
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
              if (delta_order_count_ -
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                  delta_size_ - market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
                delta_order_count_ =
                    delta_order_count_ -
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
              } else {
                delta_order_count_ =
                    delta_order_count_ -
                    (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                    (delta_size_ - delta_order_count_);
                delta_size_ = delta_order_count_;
              }
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                           delta_order_count_) {
              delta_order_count_ =
                  delta_order_count_ -
                  (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                  (delta_size_ - delta_order_count_);
              delta_size_ = delta_order_count_;
            } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                           delta_size_ &&
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                           delta_order_count_) {
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                  (delta_order_count_ - 1);
              delta_order_count_ = 1;
              delta_size_ =
                  delta_size_ -
                  (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            }
            if (delta_order_count_ <= 0 || delta_size_ <= 0) {
              break;
            }
          }
          // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is
          // true
          // we handle them independently
          // Also adjusting Delta OC will lead to changes in Delta Size
          if (delta_order_count_ > 0) {
            for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
                 index_ >= 0; index_--) {
              if (delta_order_count_ <
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                    delta_order_count_;
                delta_order_count_ = 0;
                break;
              } else if (delta_order_count_ >=
                         market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
                delta_order_count_ -=
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
                (*market_orders_mempool_)
                    .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
                // index--_;
              }
              if (delta_order_count_ == 0) break;
            }
          }

          if (delta_size_ != 0) {
            if (delta_size_ < 0) {
              market_update_info_.bid_level_order_depth_book_
                  [t_level_changed_ - 1][market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() -
                                         1]->size_ += (-1 * delta_size_);
            } else {
              for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
                   index_ >= 0; index_--) {
                if (delta_size_ == 0) {
                  break;
                } else if (delta_size_ > 0) {
                  if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                      0) {
                    if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]
                                ->order_count_ >=
                        delta_size_) {
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                          delta_size_;
                      delta_size_ = 0;
                    } else {
                      delta_size_ -=
                          (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                           market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  } else if (p_market_order_->order_count_ < 0 &&
             p_market_order_->size_ < 0)  // delta order count <-1 and delta size < 0
  {
    /* In this case we only do Pruning .
     * In Pruning We try to Remove the maximum possible from each level continuing until either of the Delta Order
     * Count or Delta Order Size is not equal to 0 .
     * While pruning we try to maintain that the level from which deletion is being performed should have order
     * count
     * <= the size
     */
    int rand_front_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
    if (trade_delta_packet_ == true || rand_front_ < prob_modify_from_top_) {
      // If last packet was trade packet corresponding to this level then modifications are performed from the top
      for (int index_ = 0; index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() &&
                               delta_size_ > 0;
           index_++) {
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                delta_order_count_) {
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                  delta_order_count_) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            delta_size_ = 0;
            break;
          } else {
            // Ensuring that Order Count <= Size
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_size_ -=
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                 market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_order_count_ = 0;
            break;
          }
        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
          if (delta_order_count_ -
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
              delta_size_ - market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_order_count_ =
                delta_order_count_ -
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            index_--;
          } else {
            // dbglogger_<< "Entering this area  3\n";
            delta_order_count_ =
                delta_order_count_ -
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                (delta_size_ - delta_order_count_);
            delta_size_ = delta_order_count_;
          }
        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          delta_order_count_ =
              delta_order_count_ -
              (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
              (delta_size_ - delta_order_count_);
          delta_size_ = delta_order_count_;
        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                       delta_order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              (delta_order_count_ - 1);
          delta_order_count_ = 1;
          delta_size_ = delta_size_ -
                        (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                         market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_order_count_ <= 0 || delta_size_ <= 0) {
          break;
        }
      }
      // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is true
      // we
      // handle them independently
      // Also adjusting Delta OC will lead to changes in Delta Size
      if (delta_order_count_ > 0) {
        for (int index_ = 0; index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size();
             index_++) {
          if (delta_order_count_ <
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            break;
          } else if (delta_order_count_ >=
                     market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            delta_order_count_ -=
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            index_--;
          }
          if (delta_order_count_ == 0) break;
        }
      }

      if (delta_size_ != 0) {
        if (delta_size_ < 0) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ += (-1 * delta_size_);
        } else {
          for (int index_ = 0;
               index_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); index_++) {
            if (delta_size_ == 0) {
              break;
            } else if (delta_size_ > 0) {
              if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                  0) {
                if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                    delta_size_) {
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                  delta_size_ = 0;
                } else {
                  delta_size_ -=
                      (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                }
              }
            }
          }
        }
      }
    } else if (trade_delta_packet_ == false) {
      for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
           index_ >= 0 && delta_size_ > 0; index_--) {
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ > delta_size_ &&
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                delta_order_count_) {
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ - delta_size_ >=
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -
                  delta_order_count_) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            delta_size_ = 0;
            break;
          } else {
            // Ensuring that Order Count <= Size
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_size_ -=
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                 market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_order_count_ = 0;
            break;
          }
        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          // This check is for the case so as to ensure that remaining Delta Order is <=remaining Delta Size
          if (delta_order_count_ -
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
              delta_size_ - market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_) {
            delta_order_count_ =
                delta_order_count_ -
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
          } else {
            // dbglogger_<< "Entering this area  4\n";
            delta_order_count_ =
                delta_order_count_ -
                (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
                (delta_size_ - delta_order_count_);
            delta_size_ = delta_order_count_;
          }

        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ >=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ <=
                       delta_order_count_) {
          delta_order_count_ =
              delta_order_count_ -
              (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ - 1);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ = 1;
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -=
              (delta_size_ - delta_order_count_);
          delta_size_ = delta_order_count_;
        } else if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ <=
                       delta_size_ &&
                   market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                       delta_order_count_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
              (delta_order_count_ - 1);
          delta_order_count_ = 1;
          delta_size_ = delta_size_ -
                        (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                         market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
        }
        if (delta_order_count_ <= 0 || delta_size_ <= 0) {
          break;
        }
      }
      // After above Pruning there may be cases when either of Delta OC or Delta Size !=0 .If either of it is true
      // we
      // handle them independently
      // Also adjusting Delta OC will lead to changes in Delta Size

      if (delta_order_count_ > 0) {
        for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1; index_ >= 0;
             index_--) {
          if (delta_order_count_ <
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ -=
                delta_order_count_;
            delta_order_count_ = 0;
            break;
          } else if (delta_order_count_ >=
                     market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_) {
            delta_order_count_ -=
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_;
            (*market_orders_mempool_)
                .DeAlloc(market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]);
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + index_);
            // index_--;
          }
          if (delta_order_count_ == 0) break;
        }
      }

      if (delta_size_ != 0) {
        if (delta_size_ < 0) {
          market_update_info_.bid_level_order_depth_book_
              [t_level_changed_ - 1][market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1]
                  ->size_ += (-1 * delta_size_);
        } else {
          for (int index_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1;
               index_ >= 0; index_--) {
            if (delta_size_ == 0) {
              break;
            } else if (delta_size_ > 0) {
              if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >
                  0) {
                if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_ >=
                    delta_size_) {
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -= delta_size_;
                  delta_size_ = 0;
                } else {
                  delta_size_ -=
                      (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ -
                       market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_);
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->size_ =
                      market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][index_]->order_count_;
                }
              }
            }
          }
        }
      }
    }
    (*market_orders_mempool_).DeAlloc(p_market_order_);
  }
  // dbglogger_<<"\n\nEND *************\n\n";
  dbglogger_.DumpCurrentBuffer();
}

void SecurityMarketView::UpdateLiffeAskOrderLevelBook(int t_level_changed_, MarketOrder *p_market_order_,
                                                      bool trade_delta_packet_, int trade_size_) {
  // int seed = (int)GetCpucycleCount();
  // srand(seed);
  int delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);

  if (trade_delta_packet_ == true &&
      market_update_info_.asklevels_[t_level_changed_ - 1].limit_size_ - p_market_order_->size_ >= trade_size_) {
    int residual_trade_volume_ = trade_size_;
    int &askside_priority_volume_ = market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_;
    int total_residual_volume_ = market_update_info_.asklevels_[t_level_changed_ - 1].limit_size_ -
                                 p_market_order_->size_ - askside_priority_volume_;

    // Filling Priority Volumes
    if (askside_priority_volume_ > 0) {
      if (askside_priority_volume_ > market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_) {
        askside_priority_volume_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
      }

      if (askside_priority_volume_ >= trade_size_) {
        residual_trade_volume_ = 0;
        askside_priority_volume_ -= trade_size_;
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= trade_size_;
      } else if (trade_size_ > askside_priority_volume_) {
        residual_trade_volume_ -= askside_priority_volume_;
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= askside_priority_volume_;
        askside_priority_volume_ = 0;
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][0]->size_ <= 0) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin());
        }
      }
    }
    // Filling Non-Piority Volumes by Pro Rata Matching Algorithm

    int preceding_size_ = 0;
    int total_volume_traded_ = 0;

    while (total_volume_traded_ < residual_trade_volume_) {
      // It may happen that because of rounding complete Trade Volume is not alloted in a single pass so we can have
      // multiple passes
      preceding_size_ = 0;
      residual_trade_volume_ = residual_trade_volume_ - total_volume_traded_;
      total_residual_volume_ = total_residual_volume_ - total_volume_traded_;
      total_volume_traded_ = 0;

      for (int x = 0; x < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
        if (total_volume_traded_ >= residual_trade_volume_) {
          break;
        }
        int trade_at_this_level_ = GetTradedQuantity(
            total_residual_volume_, preceding_size_,
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_, residual_trade_volume_);

        preceding_size_ += market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
        if (total_volume_traded_ + trade_at_this_level_ > residual_trade_volume_) {
          trade_at_this_level_ = residual_trade_volume_ - total_volume_traded_;
        }
        if (trade_at_this_level_ < market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= trade_at_this_level_;
        } else {
          trade_at_this_level_ = market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
          market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
          x--;
        }
        total_volume_traded_ += trade_at_this_level_;
      }
    }

    p_market_order_->size_ += trade_size_;
    delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);
  }

  if (p_market_order_->size_ > 0) {
    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
  } else if (p_market_order_->size_ < 0) {
    bool flag_ = false;
    for (int x = 0; x < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
      if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ == delta_size_) {
        if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
          market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ = 0;
        }
        market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
        flag_ = true;
        break;
      }
    }
    if (flag_ == false) {
      int probability_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
      if (probability_ > probability_delete_front_) {
        for (int x = (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size() - 1; x >= 0;
             x--) {
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ <= delta_size_) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ = 0;
            }
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + x);

          } else {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= delta_size_;
            delta_size_ = 0;
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              if (market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ >
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
                market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ =
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
              }
            }
            break;
          }
        }
      } else {
        for (int x = 0; x < (int)market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
          if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ <= delta_size_) {
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ = 0;
            }
            delta_size_ -= market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
            x--;
          } else {
            market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= delta_size_;
            delta_size_ = 0;
            if (market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              if (market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ >
                  market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
                market_update_info_.asklevels_[t_level_changed_ - 1].priority_size_ =
                    market_update_info_.ask_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
              }
            }
            break;
          }
        }
      }
    }
  }
}

void SecurityMarketView::UpdateLiffeBidOrderLevelBook(int t_level_changed_, MarketOrder *p_market_order_,
                                                      bool trade_delta_packet_, int trade_size_) {
  // int seed = (int)GetCpucycleCount();
  // srand(seed);
  int delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);

  if (trade_delta_packet_ == true &&
      market_update_info_.bidlevels_[t_level_changed_ - 1].limit_size_ - p_market_order_->size_ >= trade_size_) {
    int residual_trade_volume_ = trade_size_;
    int &bidside_priority_volume_ = market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_;
    int total_residual_volume_ = market_update_info_.bidlevels_[t_level_changed_ - 1].limit_size_ -
                                 p_market_order_->size_ - bidside_priority_volume_;
    // Filling Priority Volumes
    if (bidside_priority_volume_ > 0) {
      if (bidside_priority_volume_ > market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_) {
        bidside_priority_volume_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_;
      }

      if (bidside_priority_volume_ >= trade_size_) {
        residual_trade_volume_ = 0;
        bidside_priority_volume_ -= trade_size_;
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= trade_size_;
      } else if (trade_size_ > bidside_priority_volume_) {
        residual_trade_volume_ -= bidside_priority_volume_;
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ -= bidside_priority_volume_;
        bidside_priority_volume_ = 0;
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][0]->size_ <= 0) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin());
        }
      }
    }
    // Filling Non-Piority Volumes by Pro Rata Matching Algorithm
    int preceding_size_ = 0;
    int total_volume_traded_ = 0;
    while (total_volume_traded_ < residual_trade_volume_) {
      // It may happen that because of rounding complete Trade Volume is not alloted in a single pass so we can have
      // multiple passes
      preceding_size_ = 0;
      residual_trade_volume_ = residual_trade_volume_ - total_volume_traded_;
      total_residual_volume_ = total_residual_volume_ - total_volume_traded_;
      total_volume_traded_ = 0;
      for (int x = 0; x < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
        if (total_volume_traded_ >= residual_trade_volume_) {
          break;
        }
        int trade_at_this_level_ = GetTradedQuantity(
            total_residual_volume_, preceding_size_,
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_, residual_trade_volume_);

        preceding_size_ += market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
        if (total_volume_traded_ + trade_at_this_level_ > residual_trade_volume_) {
          trade_at_this_level_ = residual_trade_volume_ - total_volume_traded_;
        }
        if (trade_at_this_level_ < market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= trade_at_this_level_;
        } else {
          trade_at_this_level_ = market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
          market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
              market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
          x--;
        }
        total_volume_traded_ += trade_at_this_level_;
      }
    }

    p_market_order_->size_ +=
        trade_size_;  // If there is a TradeSize and Delta Size Mismatch then remaining Size is adjusted.
    delta_size_ = (p_market_order_->size_ > 0 ? p_market_order_->size_ : -1 * p_market_order_->size_);
  }

  if (p_market_order_->size_ > 0) {
    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].push_back(p_market_order_);
  } else if (p_market_order_->size_ < 0) {
    bool flag_ = false;
    for (int x = 0; x < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
      // Trying to find an exact Match
      if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ == delta_size_) {
        if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
          market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ = 0;
        }
        market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
        flag_ = true;
        break;
      }
    }
    if (flag_ == false) {
      int probability_ = HFSAT::RandomNumberGenerator::GetRandNum(100);
      if (probability_ > probability_delete_front_) {
        for (int x = (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size() - 1; x >= 0;
             x--) {
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ <= delta_size_) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ = 0;
            }
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
          } else {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= delta_size_;
            delta_size_ = 0;
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              if (market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ >
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
                market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ =
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
              }
            }
            break;
          }
        }
      } else {
        for (int x = 0; x < (int)market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].size(); x++) {
          if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ <= delta_size_) {
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ = 0;
            }
            delta_size_ -= market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].erase(
                market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1].begin() + x);
            x--;
          } else {
            market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_ -= delta_size_;
            delta_size_ = 0;
            if (market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->priority_ == true) {
              if (market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ >
                  market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_) {
                market_update_info_.bidlevels_[t_level_changed_ - 1].priority_size_ =
                    market_update_info_.bid_level_order_depth_book_[t_level_changed_ - 1][x]->size_;
              }
            }
            break;
          }
        }
      }
    }
  }
}

void SecurityMarketView::SanityCheckOnTrade(int int_trade_price_) {
  if ((market_update_info_.bestbid_int_price_ == (kInvalidIntPrice)) ||
      (market_update_info_.bestask_int_price_ == (kInvalidIntPrice))) {
    is_ready_ = false;  // This is to ensure the listeners don't use Invalid prices if the side is cleared after
                        // setting is_ready at first
    return;
  }

  if (market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_CR ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_EQ) {
    if (int_trade_price_ > int_price_ask_book_[base_ask_index_]) {
      if ((int_trade_price_ - int_price_ask_book_[base_ask_index_]) >= (int)base_ask_index_) {
        return;
      }

      base_ask_index_ = base_ask_index_ - (int_trade_price_ - int_price_ask_book_[base_ask_index_]);

      while (base_ask_index_ > 0 && market_update_info_.asklevels_[base_ask_index_].limit_size_ <= 0) {
        base_ask_index_--;
      }

      market_update_info_.bestask_int_price_ = int_price_ask_book_[base_ask_index_];
      market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
      market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
      market_update_info_.bestask_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_int_price_ = int_price_ask_book_[base_ask_index_];
        //          std::cout << int_price_ask_book_[base_ask_index_] << "\n";
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[int_price_ask_book_[base_ask_index_]];
        hybrid_market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
        hybrid_market_update_info_.bestask_ordercount_ = 1;
      }

      UpdateL1Prices();

      if (market_update_info_.bidlevels_[base_bid_index_].limit_size_ &&
          market_update_info_.asklevels_[base_ask_index_].limit_size_) {
        NotifyL1SizeListeners();
      }

      l1_changed_since_last_ = true;

      OnL2Update();
      OnL2OnlyUpdate();
      NotifyOnReadyListeners();
      l2_changed_since_last_ = true;
    }

    if (int_trade_price_ < int_price_bid_book_[base_bid_index_]) {
      if ((int_price_bid_book_[base_bid_index_] - int_trade_price_) >= (int)base_bid_index_) {
        return;
      }

      base_bid_index_ = base_bid_index_ - (int_price_bid_book_[base_bid_index_] - int_trade_price_);

      while (base_bid_index_ > 0 && market_update_info_.bidlevels_[base_bid_index_].limit_size_ <= 0) {
        base_bid_index_--;
      }

      market_update_info_.bestbid_int_price_ = int_price_bid_book_[base_bid_index_];
      market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
      market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
      market_update_info_.bestbid_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_int_price_ = int_price_bid_book_[base_bid_index_];
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[int_price_bid_book_[base_bid_index_]];
        hybrid_market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
        hybrid_market_update_info_.bestbid_ordercount_ = 1;
      }

      UpdateL1Prices();

      if (market_update_info_.bidlevels_[base_bid_index_].limit_size_ &&
          market_update_info_.asklevels_[base_ask_index_].limit_size_) {
        NotifyL1SizeListeners();
      }

      l1_changed_since_last_ = true;

      OnL2Update();
      OnL2OnlyUpdate();
      NotifyOnReadyListeners();
      l2_changed_since_last_ = true;
    }
  }
}

void SecurityMarketView::SanityCheckForBestBid(int int_ask_price_) {
  if ((market_update_info_.bestbid_int_price_ == (kInvalidIntPrice)) ||
      (market_update_info_.bestask_int_price_ == (kInvalidIntPrice))) {
    is_ready_ = false;  // This is to ensure the listeners don't use Invalid prices if the side is cleared after
                        // setting is_ready at first
    return;
  }

  if (market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_CR ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_EQ) {
    if (int_ask_price_ <= int_price_bid_book_[base_bid_index_]) {
      if ((int_price_bid_book_[base_bid_index_] - int_ask_price_) >= (int)base_bid_index_) {
        return;
      }

      base_bid_index_ = base_bid_index_ - (int_price_bid_book_[base_bid_index_] - int_ask_price_ + 1);

      while (base_bid_index_ > 0 && market_update_info_.bidlevels_[base_bid_index_].limit_size_ <= 0) {
        base_bid_index_--;
      }

      market_update_info_.bestbid_int_price_ = int_price_bid_book_[base_bid_index_];
      market_update_info_.bestbid_price_ = market_update_info_.bidlevels_[base_bid_index_].limit_price_;
      market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
      market_update_info_.bestbid_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_int_price_ = int_price_bid_book_[base_bid_index_];
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[int_price_bid_book_[base_bid_index_]];
        hybrid_market_update_info_.bestbid_size_ = market_update_info_.bidlevels_[base_bid_index_].limit_size_;
        hybrid_market_update_info_.bestbid_ordercount_ = 1;
      }

      UpdateL1Prices();

      if (market_update_info_.bidlevels_[base_bid_index_].limit_size_ &&
          market_update_info_.asklevels_[base_ask_index_].limit_size_) {
        NotifyL1SizeListeners();
      }

      l1_changed_since_last_ = true;

      OnL2Update();
      OnL2OnlyUpdate();
      NotifyOnReadyListeners();
      l2_changed_since_last_ = true;
    }
  }
}

void SecurityMarketView::SanityCheckForBestAsk(int int_bid_price_) {
  if ((market_update_info_.bestbid_int_price_ == (kInvalidIntPrice)) ||
      (market_update_info_.bestask_int_price_ == (kInvalidIntPrice))) {
    is_ready_ = false;  // This is to ensure the listeners don't use Invalid prices if the side is cleared after
                        // setting is_ready at first
    return;
  }

  if (market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_CR ||
      market_update_info_.exch_source_ == HFSAT::kExchSourceMICEX_EQ) {
    if (int_bid_price_ >= int_price_ask_book_[base_ask_index_]) {
      if ((int_bid_price_ - int_price_ask_book_[base_ask_index_]) >= (int)base_ask_index_) {
        return;
      }

      base_ask_index_ = base_ask_index_ - (int_bid_price_ - int_price_ask_book_[base_ask_index_] + 1);

      while (base_ask_index_ > 0 && market_update_info_.asklevels_[base_ask_index_].limit_size_ <= 0) {
        base_ask_index_--;
      }

      market_update_info_.bestask_int_price_ = int_price_ask_book_[base_ask_index_];
      market_update_info_.bestask_price_ = market_update_info_.asklevels_[base_ask_index_].limit_price_;
      market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
      market_update_info_.bestask_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_int_price_ = int_price_ask_book_[base_ask_index_];
        //          std::cout << int_price_ask_book_[base_ask_index_] << "\n";
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[int_price_ask_book_[base_ask_index_]];
        hybrid_market_update_info_.bestask_size_ = market_update_info_.asklevels_[base_ask_index_].limit_size_;
        hybrid_market_update_info_.bestask_ordercount_ = 1;
      }

      UpdateL1Prices();

      if (market_update_info_.bidlevels_[base_bid_index_].limit_size_ &&
          market_update_info_.asklevels_[base_ask_index_].limit_size_) {
        NotifyL1SizeListeners();
      }

      l1_changed_since_last_ = true;

      OnL2Update();
      OnL2OnlyUpdate();
      NotifyOnReadyListeners();
      l2_changed_since_last_ = true;
    }
  }
}

int SecurityMarketView::GetTradedQuantity(int total_volume_, int preceding_volume_, int order_volume, int trade_size_) {
  // Expanded formula for LIFFE Trade Volume Allocation
  // dbglogger_ << " From Get Traded Quantity in SMV Volume " << total_volume_ << " precev : " << preceding_volume_
  // <<
  // "Order Volume : " << order_volume << "Trade Size " << trade_size_ << " \n";
  double traded_fraction_ = (double)((2 * total_volume_ - order_volume - 2 * preceding_volume_) * order_volume) /
                            ((double)(total_volume_ * total_volume_));
  if (total_volume_ > 1000000) {
    dbglogger_ << "Error::GetTradedQuantity::Unexpectedly High Volume" << DBGLOG_ENDL_FLUSH;
  }
  if (traded_fraction_ < 0) {
    traded_fraction_ = 0;
  }
  int further_match_ = traded_fraction_ * trade_size_;
  if (further_match_ == 0) {
    // Fractional Allocations less than 1 are rounded up to 1
    further_match_ = 1;
  }
  return further_match_;
}
