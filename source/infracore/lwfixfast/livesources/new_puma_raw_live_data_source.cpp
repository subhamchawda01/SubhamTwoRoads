/**
    \file lwfixfast/livesources/new_puma_raw_live_data_source.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2010
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

#include "infracore/lwfixfast/livesources/new_puma_raw_live_data_source.hpp"
#include "infracore/lwfixfast/puma_md_processor.hpp"

#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

NewPumaRawLiveDataSource::NewPumaRawLiveDataSource(DebugLogger& dbglogger, const SecurityNameIndexer& sec_name_indexer,
                                                   HFSAT::SimpleLiveDispatcher* simple_live_dispatcher_,
                                                   HFSAT::FastMdConsumerMode_t mode, bool is_full_reference_mode,
                                                   int bmf_feed_type)
    : dbglogger_(dbglogger),
      sec_name_indexer_(sec_name_indexer),
      fast_decoder(FastStreamDecoder::GetPumaDecoder()),  // bmf_equity def_val = true
      mode_(mode),
      simple_live_dispatcher(simple_live_dispatcher_),
      bmf_feed_type_(bmf_feed_type),
      is_full_reference_mode_(is_full_reference_mode),
      p_time_keeper_(NULL),
      channelnos_(),        // std::vector of ints for channel numbers
      channel2snapaddr_(),  // std::map of int, string channel:ip
      channel2snapport_(),  // std::map of int, int  channel:port
      fd2sock_(),
      port2sock_(),  // both are std::maps of int, mcast_rec_sock_pointers
      msg_header_ptr((PUMA_MDS::Puma_Header*)calloc(1, sizeof(PUMA_MDS::Puma_Header))),
      sockets(),  // mcast_rec_socket pointers
      msg_buf((uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1)),
      fd2channel_((int*)calloc(MAX_PUMA_SOCKETS, sizeof(int))),
      currfd(0),
      chrecovercnt_(),
      ch2rsock_(),
      recovery_sockets(),
      fd2recovery_sock_(),
      channel2seqno_((uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),  // seq_number of union of all messages
      channel2chunk_no_(
          (uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),  // seq_number within the packet received
      channel2offset_((uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),
      channel2buffer_(),
      rec_channel2seqno_((uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2chunk_no_((uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2offset_((uint32_t*)calloc(MAX_PUMA_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2buffer_(),
      is_local_data_(TradingLocationUtils::GetTradingLocationExch(kExchSourceBMF) ==
                     TradingLocationUtils::GetTradingLocationFromHostname()) {}

NewPumaRawLiveDataSource::~NewPumaRawLiveDataSource() {}

void NewPumaRawLiveDataSource::createSockets(std::vector<std::string>& addresses, std::vector<int>& ports) {
  size_t num_sockets = addresses.size();
  HFSAT::MulticastReceiverSocket* sock = NULL;

  for (size_t i = 0; i < num_sockets; i++) {
    if (port2sock_.find(ports[i]) == port2sock_.end()) {
      sock = new HFSAT::MulticastReceiverSocket(addresses[i], ports[i],
                                                NetworkAccountInterfaceManager::instance().GetInterface(
                                                    HFSAT::kTLocBMF, HFSAT::kExchSourcePUMA, HFSAT::k_MktDataRaw));
      sock->setBufferSize(4000000);
      sockets.push_back(sock);
      fd2channel_[sock->socket_file_descriptor()] = channelnos_[i];

      // Priority to Local Sockets
      bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourceBMF) ==
                       TradingLocationUtils::GetTradingLocationFromHostname();

      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock->socket_file_descriptor(), isPrimary);
      fd2sock_[sock->socket_file_descriptor()] = sock;

      if (channel2buffer_.find(channelnos_[i]) == channel2buffer_.end()) {
        channel2buffer_[channelnos_[i]] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
      }
      if (rec_channel2buffer_.find(channelnos_[i]) == rec_channel2buffer_.end()) {
        rec_channel2buffer_[channelnos_[i]] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
      }
      port2sock_[ports[i]] = sock;
    } else {
      sock = port2sock_[ports[i]];
      sock->McastJoin(addresses[i]);
    }
  }
}

void NewPumaRawLiveDataSource::getChannelInfo(std::vector<std::string>& addr, std::vector<int>& ports) {
  /// populating for Cert environment .. from file
  // std::string filename =  bmf_equity? DEF_BMF_MCASTS_ : DEF_PUMA_MCASTS_;

  std::string filename;
  switch (bmf_feed_type_) {
    case 1:
      filename = DEF_PUMA_MCASTS_;
      break;
    case 2:
      if (mode_ == HFSAT::kLogger) {
        filename = DEF_NTP_FULL_MCASTS_;
      } else {
        filename = DEF_NTP_MCASTS_;
      }
      break;
    case 3:
      filename = DEF_NTP_ORD_MCASTS_;
      break;
  }

  if ((HFSAT::kLogger == mode_ || is_full_reference_mode_) && bmf_feed_type_ == 1) {
    filename = DEF_PUMA_FULL_MCASTS_;
  }

  std::fstream file(filename.c_str(), std::ofstream::in);

  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in InputProcessor::getChannelInfo", filename.c_str());
    exit(-1);
  }

  char line[1024];
  char* ticker;
  char* stype;
  std::vector<int> channelsOfInterest;

  // in raw and ComShm mode we want to listen to only those products which are added to sec_name indexer
  if (mode_ == HFSAT::kRaw || mode_ == HFSAT::kComShm) {
    unsigned int numSecurities = sec_name_indexer_.GetNumSecurityId();
    string* secNames = new string[numSecurities];

    for (unsigned int i = 0; i < numSecurities; i++) {
      secNames[i] = sec_name_indexer_.GetSecurityNameFromId(i);
    }
    std::string ref_filename;

    switch (bmf_feed_type_) {
      case 1:
        ref_filename = DEF_PUMA_REFLOC_;
        break;
      case 2:
        ref_filename = DEF_NTP_REFLOC_;
        break;
      case 3:
        ref_filename = DEF_NTP_ORD_REFLOC_;
        break;
    }

    // populating from reference file the channels fo interest to listen to
    std::fstream ref_file_reader(ref_filename.c_str(), std::ifstream::in);
    if (!ref_file_reader || !ref_file_reader.is_open()) {
      fprintf(stderr, "Could not open file %s in NewPumaRawLiveDataSource::getChannelInfo", ref_filename.c_str());
      exit(-1);
    }
    memset(line, 0, sizeof(line));
    while (ref_file_reader.getline(line, sizeof(line))) {
      ticker = NULL;
      ticker = strtok(line, "\n\t ");
      if (ticker && strstr(ticker, "#") == NULL) {
        string internalRef = string(strtok(NULL, "\n\t "));
        for (unsigned int i = 0; i < numSecurities; i++) {
          if (secNames[i] == internalRef) {
            channelsOfInterest.push_back(atoi(strtok(NULL, "\n\t ")));
            break;
          }
        }
      }
    }
    ref_file_reader.close();
  }

  memset(line, 0, sizeof(line));

  while (file.getline(line, sizeof(line))) {
    ticker = NULL;
    ticker = strtok(line, "\n\t ");

    if (ticker && strstr(ticker, "#") == NULL) {
      int channel = atoi(ticker);

      if (mode_ == HFSAT::kRaw || mode_ == HFSAT::kComShm) {
        bool found = false;
        for (unsigned int i = 0; i < channelsOfInterest.size(); i++) {
          if (channelsOfInterest[i] == channel) {
            found = true;
            break;
          }
        }
        if (!found) {
          continue;
        }
      }
      stype = strtok(NULL, "\n\t ");
      strtok(NULL, "\n\t ");    // feed
      if (mode_ == kReference)  // this work fine
      {
        if (stype[0] == 'R') {
          addr.push_back(strtok(NULL, "\n\t "));
          ports.push_back(atoi(strtok(NULL, "\n\t ")));
          channelnos_.push_back(channel);
        }
      } else {
        if (stype[0] == 'I') {
          addr.push_back(strtok(NULL, "\n\t "));
          ports.push_back(atoi(strtok(NULL, "\n\t ")));
          channelnos_.push_back(channel);
        }
        /// store data of snapshot channel 'A' for recovery purposes
        else if (stype[0] == 'S') {
          channel2snapaddr_[atoi(ticker)] = strtok(NULL, "\n\t ");
          channel2snapport_[atoi(ticker)] = atoi(strtok(NULL, "\n\t "));
        }
      }
    }
    memset(line, 0, sizeof(line));
  }
  std::cerr << "MCast IP Address & Port : \n";
  for (unsigned int i = 0; i < addr.size(); i++) {
    std::cerr << addr[i] << " " << ports[i] << "\n";
  }
  file.close();
}

inline void NewPumaRawLiveDataSource::ProcessAllEvents(int soc_fd_) {
  currfd = soc_fd_;
  if (fd2sock_.find(soc_fd_) != fd2sock_.end()) {
    int msg_len = (fd2sock_[soc_fd_])->ReadN(MAX_BMF_PACKET_SIZE, msg_buf);

    if (is_local_data_) {
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
    }

    assert(msg_len >= (int)sizeof(PUMA_MDS::Puma_Header));

    msg_header_ptr = (PUMA_MDS::Puma_Header*)msg_buf;
    msg_header_ptr->toHost();  // converting message to host format

    // Check header for correct UDP sequencing
    if (msg_header_ptr->msg_seq < channel2seqno_[fd2channel_[currfd]]) {  // Old fragment, discard.
      return;
    } else if (msg_header_ptr->msg_seq == channel2seqno_[fd2channel_[currfd]] &&
               msg_header_ptr->cur_cnk ==
                   channel2chunk_no_[fd2channel_[currfd]] + 1) {  // Same msg_seq, correct expected chunk no.

      memcpy(channel2buffer_[fd2channel_[currfd]] + channel2offset_[fd2channel_[currfd]], msg_buf + PUMA_MSG_HEADER_LEN,
             msg_header_ptr->msg_len);
      channel2offset_[fd2channel_[currfd]] += msg_header_ptr->msg_len;

    } else if (msg_header_ptr->msg_seq > channel2seqno_[fd2channel_[currfd]] &&
               msg_header_ptr->cur_cnk == 1) {  // New msg_seq, correct chunk no.
      memcpy(channel2buffer_[fd2channel_[currfd]], msg_buf + PUMA_MSG_HEADER_LEN, msg_header_ptr->msg_len);
      channel2offset_[fd2channel_[currfd]] = msg_header_ptr->msg_len;
    } else {
      // Incorrect chunk no.
      channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
      return;
    }

    // setting the channel sequence number and chunck number
    channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
    channel2chunk_no_[fd2channel_[currfd]] = msg_header_ptr->cur_cnk;

    // decode, once all the chunks have been received
    if (msg_header_ptr->cur_cnk == msg_header_ptr->num_cnk) {  // Completely formed message
      if (IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      }
      fast_decoder.decode((char*)channel2buffer_[fd2channel_[currfd]], channel2offset_[fd2channel_[currfd]]);
      if (IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      }
    }

    msg_len++;  // Only to avoid the unused warning.
  }
  // snapshot messages for recovery
  else if (fd2recovery_sock_.find(soc_fd_) != fd2recovery_sock_.end()) {
    int msg_len = fd2recovery_sock_[currfd]->ReadN(MAX_BMF_PACKET_SIZE, msg_buf);

    if (is_local_data_) {
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
    }

    assert(msg_len > 0);
    msg_header_ptr = (PUMA_MDS::Puma_Header*)msg_buf;
    msg_header_ptr->toHost();

    if (msg_header_ptr->msg_seq < rec_channel2seqno_[fd2channel_[currfd]]) {
      // Old fragment, discard.
      return;
    } else if (msg_header_ptr->msg_seq == rec_channel2seqno_[fd2channel_[currfd]] &&
               msg_header_ptr->cur_cnk == rec_channel2chunk_no_[fd2channel_[currfd]] + 1) {
      // Same msg_seq, correct chunk no.
      memcpy(rec_channel2buffer_[fd2channel_[currfd]] + rec_channel2offset_[fd2channel_[currfd]],
             msg_buf + PUMA_MSG_HEADER_LEN, msg_header_ptr->msg_len);
      rec_channel2offset_[fd2channel_[currfd]] += msg_header_ptr->msg_len;
    } else if (msg_header_ptr->msg_seq > rec_channel2seqno_[fd2channel_[currfd]] && msg_header_ptr->cur_cnk == 1) {
      // New msg_seq, correct chunk no.
      memcpy(rec_channel2buffer_[fd2channel_[currfd]], msg_buf + PUMA_MSG_HEADER_LEN, msg_header_ptr->msg_len);
      rec_channel2offset_[fd2channel_[currfd]] = msg_header_ptr->msg_len;
    } else {
      // Incorrect chunk no.
      rec_channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
      return;
    }

    rec_channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
    rec_channel2chunk_no_[fd2channel_[currfd]] = msg_header_ptr->cur_cnk;

    if (msg_header_ptr->cur_cnk == msg_header_ptr->num_cnk) {
      // Completely formed message
      if (IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      }
      fast_decoder.decode((char*)rec_channel2buffer_[fd2channel_[currfd]], rec_channel2offset_[fd2channel_[currfd]]);
      if (IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      }
    }
    msg_len++;  // Only to avoid the unused warning.
  }
}

void NewPumaRawLiveDataSource::start(bool enable_recovery)  // usally true
{
  /// actual multicast data handling ..
  std::vector<std::string> maddresses;
  std::vector<int> mports;
  maddresses.clear();
  mports.clear();
  // if mode is reference we only get reference details, else snapshot & increament in their respective std::maps
  // viz mcast_ip , mcast_port , mcast_channel
  getChannelInfo(maddresses, mports);
  /// init sockets, also creates std::maps sockets, fdsec, port , adds self as listeners to thus created external socket
  /// ->
  /// cbf: processallevents : only increment channels
  createSockets(maddresses, mports);

  bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourcePUMA) ==
                   TradingLocationUtils::GetTradingLocationFromHostname();
  if (isPrimary) {
    enable_recovery = true;  // always recover at primary locations
  }

  enable_recovery = false;

  switch (mode_) {
    case kReference:
      PUMA_MD_PROCESSOR::PumaMDProcessor::InitRefMode(this, enable_recovery, bmf_feed_type_, is_full_reference_mode_);
      break;
    case kMcast:
      if (mcast_sender_socket_ == NULL) {
        fprintf(stderr, "mcast sock is null.. exiting \n");
        exit(-1);
      }
      PUMA_MD_PROCESSOR::PumaMDProcessor::InitMcastMode(mcast_sender_socket_, this, enable_recovery, bmf_feed_type_);
      break;
    case kLogger:
      PUMA_MD_PROCESSOR::PumaMDProcessor::InitLoggerMode(this, enable_recovery, bmf_feed_type_);
      break;
    case kComShm:
      PUMA_MD_PROCESSOR::PumaMDProcessor::InitComShmMode(this, enable_recovery, bmf_feed_type_);
      break;
    default:
      fprintf(stderr, "not implemented or wrong mode\n");
      exit(-1);
  }
}

void NewPumaRawLiveDataSource::startRecovery(int channel) {
  if (chrecovercnt_.find(channel) == chrecovercnt_.end() || chrecovercnt_[channel] == 0) {
    HFSAT::MulticastReceiverSocket* sock =
        new HFSAT::MulticastReceiverSocket(channel2snapaddr_[channel], channel2snapport_[channel],
                                           NetworkAccountInterfaceManager::instance().GetInterface(
                                               HFSAT::kTLocBMF, HFSAT::kExchSourcePUMA, HFSAT::k_MktDataRaw));
    sock->setBufferSize(4000000);
    sock->SetNonBlocking();
    recovery_sockets.push_back(sock);

    simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock->socket_file_descriptor());
    fd2recovery_sock_[sock->socket_file_descriptor()] = sock;

    fd2channel_[sock->socket_file_descriptor()] = channel;

    if (channel2buffer_.find(channel) == channel2buffer_.end()) {
      channel2buffer_[channel] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
    }

    if (rec_channel2buffer_.find(channel) == rec_channel2buffer_.end()) {
      rec_channel2buffer_[channel] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
    }

    chrecovercnt_[channel] = 1;
    ch2rsock_[channel] = sock;

    fprintf(stderr, "Started recovery for channel '%d' channel2seqno_ : %d\n", channel, channel2seqno_[channel]);
  } else {
    chrecovercnt_[channel] += 1;
  }
}

void NewPumaRawLiveDataSource::endRecovery(int channel) {
  if (chrecovercnt_[channel] == 1) {
    std::cerr << "ending recovery for channel " << channel << "\n";
    std::vector<HFSAT::MulticastReceiverSocket*>::iterator sockiter =
        std::find(recovery_sockets.begin(), recovery_sockets.end(), ch2rsock_[channel]);
    assert(sockiter != recovery_sockets.end());
    fd2recovery_sock_.erase((*sockiter)->socket_file_descriptor());
    simple_live_dispatcher->RemoveSimpleExternalDataLiveListenerSocket(this, (*sockiter)->socket_file_descriptor());
    recovery_sockets.erase(sockiter);
    delete (ch2rsock_[channel]);
    ch2rsock_[channel] = NULL;
  }

  chrecovercnt_[channel] -= 1;
  rec_channel2seqno_[channel] = 0;     // Required!
  rec_channel2chunk_no_[channel] = 0;  // not really necessary
  rec_channel2offset_[channel] = 0;    // not really necessary
}

// For certification need to worry about the sequence reset msg.
void NewPumaRawLiveDataSource::seqReset() {
  rec_channel2seqno_[fd2channel_[currfd]] = 0;
  channel2seqno_[fd2channel_[currfd]] = 0;
  rec_channel2offset_[fd2channel_[currfd]] = 0;
  channel2offset_[fd2channel_[currfd]] = 0;
  rec_channel2chunk_no_[fd2channel_[currfd]] = 0;
  channel2chunk_no_[fd2channel_[currfd]] = 0;
}
}
