/**
   \file MarketAdapterCode/market_book.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/market_book.hpp"

namespace HFSAT {

#define INITIAL_BOOK_LENGTH 2048

MarketBook::MarketBook(DebugLogger& dbglogger, const Watch& watch)
    : dbglogger_(dbglogger),
      watch_(watch),
      mkt_order_mempool_(new SimpleMempool<ExchMarketOrder>()),
      bid_initialized_(false),
      ask_initialized_(false),
      bid_levels_(),
      ask_levels_(),
      bid_adjustment_(0),
      ask_adjustment_(9999999),
      top_bid_index_(0),
      top_ask_index_(0),
      book_levels_(0) {
  book_levels_ = INITIAL_BOOK_LENGTH;

  for (int i = 0; i < book_levels_; i++) {
    bid_levels_.push_back(new MktOrdLevel(watch_));
    ask_levels_.push_back(new MktOrdLevel(watch_));
  }
}

MarketBook::~MarketBook() {}

void MarketBook::InitializeBidLevels(int int_price) {
  if (!bid_initialized_) {
    top_bid_index_ = book_levels_ / 2;
    bid_adjustment_ = int_price - top_bid_index_;

    bid_initialized_ = true;
  }
}

void MarketBook::InitializeAskLevels(int int_price) {
  if (!ask_initialized_) {
    top_ask_index_ = book_levels_ / 2;
    ask_adjustment_ = int_price + top_ask_index_ - book_levels_;

    ask_initialized_ = true;
  }
}

ExchMarketOrder* MarketBook::GetExchMarketOrder(int64_t order_id, double price, int size, int int_price,
                                                uint64_t priority) {
  ExchMarketOrder* new_market_order = mkt_order_mempool_->Alloc();
  new_market_order->order_id_ = order_id;
  new_market_order->price_ = price;
  new_market_order->size_ = size;
  new_market_order->int_price_ = int_price;
  new_market_order->priority_ = priority;

  return new_market_order;
}

bool MarketBook::AdjustTopBidIndex(int index) {
  if (index == top_bid_index_ && bid_levels_[index]->Empty()) {
    int next_bid_index = top_bid_index_ - 1;
    while (next_bid_index >= 0 && bid_levels_[next_bid_index]->Empty()) {
      next_bid_index--;
    }

    if (next_bid_index < 0) {
      DBGLOG_TIME_CLASS_FUNC << "top_bid_index < 0" << DBGLOG_ENDL_FLUSH;
      return true;
    }

    top_bid_index_ = next_bid_index;

    if (top_bid_index_ < 50) {
      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "top_bid_index less than 50" << DBGLOG_ENDL_FLUSH;
      }

      int new_top_index = book_levels_ / 2;
      int shift = new_top_index - top_bid_index_;
      bid_adjustment_ -= shift;
      for (int i = book_levels_ - 1; i >= 0; i--) {
        if (i - shift >= 0 && i - shift < book_levels_) {
          bid_levels_[i]->Clear(mkt_order_mempool_);
          bid_levels_[i] = bid_levels_[i - shift];
        } else {
          bid_levels_[i] = new MktOrdLevel(watch_);
        }
      }

      top_bid_index_ = new_top_index;
    }

    return true;
  } else if (index > top_bid_index_ && !bid_levels_[index]->Empty()) {
    top_bid_index_ = index;

    if (top_bid_index_ >= book_levels_) {
      DBGLOG_TIME_CLASS_FUNC << "top_bid_index more than " << book_levels_ << DBGLOG_ENDL_FLUSH;
      // TODO: shift the levels
    }

    return true;
  } else {
    return false;
  }
}

bool MarketBook::AdjustTopAskIndex(int index) {
  if (index == top_ask_index_ && ask_levels_[index]->Empty()) {
    int next_ask_index = top_ask_index_ - 1;
    while (next_ask_index >= 0 && ask_levels_[next_ask_index]->Empty()) {
      next_ask_index--;
    }

    if (next_ask_index < 0) {
      DBGLOG_TIME_CLASS_FUNC << "top_ask_index < 0" << DBGLOG_ENDL_FLUSH;
      return true;
    }

    top_ask_index_ = next_ask_index;

    if (top_ask_index_ < 50) {
      if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << "top_ask_index less than 50" << DBGLOG_ENDL_FLUSH;
      }

      int new_top_index = book_levels_ / 2;
      int shift = new_top_index - top_ask_index_;
      ask_adjustment_ += shift;
      for (int i = book_levels_ - 1; i >= 0; i--) {
        if (i - shift >= 0 && i - shift < book_levels_) {
          ask_levels_[i]->Clear(mkt_order_mempool_);
          ask_levels_[i] = ask_levels_[i - shift];
        } else {
          ask_levels_[i] = new MktOrdLevel(watch_);
        }
      }

      top_ask_index_ = new_top_index;
    }

    return true;
  } else if (index > top_ask_index_ && !ask_levels_[index]->Empty()) {
    top_ask_index_ = index;

    if (top_ask_index_ >= book_levels_) {
      DBGLOG_TIME_CLASS_FUNC << "top_ask_index more than " << book_levels_ << DBGLOG_ENDL_FLUSH;
      // TODO: shift the levels
    }

    return true;
  } else {
    return false;
  }
}

int MarketBook::GetBestBidIndex(int int_price) {
  int bid_index = GetBidIndex(int_price);

  if (bid_index > top_bid_index_) {
    top_bid_index_ = bid_index;
  }

  if (top_bid_index_ >= book_levels_) {
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "top_bid_index more than " << book_levels_ << DBGLOG_ENDL_FLUSH;
    }

    int new_top_index = book_levels_ / 2;
    int shift = top_bid_index_ - new_top_index;
    bid_adjustment_ += shift;

    for (int i = 0; i < book_levels_; i++) {
      if (i + shift < book_levels_ && i + shift >= 0) {
        bid_levels_[i]->Clear(mkt_order_mempool_);
        bid_levels_[i] = bid_levels_[i + shift];
      } else {
        bid_levels_[i] = new MktOrdLevel(watch_);
      }
    }

    top_bid_index_ = new_top_index;
    bid_index = top_bid_index_;
  }

  return bid_index;
}

int MarketBook::GetBestAskIndex(int int_price) {
  int ask_index = GetAskIndex(int_price);

  if (ask_index > top_ask_index_) {
    top_ask_index_ = ask_index;
  }

  if (top_ask_index_ >= book_levels_) {
    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "top_ask_index more than " << book_levels_ << DBGLOG_ENDL_FLUSH;
    }

    int new_top_index = book_levels_ / 2;
    int shift = top_ask_index_ - new_top_index;
    ask_adjustment_ -= shift;

    for (int i = 0; i < book_levels_; i++) {
      if (i + shift < book_levels_ && i + shift >= 0) {
        ask_levels_[i]->Clear(mkt_order_mempool_);
        ask_levels_[i] = ask_levels_[i + shift];
      } else {
        ask_levels_[i] = new MktOrdLevel(watch_);
      }
    }

    top_ask_index_ = new_top_index;
    ask_index = top_ask_index_;
  }

  return ask_index;
}

int MarketBook::GetBidLevelSize(int index) {
  if (index < 0 || index >= book_levels_) {
    return 0;
  }

  return bid_levels_[index]->Size();
}

int MarketBook::GetAskLevelSize(int index) {
  if (index < 0 || index >= book_levels_) {
    return 0;
  }

  return ask_levels_[index]->Size();
}

bool MarketBook::AddBid(ExchMarketOrder* order, QueuePositionUpdate* queue_pos, bool position_based, bool& l1_update) {
  if (order == nullptr) {
    DBGLOG_TIME_CLASS_FUNC << "NULL order." << DBGLOG_ENDL_FLUSH;
    return false;
  }

  InitializeBidLevels(order->int_price_);
  int bid_index = GetBestBidIndex(order->int_price_);

  if (bid_index < 0) {
    // Not logging this because there would be a lot of such instances

    // setting these fields is of not much as well because we are not using them as of now

    queue_pos->action_ = QueuePositionUpdate::OrderAdd;
    queue_pos->buysell_ = kTradeTypeBuy;
    queue_pos->int_price_ = order->int_price_;
    queue_pos->size_ = order->size_;
    queue_pos->position_ = INT_MAX;

    return false;
  }

  queue_pos->action_ = QueuePositionUpdate::OrderAdd;
  queue_pos->buysell_ = kTradeTypeBuy;
  queue_pos->int_price_ = order->int_price_;
  queue_pos->size_ = order->size_;

  // Insert the given order into the mkt_orders_ vector at
  // appropriate position based on the priority of the order
  if (position_based) {
    bid_levels_[bid_index]->InsertExchOrderAtPosition(order);
  } else {
    bid_levels_[bid_index]->InsertExchOrder(order);
  }

  queue_pos->position_ = bid_levels_[bid_index]->GetPosition(order);

  l1_update = (bid_index == top_bid_index_);
  return true;
}

bool MarketBook::AddAsk(ExchMarketOrder* order, QueuePositionUpdate* queue_pos, bool position_based, bool& l1_update) {
  if (order == NULL) {
    DBGLOG_TIME_CLASS_FUNC << "NULL order." << DBGLOG_ENDL_FLUSH;
    return false;
  }

  InitializeAskLevels(order->int_price_);
  int ask_index = GetBestAskIndex(order->int_price_);

  if (ask_index < 0) {
    // Not logging this because there would be a lot of such instances

    // setting these fields is of not much as well because we are not using them as of now
    queue_pos->action_ = QueuePositionUpdate::OrderAdd;
    queue_pos->buysell_ = kTradeTypeSell;
    queue_pos->int_price_ = order->int_price_;
    queue_pos->size_ = order->size_;
    queue_pos->position_ = INT_MAX;

    return false;
  }

  queue_pos->action_ = QueuePositionUpdate::OrderAdd;
  queue_pos->buysell_ = kTradeTypeSell;
  queue_pos->int_price_ = order->int_price_;
  queue_pos->size_ = order->size_;

  // Insert the given order into the mkt_orders_ vector at
  // appropriate position based on the priority of the order
  if (position_based) {
    ask_levels_[ask_index]->InsertExchOrderAtPosition(order);
  } else {
    ask_levels_[ask_index]->InsertExchOrder(order);
  }

  queue_pos->position_ = GetAskPosition(order);

  l1_update = (ask_index == top_ask_index_);
  return true;
}

ExchMarketOrder* MarketBook::FindMarketBid(int64_t order_id, int int_price, int size) {
  return FindMarketBid(order_id, int_price);
}

ExchMarketOrder* MarketBook::FindMarketAsk(int64_t order_id, int int_price, int size) {
  return FindMarketAsk(order_id, int_price);
}

ExchMarketOrder* MarketBook::FindMarketBid(int64_t order_id, int int_price) {
  InitializeBidLevels(int_price);
  int bid_index = GetBidIndex(int_price);

  if (bid_index > top_bid_index_ || bid_index < 0) {
    return NULL;
  }

  // At this point, bid_index_ is a valid index
  return bid_levels_[bid_index]->FindOrder(order_id);
}

ExchMarketOrder* MarketBook::FindMarketAsk(int64_t order_id, int int_price) {
  InitializeAskLevels(int_price);
  int ask_index = GetAskIndex(int_price);

  if (ask_index > top_ask_index_ || ask_index < 0) {
    return NULL;
  }

  // At this point, ask_index_ is a valid index
  return ask_levels_[ask_index]->FindOrder(order_id);
}

// Returns true if there is a change in l1
bool MarketBook::RemoveMarketBid(ExchMarketOrder* order, QueuePositionUpdate* queue_pos) {
  if (order == NULL) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: NULL order." << DBGLOG_ENDL_FLUSH;
  }

  InitializeBidLevels(order->int_price_);
  int bid_index = GetBidIndex(order->int_price_);

  if (bid_index > top_bid_index_ || bid_index < 0) {
    return false;
  }

  // At this point, bid_index_ is a valid index
  queue_pos->buysell_ = kTradeTypeBuy;
  bid_levels_[bid_index]->Remove(order, queue_pos, mkt_order_mempool_);

  // Level 1 is changed if the order was removed and if it was removed from
  // the level represented by top bid index
  bool l1_changed = (bid_index == top_bid_index_ && queue_pos->action_ == QueuePositionUpdate::OrderRemove);

  AdjustTopBidIndex(bid_index);

  return l1_changed;
}

// Returns true if there is a change in l1
bool MarketBook::RemoveMarketAsk(ExchMarketOrder* order, QueuePositionUpdate* queue_pos) {
  if (order == NULL) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR: NULL order." << DBGLOG_ENDL_FLUSH;
  }

  InitializeAskLevels(order->int_price_);
  int ask_index = GetAskIndex(order->int_price_);

  if (ask_index > top_ask_index_ || ask_index < 0) {
    return false;
  }

  // At this point, ask_index_ is a valid index
  queue_pos->buysell_ = kTradeTypeSell;
  ask_levels_[ask_index]->Remove(order, queue_pos, mkt_order_mempool_);

  // Level 1 is changed if the order was removed and if it was removed from
  // the level represented by top bid index
  bool l1_changed = (ask_index == top_ask_index_ && queue_pos->action_ == QueuePositionUpdate::OrderRemove);

  AdjustTopAskIndex(ask_index);

  return l1_changed;
}

/**
 * Handle the case when order gets executed
 * @param order
 * @param queue_pos
 */
