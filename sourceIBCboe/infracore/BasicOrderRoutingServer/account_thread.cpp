/**
\file BasicOrderRoutingServer/account_thread.cpp

\author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite 217, Level 2, Prestige Omega,
No 104, EPIP Zone, Whitefield,
Bangalore - 560066
India
+91 80 4060 0717
*/

#include <math.h>

#include "infracore/BasicOrderRoutingServer/account_thread.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/multisession_engine.hpp"
#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/BSE/BSEEngine.hpp"
#include "infracore/CBOE/CBOEEngine.hpp"
#include "infracore/SimEngine/SimEngine.hpp"
#include "infracore/BasicOrderRoutingServer/ors_controller_thread.hpp"
#include "dvccode/CDef/assumptions.hpp"

using std::string;

namespace HFSAT {
namespace ORS {

AccountThread::AccountThread(DebugLogger& _dbglogger_, OrderManager& _order_manager_, Settings& r_settings_,
                             HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_ptr_,
                             std::string t_output_log_dir_)
    : dbglogger_(_dbglogger_),
      simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      position_manager_(PositionManager::GetUniqueInstance()),
      order_manager_(_order_manager_),
      bcast_manager_(BroadcastManager::GetUniqueInstance(_dbglogger_, "", 0,
                                                         (atoi(r_settings_.getValue("Client_Base").c_str())) << 16)),
      m_settings(r_settings_),
      client_logging_segment_initializer_ptr_(_client_logging_segment_initializer_ptr_),
      p_base_engine_(NULL),
      connected_(false),
      loggedin_(false),
      margin_(0),
      ismarginstatechanged_(false),
      executed_saos_map_(),
      this_exchange_source_(HFSAT::StringToExchSource(m_settings.getValue("Exchange"))),
      saos_to_eaos_map_lock_(),
      sec_id_to_reject_side_(),
      using_notional_value_checks_("BMFEQ" == r_settings_.getValue("Exchange") && r_settings_.has("EquityMargin") &&
                                   r_settings_.getValue("EquityMargin") == "Y"),
      log_buffer_(new HFSAT::CDef::LogBuffer()),
      was_last_order_throttled_(false),
      use_lockfree_ors_(((r_settings_.has("UseLockFreeORS") && r_settings_.getValue("UseLockFreeORS") == "Y"))),
      ors_pnl_manager_(HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_)),
      ors_margin_manager_(HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_)) {
  // Initialize engine and heartbeat manager based on exchange
  Initialize(t_output_log_dir_);

  memset((void*)log_buffer_, 0, sizeof(HFSAT::CDef::LogBuffer));

  // Assign Type
  log_buffer_->content_type_ = HFSAT::CDef::ORSTrade;
}

AccountThread::~AccountThread() {
  if (p_base_engine_ != NULL) {
    delete p_base_engine_;
    p_base_engine_ = NULL;
  }
}

void AccountThread::Connect() {
  if (!connected_) {
    p_base_engine_->Connect();
    p_base_engine_->run();  // starts a new thread

    // Only if we are using normal lock based ORS we will run the extra thread
    if (false == use_lockfree_ors_) HFSAT::ORS::ORSControllerThread::GetUniqueInstance().run();
  }
}

void AccountThread::Initialize(std::string t_output_log_dir_) {
  string exchange = m_settings.getValue("Exchange");
  if (exchange == "BMFEP" || "BMFEQ" == exchange || "NSE_FO" == exchange ||
      "NSE_CD" == exchange || "NSE_EQ" == exchange || "MSSIM" == exchange ||
      "BSE_FO" == exchange || "BSE_CD" == exchange || "BSE_EQ" == exchange ||
      "CBOE_FO" == exchange || "CBOE_EQ" == exchange || "CBOE_CD" == exchange) {
    p_base_engine_ = makeMultiSessionEngine(exchange, m_settings, dbglogger_, t_output_log_dir_);
  } else if ("SIM" == exchange) {
    p_base_engine_ = new HFSAT::SimEngine::SIMEngine(m_settings, dbglogger_, t_output_log_dir_, -1, nullptr, nullptr);
  } else {
    p_base_engine_ = NULL;
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Fatal Error: Specify Exchange in Config File.\n";
      dbglogger_.DumpCurrentBuffer();
    }
    // fprintf( stderr, "Fatal Error: Specify Exchange in Config File .\n" );
    dbglogger_.Close();
    exit(-1);
  }
  p_base_engine_->setListener(this);
}

void AccountThread::Login() {
  // Engine Should be responsible for checking whether it's already logged in or not
  // while listener needs to process the control as expected.

  if (!connected_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Technical Error: Login called without being connected. But not exiting \n";
      dbglogger_.DumpCurrentBuffer();
    }
  } else {  // If Engine is already loggeed in it will have to check that
    // and return from there.

    if (HFSAT::kExchSourceNSE != this_exchange_source_ && HFSAT::kExchSourceNSE_FO != this_exchange_source_ &&
        HFSAT::kExchSourceNSE_EQ != this_exchange_source_ && HFSAT::kExchSourceNSE_CD != this_exchange_source_ &&
        HFSAT::kExchSourceBSE != this_exchange_source_ && HFSAT::kExchSourceBSE_FO != this_exchange_source_ &&
        HFSAT::kExchSourceBSE_EQ != this_exchange_source_ && HFSAT::kExchSourceBSE_CD != this_exchange_source_ ) {
      p_base_engine_->Login();
    }
  }

}

void AccountThread::Logout() {
  if (!loggedin_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Technical Error: Logout called without being Logged on. But not exiting now\n ";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  // Once we log out, we will not be able to cancel orders from this session easily.
  // Cancel any pending orders.
  CancelAllPendingOrders();
  p_base_engine_->Logout();
}

void AccountThread::DisConnect() {
  if (!connected_ || loggedin_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Technical Error: Disconnect called improperly butnot exiting. Connected: "
                 << (connected_ ? "YES" : "NO") << " \t LoggedIn: " << (loggedin_ ? "YES" : "NO") << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }
  if (connected_) {
    p_base_engine_->DisConnect();
  }
}

void AccountThread::DumpSettingsToFile() { p_base_engine_->DumpSettingsToFile(); }

void AccountThread::CommitFixSequences() { p_base_engine_->CommitFixSequences(); }

void AccountThread::SendTrade(Order* p_new_order_) {
  if (!loggedin_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " SendTrade called without being logged in.\n";
      dbglogger_.DumpCurrentBuffer();
    }
    return;
  }

  order_manager_.AddToActiveMap(p_new_order_);

  // write access to order size sum maps on control flow of ClientThread
  if (p_new_order_->buysell_ == kTradeTypeBuy) {
    position_manager_.AddBidSize(p_new_order_->security_id_, p_new_order_->size_remaining_,
                                 p_new_order_->size_remaining_, 1);

  } else {
    position_manager_.AddAskSize(p_new_order_->security_id_, p_new_order_->size_remaining_,
                                 p_new_order_->size_remaining_, 1);
  }

  p_base_engine_->SendOrder(p_new_order_);
}

