/**
   \file ORSUtilsCode/broadcast_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#include "baseinfra/OrderRouting/broadcast_manager_sim.hpp"

#include "dvccode/Utils/rdtscp_timer.hpp"
#define WORKER_LOGGED_DATA_PREFIX "/spare/local/ORSBCAST_MULTISHM/"

namespace HFSAT {

BroadcastManagerSim* BroadcastManagerSim::GetUniqueInstance(DebugLogger& t_dbglogger_, HFSAT::Watch& _watch_,
                                                      int base_writer_id) {
  static BroadcastManagerSim* bcast_manager_ = NULL;
  if (bcast_manager_ == NULL) {

    bcast_manager_ = new BroadcastManagerSim(t_dbglogger_,_watch_, base_writer_id);
  }

  return bcast_manager_;
}

BroadcastManagerSim::BroadcastManagerSim(DebugLogger& t_dbglogger_, HFSAT::Watch& _watch_,
                                   int32_t base_writer_id)
    : dbglogger_(t_dbglogger_),watch_(_watch_), position_manager_(HFSAT::ORS::PositionManager::GetUniqueInstance()), ors_recovery_reply(),
        simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),base_writer_id_(base_writer_id) {
  memset((void*)&ors_recovery_reply, 0, sizeof(GenericORSReplyStructLiveProShm));
}
BroadcastManagerSim::~BroadcastManagerSim() { }

void BroadcastManagerSim::processMsgRecvd(GenericORSReplyStructLiveProShm generic_ors_reply_struct_) {
        return;
        std::string symbol_got(generic_ors_reply_struct_.symbol_);
        if (exchange_symbol_to_ors_bcast_log_map_.find(symbol_got) == exchange_symbol_to_ors_bcast_log_map_.end()) {
            AddToMap(symbol_got);
        }
        std::ofstream* file_streamer = exchange_symbol_to_ors_bcast_log_map_[symbol_got];
        file_streamer->write((char*)(&generic_ors_reply_struct_), (int)sizeof(HFSAT::GenericORSReplyStructLiveProShm));
        file_streamer->flush();
}


void BroadcastManagerSim::AddToMap(std::string exchange_symbol) {
    time_t m_time_t;
    time(&m_time_t);
    struct tm m_tm;
    localtime_r(&m_time_t, &m_tm);
    unsigned int yyyymmdd = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;
    char date_str[9] = {'\0'};
    sprintf(date_str, "%d", yyyymmdd);
    std::string filename = WORKER_LOGGED_DATA_PREFIX + std::string("NSE/") + exchange_symbol + "_" + date_str + "_" + std::to_string(base_writer_id_);
    // std::cout << "Total Filename :" << filename << std::endl;
    std::ofstream* new_file = new std::ofstream();

    HFSAT::FileUtils::MkdirEnclosing(filename);

    new_file->open(filename.c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    if (!new_file->is_open()) {
        fprintf(stderr, "ors_bcast_binary could not open file %s for writing \n", filename.c_str());
        exit(-1);
    }
    exchange_symbol_to_ors_bcast_log_map_[exchange_symbol] = new_file;
}


void BroadcastManagerSim::BroadcastORSRejection(const BaseSimOrder* t_this_order_,
                                             const ORSRejectionReason_t t_rejection_reason_, const uint64_t min_throttle_wait_ /*= 0*/) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  timeval tv;
  gettimeofday(&tv, NULL);
  generic_ors_reply_struct_.client_request_time_ = tv;

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_->server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = t_rejection_reason_;   // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ = 0;
  generic_ors_reply_struct_.global_position_ = 0;
  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastORSCancelReplaceRejection(const char *security_name_, int server_assigned_client_id_, int client_assigned_order_sequence_,
                                                    HFSAT::TradeType_t buysell_, int size_remaining_, double old_price, int old_int_price) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();
  memcpy(generic_ors_reply_struct_.symbol_, security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = old_price;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxReRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = size_remaining_;
  generic_ors_reply_struct_.buysell_ = buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = HFSAT::CxlRejectReason_t::kCxlRejectReasonOther;  // used to convey rejection_reason_ Not Sure of the reason to send as missing
  generic_ors_reply_struct_.int_price_ = old_int_price;            // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ = 0;
  generic_ors_reply_struct_.global_position_ = 0;

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.order_flags_ = 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastCancelRejection(const BaseSimOrder* t_this_order_,
                                                const CxlRejectReason_t t_exch_level_rejection_reason_, uint64_t min_throttle_wait_ /*= 0*/) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
  const int security_id_ = simple_security_symbol_indexer_.GetIdFromChar16(t_this_order_->security_name_);
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxlRejc;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_->server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_->server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = t_exch_level_rejection_reason_;  // used to convey rejection_reason_
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;            // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.query_start_cycles_delta_ = min_throttle_wait_;
  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastSequenced(const BaseSimOrder* t_this_order_, int saos) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
  const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
   generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);
  generic_ors_reply_struct_.client_request_time_ = watch_.tv(); 
  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_Seqd;
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_->server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;
  if (saos > 0) {
    generic_ors_reply_struct_.server_assigned_order_sequence_ = saos;
  } else {
    generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_->server_assigned_order_sequence_;
  }
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;
  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;
  processMsgRecvd(generic_ors_reply_struct_);    
}