void MarketBook::ExecMarketBid(ExchMarketOrder* order, QueuePositionUpdate* queue_pos) {
  /*
   * Currently we are handling it in MarketOrdersView only as all information is available there only.
   * This function call is created to be used in other tools
   */
}

/**
 * Handle the case when order gets executed
 * @param order
 * @param queue_pos
 */
void MarketBook::ExecMarketAsk(ExchMarketOrder* order, QueuePositionUpdate* queue_pos) {
  /*
   * Currently we are handling it in MarketOrdersView only as all information is available there only.
   * This function call is created to be used in other tools
   */
}

void MarketBook::Reset() {
  // Clear bids
  for (auto bid_level : bid_levels_) {
    bid_level->Clear(mkt_order_mempool_);
  }

  // Clear asks
  for (auto ask_level : ask_levels_) {
    ask_level->Clear(mkt_order_mempool_);
  }

  // Reset variables
  bid_initialized_ = false;
  ask_initialized_ = false;
  bid_adjustment_ = 0;
  ask_adjustment_ = 0;
  top_bid_index_ = 0;
  top_ask_index_ = 0;
}

int MarketBook::GetBidPosition(ExchMarketOrder* order) {
  InitializeBidLevels(order->int_price_);
  int bid_index = GetBidIndex(order->int_price_);

  if (bid_index > top_bid_index_ || bid_index < 0) {
    return -1;
  }

  // At this point, bid_index_ is a valid index
  return bid_levels_[bid_index]->GetPosition(order);
}