void AccountThread::Cancel(const int _server_assigned_order_sequence_) {
  if (!loggedin_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " called without being logged in.\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(_server_assigned_order_sequence_);

  if ((p_this_order_ != NULL) && (!p_this_order_->order_not_found_)) {  // Found order

    if (p_this_order_->is_confirmed_) {
      // dbglogger_<<"Canceling "<<_server_assigned_order_sequence_<<" through here 1\n";
      p_base_engine_->CancelOrder(p_this_order_);
    } else {
      bcast_manager_->BroadcastCancelRejection(*p_this_order_, kCxlRejectReasonOrderNotConfirmed);
    }
  }
  // Else - most likely already cancelled or filled
}

void AccountThread::Cancel(Order* p_this_order_) {
  // dbglogger_<<"Canceling "<<p_this_order_->server_assigned_order_sequence_<<" through here 2\n";
  p_base_engine_->CancelOrder(p_this_order_);
}

void AccountThread::CxlReplace(Order* p_new_order_) {
  if (!loggedin_) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " called without being logged in.\n";
      dbglogger_.DumpCurrentBuffer();
    }
    return;
  }

  Order* p_curr_order_ = order_manager_.GetOrderByOrderSequence(p_new_order_->server_assigned_order_sequence_);
  if (p_curr_order_ == NULL || (p_curr_order_->order_not_found_)) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for order that cannot be located "
                 << p_new_order_->server_assigned_order_sequence_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    return;
  }

  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << "AccountThread :: CxlReplace\n\n";
    dbglogger_.DumpCurrentBuffer();
  }

  p_base_engine_->ModifyOrder(p_new_order_, p_new_order_);
}

void AccountThread::OnOrderConf(int server_assigned_seqnum, const char* exch_assigned_seqnum, double price, int size,
                                int exch_assigned_seqnum_length_, uint64_t _exch_seq_, int entry_dt, int64_t last_activity_ref) {

  DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER CONF FOR SAOS : " <<  server_assigned_seqnum <<  DBGLOG_ENDL_DUMP;
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
  if (p_this_order_ == NULL) {
    p_this_order_ = order_manager_.GetMirrorOrderBySAOS(server_assigned_seqnum);
    if (NULL == p_this_order_) {
      if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
        dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderConf for SAOS "
                   << server_assigned_seqnum << "\n";
        dbglogger_.DumpCurrentBuffer();
      }
      return;
    }
  }

  if (this_exchange_source_ == kExchSourceEUREX) {
    // Checking if the variables price,size are not pre-filled before this function call
    if (std::fabs(price - (ETI_DUMMY_ORDER_PRICE_VALUE)) <
        0.0001)  // correct way of checking if two floating point values are equal
      price = p_this_order_->price_;
    if (size == ETI_DUMMY_ORDER_SIZE_VALUE) size = p_this_order_->size_remaining_;
  }

  bzero(p_this_order_->exch_assigned_order_sequence_, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);
  memcpy(p_this_order_->exch_assigned_order_sequence_, exch_assigned_seqnum, exch_assigned_seqnum_length_ + 1);

  p_this_order_->exch_assigned_order_sequence_length_ = exch_assigned_seqnum_length_;
  p_this_order_->exch_assigned_seq_ = _exch_seq_;
  p_this_order_->entry_dt_ = entry_dt;
  p_this_order_->last_mod_dt_ = entry_dt;
  p_this_order_->last_activity_reference_ = last_activity_ref;
  //p_this_order_->last_confirmed_size_remain_ = size;
  p_this_order_->is_confirmed_ = true;

  if (fabs(p_this_order_->price_ - price) > 0.00001) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Order confirmed at different price! Origprice: " << p_this_order_->price_ << " : " << price
                 << " SAOS: " << p_this_order_->server_assigned_order_sequence_
                 << " SACI: " << p_this_order_->server_assigned_client_id_ << " \n";
      dbglogger_.DumpCurrentBuffer();
    }

    std::ostringstream t_temp_oss;
    t_temp_oss << typeid(*this).name() << ':' << __func__
               << " Order confirmed at different price! Origprice: " << p_this_order_->price_ << " : " << price
               << " SAOS: " << p_this_order_->server_assigned_order_sequence_
               << " SACI: " << p_this_order_->server_assigned_client_id_ << " \n";

    //EmailForWeirdEngineReply(t_temp_oss.str());
    //p_this_order_->price_ = price;
  }
/*
  if (p_this_order_->size_remaining_ != size) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order confirmed for a different size "
                 << p_this_order_->size_remaining_ << " : " << size
                 << " SAOS: " << p_this_order_->server_assigned_order_sequence_
                 << " SACI: " << p_this_order_->server_assigned_client_id_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    std::ostringstream t_temp_oss;
    t_temp_oss << typeid(*this).name() << ':' << __func__ << " Order confirmed for a different size "
               << p_this_order_->size_remaining_ << " : " << size
               << " SAOS: " << p_this_order_->server_assigned_order_sequence_
               << " SACI: " << p_this_order_->server_assigned_client_id_ << "\n";

    EmailForWeirdEngineReply(t_temp_oss.str());

    p_this_order_->size_remaining_ = size;
  }
*/

  bcast_manager_->BroadcastConfirm(*p_this_order_);
}

void AccountThread::OnOrderConfBMF(int server_assigned_seqnum, uint64_t _exch_seq_) {
  //      std::cout << "exch_seq: " << _exch_seq_ << std::endl << std::flush;

  // find the order that has SAOS = _cl_ord_id_
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
  if (p_this_order_ == NULL) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderConf for SAOS "
                 << server_assigned_seqnum << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    // TODO might put other handling after checking
    return;
  }

  p_this_order_->exch_assigned_seq_ = _exch_seq_;

  // saos_to_eaos_map_lock_.LockMutex ();
  // std::string exch_assigned_seq_ = exch_assigned_seqnum;
  // saos_to_eaos_map_[server_assigned_seqnum] = exch_assigned_seq_;
  // saos_to_eaos_map_lock_.UnlockMutex ();

  p_this_order_->is_confirmed_ = true;

  bcast_manager_->BroadcastConfirm(*p_this_order_);
}

void AccountThread::OnOrderConf(int server_assigned_seqnum, const char* exch_assigned_seqnum,
                                int exch_assigned_seqnum_length_, uint64_t _exch_seq_) {
  // find the order that has SAOS = _cl_ord_id_
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
  if (p_this_order_ == NULL) {
    if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderConf for SAOS "
                 << server_assigned_seqnum << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    // TODO might put other handling after checking
    return;
  }

  bzero(p_this_order_->exch_assigned_order_sequence_, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);
  // memcpy ( p_this_order_->exch_assigned_order_sequence_, exch_assigned_seqnum, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN );
  memcpy(p_this_order_->exch_assigned_order_sequence_, exch_assigned_seqnum, exch_assigned_seqnum_length_ + 1);

  p_this_order_->exch_assigned_order_sequence_length_ = exch_assigned_seqnum_length_;
  p_this_order_->exch_assigned_seq_ = _exch_seq_;
  p_this_order_->is_confirmed_ = true;

  bcast_manager_->BroadcastConfirm(*p_this_order_);
}
void AccountThread::OnOrderConf(Order* ord) {
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for saos " << ord->server_assigned_order_sequence_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  OnOrderConf(ord->server_assigned_order_sequence_, ord->exch_assigned_order_sequence_, ord->price_,
              ord->size_remaining_, 0, ord->exch_assigned_seq_);
}

