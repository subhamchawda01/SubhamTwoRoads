/**
   \file ExecLogicCode/arb_trading.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "dvctrade/ExecLogic/arb_trading.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

#define FAT_FINGER_FACTOR 5
namespace HFSAT {

void ArbTrading::CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                      const std::string& dep_shortcode, std::vector<std::string>& source_shortcode_vec,
                                      std::vector<std::string>& ors_source_needed_vec,
                                      std::vector<std::string>& dependant_shortcode_vec) {
  if (strategy_name != StrategyName()) {
    return;
  }

  if (dep_shortcode.compare("BR_DOL_0") == 0 || dep_shortcode.compare("BR_WDO_0") == 0) {
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("BR_WDO_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("BR_WDO_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("BR_WDO_0"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("BR_DOL_0"));
  }

  if (dep_shortcode.compare("BR_IND_0") == 0 || dep_shortcode.compare("BR_WIN_0") == 0) {
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("BR_WIN_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("BR_WIN_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("BR_WIN_0"));
    VectorUtils::UniqueVectorAdd(source_shortcode_vec, std::string("BR_IND_0"));
    VectorUtils::UniqueVectorAdd(ors_source_needed_vec, std::string("BR_IND_0"));
    VectorUtils::UniqueVectorAdd(dependant_shortcode_vec, std::string("BR_IND_0"));
  }
}

void ArbTrading::GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec) {
  if (dep_shortcode.compare("BR_DOL_0") == 0 || dep_shortcode.compare("BR_WDO_0") == 0) {
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("BR_WDO_0"));
  }

  if (dep_shortcode.compare("BR_IND_0") == 0 || dep_shortcode.compare("BR_WIN_0") == 0) {
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("BR_IND_0"));
    VectorUtils::UniqueVectorAdd(dep_shortcode_vec, std::string("BR_WIN_0"));
  }
}

ArbTrading::ArbTrading(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& dol_smv,
                       const SecurityMarketView& wdo_smv, SmartOrderManager& dol_order_manager,
                       SmartOrderManager& wdo_order_manager, const std::string& paramfilename, const bool livetrading,
                       ttime_t start_time, ttime_t end_time, int runtime_id, PnlWriter* pnl_writer)
    : ExecInterface(dbglogger, watch, dol_smv, dol_order_manager, paramfilename, livetrading),
      dol_smv_(dol_smv),
      wdo_smv_(wdo_smv),
      dol_om_(dol_order_manager),
      wdo_om_(wdo_order_manager),
      dol_secid_(dol_smv.security_id()),
      wdo_secid_(wdo_smv.security_id()),
      dol_bid_int_price_(0),
      dol_ask_int_price_(0),
      dol_bid_size_(0),
      dol_ask_size_(0),
      dol_bid_orders_(0),
      dol_ask_orders_(0),
      wdo_bid_int_price_(0),
      wdo_ask_int_price_(0),
      wdo_bid_size_(0),
      wdo_ask_size_(0),
      wdo_bid_orders_(0),
      wdo_ask_orders_(0),
      start_time_(start_time),
      end_time_(end_time),
      last_arb_start_msecs_(0),
      last_wdo_arb_bid_int_price_(0),
      last_wdo_arb_ask_int_price_(0),
      last_dol_arb_bid_int_price_(0),
      last_dol_arb_ask_int_price_(0),
      external_getflat_(livetrading),
      runtime_id_(runtime_id),
      getflat_due_to_maxloss_(false),
      num_arb_chances_(0),
      last_arb_size_(0),
      combined_pnl_(new CombinedPnl(dbglogger, watch, dol_smv, wdo_smv, pnl_writer)),
      leading_secid_(dol_secid_),
      dol_mkt_status_(kMktTradingStatusOpen),
      wdo_mkt_status_(kMktTradingStatusOpen),
      getflat_due_to_mkt_status_(false) {
  dol_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  wdo_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);

  dol_smv_.subscribe_MktStatus(this);
  wdo_smv_.subscribe_MktStatus(this);

  wdo_om_.AddPositionChangeListener(this);
  wdo_om_.AddExecutionListener(this);
  wdo_om_.AddCancelRejectListener(this);
  wdo_om_.AddRejectDueToFundsListener(this);
  wdo_om_.AddFokFillRejectListener(this);
}

void ArbTrading::GetFlatLogic() {
  if (my_position() == 0) {
    dol_om_.CancelAllOrders();
    wdo_om_.CancelAllOrders();
  } else if (my_position() > 0) {
    dol_om_.CancelAllBidOrders();
    wdo_om_.CancelAllBidOrders();

    dol_om_.CancelAsksBelowIntPrice(dol_ask_int_price_);
    wdo_om_.CancelAsksBelowIntPrice(wdo_ask_int_price_);

    int pos_to_close = my_position();

    int dol_size_ordered = dol_om_.GetTotalAskSizeEqAboveIntPx(dol_ask_int_price_) * 5;
    int wdo_size_ordered = wdo_om_.GetTotalAskSizeEqAboveIntPx(wdo_ask_int_price_);

    if (pos_to_close - dol_size_ordered - wdo_size_ordered >= 25 && dol_size_ordered == 0) {
      double dol_px = dol_smv_.GetDoublePx(dol_ask_int_price_);
      dol_om_.SendTrade(dol_px, dol_ask_int_price_, 5, kTradeTypeSell, 'B', false);
      pos_to_close -= 25;
    }

    if (pos_to_close - dol_size_ordered - wdo_size_ordered > 0 && wdo_size_ordered == 0) {
      int wdo_size = std::min(6, pos_to_close - dol_size_ordered - wdo_size_ordered);
      double wdo_px = wdo_smv_.GetDoublePx(wdo_ask_int_price_);
      wdo_om_.SendTrade(wdo_px, wdo_ask_int_price_, wdo_size, kTradeTypeSell, 'B', false);
      pos_to_close -= wdo_size;
    }
  } else if (my_position() < 0) {
    dol_om_.CancelAllAskOrders();
    wdo_om_.CancelAllAskOrders();

    dol_om_.CancelBidsBelowIntPrice(dol_bid_int_price_);
    wdo_om_.CancelBidsBelowIntPrice(wdo_bid_int_price_);

    int pos_to_close = -my_position();

    int dol_size_ordered = dol_om_.GetTotalBidSizeEqAboveIntPx(dol_bid_int_price_) * 5;
    int wdo_size_ordered = wdo_om_.GetTotalBidSizeEqAboveIntPx(wdo_bid_int_price_);

    if (pos_to_close - dol_size_ordered - wdo_size_ordered >= 25 && dol_size_ordered == 0) {
      double dol_px = dol_smv_.GetDoublePx(dol_bid_int_price_);
      dol_om_.SendTrade(dol_px, dol_bid_int_price_, 5, kTradeTypeBuy, 'B', false);
      pos_to_close -= 25;
    }

    if (pos_to_close - dol_size_ordered - wdo_size_ordered > 0 && wdo_size_ordered == 0) {
      int wdo_size = std::min(6, pos_to_close - dol_size_ordered - wdo_size_ordered);
      double wdo_px = wdo_smv_.GetDoublePx(wdo_bid_int_price_);
      wdo_om_.SendTrade(wdo_px, wdo_bid_int_price_, wdo_size, kTradeTypeBuy, 'B', false);
      pos_to_close -= wdo_size;
    }
  }
}

// TODO: make this check lighter
bool ArbTrading::ValidPrices() {
  if (dol_bid_int_price_ == kInvalidIntPrice || dol_ask_int_price_ == kInvalidIntPrice ||
      wdo_bid_int_price_ == kInvalidIntPrice || wdo_ask_int_price_ == kInvalidIntPrice ||
      dol_bid_int_price_ >= dol_ask_int_price_ || wdo_bid_int_price_ >= wdo_ask_int_price_ || dol_bid_size_ == 0 ||
      dol_ask_size_ == 0 || wdo_bid_size_ == 0 || wdo_ask_size_ == 0 || dol_bid_orders_ == 0 || dol_ask_orders_ == 0 ||
      wdo_bid_orders_ == 0 || wdo_ask_orders_ == 0) {
    return false;
  }

  return true;
}

std::string ArbTrading::GetCurrentMarket() {
  std::ostringstream oss;
  oss << " Position: " << my_position() << "(" << combined_pnl_->GetDOLPosition() << "+"
      << combined_pnl_->GetWDOPosition() << ").";
  oss << " DOL: [" << dol_bid_size_ << " " << dol_bid_int_price_ << " * " << dol_ask_int_price_ << " " << dol_ask_size_
      << "]"
      << " WDO: [" << wdo_bid_size_ << " " << wdo_bid_int_price_ << " * " << wdo_ask_int_price_ << " " << wdo_ask_size_
      << "]";
  oss << " Pnl: " << combined_pnl_->GetUnrealizedPnl();

  return oss.str();
}

bool ArbTrading::ShouldGetFlat() {
  if (!dol_smv_.is_ready() || !wdo_smv_.is_ready()) {
    return true;
  }

  if (dol_mkt_status_ != kMktTradingStatusOpen || wdo_mkt_status_ != kMktTradingStatusOpen) {
    if (!getflat_due_to_mkt_status_) {
      getflat_due_to_mkt_status_ = true;
      DBGLOG_TIME << "getflat_due_to_mkt_status. DOL mktstatus: " << dol_mkt_status_
                  << " WDO mktstatus: " << wdo_mkt_status_ << DBGLOG_ENDL_FLUSH;
    }
  } else {
    if (getflat_due_to_mkt_status_) {
      DBGLOG_TIME << "resuming from getflat_due_to_mkt_status." << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    getflat_due_to_mkt_status_ = false;
  }

  if (getflat_due_to_mkt_status_) {
    return true;
  }

  if (!ValidPrices()) {
    return true;
  }

  if (watch_.tv() < start_time_ || watch_.tv() > end_time_) {
    return true;
  }

  if (external_getflat_) {
    return true;
  }

  if (combined_pnl_->GetUnrealizedPnl() < -param_set_.max_loss_) {
    if (!getflat_due_to_maxloss_) {
      getflat_due_to_maxloss_ = true;
      DBGLOG_TIME << " getflat_due_to_max_loss_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

      // Send the mail if live-trading and within trading window
      if (livetrading_ && watch_.tv() > start_time_ && watch_.tv() < end_time_) {
        // Send out an email / alert
        char hostname[128];
        hostname[127] = '\0';
        gethostname(hostname, 127);

        std::string getflat_email_string = "";
        {
          std::ostringstream oss;
          oss << "Strategy: " << runtime_id_ << " getflat_due_to_max_loss_: " << combined_pnl_->GetUnrealizedPnl()
              << " ArbTrading products " << dol_smv_.shortcode() << " (" << dol_smv_.secname() << ") and "
              << dol_smv_.shortcode() << " (" << dol_smv_.secname() << ")"
              << " on " << hostname << "\n";
          getflat_email_string = oss.str();
        }

        Email email;
        email.setSubject(getflat_email_string);
        email.addRecepient("nseall@tworoads.co.in");
        email.addSender("nseall@tworoads.co.in");
        email.content_stream << getflat_email_string << "<br/>";
        email.sendMail();
      }
    }
  } else {
    if (getflat_due_to_maxloss_) {
      DBGLOG_TIME << "resuming from getflat_due_to_maxloss." << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    getflat_due_to_maxloss_ = false;
  }

  if (getflat_due_to_maxloss_) {
    return true;
  }

  return false;
}

// Checks if we can initiate a new arb trade at this point
bool ArbTrading::IsReadyToTakeArbTrade() {
  // For the time being, just check the total position
  // TODO: make "50" a variable !
  if (abs(my_position()) > 50) {
    return false;
  }

  //  if (leading_secid_ == wdo_secid_) {
  //    return false;
  //  }

  if (watch_.msecs_from_midnight() - last_arb_start_msecs_ >= 2000) {
    return true;
  }

  return false;
}

bool ArbTrading::StartArbTrade() {
  if (wdo_bid_int_price_ <= dol_ask_int_price_ && wdo_ask_int_price_ >= dol_bid_int_price_) {
    // No arb opportunity at this point
    return false;
  }

  num_arb_chances_++;
  int size_placed = 0;

  if (wdo_bid_int_price_ > dol_ask_int_price_) {
    if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "AggressWDOSell." << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
    }

    int size_available = 0;
    int wdo_int_price = wdo_bid_int_price_;
    for (; wdo_int_price > dol_ask_int_price_; wdo_int_price--) {
      size_available += wdo_smv_.bid_size_at_int_price(wdo_int_price);
      if (size_available >= 50) {
        break;
      }
    }

    // Make sure that wdo_int_price > dol_ask_int_price_
    wdo_int_price = std::max(dol_ask_int_price_ + 1, wdo_int_price);

    last_arb_size_ = size_available;

    size_available = std::min(size_available, 50);

    if (std::max(size_available, size_available + my_position()) >= param_set_.min_size_to_join_) {
      // Place aggressive IOC order in wdo
      int size_ordered = wdo_om_.GetTotalAskSizeEqAboveIntPx(
          wdo_bid_int_price_);  // TODO - hardik: we should include size_ordered in position check
      int wdo_size = std::min(size_available - size_ordered, 50);  // Restrict max order size by 50 here.

      if (wdo_size > 2) {
        wdo_om_.SendTrade(wdo_smv_.GetDoublePx(wdo_int_price), wdo_int_price, wdo_size, kTradeTypeSell, 'A', kOrderIOC);
        size_placed += wdo_size;
      }

      //      if (size_placed >= 30) {
      //        dol_om_.SendTrade(dol_smv_.GetDoublePx(dol_ask_int_price_), dol_ask_int_price_, 5, kTradeTypeBuy, 'A',
      //        kOrderIOC);
      //        size_placed += 25;
      //      }
    }
  } else if (wdo_ask_int_price_ < dol_bid_int_price_) {
    if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "AggressWDOBuy." << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
    }

    int size_available = 0;
    int wdo_int_price = wdo_ask_int_price_;
    for (; wdo_int_price < dol_bid_int_price_; wdo_int_price++) {
      size_available += wdo_smv_.ask_size_at_int_price(wdo_int_price);
      if (size_available >= 50) {
        break;
      }
    }

    // Make sure that wdo_int_price < dol_bid_int_price_
    wdo_int_price = std::min(dol_bid_int_price_ - 1, wdo_int_price);

    last_arb_size_ = size_available;

    size_available = std::min(size_available, 50);

    if (std::max(size_available, size_available - my_position()) >= param_set_.min_size_to_join_) {
      // Place aggressive IOC order in wdo
      int size_ordered = wdo_om_.GetTotalBidSizeEqAboveIntPx(wdo_ask_int_price_);
      int wdo_size = std::min(size_available - size_ordered, 50);

      // What is the minimum size worth sending order for?
      if (wdo_size > 2) {
        wdo_om_.SendTrade(wdo_smv_.GetDoublePx(wdo_int_price), wdo_int_price, wdo_size, kTradeTypeBuy, 'A', kOrderIOC);
        size_placed += wdo_size;
      }

      //      if (size_placed >= 30) {
      //        dol_om_.SendTrade(dol_smv_.GetDoublePx(dol_bid_int_price_), dol_bid_int_price_, 5, kTradeTypeSell, 'A',
      //        kOrderIOC);
      //        size_placed += 25;
      //      }
    }
  }

  if (size_placed > 0) {
    last_arb_start_msecs_ = watch_.msecs_from_midnight();
    last_wdo_arb_bid_int_price_ = wdo_bid_int_price_;
    last_wdo_arb_ask_int_price_ = wdo_ask_int_price_;
    last_dol_arb_bid_int_price_ = dol_bid_int_price_;
    last_dol_arb_ask_int_price_ = dol_ask_int_price_;
  }

  return (size_placed > 0);
}

int ArbTrading::SendDOLBid(int size, int int_price) {
  if (size == 0) {
    return 0;
  }

  if (int_price == 0) {
    int_price = dol_bid_int_price_;
  }
  size = std::min(10, size);  // Restricting to size 10 (equivalent to 50 WDO)

  dol_om_.SendTrade(dol_smv_.GetDoublePx(int_price), int_price, size, kTradeTypeBuy, 'B', kOrderDay);

  return size;
}

int ArbTrading::SendDOLAsk(int size, int int_price) {
  if (size == 0) {
    return 0;
  }

  if (int_price == 0) {
    int_price = dol_ask_int_price_;
  }
  size = std::min(10, size);  // Restricting to size 10 (equivalent to 50 WDO)

  dol_om_.SendTrade(dol_smv_.GetDoublePx(int_price), int_price, size, kTradeTypeSell, 'B', kOrderDay);

  return size;
}

int ArbTrading::SendWDOBid(int size, int int_price) {
  if (size == 0) {
    return 0;
  }

  if (int_price == 0) {
    int_price = wdo_bid_int_price_;
  }
  size = std::min(12, size);  // Restricting to size 12

  wdo_om_.SendTrade(wdo_smv_.GetDoublePx(int_price), int_price, size, kTradeTypeBuy, 'B', kOrderDay);

  return size;
}

int ArbTrading::SendWDOAsk(int size, int int_price) {
  if (size == 0) {
    return 0;
  }

  if (int_price == 0) {
    int_price = wdo_ask_int_price_;
  }
  size = std::min(12, size);  // Restricting to size 12

  wdo_om_.SendTrade(wdo_smv_.GetDoublePx(int_price), int_price, size, kTradeTypeSell, 'B', kOrderDay);

  return size;
}

int ArbTrading::ClosePositivePosition() {
  int size_placed = 0;

  // Get the ask size already placed
  int dol_size_placed = dol_om_.SumAskSizes() * 5;
  int wdo_size_placed = wdo_om_.SumAskSizes();

  // Compute the size we want to close in this function call
  int size_to_close = my_position() - dol_size_placed - wdo_size_placed;

  // Determine the price at which we want to close
  // TODO: Change this to place improve/aggres orders only when arb still exists. Otherwise, use different passive close
  // logic
  int int_price = dol_ask_int_price_;
  if (dol_bid_int_price_ >= last_dol_arb_bid_int_price_) {
    // Arb still exists, improve
    int_price = dol_ask_int_price_ - 1;
  }

  // Place closing orders
  if (size_to_close > 0) {
    int dol_size = ((size_to_close + 12) / 25) * 5;
    int dol_placed = SendDOLAsk(dol_size, int_price);
    size_placed += dol_placed * 5;

    if (size_to_close - size_placed > 0) {
      // Only place more orders only when existing orders are either executed or cancelled
      // There is no point in accumulating the orders.
      if (wdo_size_placed <= 5) {
        int wdo_placed = SendWDOAsk(size_to_close - size_placed, int_price);
        size_placed += wdo_placed;
      }

      // Cancel orders below the price at which we just placed the orders
      wdo_om_.CancelAsksBelowIntPrice(int_price);

      // Cancel all bids
      wdo_om_.CancelAllBidOrders();
    } else {
      int wdo_size_to_close = size_placed - size_to_close;
      int wdo_bids_placed = wdo_om_.SumBidSizes();

      if (wdo_size_to_close > wdo_bids_placed) {
        int wdo_placed = SendWDOBid(wdo_size_to_close - wdo_bids_placed, wdo_bid_int_price_);
        size_placed += wdo_placed;

        // Cancel bids below best bid in WDO
        wdo_om_.CancelBidsBelowIntPrice(wdo_bid_int_price_);
      } else {
        // TODO: cancel from far?
      }
      // Cancel all asks
      wdo_om_.CancelAllAskOrders();
    }

    // Cancel DOL orders below the price at which just placed the orders
    dol_om_.CancelAsksBelowIntPrice(int_price);
  } else {
    // Cancel orders below best levels
    dol_om_.CancelAsksBelowIntPrice(dol_ask_int_price_);
    wdo_om_.CancelAsksBelowIntPrice(wdo_ask_int_price_);
  }

  return size_placed;
}

int ArbTrading::CloseNegativePosition() {
  int size_placed = 0;

  // Get the bid size already placed
  int dol_size_placed = dol_om_.SumBidSizes() * 5;
  int wdo_size_placed = wdo_om_.SumBidSizes();

  // Compute the size to close in this function call
  int size_to_close = -my_position() - dol_size_placed - wdo_size_placed;

  // Determine the price at which we want to close this risk
  int int_price = dol_bid_int_price_;
  if (dol_ask_int_price_ <= last_dol_arb_ask_int_price_) {
    // Arb still exists, improve
    int_price = dol_bid_int_price_ + 1;
  }

  if (size_to_close > 0) {
    int dol_size = ((size_to_close + 12) / 25) * 5;
    int dol_placed = SendDOLBid(dol_size, int_price);
    size_placed += dol_placed * 5;

    if (size_to_close - size_placed > 0) {
      // Only place more orders only when existing orders are either executed or cancelled
      // There is no point in accumulating the orders.
      if (wdo_size_placed <= 5) {
        int wdo_placed = SendWDOBid(size_to_close - size_placed, int_price);
        size_placed += wdo_placed;
      }

      // Cancel bids below this price
      wdo_om_.CancelBidsBelowIntPrice(int_price);

      // Cancel all asks
      wdo_om_.CancelAllAskOrders();
    } else {
      int wdo_size_to_close = size_placed - size_to_close;
      int wdo_ask_placed = wdo_om_.SumAskSizes();

      if (wdo_size_to_close > wdo_ask_placed) {
        int wdo_placed = SendWDOAsk(wdo_size_to_close - wdo_ask_placed, wdo_ask_int_price_);
        size_placed += wdo_placed;

        // Cancel bids below best ask in WDO
        wdo_om_.CancelAsksBelowIntPrice(wdo_ask_int_price_);
      } else {
        // TODO: cancel some asks?
      }
      // Cancel all bids
      wdo_om_.CancelAllBidOrders();
    }

    // Cancel DOL orders below the price at which we just placed the orders
    dol_om_.CancelBidsBelowIntPrice(int_price);
  } else {
    // Cancel orders below best level
    dol_om_.CancelBidsBelowIntPrice(dol_bid_int_price_);
    wdo_om_.CancelBidsBelowIntPrice(wdo_bid_int_price_);
  }

  return size_placed;
}

bool ArbTrading::IsLastIOCOrderIncomplete() {
  if (wdo_om_.IOCOrderExists() || dol_om_.IOCOrderExists()) {
    return true;
  }
  return false;
}

bool ArbTrading::CloseArbTrade() {
  int size_placed = 0;

  // If we're flat, we might want to cancel the existing orders
  if (my_position() == 0) {
    dol_om_.CancelAllOrders();
    wdo_om_.CancelAllOrders();
  } else if (IsLastIOCOrderIncomplete()) {
    return false;
  }

  // Sell in DOL/WDO
  else if (my_position() > 0) {
    size_placed += ClosePositivePosition();
  }
  // Buy in DOL/WDO
  else if (my_position() < 0) {
    size_placed += CloseNegativePosition();
  }

  if (size_placed > 0 && dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "ClosePositions: " << size_placed << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }

  return (size_placed > 0);
}

void ArbTrading::TradingLogic(int t_security_id_of_the_product_whose_event_we_are_responding_to) {
  // change the if condition to optimize the
  // leg that is latency sensitive
  if (ShouldGetFlat()) {
    GetFlatLogic();
    return;
  }

  if (IsReadyToTakeArbTrade() && StartArbTrade()) {
    // If we have placed arb order, return from this point.
    // Otherwise, we might want to close the open position
    return;
  }

  if (CloseArbTrade()) {
    // If we have placed order to close arb position, return from this point.
    // Otherwise, we might want to close the open position
    return;
  }
}

void ArbTrading::UpdateVariables(int security_id) {
  // DOL
  if (security_id == dol_secid_) {
    dol_bid_int_price_ = dol_smv_.bestbid_int_price();
    dol_ask_int_price_ = dol_smv_.bestask_int_price();
    dol_bid_size_ = dol_smv_.bestbid_size();
    dol_ask_size_ = dol_smv_.bestask_size();
    dol_bid_orders_ = dol_smv_.bestbid_ordercount();
    dol_ask_orders_ = dol_smv_.bestask_ordercount();
  }

  // WDO
  else if (security_id == wdo_secid_) {
    wdo_bid_int_price_ = wdo_smv_.bestbid_int_price();
    wdo_ask_int_price_ = wdo_smv_.bestask_int_price();
    wdo_bid_size_ = wdo_smv_.bestbid_size();
    wdo_ask_size_ = wdo_smv_.bestask_size();
    wdo_bid_orders_ = wdo_smv_.bestbid_ordercount();
    wdo_ask_orders_ = wdo_smv_.bestask_ordercount();
  }
}

void ArbTrading::OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info) {
  UpdateVariables(security_id);

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OpenPosition." << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }

  TradingLogic(security_id);
}

void ArbTrading::OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                              const MarketUpdateInfo& market_update_info) {
  UpdateVariables(security_id);

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "OpenPosition. ";

    if ((int)security_id == dol_secid_) {
      dbglogger_ << dol_smv_.secname() << " ";
    } else if ((int)security_id == wdo_secid_) {
      dbglogger_ << wdo_smv_.secname() << " ";
    }

    dbglogger_ << trade_print_info.ToString() << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
  }

  leading_secid_ = security_id;

  TradingLogic(security_id);
}

// Called from BaseOrderManager, because we have subscribed to execs by calling AddExecutionListener
// OnExec is called before OnPositionChange
// Note: base_pnl writes the trade upon OnExec call and not OnPositionChange call
// Subscribed for both DOL and WDO.
void ArbTrading::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
                        const int int_price, const int _security_id_) {
  combined_pnl_->OnExec(new_position, exec_quantity, buysell, price, int_price, _security_id_);
  TradingLogic(_security_id_);
}

// Called from BaseOrderManager, because we have subscribed to position change by calling AddPositionListener
// Subscribed for both DOL and WDO.
void ArbTrading::OnPositionChange(int new_position, int position_diff, const unsigned int security_id) {}

void ArbTrading::ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close) {
  double dol_pnl = combined_pnl_->GetDOLPnl();
  double wdo_pnl = combined_pnl_->GetWDOPnl();
  // double total_pnl = dol_pnl + wdo_pnl;

  int dol_volume = dol_om_.trade_volume();
  int wdo_volume = wdo_om_.trade_volume();
  int total_volume = dol_volume * 5 + wdo_volume;

  std::cout << "DOL. PNL: " << dol_pnl << " Volume: " << dol_volume << " Position: " << combined_pnl_->GetDOLPosition()
            << std::endl;
  std::cout << "WDO. PNL: " << wdo_pnl << " Volume: " << wdo_volume << " Position: " << combined_pnl_->GetWDOPosition()
            << std::endl;
  std::cout << "Total. PNL: " << combined_pnl_->GetUnrealizedPnl() << " Volume: " << total_volume
            << " Position: " << combined_pnl_->GetTotalPosition() << std::endl;
}

void ArbTrading::get_positions(std::vector<std::string>& instrument_vec, std::vector<int>& position_vec) {
  if (abs(combined_pnl_->GetDOLPosition()) > 0) {
    instrument_vec.push_back(dol_smv_.secname());
    position_vec.push_back(combined_pnl_->GetDOLPosition());
  }

  if (abs(combined_pnl_->GetWDOPosition()) > 0) {
    instrument_vec.push_back(wdo_smv_.secname());
    position_vec.push_back(combined_pnl_->GetWDOPosition());
  }
}

void ArbTrading::OnWakeUpifRejectDueToFunds() {}

void ArbTrading::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {}

void ArbTrading::OnMarketDataResumed(const unsigned int security_id) {}

void ArbTrading::OnGlobalPNLChange(double new_global_PNL) {}

void ArbTrading::OnOrderChange() {}

void ArbTrading::OnGlobalPositionChange(const unsigned int security_id, int new_global_position) {}

void ArbTrading::OnControlUpdate(const ControlMessage& control_message, const char* symbol, const int trader_id) {
  switch (control_message.message_code_) {
    case kControlMessageCodeGetflat: {
      DBGLOG_TIME_CLASS_FUNC << "getflat_due_to_external_getflat_ " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      external_getflat_ = true;
      break;
    }
    case kControlMessageCodeStartTrading: {
      DBGLOG_TIME_CLASS_FUNC << "StartTrading Called " << runtime_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      external_getflat_ = false;
      break;
    }
    case kControlMessageCodeShowIndicators: {
      DBGLOG_TIME_CLASS_FUNC << "ShowIndicators " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << dol_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << dol_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << wdo_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << wdo_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << wdo_smv_.secname()
                             << " messages: " << wdo_om_.SendOrderCount() + wdo_om_.CxlOrderCount() << " "
                             << dol_smv_.secname()
                             << " messages: " << dol_om_.SendOrderCount() + dol_om_.CxlOrderCount()
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << dol_smv_.secname() << " status: " << dol_mkt_status_ << " " << wdo_smv_.secname()
                             << " status: " << wdo_mkt_status_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
      break;
    }
    case kControlMessageCodeShowOrders: {
      DBGLOG_TIME_CLASS_FUNC << "ShowOrders " << runtime_id_ << " called." << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << dol_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << dol_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << wdo_smv_.secname() << " OM: " << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME << wdo_om_.ShowOrders() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << GetCurrentMarket() << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << "NumArbChances: " << num_arb_chances_ << " last_size_available: " << last_arb_size_
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << wdo_smv_.secname()
                             << " messages: " << wdo_om_.SendOrderCount() + wdo_om_.CxlOrderCount() << " "
                             << dol_smv_.secname()
                             << " messages: " << dol_om_.SendOrderCount() + dol_om_.CxlOrderCount()
                             << DBGLOG_ENDL_FLUSH;
      DBGLOG_TIME_CLASS_FUNC << " " << dol_smv_.secname() << " status: " << dol_mkt_status_ << " " << wdo_smv_.secname()
                             << " status: " << wdo_mkt_status_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeShowParams: {
      break;
    }
    case kControlMessageCodeSetMaxLoss: {
      if (control_message.intval_1_ > param_set_.max_loss_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_loss_ * FAT_FINGER_FACTOR) {
        param_set_.max_loss_ = control_message.intval_1_;
      }

      DBGLOG_TIME_CLASS << "SetMaxLoss " << runtime_id_ << " called for abs_max_loss_ = " << control_message.intval_1_
                        << " and MaxLoss for param set to " << param_set_.max_loss_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeSetOpenTradeLoss: {
      if (control_message.intval_1_ > param_set_.max_opentrade_loss_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_opentrade_loss_ * FAT_FINGER_FACTOR) {
        param_set_.max_opentrade_loss_ = control_message.intval_1_;
      }

      DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << runtime_id_
                        << " called for abs_opentrade_loss_ = " << control_message.intval_1_
                        << " and OpenTradeLoss for param set to " << param_set_.max_opentrade_loss_
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      break;
    }
    case kControlMessageCodeSetMaxPosition: {
      if (control_message.intval_1_ > param_set_.max_position_ / FAT_FINGER_FACTOR &&
          control_message.intval_1_ < param_set_.max_position_ * FAT_FINGER_FACTOR) {
        param_set_.max_position_ = std::max(1, control_message.intval_1_);
        param_set_.max_position_original_ = param_set_.max_position_;
      }
      DBGLOG_TIME_CLASS << "SetMaxPosition " << runtime_id_ << " called with " << control_message.intval_1_
                        << " and MaxPosition for param set to " << param_set_.max_position_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } break;
    default: { break; }
  }
}

bool ArbTrading::UpdateTarget(double new_target, double targetbias_numbers, int modelmath_index) { return false; }

void ArbTrading::OnFokReject(const TradeType_t buysell, const double price, const int int_price,
                             const int size_remaining) {}

void ArbTrading::TargetNotReady() {}

void ArbTrading::OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status) {
  if ((int)security_id == dol_secid_) {
    dol_mkt_status_ = new_market_status;
  } else if ((int)security_id == wdo_secid_) {
    wdo_mkt_status_ = new_market_status;
  }
  ShouldGetFlat();
  DBGLOG_TIME_CLASS_FUNC << "MktStatus for security_id: " << security_id << " changed to: " << new_market_status
                         << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}
}
