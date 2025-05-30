/**
    \file fixfast/BSERawMDHandler.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2010
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */
#pragma once

#include <string>
#include <map>
#include <assert.h>
#include <set>
#include <tr1/unordered_map>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/char16_map.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"

#include "infracore/BSEMD/bse_decoder.hpp"

namespace HFSAT {
#define BSE_RAW_MD_CHANNELS "bse-tbt-mcast.txt"
#define MAX_UDP_MSG_LEN 72000

class BSERawMDHandler : public SimpleExternalDataLiveListener {
 public:
  BSERawMDHandler(DebugLogger& dbglogger, SecurityNameIndexer& sec_name_indexer,
                           HFSAT::SimpleLiveDispatcher* simple_live_dispatcher_, HFSAT::FastMdConsumerMode_t mode,
                           bool full_mode = false);
  ~BSERawMDHandler();

  void start(bool enable_recovery = false);

  bool IsLocalData() { return is_local_data_; }

 private:
  DebugLogger& dbglogger_;                 ///< error logger
  SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher;  // for updating the sockets info

  std::map<int, HFSAT::MulticastReceiverSocket*> port2sock_;
  std::map<int, HFSAT::MulticastReceiverSocket*> fd2sock_;

  std::map<std::string, int> prod2snapport_;
  std::map<std::string, std::string> prod2snapaddr_;

  /// sockets specific variables
  std::vector<HFSAT::MulticastReceiverSocket*> sockets;
  char* msg_buf;

  std::map<int, HFSAT::MulticastReceiverSocket*> port2rsock_;  /// std::map of recovery port->multicast receiver socket
  std::vector<HFSAT::MulticastReceiverSocket*> recovery_sockets_;
  std::map<int, int> recovery_port_ctr_;           /// std::map of port no->number of instruments in recovery
  std::map<std::string, int> recovery_mcast_ctr_;  /// std::map of mcast address->number of instruments in recovery

  HFSAT::ExternalTimeListener* p_time_keeper_;
  HFSAT::MulticastSenderSocket* mcast_sender_socket_;
//  FastStreamDecoder fast_decoder;
  HFSAT::EobiDecoder eobi_decoder_;
  HFSAT::FastMdConsumerMode_t mode_;

  std::string interface_a_;
  std::string interface_b_;

  std::set<uint64_t> processed_sequence_;

  std::tr1::unordered_map<int32_t, uint32_t> application_seq_num_;
  bool full_mode_;  // If this is true we will create mcast and ref file for all mkt segments and data files over them.

  bool is_local_data_;

 private:
  void createSockets(std::vector<std::string>& addr, std::vector<int>& ports, std::vector<std::string>& ifaces);
  void getChannelInfo(std::vector<std::string>& addr, std::vector<int>& ports, std::vector<std::string>& ifaces);

 public:
  void startRecovery(const char* p_prod_);
  void endRecovery(const char* p_prod_);

  void startRecoveryForAllProducts();
  void endRecoveryForAllProducts();

  inline void SetMultiSenderSocket(HFSAT::MulticastSenderSocket* mcast_sock_) { mcast_sender_socket_ = mcast_sock_; }

  inline HFSAT::ExternalTimeListener* GetTimeListener() { return p_time_keeper_; }
  inline HFSAT::MulticastSenderSocket* GetMulticastSenderSocketListener() { return mcast_sender_socket_; }
  inline void SetExternalTimeListener(HFSAT::ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }
  inline const SecurityNameIndexer& getSecurityNameIndexer() { return sec_name_indexer_; }

  inline void ProcessAllEvents(int sock_fd_);
};
}