void AccountThread::OnReject(int server_assigned_seqnum,
                             const ORSRejectionReason_t rejection_reason_ = kExchOrderReject, uint64_t _exch_seq_, uint64_t min_throttle_wait_) {
  
  DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER REJECTS FOR SAOS : " << server_assigned_seqnum << DBGLOG_ENDL_DUMP;

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);

  if (NULL == p_this_order_) {
    p_this_order_ = order_manager_.GetMirrorOrderBySAOS(server_assigned_seqnum);
    if (NULL == p_this_order_) {
      dbglogger_ << " SAOS FOR NULL ORDER : " << server_assigned_seqnum << "\n";
      dbglogger_.DumpCurrentBuffer();
      return;
    }
  }

  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called with server_seqno " << server_assigned_seqnum
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  // update PositionManager
  if (p_this_order_->buysell_ == kTradeTypeBuy) {
    position_manager_.DecBidSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 -p_this_order_->size_remaining_, -1);
  } else {
    position_manager_.DecAskSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 -p_this_order_->size_remaining_, -1);
  }

  p_this_order_->exch_assigned_seq_ = _exch_seq_;
  // notify cliento

  if (rejection_reason_ == HFSAT::kORSRejectThrottleLimitReached) {
    bcast_manager_->BroadcastORSRejection(*p_this_order_, rejection_reason_, min_throttle_wait_);
  } else {
    bcast_manager_->BroadcastExchRejection(*p_this_order_, rejection_reason_);
  }

  // Before we deactive order we need to have these variables. In order to update self trade check
  // maps to second best if needed, we look in only active order list. So it is important to
  // deactive the order first before AdjustBestBidAskMap

  // remove from map
  order_manager_.DeactivateOrder(p_this_order_);

  if (rejection_reason_ != HFSAT::kORSRejectThrottleLimitReached) {
    dbglogger_ << "Order Rejected With SAOS : " << server_assigned_seqnum
               << " Reason : " << HFSAT::ORSRejectionReasonStr(rejection_reason_) << "\n";
    dbglogger_.DumpCurrentBuffer();
  } else {
    was_last_order_throttled_ = true;
  }
}

void AccountThread::OnORSReject(Order* ord, const ORSRejectionReason_t rejection_reason_, uint64_t min_throttle_wait_) {
  bcast_manager_->BroadcastORSRejection(*ord, rejection_reason_, min_throttle_wait_);
  was_last_order_throttled_ = true;
}

void AccountThread::OnOrderModReject(int server_assigned_sequence_number,
                                     const CxlReplaceRejectReason_t rejection_reason_) {
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_sequence_number);

  if (NULL == p_this_order_) {
    //    dbglogger_ << " SAOS FOR NULL ORDER : " << server_assigned_sequence_number << "\n";
    //    dbglogger_.DumpCurrentBuffer();
    return;
  }

  bcast_manager_->BroadcastCancelReplaceExchRejection(*p_this_order_, kExchCancelReplaceReject);

  //  dbglogger_ << "OrderModify Rejected From Exchagne With SAOS : " << server_assigned_sequence_number << "\n";
  //  dbglogger_.DumpCurrentBuffer();
}

void AccountThread::OnReject(Order* ord) {
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for saos " << ord->server_assigned_order_sequence_
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  OnReject(ord->server_assigned_order_sequence_);
}

void AccountThread::OnCxlReject(int server_assigned_seqnum, CxlRejectReason_t _cxl_reject_reason_,
                                uint64_t _exch_seq_) {
  // --
  // This method could be called, because our cancel request was rejected for one of the following reasons:
  // (1) Throttling (2) Order does not exist (3) Order has already been executed, which means we have received or will
  // receive an Execution report. (4) Other ??
  // For cases (3), (4) & (5), we simply ignore the order, remove it from the map.
  // For case (1), we retry cancelling.

  /// TODO -- check if handling is required for certain cases

  // check if crossed order is getting cancel_reject, remove it from crossed order map, readjust the position of both
  // clients

  // Assume no change ???
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);

  if (p_this_order_ == NULL) {
#if NOT_USING_ZERO_LOGGIN_ORS
    dbglogger_ << typeid(*this).name() << ':' << __func__ << ' ' << "Order not found for SAOS "
               << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();
#endif
    return;
  }

  // notify client
  bcast_manager_->BroadcastCancelRejection(*p_this_order_, HFSAT::kExchCancelReject);

  // Retry cancellation or remove from map --
  switch (_cxl_reject_reason_) {
    case kCxlRejectReasonTooLate:
    case kCxlRejectReasonUnknownOrder:
      // Remove from map.
      // // update PositionManager
      // if ( p_this_order_->buysell_ == kTradeTypeBuy )
      //   {
      //     position_manager_.DecBidSize ( p_this_order_->security_id_, p_this_order_->size_remaining_ ) ;
      //   }
      // else
      //   {
      //     position_manager_.DecAskSize ( p_this_order_->security_id_, p_this_order_->size_remaining_ ) ;
      //   }

      // // remove from map
      // order_manager_.DeactivateOrder ( p_this_order_ );

      p_this_order_->order_not_found_ = true;

    case kCxlRejectReasonMarketClosed:
    case kCxlRejectReasonOrderAlreadyInPendingQueue:
    case kCxlRejectReasonDuplicateClOrdID:
    case kCxlRejectReasonOther:
    case kExchCancelReject:
      // Neglect and retry cancel request.
      // depending on the clients need
      break;

    default:
      dbglogger_ << typeid(*this).name() << ':' << " Unknown cancel reject reason -- \n";
      dbglogger_.DumpCurrentBuffer();
      break;
  }

#if NOT_USING_ZERO_LOGGIN_ORS

  dbglogger_ << "OnCxlReject\n" << order_manager_.DumpOMState() << "PM\n" << position_manager_.DumpPMState() << "\n";
  dbglogger_.DumpCurrentBuffer();

#endif
}

void AccountThread::OnCxlReject(Order* ord, CxlRejectReason_t _cxl_reject_reason_)  // --
{
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for saos " << ord->server_assigned_order_sequence_
               << "\n";
    dbglogger_ << " Order details " << ord->toString() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
  OnCxlReject(ord->server_assigned_order_sequence_, HFSAT::kExchCancelReject);
}

