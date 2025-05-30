/**
   \file OrderRouting/base_live_trader.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_BASE_LIVE_TRADER_H
#define BASE_ORDERROUTING_BASE_LIVE_TRADER_H

#include <boost/algorithm/string/replace.hpp>
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Utils/shared_mem_writer.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "baseinfra/OrderRouting/base_trader.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "baseinfra/BaseTrader/query_tag_info.hpp"
#include "dvccode/CDef/assumptions.hpp"

#include <stdint.h>
#define CCPROFILING_TRADEINIT 1
namespace HFSAT {
/** BaseLiveTrader extends interface BaseTrader. It is used to send order-routing requests to the ORS
 */
class BaseLiveTrader : public BaseTrader {
 protected:
  TCPClientSocket tcpsock_;
  int client_id_;
  HFSAT::SharedMemWriter<GenericORSRequestStruct>* shm_writer_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  bool ignore_from_global_pos_;
  DataWriterIdPair<GenericORSRequestStruct> send_order_request_;
  GenericORSRequestStruct& order_req_;

 public:
  BaseLiveTrader(const HFSAT::ExchSource_t& _exchange_, const std::string& ors_account, const std::string& ors_ip,
                 const int ors_port, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                 int null_trader_client_id_ = -1)
      : tcpsock_(),
        client_id_(-1),
        shm_writer_(NULL),
        watch_(_watch_),
        dbglogger_(_dbglogger_),
        ignore_from_global_pos_(false),
        send_order_request_(),
        order_req_(send_order_request_.data) {
    if (null_trader_client_id_ > 0) {  // For null - traders , do not connect to shm / tcp space.
      client_id_ = null_trader_client_id_;
    } else {
      bool use_shared_mem = true;  // first try shared mem ors, if fails, try

      if (use_shared_mem) {
        // Each BaseLiveTrader "writer" has a client id to identify it's requests.
        // Get one.

        // in case initialization fails we will check IPCS dump before and after the initialization attempt

        shm_writer_ = new SharedMemWriter<GenericORSRequestStruct>(_exchange_);
        client_id_ = shm_writer_->intilizeWriter();
        if (client_id_ == -1) {
          fprintf(stderr, "BaseLiveTrader : Could not initialize shared mem writer.\n");
          delete (shm_writer_);
          shm_writer_ = NULL;

          HFSAT::Email e;
          e.addRecepient("dvctech@circulumvite.com");
          e.addSender("dvctech@circulumvite.com");
          FILE* status_p = popen("ipcs", "r");
          fseek(status_p, 0, SEEK_END);
          size_t size = ftell(status_p);
          char buffer[4096];
          fread(buffer, sizeof(buffer), size, status_p);
          pclose(status_p);
          std::string buffer_string_ = std::string(buffer);
          boost::replace_all(buffer_string_, "\n", "<br/>");
          e.content_stream << "IPCS dump at time of initialization fail:<br/> " << buffer_string_ << "<br/>";
          char hostname[128];
          hostname[127] = '\0';
          gethostname(hostname, 127);
          std::string alert_message = "Query could not connect via SHM: " + std::string(hostname);
          e.setSubject(alert_message);
          e.sendMail();
          use_shared_mem = false;

          exit(-1);
        } else {
          send_order_request_.writer_id = client_id_;
          if (QueryTagInfo::GetUniqueInstance().getQueryId() != 0 &&
              QueryTagInfo::GetUniqueInstance().getQueryTags().length() !=
                  0) {  // Iff query id is zero we might have contention for writing on same file and tags are non-zero
            std::ostringstream temp_oss;
            int trading_date = QueryTagInfo::GetUniqueInstance().getTradingDate();
            std::string query_tags = QueryTagInfo::GetUniqueInstance().getQueryTags();
            int query_id = QueryTagInfo::GetUniqueInstance().getQueryId();
            temp_oss << PATH_TO_TAG_MAP_FILE << "qid_saci_tag." << query_id << "."
                     << trading_date;  // Name of file containing the saci tag mapping queryid wise
            std::ofstream saci_tag_map_file;
            saci_tag_map_file.open(temp_oss.str().c_str(),
                                   std::ofstream::out | std::ofstream::app);  // Open the file in append mode as a
                                                                              // strategy might have multiple
                                                                              // BaseLiveTraders
            saci_tag_map_file << query_id << ", " << client_id_ << ", " << query_tags << std::endl;
            saci_tag_map_file.close();
          }

          dbglogger_ << "RMC_SACI: " << client_id_ << " (logging for risk monitor)\n";  // Do not change logging format
                                                                                        // (risk monitor uses this
                                                                                        // line): Abhishek
          dbglogger_.DumpCurrentBuffer();
        }
      }
    }
  }

