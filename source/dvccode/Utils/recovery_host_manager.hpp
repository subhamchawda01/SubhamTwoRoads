// =====================================================================================
//
//       Filename:  recovery_host_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/15/2016 08:42:55 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/recovery_host_interface.hpp"
#include "dvccode/Utils/tcp_server_manager.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"

namespace HFSAT {
namespace Utils {

class RecoveryHostManager : public HFSAT::Utils::TCPServerSocketListener {
 private:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Utils::TCPServerManager tcp_server_manager_;
  HFSAT::CDef::BaseRecoveryHostInterface *base_recovery_host_interface_;

 public:
  RecoveryHostManager(int32_t const &recovery_host_port, HFSAT::DebugLogger &dbglogger,
                      HFSAT::CDef::BaseRecoveryHostInterface *recovery_host_interface)
      : dbglogger_(dbglogger),
        tcp_server_manager_(recovery_host_port, dbglogger_),
        base_recovery_host_interface_(recovery_host_interface) {
    tcp_server_manager_.SubscribeForUpdates(this);
  }

  ~RecoveryHostManager() {}

  void StartRecoveryServer() { tcp_server_manager_.run(); }

  void OnClientRequest(int32_t client_fd, char *buffer, uint32_t const &length) {
    if (length < RECOVERY_HANDSHAKE_LENGTH || length > RECOVERY_HANDSHAKE_LENGTH) {
      tcp_server_manager_.RespondFailureToClient(client_fd);
      return;
    }

    char exchsymbol[RECOVERY_HANDSHAKE_LENGTH];
    memset((void *)exchsymbol, 0, RECOVERY_HANDSHAKE_LENGTH);

    memcpy((void *)exchsymbol, (void *)buffer, RECOVERY_HANDSHAKE_LENGTH);

    if (false == base_recovery_host_interface_->IsSecurityAvailableForRecovery(exchsymbol)) {
      tcp_server_manager_.RespondFailureToClient(client_fd);
      return;
    }

    char *packet_buffer = new char[MAX_RECOVERY_PACKET_SIZE];

    if (false ==
        base_recovery_host_interface_->FetchProcessedLiveOrdersMap(exchsymbol, packet_buffer,
                                                                   MAX_RECOVERY_PACKET_SIZE)) {
      tcp_server_manager_.RespondFailureToClient(client_fd);
      return;
    }

    int32_t *packet_length_ptr = (int32_t *)((char *)packet_buffer);
    int32_t packet_length = *packet_length_ptr;
    packet_length += sizeof(int32_t);

    tcp_server_manager_.RespondToClient(client_fd, packet_buffer, packet_length);

    delete packet_buffer;
  }
};
}
}