// For fix based exchange , we will use this function.Onordercxl is used as a full cancel
// i.e we are not expecting any executions for the order in future
// once we get cxl message for that order  TODO{} if any other than above logic found to exist
void AccountThread::OnOrderCxl(int server_assigned_seqnum, uint64_t _exch_seq_) {

  DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER CANCELED FOR SAOS : " <<  server_assigned_seqnum <<  DBGLOG_ENDL_DUMP;

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
  if (p_this_order_ == NULL) {
    if(server_assigned_seqnum != 0){
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderCxl for server id "
                 << server_assigned_seqnum << "\n";
      dbglogger_.DumpCurrentBuffer();
      return;
    }else{
      p_this_order_ = order_manager_.GetOrderByExchAssignedOrderId(_exch_seq_);
      if (p_this_order_ == NULL) {
        dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderCxl for exch Order id "
                   << _exch_seq_ << " SAOS: " << server_assigned_seqnum << "\n";
        dbglogger_.DumpCurrentBuffer();
        return;
      }else{
	dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order found in OnOrderCxl for exch Order id "
                   << _exch_seq_ << " EXCH_SAOS: " << server_assigned_seqnum 
		   << " ACTUAL_SAOS: " << p_this_order_->server_assigned_order_sequence_ << "\n";
        dbglogger_.DumpCurrentBuffer();
      }
    }
  }
#if NOT_USING_ZERO_LOGGIN_ORS

  dbglogger_ << "\nCXL SZR: " << p_this_order_->size_remaining_ << "\t SIZE EXEC: " << p_this_order_->size_executed_
             << "\n";

#endif
  // update PositionManager
  if (p_this_order_->buysell_ == kTradeTypeBuy) {
    position_manager_.DecBidSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 p_this_order_->size_remaining_, 1);

  } else {
    position_manager_.DecAskSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 p_this_order_->size_remaining_, 1);
  }

  p_this_order_->exch_assigned_seq_ = _exch_seq_;

  bcast_manager_->BroadcastCancelNotification(*p_this_order_);
// Before we deactive order we need to have these variables. In order to update self trade check
// maps to second best if needed, we look in only active order list. So it is important to
// deactive the order first before AdjustBestBidAskMap
#if NOT_USING_ZERO_LOGGIN_ORS
  std::string p_this_order_str = p_this_order_->toString();
#endif

  order_manager_.DeactivateOrder(p_this_order_);

#if NOT_USING_ZERO_LOGGIN_ORS
  dbglogger_ << "OnOrderCxl\n"
             << p_this_order_str << "\n"
             << "PM\n"
             << position_manager_.DumpPMState() << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif
}

void AccountThread::OnOrderCxlBMF(int server_assigned_seqnum) {
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
  if (p_this_order_ == NULL) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderCxl for server id "
               << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

#if NOT_USING_ZERO_LOGGIN_ORS

  dbglogger_ << "\nCXL SZR: " << p_this_order_->size_remaining_ << "\t SIZE EXEC: " << p_this_order_->size_executed_
             << "\n";

#endif

  // update PositionManager
  if (p_this_order_->buysell_ == kTradeTypeBuy) {
    position_manager_.DecBidSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 p_this_order_->size_remaining_, 1);

  } else {
    position_manager_.DecAskSize(p_this_order_->security_id_, p_this_order_->size_remaining_,
                                 p_this_order_->size_remaining_, 1);
  }

  bcast_manager_->BroadcastCancelNotification(*p_this_order_);

// Before we deactive order we need to have these variables. In order to update self trade check
// maps to second best if needed, we look in only active order list. So it is important to
// deactive the order first before AdjustBestBidAskMap
#if NOT_USING_ZERO_LOGGIN_ORS
  std::string p_this_order_str = p_this_order_->toString();
#endif

  order_manager_.DeactivateOrder(p_this_order_);

#if NOT_USING_ZERO_LOGGIN_ORS
  dbglogger_ << "OnOrderCxl\n"
             << p_this_order_str << "\n"
             << "PM\n"
             << position_manager_.DumpPMState() << "\n";
  dbglogger_.DumpCurrentBuffer();
#endif
}

void AccountThread::OnOrderCxl(Order* ord) {
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for saos " << ord->server_assigned_order_sequence_
               << "\n";
    dbglogger_ << " Order details " << ord->toString() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  OnOrderCxl(ord->server_assigned_order_sequence_);
}

void AccountThread::OnOrderMod(int server_assigned_seqnum, const char* exch_assigned_seqnum, double price, int size) {
  // find the order with SAOS = _cl_ord_id_
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);

  if (p_this_order_ == NULL) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderMod for server id "
               << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  // assign exch_ord_id = _exch_ord_id_.
  // Note this step could be done after broadcasting to client.
  // Choosing to do it first becasue of the very minute possibility of the client actually sending
  // the cancel or sth of this order before this thread is given the goahead to do the next step of memcpy
  if (exch_assigned_seqnum) {
    bzero(p_this_order_->exch_assigned_order_sequence_, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);
    memcpy(p_this_order_->exch_assigned_order_sequence_, exch_assigned_seqnum, EXCH_ASSIGNED_ORDER_SEQUENCE_LEN);
  }

  if (fabs(p_this_order_->price_ - price) > 0.00001) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order modified to different price "
                 << p_this_order_->price_ << " : " << price << " \n";
      dbglogger_.DumpCurrentBuffer();
    }
    p_this_order_->price_ = price;
  }

  if (p_this_order_->size_remaining_ != size) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order modified to a different size "
                 << p_this_order_->size_remaining_ << " : " << size << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    if (p_this_order_->buysell_ == kTradeTypeBuy) {
      position_manager_.AddBidSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    } else
      position_manager_.AddAskSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    p_this_order_->size_remaining_ = size;
  }

  p_this_order_->is_modified = true;

  bcast_manager_->BroadcastConfirmCxlReplace(*p_this_order_);
}

void AccountThread::OnOrderCancelReplacedBMF(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num,
                                             double price, int32_t size,
                                             std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_) {
  // find the order with SAOS = _cl_ord_id_
  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seq_num);

  if (p_this_order_ == NULL) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderMod for server id "
               << server_assigned_seq_num << "\n";
    dbglogger_.DumpCurrentBuffer();
    return;
  }

  // Update the order with new sequenced recevied from cancel replaced
  p_this_order_->exch_assigned_seq_ = exch_assigned_seq_num;

  p_this_order_->price_ = price;
  p_this_order_->int_price_ = fast_px_convertor_vec_[p_this_order_->security_id_] == NULL
                                  ? 0
                                  : fast_px_convertor_vec_[p_this_order_->security_id_]->GetFastIntPx(price);

  // Almost always the case, let's log it for now
  if (p_this_order_->size_remaining_ != size) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order modified to a different size "
                 << p_this_order_->size_remaining_ << " : " << size << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    if (p_this_order_->buysell_ == kTradeTypeBuy) {
      // Modify the size now / Confirmed positions
      position_manager_.AddBidSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    }
    if (p_this_order_->buysell_ == kTradeTypeSell) {
      // Modify the size now / Confirmed positions
      position_manager_.AddAskSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    }
    p_this_order_->size_remaining_ = size;
  }

  bcast_manager_->BroadcastConfirmCxlReplace(*p_this_order_);
}

