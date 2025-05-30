/**
   \file MDSMessages/ntp_raw_live_data_source.hpp

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

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/char16_map.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"

#include "infracore/lwfixfast/fast_stream_decoder.hpp"

namespace HFSAT {
#define MAX_NTP_CHANNEL_NUM 200
#define MAX_NTP_SOCKETS 200
#define NTP_MSG_HEADER_LEN 10

// BMF Equity data seems to be quite large, hence making buffer size as 1 million
#define MAX_BMF_PACKET_SIZE (1 << 20)

class NewNTPRawLiveDataSource : public SimpleExternalDataLiveListener {
 public:
  NewNTPRawLiveDataSource(DebugLogger& dbglogger, const SecurityNameIndexer& sec_name_indexer,
                          HFSAT::SimpleLiveDispatcher* simple_live_dispatcher_, HFSAT::FastMdConsumerMode_t mode,
                          int t_bmf_feed_type_ = 1);

  ~NewNTPRawLiveDataSource();

  // We will choose to turn off recovery in NY4 utils.
  void start(bool enable_recovery = false);

  /// recovery .. NewNTP
  void startRecovery(int channel);
  void endRecovery(int channel);

  HFSAT::ExternalTimeListener* GetTimeListener() { return p_time_keeper_; }

  void SetMultiSenderSocket(HFSAT::MulticastSenderSocket* mcast_sock_) { mcast_sender_socket_ = mcast_sock_; }
  void SetExternalTimeListener(HFSAT::ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  HFSAT::MulticastSenderSocket* GetMulticastSenderSocketListener() { return mcast_sender_socket_; }
  const SecurityNameIndexer& getSecurityNameIndexer() { return sec_name_indexer_; }

  void ProcessAllEvents(int soc_fd_);

  bool checkSnapshotEnd(int tot_num_reports_);
  void seqReset();
  int getCurrChannel() { return fd2channel_[currfd]; }

  bool IsLocalData() { return is_local_data_; }

 private:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  FastStreamDecoder fast_decoder;
  HFSAT::FastMdConsumerMode_t mode_;
  HFSAT::SimpleLiveDispatcher* simple_live_dispatcher;  // for updating the sockets info
  HFSAT::MulticastSenderSocket* mcast_sender_socket_;

  int bmf_feed_type_;  // 1 - ntp, 2 - ntp_ord, 3 - bmf_eq, 4 - bmf_eq_ord

  HFSAT::ExternalTimeListener* p_time_keeper_;

  std::vector<int> channelnos_;
  std::map<int, std::string> channel2snapaddr_;
  std::map<int, int> channel2snapport_;

  std::map<int, HFSAT::MulticastReceiverSocket*> fd2sock_;
  std::map<int, HFSAT::MulticastReceiverSocket*> port2sock_;

  NTP_MDS::NTP_Header* msg_header_ptr;

  // socket specific variables
  std::vector<HFSAT::MulticastReceiverSocket*> sockets;

  uint8_t* msg_buf;

  // Socket fd to channel no.
  int* fd2channel_;
  int currfd;

  /// NTP recovery specific
  std::map<int, int> chrecovercnt_;
  std::map<int, HFSAT::MulticastReceiverSocket*> ch2rsock_;
  std::vector<HFSAT::MulticastReceiverSocket*> recovery_sockets;
  std::map<int, HFSAT::MulticastReceiverSocket*> fd2recovery_sock_;

  uint32_t* channel2seqno_;
  uint32_t* channel2chunk_no_;
  uint32_t* channel2offset_;
  std::map<int, uint8_t*> channel2buffer_;

  uint32_t* rec_channel2seqno_;
  uint32_t* rec_channel2chunk_no_;
  uint32_t* rec_channel2offset_;
  std::map<int, uint8_t*> rec_channel2buffer_;

  bool is_local_data_;

  std::string GetMcastFile();
  std::string GetRefFile();
  void createSockets(std::vector<std::string>& addr, std::vector<int>& ports);
  void getChannelInfo(std::vector<std::string>& addr, std::vector<int>& ports);

  void CleanUp();
};
}