  ~BaseLiveTrader() {
    if (shm_writer_ != NULL) delete (shm_writer_);

    if (tcpsock_.IsOpen()) {
      tcpsock_.Close();
    }
  }

  void IgnoreFromGlobalPos() {
    ignore_from_global_pos_ = true;
    std::cout << "ignore_from_global_pos enabled." << std::endl;
  }

  inline void SendTrade(const BaseOrder& _order_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(27);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(28);
#endif

    if (_order_.is_fok_) {
      order_req_.orq_request_type_ = ORQ_FOK_SEND;
    } else if (_order_.is_ioc_) {
      order_req_.orq_request_type_ = ORQ_IOC;
    } else {
      order_req_.orq_request_type_ = ORQ_SEND;
    }

    strncpy(order_req_.symbol_, _order_.security_name_,
            kSecNameLen);  // TODO_OPT replace this with two 64-bit equality operations, which might be faster ?
    order_req_.price_ = _order_.price_;

    //@also set the int_price else (it gets -1)
    order_req_.int_price_ = _order_.int_price_;

    order_req_.size_requested_ = _order_.size_requested_;
    order_req_.size_disclosed_ = _order_.size_disclosed_;
    order_req_.buysell_ = _order_.buysell_;
    order_req_.client_assigned_order_sequence_ = _order_.client_assigned_order_sequence_;

    order_req_.client_request_time_ = watch_.tv();
    order_req_.ignore_from_global_pos = ignore_from_global_pos_;

    HFSAT::RuntimeProfiler& prof = HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT);
    prof.End();
    const ProfilerTimeInfo& time_info = prof.GetTimeInfo();
    order_req_.t2t_cshmw_start_time_ = time_info.cshmw_start_time;
    order_req_.t2t_cshmw_end_time_ = time_info.cshmw_end_time;
    order_req_.t2t_tradeinit_start_time_ = time_info.tradeinit_start_time;
    order_req_.t2t_tradeinit_end_time_ = time_info.tradeinit_end_time;

    shm_writer_->writeT(send_order_request_);
    int start_tag = 22;
    int end_tag = 25;
    for (int i = start_tag; i <= end_tag; i++){
      HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(i);
    }
    HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(27);
  }

  //@@Function - SendRecoveryRequest
  //
  // Send a recovery request for the dropped packet to ORS, only two fields are relavant and the rest
  // of the fields are unused
  //
  //@@Args : SendRecoveryRequest
  //
  //@missing_sequence - the dropped packet sequence number
  //@server_assigned_client_id - client id
  inline void SendRecoveryRequest(const int32_t missing_sequence, const int32_t server_assigned_client_id,
                                  std::set<int32_t>& clear_storage) const {
    if (NULL != shm_writer_) {
      DataWriterIdPair<GenericORSRequestStruct> recovery_request;
      recovery_request.writer_id = client_id_;
      recovery_request.data.orq_request_type_ = ORQ_RECOVER;  // Recovery request

      // Note that the saos and caos are place holders for transmitting message sequence and saci
      recovery_request.data.server_assigned_order_sequence_ = missing_sequence;
      recovery_request.data.client_assigned_order_sequence_ = server_assigned_client_id;

      shm_writer_->writeT(recovery_request);
    }
  }

  inline void Cancel(const BaseOrder& _order_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(26);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(28);
#endif
    order_req_.orq_request_type_ = ORQ_CANCEL;
    strncpy(order_req_.symbol_, _order_.security_name_, kSecNameLen);
    order_req_.price_ = _order_.price_;
    //@also set the int_price else (it gets -1)
    order_req_.int_price_ = _order_.int_price_;
    order_req_.server_assigned_order_sequence_ = _order_.server_assigned_order_sequence_;
    order_req_.size_requested_ = _order_.size_requested_;
    order_req_.size_disclosed_ = 0;
    order_req_.buysell_ = _order_.buysell_;
    order_req_.client_assigned_order_sequence_ = _order_.client_assigned_order_sequence_;
    // Written to client_thread.

    order_req_.client_request_time_ = watch_.tv();
    order_req_.ignore_from_global_pos = ignore_from_global_pos_;

    HFSAT::RuntimeProfiler& prof = HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT);
    prof.End();
    const ProfilerTimeInfo& time_info = prof.GetTimeInfo();
    order_req_.t2t_cshmw_start_time_ = time_info.cshmw_start_time;
    order_req_.t2t_cshmw_end_time_ = time_info.cshmw_end_time;
    order_req_.t2t_tradeinit_start_time_ = time_info.tradeinit_start_time;
    order_req_.t2t_tradeinit_end_time_ = time_info.tradeinit_end_time;

    shm_writer_->writeT(send_order_request_);  // This is read in Client Receiver.
    int start_tag = 22;
    int end_tag = 25;
    for (int i = start_tag; i <= end_tag; i++){
      HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(i);
    }
    HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(26);
  }

  inline void Modify(const BaseOrder& _order_, double _new_price_, int _new_int_price_, int _new_size_requested_) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(27);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(28);