void AccountThread::OnOrderCancelReplaced(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num, double price,
                                          int32_t size, std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_,
                                          int32_t last_mod_dt, int64_t last_activity_ref) {

  // find the order with SAOS = _cl_ord_id_
  DBGLOG_CLASS_FUNC_LINE_INFO << " ORDER MODIFY FOR SAOS : " <<  server_assigned_seq_num <<  DBGLOG_ENDL_DUMP;

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seq_num);

  if (p_this_order_ == NULL) {
    if(server_assigned_seq_num != 0){
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderMod for server id "
                 << server_assigned_seq_num << "\n";
      dbglogger_.DumpCurrentBuffer();
      return;
    }else{
      p_this_order_ = order_manager_.GetOrderByExchAssignedOrderId(exch_assigned_seq_num);
      if (p_this_order_ == NULL) {
        dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderMod for exch Order id "
                   << exch_assigned_seq_num << " SAOS: " << server_assigned_seq_num << "\n";
        dbglogger_.DumpCurrentBuffer();
        return;
      }else{
	dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order found in OnOrderMod for exch Order id "
                   << exch_assigned_seq_num << " EXCH_SAOS: " << server_assigned_seq_num 
		   << " ACTUAL_SAOS: " << p_this_order_->server_assigned_order_sequence_ << "\n";
        dbglogger_.DumpCurrentBuffer();
      }
    }
  }

  // Update the order with new sequenced recevied from cancel replaced
  p_this_order_->exch_assigned_seq_ = exch_assigned_seq_num;
  p_this_order_->last_mod_dt_ = last_mod_dt;
  p_this_order_->last_activity_reference_ = last_activity_ref;
  p_this_order_->price_ = price;
  p_this_order_->int_price_ = fast_px_convertor_vec_[p_this_order_->security_id_] == NULL
                                  ? 0
                                  : fast_px_convertor_vec_[p_this_order_->security_id_]->GetFastIntPx(price);

  // Almost always the case, let's log it for now
  if (p_this_order_->size_remaining_ != size) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order modified to a different size "
                 << p_this_order_->size_remaining_ << " : " << size << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    if (p_this_order_->buysell_ == kTradeTypeBuy) {
      // Modify the size now / Confirmed positions
      position_manager_.AddBidSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    }
    if (p_this_order_->buysell_ == kTradeTypeSell) {
      // Modify the size now / Confirmed positions
      position_manager_.AddAskSize(p_this_order_->security_id_, size - p_this_order_->size_remaining_,
                                   size + p_this_order_->size_remaining_, 2);
    }
    p_this_order_->size_remaining_ = size;
    p_this_order_->last_confirmed_size_remain_ = size;
  }

  bcast_manager_->BroadcastConfirmCxlReplace(*p_this_order_);
}

void AccountThread::OnOrderMod(Order* ord) {
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " called for saos " << ord->server_assigned_order_sequence_
               << "\n";
    dbglogger_ << " Order details " << ord->toString() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  OnOrderMod(ord->server_assigned_order_sequence_, ord->exch_assigned_order_sequence_, ord->price_,
             ord->size_remaining_);
}

void AccountThread::OnOrderExec(int server_assigned_seqnum, const char* symbol, TradeType_t trade_type, double price,
                                int size_executed, int size_remaining, uint64_t _exch_seq_, int32_t last_mod_dt, int64_t last_activity_ref) {
  // NO precision loss while printing price
  char print_price[20] = {'\0'};
  sprintf(print_price, "%.7f", price);

  // This is when we have missing exec, Rare Occurance, but sending an email should be useful - @ravi
  if (executed_saos_map_.find(server_assigned_seqnum) != executed_saos_map_.end()) {
    std::ostringstream mail_body_;

    mail_body_ << "High Possibility Of Missing This Exec, Please Check Front-End, "
               << " "
               << "Symbol : " << symbol << " "
               << "Trade Type : " << trade_type << " "
               << "Size Executed : " << size_executed << " "
               << "Price : " << print_price << " "
               << "SAOS : " << server_assigned_seqnum << '\n';

    EmailForIncorrectPositions(mail_body_.str());

    // Multiple execution reports for the same saos.
    dbglogger_ << "Received multiple execs for saos : " << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();

    // We should return now.
    // While this should not happen, if it does, our positions
    // will be incorrect, in which case, we can use this to debug the problem.
    return;
  }

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);

  const int m_sec_id_ = simple_security_symbol_indexer_.GetIdFromSecname(symbol);

  if (p_this_order_ == NULL) {
    std::ostringstream mail_body_;

    mail_body_ << "High Possibility Of Missing This Exec, Please Check Front-End, "
               << " "
               << "Found Null Order : "
               << "Symbol : " << symbol << " "
               << "Trade Type : " << trade_type << " "
               << "Size Executed : " << size_executed << " "
               << "Price : " << print_price << " "
               << "SAOS : " << server_assigned_seqnum << '\n';

    EmailForIncorrectPositions(mail_body_.str());

    dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderExec for SAOS "
               << server_assigned_seqnum << " sec_id " << m_sec_id_ << " sym " << symbol << "\n";
    dbglogger_.DumpCurrentBuffer();

    // should not happen apart from control command testing
    if (m_sec_id_ >= 0) {
      /// update global position but not of any specific client
      if (trade_type == kTradeTypeBuy) {
        position_manager_.AddBuyTrade(m_sec_id_, -1, size_executed);
      } else {
        position_manager_.AddSellTrade(m_sec_id_, -1, size_executed);
      }

#if NOT_USING_ZERO_LOGGIN_ORS
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " OnOrderExec: PM\n" << position_manager_.DumpPMState();
      dbglogger_.DumpCurrentBuffer();
#endif
    } else {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " m_sec_id < 0 in OnOrderExec for SAOS "
                 << server_assigned_seqnum << " sec_id " << m_sec_id_ << " sym " << symbol << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    char delimiter = (char)1;
    dbglogger_ << symbol << delimiter << trade_type << delimiter << size_executed << delimiter << print_price
               << delimiter << server_assigned_seqnum << '\n';
    dbglogger_.DumpCurrentBuffer();

    LogORSTrade(symbol, trade_type, size_executed, price, server_assigned_seqnum, _exch_seq_, -1);

    return;
  }

  // This is required as in RTSTWIME we don't get the side info in exec
  if ((trade_type == kTradeTypeNoInfo) || (this_exchange_source_ == kExchSourceEUREX)) {
    trade_type = p_this_order_->buysell_;
  }

  // Update Pnls
  ors_pnl_manager_.OnOrderExec(m_sec_id_, size_executed, price, trade_type);

  int32_t saci = p_this_order_->server_assigned_client_id_;
  double orig_dbl_px = p_this_order_->price_;
  p_this_order_->size_executed_ += size_executed;
  p_this_order_->exch_assigned_seq_ = _exch_seq_;

  if (size_remaining > 0) {
    p_this_order_->last_mod_dt_ = last_mod_dt;
    p_this_order_->last_activity_reference_ = last_activity_ref;
  }

  // For cases where we get a cxl before exec, we should not reassign size_remaining to what
  // exchange sent, since we have already adjusted in onordercxlets & what exchange sent was
  // old message which went out-of-seq
  if (!p_this_order_->partial_cxl_before_exec_) {
    if (size_remaining >= 0) {
      p_this_order_->size_remaining_ = size_remaining;
      p_this_order_->last_confirmed_size_remain_ = size_remaining;

    } else {
      // Added for OSE race condition between Cxl and PartialExec
      __sync_sub_and_fetch(&p_this_order_->size_remaining_, size_executed);
    }
  } else {
    p_this_order_->size_remaining_ -= size_executed;
    if (p_this_order_->size_remaining_ < 0) {
      dbglogger_ << "PCXL case before Exec " << p_this_order_->size_remaining_ << " EXEC: " << size_executed << "\n";
      dbglogger_.DumpCurrentBuffer();
      p_this_order_->size_remaining_ = 0;
    }
  }

  if (trade_type == kTradeTypeBuy) {
    position_manager_.AddBuyTrade(p_this_order_->security_id_, p_this_order_->server_assigned_client_id_,
                                  size_executed);
    position_manager_.DecBidSize(p_this_order_->security_id_, size_executed, 0, 0);
  } else {
    position_manager_.AddSellTrade(p_this_order_->security_id_, p_this_order_->server_assigned_client_id_,
                                   size_executed);
    position_manager_.DecAskSize(p_this_order_->security_id_, size_executed, 0, 0);
  }

  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__
               << " OnOrderExec after setting p_this_order_->size_remaining_ and position_manager ( AddBuyTrade and "
                  "DecBidSize ) PM:\n"
               << position_manager_.DumpPMState() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  /// if exec at different price change px temporarily
  if (fabs(p_this_order_->price_ - price) > 0.00001) {
    //    dbglogger_ << typeid(*this).name() << ':' << __func__ << " OnOrderExec had orig_order_price "
    //               << p_this_order_->price_ << " diff than exec price: " << price << "\n";
    //    dbglogger_.DumpCurrentBuffer();

    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " calling BroadcastExecNotification "
                 << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

#define PRICE_DEVIATION_THRESHOLD 150

    if ((fabs(p_this_order_->price_ - price) / std::max(fabs(p_this_order_->price_), fabs(price))) * 100 >
        PRICE_DEVIATION_THRESHOLD) {
      // something went wrong, exec price is off by more than 50%

      std::ostringstream t_temp_oss;
      t_temp_oss << typeid(*this).name() << ':' << __func__ << " OnOrderExec had orig_order_price "
                 << p_this_order_->price_ << " diff than exec price: " << price << "\n";

      EmailForWeirdEngineReply(t_temp_oss.str());

      return;  // Avoid Passing The same to queries
    }

    // Px changed temporarily to reflect correct Exec price in the broadcast
    p_this_order_->price_ = price;

    bcast_manager_->BroadcastExecNotification(*p_this_order_);

    // Reset to original price
    p_this_order_->price_ = orig_dbl_px;

#undef PRICE_DEVIATION_THRESHOLD

  } else {
#if NOT_USING_ZERO_LOGGIN_ORS

    dbglogger_ << typeid(*this).name() << ':' << __func__ << " OnOrderExec had orig_order_price "
               << p_this_order_->price_ << " same as exec price: " << price << "\n";

#endif
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " calling BroadcastExecNotification "
                 << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    bcast_manager_->BroadcastExecNotification(*p_this_order_);
  }

#if NOT_USING_ZERO_LOGGIN_ORS

  char delimiter = (char)1;

  dbglogger_ << p_this_order_->symbol_ << delimiter << p_this_order_->buysell_ << delimiter << size_executed
             << delimiter << print_price << delimiter << server_assigned_seqnum << '\n';
  dbglogger_.DumpCurrentBuffer();

#endif

  LogORSTrade(p_this_order_->symbol_, p_this_order_->buysell_, size_executed, price, server_assigned_seqnum, _exch_seq_,
              saci);

  // Copy these before we deactive the order in case size remaining becomes zero, then variables
  // of p_this_order_ are not reliable

  if (p_this_order_->size_remaining_ == 0) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Since size_remaining_ is now 0, deactivating it. \nOM: \n"
                 << order_manager_.DumpOMState() << "PM\n"
                 << position_manager_.DumpPMState() << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    order_manager_.DeactivateOrder(p_this_order_);
/*
    Order* p_this_order_test = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);
    if (p_this_order_test == NULL) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "DeactivatedNull " << "SAOS: " << p_this_order_->server_assigned_order_sequence_ 
                                  << "SizeRem: " << p_this_order_->size_remaining_ <<  DBGLOG_ENDL_FLUSH;
    } else {
      DBGLOG_CLASS_FUNC_LINE_INFO << "DeactivatedNotNull " << "SAOS: " << p_this_order_->server_assigned_order_sequence_ 
                                  << "SizeRem: " << p_this_order_->size_remaining_ <<  DBGLOG_ENDL_FLUSH;
    }
*/
    // This should only be done when this order is no longer active.
    // If this was a partial execution, we may receive another execution with the same SAOS.
    // Add this to list of COMPLETELY executed saos.
    executed_saos_map_[server_assigned_seqnum] = true;
  }
  // Only Enabled For NSE & BSE For Now
  ors_margin_manager_.OnOrderExec(m_sec_id_);
}