int MarketBook::GetAskPosition(ExchMarketOrder* order) {
  InitializeAskLevels(order->int_price_);
  int ask_index = GetAskIndex(order->int_price_);

  if (ask_index > top_ask_index_ || ask_index < 0) {
    return -1;
  }

  // At this point, ask_index_ is a valid index
  return ask_levels_[ask_index]->GetPosition(order);
}

bool MarketBook::IsBidIntPriceL1(int int_price) { return GetBidIndex(int_price) == top_bid_index_; }

bool MarketBook::IsAskIntPriceL1(int int_price) { return GetAskIndex(int_price) == top_ask_index_; }

std::string MarketBook::GetMarketString() {
  if (!IsReady()) {
    return "";
  }

  std::ostringstream oss;
  oss << "[";
  if (top_bid_index_ >= 0 && top_bid_index_ < book_levels_) {
    oss << bid_levels_[top_bid_index_]->NumOrders() << " " << bid_levels_[top_bid_index_]->Size() << " "
        << GetBidIntPrice(top_bid_index_) << " * ";
  } else {
    oss << "--(" << top_bid_index_ << ")-- * ";
  }

  if (top_ask_index_ >= 0 && top_ask_index_ < book_levels_) {
    oss << GetAskIntPrice(top_ask_index_) << " " << ask_levels_[top_ask_index_]->Size() << " "
        << ask_levels_[top_ask_index_]->NumOrders();
  } else {
    oss << "--(" << top_ask_index_ << ")--";
  }

  if (GetBidIntPrice(top_bid_index_) >= GetAskIntPrice(top_ask_index_)) {
    oss << " Cross";
  }
  oss << "]";

  return oss.str();
}