#endif
    order_req_.orq_request_type_ = ORQ_CXLREPLACE;
    strncpy(order_req_.symbol_, _order_.security_name_, kSecNameLen);
    order_req_.price_ = _new_price_;
    //@also set the int_price else (it gets -1)
    order_req_.int_price_ = _new_int_price_;
    order_req_.server_assigned_order_sequence_ = _order_.server_assigned_order_sequence_;
    order_req_.size_requested_ = _new_size_requested_;
    order_req_.size_disclosed_ = _order_.size_disclosed_;
    order_req_.buysell_ = _order_.buysell_;
    order_req_.client_assigned_order_sequence_ = _order_.client_assigned_order_sequence_;

    order_req_.client_request_time_ = watch_.tv();
    order_req_.ignore_from_global_pos = ignore_from_global_pos_;

    HFSAT::RuntimeProfiler& prof = HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT);
    prof.End();
    const ProfilerTimeInfo& time_info = prof.GetTimeInfo();
    order_req_.t2t_cshmw_start_time_ = time_info.cshmw_start_time;
    order_req_.t2t_cshmw_end_time_ = time_info.cshmw_end_time;
    order_req_.t2t_tradeinit_start_time_ = time_info.tradeinit_start_time;
    order_req_.t2t_tradeinit_end_time_ = time_info.tradeinit_end_time;

    shm_writer_->writeT(send_order_request_);  // This is read in Client Receiver.
    int start_tag = 22;
    int end_tag = 25;
    for (int i = start_tag; i <= end_tag; i++){
      HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(i);
    }
    HFSAT::CpucycleProfiler::GetUniqueInstance().RecordLastSessionForTag(27);
  }

  void NotifyRiskLimitsToOrs(const char* sec_name, int max_pos) {
    // Notifies the strategies risk limits to the ors
    if (shm_writer_ != NULL) {
      // Update the packet info
      order_req_.orq_request_type_ = ORQ_RISK;
      strncpy(order_req_.symbol_, sec_name,
              kSecNameLen);  // TODO_OPT replace this with two 64-bit equality operations, which might be faster ?
      order_req_.size_requested_ = max_pos;
      order_req_.client_request_time_ = watch_.tv();
      order_req_.ignore_from_global_pos = ignore_from_global_pos_;

      HFSAT::RuntimeProfiler& prof = HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT);
      prof.End();
      const ProfilerTimeInfo& time_info = prof.GetTimeInfo();
      order_req_.t2t_cshmw_start_time_ = time_info.cshmw_start_time;
      order_req_.t2t_cshmw_end_time_ = time_info.cshmw_end_time;
      order_req_.t2t_tradeinit_start_time_ = time_info.tradeinit_start_time;
      order_req_.t2t_tradeinit_end_time_ = time_info.tradeinit_end_time;

      shm_writer_->writeT(send_order_request_);  // This is read in Client Receiver.
    }
  }

  inline int GetClientId() const { return client_id_; }

  inline void FakeSendTrade() {
    // Loading up order_req struct for fake send (CPU cache warming)
    if (order_req_.size_requested_ == -10001){
      dbglogger_ << "Error: These values shouldn't be present " << order_req_.size_requested_ << DBGLOG_ENDL_FLUSH;
    }
  }
};
}

#endif  // BASE_ORDERROUTING_BASE_LIVE_TRADER_H