void AccountThread::OnOrderExecBMF(int server_assigned_seqnum, double price, int size_executed, int size_remaining) {
  HFSAT::TradeType_t trade_type = HFSAT::kTradeTypeNoInfo;

  // NO precision loss while printing price
  char print_price[20] = {'\0'};
  sprintf(print_price, "%.7f", price);

  // This is when we have missing exec, Rare Occurance, but sending an email should be useful - @ravi
  if (executed_saos_map_.find(server_assigned_seqnum) != executed_saos_map_.end()) {
    std::ostringstream mail_body_;

    mail_body_ << "High Possibility Of Missing This Exec, Please Check Front-End, "
               << " "
               << "Symbol : "
               << " UNKNOWN "
               << " "
               << "Trade Type : "
               << " UNKNOWN "
               << " "
               << "Size Executed : " << size_executed << " "
               << "Price : " << print_price << " "
               << "SAOS : " << server_assigned_seqnum << '\n';

    EmailForIncorrectPositions(mail_body_.str());

    // Multiple execution reports for the same saos.
    dbglogger_ << "Received multiple execs for saos : " << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();

    // We should return now.
    // While this should not happen, if it does, our positions
    // will be incorrect, in which case, we can use this to debug the problem.
    return;
  }

  Order* p_this_order_ = order_manager_.GetOrderByOrderSequence(server_assigned_seqnum);

  if (p_this_order_ == NULL) {
    std::ostringstream mail_body_;

    mail_body_ << "High Possibility Of Missing This Exec, Please Check Front-End, "
               << " "
               << "Found Null Order : "
               << "Symbol : "
               << " UNKNOWN "
               << " "
               << "Trade Type : "
               << " UNKNOWN "
               << " "
               << "Size Executed : " << size_executed << " "
               << "Price : " << print_price << " "
               << "SAOS : " << server_assigned_seqnum << '\n';

    EmailForIncorrectPositions(mail_body_.str());

    dbglogger_ << typeid(*this).name() << ':' << __func__ << " Order not found in OnOrderExec for SAOS "
               << server_assigned_seqnum << "\n";
    dbglogger_.DumpCurrentBuffer();

    char delimiter = (char)1;
    dbglogger_ << size_executed << delimiter << print_price << delimiter << server_assigned_seqnum << '\n';
    dbglogger_.DumpCurrentBuffer();

    LogORSTrade("INVALID", HFSAT::kTradeTypeNoInfo, size_executed, server_assigned_seqnum, price, -1, -1);

    return;
  }

  trade_type = p_this_order_->buysell_;

  // Update Pnls
  ors_pnl_manager_.OnOrderExec(p_this_order_->security_id_, size_executed, price, trade_type);

  int32_t saci = p_this_order_->server_assigned_client_id_;
  p_this_order_->size_executed_ += size_executed;
  //      p_this_order_ -> exch_assigned_seq_ = _exch_seq_ ;

  // For cases where we get a cxl before exec, we should not reassign size_remaining to what
  // exchange sent, since we have already adjusted in onordercxlets & what exchange sent was
  // old message which went out-of-seq
  if (!p_this_order_->partial_cxl_before_exec_) {
    if (size_remaining >= 0) {
      p_this_order_->size_remaining_ = size_remaining;
    } else {
      p_this_order_->size_remaining_ -= size_executed;
    }
  } else {
    p_this_order_->size_remaining_ -= size_executed;
    if (p_this_order_->size_remaining_ < 0) {
      dbglogger_ << "PCXL case before Exec " << p_this_order_->size_remaining_ << " EXEC: " << size_executed << "\n";
      dbglogger_.DumpCurrentBuffer();
      p_this_order_->size_remaining_ = 0;
    }
  }

  if (trade_type == kTradeTypeBuy) {
    position_manager_.AddBuyTrade(p_this_order_->security_id_, p_this_order_->server_assigned_client_id_,
                                  size_executed);
    position_manager_.DecBidSize(p_this_order_->security_id_, size_executed, 0, 0);
  } else {
    position_manager_.AddSellTrade(p_this_order_->security_id_, p_this_order_->server_assigned_client_id_,
                                   size_executed);
    position_manager_.DecAskSize(p_this_order_->security_id_, size_executed, 0, 0);
  }

  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__
               << " OnOrderExec after setting p_this_order_->size_remaining_ and position_manager ( AddBuyTrade and "
                  "DecBidSize ) PM:\n"
               << position_manager_.DumpPMState() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  /// if exec at different price change px temporarily
  if (fabs(p_this_order_->price_ - price) > 0.00001) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " calling BroadcastExecNotification "
                 << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

#define PRICE_DEVIATION_THRESHOLD 150

    if ((fabs(p_this_order_->price_ - price) / std::max(fabs(p_this_order_->price_), fabs(price))) * 100 >
        PRICE_DEVIATION_THRESHOLD) {
      // something went wrong, exec price is off by more than 50%

      std::ostringstream t_temp_oss;
      t_temp_oss << typeid(*this).name() << ':' << __func__ << " OnOrderExec had orig_order_price "
                 << p_this_order_->price_ << " diff than exec price: " << price << "\n";

      EmailForWeirdEngineReply(t_temp_oss.str());

      return;  // Avoid Passing The same to queries
    }

    p_this_order_->price_ = price;

    bcast_manager_->BroadcastExecNotification(*p_this_order_);

#undef PRICE_DEVIATION_THRESHOLD

  } else {
#if NOT_USING_ZERO_LOGGIN_ORS

    dbglogger_ << typeid(*this).name() << ':' << __func__ << " OnOrderExec had orig_order_price "
               << p_this_order_->price_ << " same as exec price: " << price << "\n";

#endif
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__ << " calling BroadcastExecNotification "
                 << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    bcast_manager_->BroadcastExecNotification(*p_this_order_);
  }