int MarketBook::GetBidSize(int int_price) {
  int bid_index = GetBidIndex(int_price);

  if (bid_index < 0 || bid_index > top_bid_index_) {
    return 0;
  }

  return GetBidLevelSize(bid_index);
}

int MarketBook::GetAskSize(int int_price) {
  int ask_index = GetAskIndex(int_price);

  if (ask_index < 0 || ask_index > top_ask_index_) {
    return 0;
  }

  return GetAskLevelSize(ask_index);
}

int MarketBook::GetBestBidIntPrice() { return GetBidIntPrice(top_bid_index_); }

int MarketBook::GetBestAskIntPrice() { return GetAskIntPrice(top_ask_index_); }

int MarketBook::GetTopOrderSizeBidLevels(int int_price) {
  int bid_index = GetBidIndex(int_price);
  if (bid_index < 0 || bid_index > top_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR" << DBGLOG_ENDL_FLUSH;

    return 0;
  }

  return bid_levels_[bid_index]->GetTopOrderSize();
}

int MarketBook::GetTopOrderSizeAskLevels(int int_price) {
  int ask_index = GetAskIndex(int_price);
  if (ask_index < 0 || ask_index > top_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR" << DBGLOG_ENDL_FLUSH;

    return 0;
  }

  return ask_levels_[ask_index]->GetTopOrderSize();
}

void MarketBook::RemoveBidTopOrder(int int_price) {
  int bid_index = GetBidIndex(int_price);
  bid_levels_[bid_index]->RemoveTopOrderPriority();
}

