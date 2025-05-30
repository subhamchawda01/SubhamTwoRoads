/**
    \file fixfast/NewNtpRawLiveDataSource.cpp

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

#include "infracore/lwfixfast/livesources/new_ntp_raw_live_data_source.hpp"
#include "infracore/lwfixfast/ntp_md_processor.hpp"
#include "infracore/lwfixfast/bmf_md_processor.hpp"
#include "infracore/lwfixfast/ntp_ord_md_processor.hpp"

#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

NewNTPRawLiveDataSource::NewNTPRawLiveDataSource(DebugLogger& dbglogger, const SecurityNameIndexer& sec_name_indexer,
                                                 SimpleLiveDispatcher* simple_live_dispatcher_,
                                                 FastMdConsumerMode_t mode, int t_bmf_feed_type_)
    : dbglogger_(dbglogger),
      sec_name_indexer_(sec_name_indexer),
      fast_decoder(
          t_bmf_feed_type_ == 1
              ? FastStreamDecoder::GetNtpDecoder()
              : (t_bmf_feed_type_ == 2
                     ? FastStreamDecoder::GetNtpOrdDecoder()
                     : (t_bmf_feed_type_ == 3 ? FastStreamDecoder::GetPumaDecoder()
                                              : (t_bmf_feed_type_ == 4 ? FastStreamDecoder::GetBmfDecoder()
                                                                       : FastStreamDecoder::GetBMFPumaDecoder())))),
      mode_(mode),
      simple_live_dispatcher(simple_live_dispatcher_),
      bmf_feed_type_(t_bmf_feed_type_),
      p_time_keeper_(NULL),
      channelnos_(),
      channel2snapaddr_(),
      channel2snapport_(),
      fd2sock_(),
      port2sock_(),
      msg_header_ptr(NULL),
      sockets(),
      msg_buf((uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1)),
      fd2channel_((int*)calloc(MAX_NTP_SOCKETS, sizeof(int))),
      currfd(0),
      chrecovercnt_(),
      ch2rsock_(),
      recovery_sockets(),
      fd2recovery_sock_(),
      channel2seqno_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      channel2chunk_no_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      channel2offset_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      channel2buffer_(),
      rec_channel2seqno_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2chunk_no_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2offset_((uint32_t*)calloc(MAX_NTP_CHANNEL_NUM, sizeof(uint32_t))),
      rec_channel2buffer_(),
      is_local_data_(false) {}

NewNTPRawLiveDataSource::~NewNTPRawLiveDataSource() {}

void NewNTPRawLiveDataSource::createSockets(std::vector<std::string>& addresses, std::vector<int>& ports) {
  size_t num_sockets = addresses.size();
  MulticastReceiverSocket* sock = NULL;

  auto interface = NetworkAccountInterfaceManager::instance().GetInterface(kExchSourceBMF, k_MktDataRaw);

  for (size_t i = 0; i < num_sockets; i++) {
    if (port2sock_.find(ports[i]) == port2sock_.end()) {
      sock = new MulticastReceiverSocket(addresses[i], ports[i], interface);
      std::cout << "IP: " << addresses[i] << " Port: " << ports[i] << " Interface: " << interface << std::endl;
      sock->setBufferSize(4000000);
      sockets.push_back(sock);

      int sock_descriptor = sock->socket_file_descriptor();
      fd2channel_[sock_descriptor] = channelnos_[i];

      // Priority to Local Sockets
      bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourceNTP) ==
                       TradingLocationUtils::GetTradingLocationFromHostname();

      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock_descriptor, isPrimary);
      fd2sock_[sock_descriptor] = sock;

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

std::string NewNTPRawLiveDataSource::GetMcastFile() {
  switch (bmf_feed_type_) {
    case 1: {
      if (mode_ == kLogger) {
        return DEF_NTP_FULL_MCASTS_;
      } else {
        return DEF_NTP_MCASTS_;
      }
      break;
    }

    case 2: {
      return DEF_NTP_ORD_MCASTS_;
      break;
    }

    case 3: {
      return DEF_PUMA_MCASTS_;
      break;
    }

    case 4: {
      return DEF_BMF_MCASTS_;
      break;
    }

    case 5: {
      return DEF_BMF_PUMA_MCASTS_;
      break;
    }

    default: {
      std::cerr << __func__ << " : " << __LINE__ << "Unknown BMF Feed Type : " << bmf_feed_type_ << std::endl;
      exit(-1);
    }
  }

  return DEF_BMF_MCASTS_;
}

std::string NewNTPRawLiveDataSource::GetRefFile() {
  switch (bmf_feed_type_) {
    case 1: {
      if (mode_ == kLogger) {
        return DEF_NTP_FULL_REFLOC_;
      } else {
        return DEF_NTP_REFLOC_;
      }
      break;
    }

    case 2: {
      return DEF_NTP_ORD_REFLOC_;
      break;
    }

    case 3: {
      return DEF_PUMA_REFLOC_;
      break;
    }

    case 4: {
      return DEF_BMF_REFLOC_;
      break;
    }

    case 5: {
      return DEF_BMF_PUMA_REFLOC_;
      break;
    }

    default: {
      std::cerr << __func__ << " : " << __LINE__ << "Unknown BMF Feed Type : " << bmf_feed_type_ << std::endl;
      exit(-1);
    }
  }

  return DEF_BMF_REFLOC_;
}

void NewNTPRawLiveDataSource::getChannelInfo(std::vector<std::string>& addr, std::vector<int>& ports) {
  // Get file containing mcast addresses
  std::string filename = GetMcastFile();

  std::fstream file(filename.c_str(), std::ofstream::in);
  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in InputProcessor::getChannelInfo", filename.c_str());
    exit(-1);
  }

  char line[1024];
  char* ticker;
  char* stype;
  std::vector<int> channelsOfInterest;

  // In raw mode we want to listen to only those products which are added to sec_name_indexer
  if (mode_ == HFSAT::kRaw || mode_ == HFSAT::kComShm) {
    unsigned int numSecurities = sec_name_indexer_.GetNumSecurityId();
    string* secNames = new string[numSecurities];

    for (unsigned int i = 0; i < numSecurities; i++) {
      secNames[i] = sec_name_indexer_.GetSecurityNameFromId(i);
    }

    std::string ref_filename = GetRefFile();

    /// populating from reference file the channels fo interest to listen to
    std::fstream ref_file_reader(ref_filename.c_str());

    if (!ref_file_reader || !ref_file_reader.is_open()) {
      fprintf(stderr, "Could not open file %s in NewNTPRawLiveDataSource::getChannelInfo", ref_filename.c_str());
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
        if (!found) continue;
      }

      stype = strtok(NULL, "\n\t ");
      strtok(NULL, "\n\t ");  // feed
      if (mode_ == kReference) {
        if (stype[0] == 'R') {
          addr.push_back(strtok(NULL, "\n\t "));
          ports.push_back(atoi(strtok(NULL, "\n\t ")));
          channelnos_.push_back(channel);
        }
      } else {
        /// use only incremental channels for normal processing
        // for incremental use feed other than A (i.e. feed B) only when the location is CHI
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
  file.close();
}

inline void NewNTPRawLiveDataSource::ProcessAllEvents(int soc_fd_) {
  currfd = soc_fd_;

  if (fd2sock_.find(soc_fd_) != fd2sock_.end()) {
    int msg_len = fd2sock_[soc_fd_]->ReadN(MAX_BMF_PACKET_SIZE, msg_buf);
    if (is_local_data_) {
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
    }

    assert(msg_len >= (int)sizeof(NTP_MDS::NTP_Header));
    msg_header_ptr = (NTP_MDS::NTP_Header*)msg_buf;
    msg_header_ptr->toHost();

    // Check header for correct UDP sequencing
    if (msg_header_ptr->msg_seq < channel2seqno_[fd2channel_[currfd]]) return;

    if (msg_header_ptr->msg_seq < channel2seqno_[fd2channel_[currfd]]) {
      // Old fragment, discard.
      return;
    } else if (msg_header_ptr->msg_seq == channel2seqno_[fd2channel_[currfd]] &&
               msg_header_ptr->cur_cnk == channel2chunk_no_[fd2channel_[currfd]] + 1) {
      // Same msg_seq, correct chunk no.
      memcpy(channel2buffer_[fd2channel_[currfd]] + channel2offset_[fd2channel_[currfd]], msg_buf + NTP_MSG_HEADER_LEN,
             msg_header_ptr->msg_len);
      channel2offset_[fd2channel_[currfd]] += msg_header_ptr->msg_len;
    } else if (msg_header_ptr->msg_seq > channel2seqno_[fd2channel_[currfd]] && msg_header_ptr->cur_cnk == 1) {
      // New msg_seq, correct chunk no.
      memcpy(channel2buffer_[fd2channel_[currfd]], msg_buf + NTP_MSG_HEADER_LEN, msg_header_ptr->msg_len);
      channel2offset_[fd2channel_[currfd]] = msg_header_ptr->msg_len;
    } else {
      // Incorrect chunk no.
      channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
      return;
    }

    channel2seqno_[fd2channel_[currfd]] = msg_header_ptr->msg_seq;
    channel2chunk_no_[fd2channel_[currfd]]++;

    if (msg_header_ptr->cur_cnk == msg_header_ptr->num_cnk) {
      // Completely formed message
      if (is_local_data_) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      }
      fast_decoder.decode((char*)channel2buffer_[fd2channel_[currfd]], channel2offset_[fd2channel_[currfd]]);
      if (is_local_data_) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      }
    }
    msg_len++;  // Only to avoid the unused warning.

  } else if (fd2recovery_sock_.find(soc_fd_) != fd2recovery_sock_.end()) {
    int msg_len = fd2recovery_sock_[currfd]->ReadN(MAX_BMF_PACKET_SIZE, msg_buf);
    if (is_local_data_) {
      HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
    }
    assert(msg_len > 0);
    msg_header_ptr = (NTP_MDS::NTP_Header*)msg_buf;
    msg_header_ptr->toHost();

    if (msg_header_ptr->msg_seq < rec_channel2seqno_[fd2channel_[currfd]]) {
      // Old fragment, discard.
      return;
    } else if (msg_header_ptr->msg_seq == rec_channel2seqno_[fd2channel_[currfd]] &&
               msg_header_ptr->cur_cnk == rec_channel2chunk_no_[fd2channel_[currfd]] + 1) {
      // Same msg_seq, correct chunk no.
      memcpy(rec_channel2buffer_[fd2channel_[currfd]] + rec_channel2offset_[fd2channel_[currfd]],
             msg_buf + NTP_MSG_HEADER_LEN, msg_header_ptr->msg_len);
      rec_channel2offset_[fd2channel_[currfd]] += msg_header_ptr->msg_len;
    } else if (msg_header_ptr->msg_seq > rec_channel2seqno_[fd2channel_[currfd]] && msg_header_ptr->cur_cnk == 1) {
      // New msg_seq, correct chunk no.
      memcpy(rec_channel2buffer_[fd2channel_[currfd]], msg_buf + NTP_MSG_HEADER_LEN, msg_header_ptr->msg_len);
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
      if (is_local_data_) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      }
      fast_decoder.decode((char*)rec_channel2buffer_[fd2channel_[currfd]], rec_channel2offset_[fd2channel_[currfd]]);
      if (is_local_data_) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      }
    }
    msg_len++;  // Only to avoid the unused warning.
  }
}

void NewNTPRawLiveDataSource::start(bool enable_recovery) {
  /// Actual multicast data handling
  std::vector<std::string> maddresses;
  std::vector<int> mports;
  maddresses.clear();
  mports.clear();
  getChannelInfo(maddresses, mports);
  /// init sockets
  createSockets(maddresses, mports);

  bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourceBMF) ==
                   TradingLocationUtils::GetTradingLocationFromHostname();
  if (isPrimary) {
    enable_recovery = true;  // always recover at primary locations
  }

  is_local_data_ = isPrimary;

  switch (bmf_feed_type_) {
    case 1: {
      switch (mode_) {
        case kReference:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitRefMode(this, enable_recovery);
          break;
        case kMcast:
          if (mcast_sender_socket_ == NULL) {
            fprintf(stderr, "mcast sock is null... exiting \n");
            exit(-1);
          }
          NTP_MD_PROCESSOR::NtpMDProcessor::InitMcastMode(mcast_sender_socket_, this, enable_recovery);
          break;
        case kComShm:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitComShmMode(this, enable_recovery);
          break;
        case kProShm:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitProShmMode(this, enable_recovery);
          break;
        case kLogger:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitLoggerMode(this, enable_recovery);
          break;
        default:
          fprintf(stderr, "not implemented, wrong mode\n");
          exit(-1);
      }
    } break;

    case 2: {
      enable_recovery = false;
      switch (mode_) {
        case kReference:
          NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::InitRefMode(this, enable_recovery);
          break;
        case kMcast:
          if (mcast_sender_socket_ == NULL) {
            fprintf(stderr, "mcast sock is null... exiting \n");
            exit(-1);
          }
          NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::InitMcastMode(mcast_sender_socket_, this, enable_recovery);
          break;
        case kComShm:
          NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::InitComShmMode(this, enable_recovery);
          break;
        case kProShm:
          NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::InitProShmMode(this, enable_recovery);
          break;
        case kLogger:
          NTP_ORD_MD_PROCESSOR::NtpOrdMdProcessor::InitLoggerMode(this, enable_recovery);
          break;
        default:
          fprintf(stderr, "not implemented, wrong mode\n");
          exit(-1);
      }
    } break;

    case 3: {
    } break;

    case 4: {
      switch (mode_) {
        case kReference:
          BMF_MD_PROCESSOR::BmfMDProcessor::InitRefMode(this, enable_recovery);
          break;
        case kMcast:
          if (mcast_sender_socket_ == NULL) {
            fprintf(stderr, "mcast sock is null... exiting \n");
            exit(-1);
          }
          BMF_MD_PROCESSOR::BmfMDProcessor::InitMcastMode(mcast_sender_socket_, this, enable_recovery);
          break;
        case kLogger:
          BMF_MD_PROCESSOR::BmfMDProcessor::InitLoggerMode(this, enable_recovery);
          break;
        default:
          fprintf(stderr, "not implemented, wrong mode\n");
          exit(-1);
      }
    } break;

    case 5: {
      switch (mode_) {
        case kReference:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitRefMode(this, enable_recovery);
          break;
        case kMcast:
          if (mcast_sender_socket_ == NULL) {
            fprintf(stderr, "mcast sock is null... exiting \n");
            exit(-1);
          }
          NTP_MD_PROCESSOR::NtpMDProcessor::InitMcastMode(mcast_sender_socket_, this, enable_recovery);
          break;
        case kComShm:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitComShmMode(this, enable_recovery);
          break;
        case kProShm:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitProShmMode(this, enable_recovery);
          break;
        case kLogger:
          NTP_MD_PROCESSOR::NtpMDProcessor::InitLoggerMode(this, enable_recovery);
          break;
        default:
          fprintf(stderr, "not implemented, wrong mode\n");
          exit(-1);
      }
    } break;

    default:
      break;
  }
}

void NewNTPRawLiveDataSource::startRecovery(int channel) {
  if (chrecovercnt_.find(channel) == chrecovercnt_.end() || chrecovercnt_[channel] == 0) {
    auto interface = NetworkAccountInterfaceManager::instance().GetInterface(kExchSourceBMF, k_MktDataRaw);

    /// Don't need to worry about multiple sockets since only snapshot A feed is used
    auto sock = new MulticastReceiverSocket(channel2snapaddr_[channel], channel2snapport_[channel], interface);
    sock->setBufferSize(4000000);
    sock->SetNonBlocking();
    recovery_sockets.push_back(sock);

    auto sock_descriptor = sock->socket_file_descriptor();
    simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock_descriptor);
    fd2recovery_sock_[sock_descriptor] = sock;

    fd2channel_[sock_descriptor] = channel;

    if (channel2buffer_.find(channel) == channel2buffer_.end()) {
      channel2buffer_[channel] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
    }

    if (rec_channel2buffer_.find(channel) == rec_channel2buffer_.end()) {
      rec_channel2buffer_[channel] = (uint8_t*)calloc(MAX_BMF_PACKET_SIZE, 1);
    }

    chrecovercnt_[channel] = 1;
    ch2rsock_[channel] = sock;

  } else {
    chrecovercnt_[channel] += 1;
  }
}

void NewNTPRawLiveDataSource::endRecovery(int channel) {
  if (chrecovercnt_[channel] == 1) {
    auto sockiter = std::find(recovery_sockets.begin(), recovery_sockets.end(), ch2rsock_[channel]);
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
void NewNTPRawLiveDataSource::seqReset() {
  rec_channel2seqno_[fd2channel_[currfd]] = 0;
  channel2seqno_[fd2channel_[currfd]] = 0;
  rec_channel2offset_[fd2channel_[currfd]] = 0;
  channel2offset_[fd2channel_[currfd]] = 0;
  rec_channel2chunk_no_[fd2channel_[currfd]] = 0;
  channel2chunk_no_[fd2channel_[currfd]] = 0;
}

void NewNTPRawLiveDataSource::CleanUp() {
  FastStreamDecoder::RemoveNtpDecoder(fast_decoder);

  channelnos_.clear();
  channel2snapaddr_.clear();

  for (auto socket_ptr : sockets) {
    if (NULL != socket_ptr) {
      socket_ptr->Close();
      delete socket_ptr;
      socket_ptr = NULL;
    }
  }

  sockets.clear();

  if (NULL != msg_buf) {
    free(msg_buf);
    msg_buf = NULL;
  }

  if (NULL != fd2channel_) {
    free(fd2channel_);
    fd2channel_ = NULL;
  }

  chrecovercnt_.clear();

  for (auto socket_ptr : recovery_sockets) {
    if (NULL != socket_ptr) {
      socket_ptr->Close();
      delete socket_ptr;
      socket_ptr = NULL;
    }
  }

  if (NULL != channel2seqno_) {
    free(channel2seqno_);
    free(channel2chunk_no_);
    free(channel2offset_);
    free(rec_channel2seqno_);
    free(rec_channel2chunk_no_);
    free(rec_channel2offset_);

    channel2seqno_ = channel2chunk_no_ = channel2offset_ = rec_channel2seqno_ = rec_channel2chunk_no_ =
        rec_channel2offset_ = NULL;
  }

  for (auto itr : channel2buffer_) {
    if (NULL != itr.second) {
      free(itr.second);
      itr.second = NULL;
    }
  }

  for (auto itr : rec_channel2buffer_) {
    if (NULL != itr.second) {
      free(itr.second);
      itr.second = NULL;
    }
  }
}
}