#if NOT_USING_ZERO_LOGGIN_ORS

  char delimiter = (char)1;

  dbglogger_ << p_this_order_->symbol_ << delimiter << p_this_order_->buysell_ << delimiter << size_executed
             << delimiter << print_price << delimiter << server_assigned_seqnum << '\n';
  dbglogger_.DumpCurrentBuffer();

#endif

  LogORSTrade(p_this_order_->symbol_, p_this_order_->buysell_, size_executed, price, server_assigned_seqnum, -1, saci);

  // Copy these before we deactive the order in case size remaining becomes zero, then variables
  // of p_this_order_ are not reliable

  if (p_this_order_->size_remaining_ == 0) {
    if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
      dbglogger_ << typeid(*this).name() << ':' << __func__
                 << " Since size_remaining_ is now 0, deactivating it. \nOM: \n"
                 << order_manager_.DumpOMState() << "PM\n"
                 << position_manager_.DumpPMState() << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
    order_manager_.DeactivateOrder(p_this_order_);

    // This should only be done when this order is no longer active.
    // If this was a partial execution, we may receive another execution with the same SAOS.
    // Add this to list of COMPLETELY executed saos.
    executed_saos_map_[server_assigned_seqnum] = true;
  }
}

void AccountThread::OnOrderExec(Order* ord) {
  if (dbglogger_.CheckLoggingLevel(ORS_INFO)) {
    dbglogger_ << typeid(*this).name() << ':' << __func__ << " incoming_order_struct: \n" << ord->toString() << "\n";
    dbglogger_.DumpCurrentBuffer();
  }

  OnOrderExec(ord->server_assigned_order_sequence_, ord->symbol_, ord->buysell_, ord->price_, ord->size_executed_,
              ord->size_remaining_);
}

// Cancel all pending orders. Also make sure that no new orders can be placed while this is happening
void AccountThread::CancelAllPendingOrders() {
  if (!loggedin_) {
    return;
  }
  dbglogger_ << "Disabling new orders\n";
  order_manager_.DisableNewOrders();  // No more new orders can be placed.

  dbglogger_ << "Cancelling all placed orders\n";
  dbglogger_.DumpCurrentBuffer();
  std::vector<Order*> pending_orders = order_manager_.GetAllOrders();
  size_t pending_orders_size_ = -1;

  for (; pending_orders_size_ != pending_orders.size();
       pending_orders_size_ = pending_orders.size(), pending_orders = order_manager_.GetAllOrders()) {
    // Cancel each order.
    for (std::vector<Order*>::iterator _iter_ = pending_orders.begin(); _iter_ != pending_orders.end(); ++_iter_) {
      Cancel(*(_iter_));
    }

    sleep(1);  // Sleep a second before attempting to resend cancel requests. Deals with throttling and pending cancels
               // on the exchange.
  }
}

void AccountThread::LogORSTrade(const char* symbol, TradeType_t buysell, int size_executed, double price, int saos,
                                int64_t exch_assigned_seq_num, int32_t saci) {
  memcpy((void*)log_buffer_->buffer_data_.ors_trade_.symbol_, (void*)symbol, std::min(16, (int32_t)strlen(symbol) + 1));
  log_buffer_->buffer_data_.ors_trade_.trade_type_ = (int32_t)buysell;
  log_buffer_->buffer_data_.ors_trade_.size_executed_ = size_executed;
  log_buffer_->buffer_data_.ors_trade_.price_ = price;
  log_buffer_->buffer_data_.ors_trade_.saos_ = saos;
  log_buffer_->buffer_data_.ors_trade_.exch_order_sequence_ = exch_assigned_seq_num;
  log_buffer_->buffer_data_.ors_trade_.saci_ = saci;

  client_logging_segment_initializer_ptr_->Log(log_buffer_);
}
void AccountThread::OnBatchCxlAlert(int32_t user_id_){
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    HFSAT::Email e;

    std::string exchange = (std::string(hostname).find("sdv-indb-") != std::string::npos) ? "BSE" : "NSE";
    std::string subject = "Error: "+exchange+" Received Batch ORDER CANCELLATION : " + std::string(hostname);

    e.setSubject(subject);
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << "User ID: " << user_id_ << "<br/>";
    e.content_stream << "Enable Order from Strat \n";
    e.content_stream << "user_msg --traderid ID --unfreezedataentryrejectdisable\n";
    e.content_stream << "Send Flat to Strat\n";
    e.content_stream << "Once IOC order are all sent/Position Cleared \n";
    e.content_stream << "ors_control_exec 21240 GEN RiskReductionDisable \n";
    e.sendMail();
}