void MarketBook::RemoveAskTopOrder(int int_price) {
  int ask_index = GetAskIndex(int_price);
  ask_levels_[ask_index]->RemoveTopOrderPriority();
}

std::vector<ExchMarketOrder*>& MarketBook::GetBidOrders(int int_price) {
  int bid_index = GetBidIndex(int_price);

  if (bid_index < 0 || bid_index > top_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " IntPrice: " << int_price << " BidIndex: " << bid_index << DBGLOG_ENDL_FLUSH;

    return bid_levels_[0]->ord_;
  }

  return bid_levels_[bid_index]->ord_;
}

std::vector<ExchMarketOrder*>& MarketBook::GetAskOrders(int int_price) {
  int ask_index = GetAskIndex(int_price);

  if (ask_index < 0 || ask_index > top_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR "
                           << " IntPrice: " << int_price << " AskIndex: " << ask_index << DBGLOG_ENDL_FLUSH;

    return ask_levels_[0]->ord_;
  }

  return ask_levels_[ask_index]->ord_;
}

/**
 * Get the order for last removed order in deletion
 * @param int_price
 * @return
 */
ExchMarketOrder* MarketBook::GetLastRemovedBidOrder(int int_price) {
  auto bid_index = GetBidIndex(int_price);
  if (bid_index < 0 || bid_index >= (int)bid_levels_.size()) {
    // Not checking for top_bid_index as in cases where top level got deleted by last delete message, top_bid_index
    // won't correspond to price of last deleted order
    DBGLOG_TIME_CLASS_FUNC << " ERROR "
                           << " InPrice: " << int_price << " BidIndex: " << bid_index << DBGLOG_ENDL_FLUSH;
    return bid_levels_[0]->last_removed_order_;
  }
  return bid_levels_[bid_index]->last_removed_order_;
}

/**
 * Get the order for last removed order in deletion
 * @param int_price
 * @return
 */
ExchMarketOrder* MarketBook::GetLastRemovedAskOrder(int int_price) {
  auto ask_index = GetAskIndex(int_price);
  if (ask_index < 0 || ask_index >= (int)ask_levels_.size()) {
    // Not checking for top_bid_index as in cases where top level got deleted by last delete message, top_bid_index
    // won't correspond to price of last deleted order
    DBGLOG_TIME_CLASS_FUNC << "ERROR "
                           << " IntPrice: " << int_price << " AskIndex: " << ask_index << DBGLOG_ENDL_FLUSH;
    return ask_levels_[0]->last_removed_order_;
  }
  return ask_levels_[ask_index]->last_removed_order_;
}

/**
 * Get Get the bid order at given price with given position
 * @param position
 * @param int_price
 * @return
 */
ExchMarketOrder* MarketBook::GetBidOrderAtPositionPrice(int position, int int_price) {
  auto bid_index = GetBidIndex(int_price);
  if (bid_index < 0 || bid_index > top_bid_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR "
                           << " IntPrice: " << int_price << " BidIndex: " << bid_index << DBGLOG_ENDL_FLUSH;
    // We can return nullptr as well;
    return bid_levels_[0]->GetOrderAtPosition(position);
  }
  return bid_levels_[bid_index]->GetOrderAtPosition(position);
}

/**
 *
 * @param position
 * @param int_price
 * @return
 */
ExchMarketOrder* MarketBook::GetAskOrderAtPositionPrice(int position, int int_price) {
  auto ask_index = GetAskIndex(int_price);

  if (ask_index < 0 || ask_index > top_ask_index_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR IntPrice: " << int_price << " AskIndex " << ask_index << DBGLOG_ENDL_FLUSH;
    return ask_levels_[0]->GetOrderAtPosition(position);
  }

  return ask_levels_[ask_index]->GetOrderAtPosition(position);
}

bool MarketBook::IsReady() { return bid_initialized_ && ask_initialized_; }

ttime_t MarketBook::GetLastBidAdd(int int_price, int* size) {
  int bid_index = GetBidIndex(int_price);

  if (bid_index < 0 || bid_index > book_levels_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR" << DBGLOG_ENDL_FLUSH;

    *size = 0;
    return ttime_t(0, 0);
  }

  return bid_levels_[bid_index]->GetLastAdd(size);
}

ttime_t MarketBook::GetLastAskAdd(int int_price, int* size) {
  int ask_index = GetAskIndex(int_price);

  if (ask_index < 0 || ask_index > book_levels_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR" << DBGLOG_ENDL_FLUSH;

    *size = 0;
    return ttime_t(0, 0);
  }

  return ask_levels_[ask_index]->GetLastAdd(size);
}
}