void BroadcastManagerSim::BroadcastCxlSequenced(const BaseSimOrder* t_this_order_,
                                             const HFSAT::ttime_t& t_client_request_time_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;

  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);

  generic_ors_reply_struct_.price_ = t_this_order_->price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxlSeqd;  // CxlSequenced
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_->server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_->server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastCxlReSequenced(const BaseSimOrder* t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);

  generic_ors_reply_struct_.price_ = t_this_order_->price_;

  generic_ors_reply_struct_.orr_type_ = kORRType_CxReSeqd;  // CxlReSequenced
  generic_ors_reply_struct_.server_assigned_client_id_ = t_this_order_->server_assigned_client_id_;
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;
  generic_ors_reply_struct_.server_assigned_order_sequence_ = t_this_order_->server_assigned_order_sequence_;
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;
  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastConfirm(const BaseSimOrder* t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  // could take some time, should be done outside mutex, does not matter if not thread safe
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();
  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_Conf;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_->server_assigned_client_id_;                               // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_->server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
    //  t_this_order_->client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                      // sent earlier )
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0; // THERE IS NOT RESERVER TYPE

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastConfirmCxlReplace(const BaseSimOrder* t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;
  generic_ors_reply_struct_.orr_type_ = kORRType_CxRe;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_->server_assigned_client_id_;                               // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_->server_assigned_order_sequence_;  // new SAOS .. the client has the SAOS of the old order that was canceled
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
    //  t_this_order_->client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                      // sent earlier )
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastCancelNotification(const BaseSimOrder* t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_Cxld;      // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_->server_assigned_client_id_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_->server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
   //   t_this_order_->client_assigned_order_sequence_;                // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_executed_ = 0;                     // ignored in client
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  }
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}

void BroadcastManagerSim::BroadcastExecNotification(const BaseSimOrder* t_this_order_) {
  GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  generic_ors_reply_struct_.client_request_time_ = watch_.tv();

  memcpy(generic_ors_reply_struct_.symbol_, t_this_order_->security_name_, kSecNameLen);
  generic_ors_reply_struct_.price_ = t_this_order_->price_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.orr_type_ = kORRType_Exec;      // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_client_id_ =
      t_this_order_->server_assigned_client_id_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_remaining_ = t_this_order_->size_remaining_;
  generic_ors_reply_struct_.buysell_ = t_this_order_->buysell_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.server_assigned_order_sequence_ =
      t_this_order_->server_assigned_order_sequence_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_assigned_order_sequence_ = base_writer_id_;
   //   t_this_order_->client_assigned_order_sequence_;                        // used by BaseOrderManager in client
  generic_ors_reply_struct_.size_executed_ = t_this_order_->size_executed_;  // used by BaseOrderManager in client
  generic_ors_reply_struct_.int_price_ = t_this_order_->int_price_;          // used by BaseOrderManager in client
  generic_ors_reply_struct_.client_position_ =
      position_manager_.GetClientPosition(t_this_order_->server_assigned_client_id_);
const char* exch_symbol_= t_this_order_->security_name_;
  int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  if (security_id_ < 0 ){
     HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(exch_symbol_);
    security_id_ = simple_security_symbol_indexer_.GetIdFromSecname(exch_symbol_);
  } 
  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(security_id_);

  generic_ors_reply_struct_.query_order_ptr_ = 0;
  generic_ors_reply_struct_.ors_order_ptr_ = 0;

  generic_ors_reply_struct_.ors_end_cycles_from_exchange_ = HFSAT::GetCpucycleCountForTimeTick();

  generic_ors_reply_struct_.order_flags_ =
      ( true == t_this_order_->is_ioc_) ? 1 : 0;

  processMsgRecvd(generic_ors_reply_struct_);
}
}