void AccountThread::ForceBatchCancelBroadcastAll(bool isMarginBreached = false){
  //get all orders which are live 
  std::vector<Order*> orders = order_manager_.GetAllOrders();

  for(auto order : orders){
    if (isMarginBreached){
        order->new_server_assigned_order_sequence=1;
    }
    dbglogger_ << "Cancelling " << order->toStringShort(); // Extra Logging 
    bcast_manager_->BroadcastCancelNotification(*order);
    // order_manager_.DeactivateOrder(order); don't remove order just set status as false
    order->is_active_order = false;
      // update PositionManager
    if (order->buysell_ == kTradeTypeBuy) {
      position_manager_.DecBidSize(order->security_id_, order->size_remaining_,
                                  order->size_remaining_, 1);
    } else {
      position_manager_.DecAskSize(order->security_id_, order->size_remaining_,
                                  order->size_remaining_, 1);
    }
    if (isMarginBreached){
        order->new_server_assigned_order_sequence=0;
    }
    HFSAT::usleep(10);
  }
}

void AccountThread::ForceBatchCancelBroadcastAllOrderRemoval(){
  //get all orders which are live and Remove From ORS MAP
  std::vector<Order*> orders = order_manager_.GetAllOrders();

  for(auto order : orders){
    dbglogger_ << "Cancelling " << order->toStringShort(); // Extra Logging 
    bcast_manager_->BroadcastCancelNotification(*order); 
//    order_manager_.DeactivateOrder(order); // don't remove order just set status as false
    order->is_active_order = false;
      // update PositionManager
    if (order->buysell_ == kTradeTypeBuy) {
      position_manager_.DecBidSize(order->security_id_, order->size_remaining_,
                                  order->size_remaining_, 1);
    } else {
      position_manager_.DecAskSize(order->security_id_, order->size_remaining_,
                                  order->size_remaining_, 1);
    }
    HFSAT::usleep(10);
  }
}

void AccountThread::RequestORSPNL(){

  dbglogger_ << "AccountThread::RequestORSPNL\n";
  if (p_base_engine_) {
    p_base_engine_->RequestORSPNL();
  }

}

void AccountThread::RequestORSOpenPositions(){

  dbglogger_ << "AccountThread::RequestORSOpenPositions\n";
  if (p_base_engine_) {
    p_base_engine_->RequestORSOpenPositions();
  }

}


void AccountThread::ForceBatchCancelBroadcastSAOS(char const * _saos_){
  int32_t saos = std::atoi(_saos_);

  //get all orders which are live 
  std::vector<Order*> orders = order_manager_.GetAllOrders();

  for(auto order : orders){
    if(NULL != order && order->server_assigned_order_sequence_ == saos){
      dbglogger_ << "Cancelling " << order->toStringShort(); // Extra Logging 
      bcast_manager_->BroadcastCancelNotification(*order);
//      order_manager_.DeactivateOrder(order); // don't remove order just set status as false
      order->is_active_order = false;
      // update PositionManager
      if (order->buysell_ == kTradeTypeBuy) {
        position_manager_.DecBidSize(order->security_id_, order->size_remaining_,
                                    order->size_remaining_, 1);
      } else {
        position_manager_.DecAskSize(order->security_id_, order->size_remaining_,
                                    order->size_remaining_, 1);
      }
      break;
    }
  }
}

void AccountThread::ForceBatchCancelBroadcastSymbol(char const * _symbol_){

  int32_t sec_id = simple_security_symbol_indexer_.GetIdFromSecname(_symbol_);

  if( sec_id >= 0 ){
    //get all orders which are live 
    std::vector<Order*> orders = order_manager_.GetAllOrders();

    for(auto order : orders){
      if(NULL != order && order->security_id_ == sec_id){
        dbglogger_ << "Cancelling " << order->toStringShort(); // Extra Logging 
        bcast_manager_->BroadcastCancelNotification(*order);
  //      order_manager_.DeactivateOrder(order); // don't remove order just set status as false
  	order->is_active_order = false;
          // update PositionManager
        if (order->buysell_ == kTradeTypeBuy) {
          position_manager_.DecBidSize(order->security_id_, order->size_remaining_,
                                      order->size_remaining_, 1);
        } else {
          position_manager_.DecAskSize(order->security_id_, order->size_remaining_,
                                      order->size_remaining_, 1);
        }
        HFSAT::usleep(10);
      }
    }
  }
}

void AccountThread::ProcessGeneralControlCommand(const char* input_stream, int stream_length) {
  if (p_base_engine_) {
    p_base_engine_->ProcessGeneralControlCommand(input_stream, stream_length);
  }
}

void AccountThread::SetMarginValue(double margin){
  // std::cout<<" MARGIN : "<<margin<<std::endl;
    if (margin > 70){
        if ((std::abs(margin - margin_) >= 1.0) || (margin >= 85 )) {
            bcast_manager_->BroadcastMargin(margin);
            dbglogger_ << "margin state changed " << margin << DBGLOG_ENDL_DUMP;
            margin_ = margin;
	          ismarginstatechanged_=true;
        }
    }
    
    if(true == ismarginstatechanged_){
      if (margin < 70 ){
	      ismarginstatechanged_=false;
	      bcast_manager_->BroadcastMargin(margin);
	      margin_ = margin;
	      dbglogger_<<"Margin less than 70 now."<<DBGLOG_ENDL_DUMP;
      }
    }
}
void AccountThread::KillSwitch(int sec_id){
  DBGLOG_CLASS_FUNC_LINE_INFO << " KILL SWITCH FOR SEC ID  : " <<  sec_id <<  DBGLOG_ENDL_DUMP;
  if (p_base_engine_) {
    p_base_engine_->KillSwitch(sec_id);
  }
}

void AccountThread::FetchMarginUsage(){
  DBGLOG_CLASS_FUNC_LINE_INFO << "MARGIN USAGE "  <<  DBGLOG_ENDL_DUMP;
  if (p_base_engine_) {
    p_base_engine_->FetchMarginUsage();
  }
}

void AccountThread::RequestExecutedOrders(){
  DBGLOG_CLASS_FUNC_LINE_INFO << "Executed Orders "  <<  DBGLOG_ENDL_DUMP;
  if (p_base_engine_) {
    p_base_engine_->RequestExecutedOrders();
  }
}

void AccountThread::KillSwitchNewForSecId( int32_t sec_id){
    if(p_base_engine_) {
      p_base_engine_->KillSwitchNewForSecId(sec_id);
    }
  }
}


}


