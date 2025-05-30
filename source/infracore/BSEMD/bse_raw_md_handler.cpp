/**
    \file fixfast/BSERawMDHandler.cpp

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

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CDef/refdata_locator.hpp"
#include "infracore/BSEMD/bse_md_processor.hpp"
#include "infracore/BSEMD/bse_raw_md_handler.hpp"

namespace HFSAT {

BSERawMDHandler::BSERawMDHandler(DebugLogger& dbglogger, SecurityNameIndexer& sec_name_indexer,
                                                   HFSAT::SimpleLiveDispatcher* simple_live_dispatcher_,
                                                   HFSAT::FastMdConsumerMode_t mode, bool full_mode)
    : dbglogger_(dbglogger),
      sec_name_indexer_(sec_name_indexer),
      simple_live_dispatcher(simple_live_dispatcher_),
      mode_(mode),
      interface_a_(
          NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRawSideA)),
      interface_b_(
          NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRawSideB)),
      processed_sequence_(),
      full_mode_(full_mode),
      is_local_data_(false) {
  prod2snapport_.clear();
  prod2snapaddr_.clear();

  application_seq_num_.clear();

  msg_buf = (char*)calloc(MAX_UDP_MSG_LEN, 1);
  fd2sock_.clear();
  port2sock_.clear();

  if (interface_a_ == UNDEFINED_INTERFACE_STRING || interface_b_ == UNDEFINED_INTERFACE_STRING) {
    std::cerr << " Interface Not Specified For Either A Side Or B Side \n";
    exit(-1);
  }
}

BSERawMDHandler::~BSERawMDHandler() {}

void BSERawMDHandler::getChannelInfo(std::vector<std::string>& addr, std::vector<int>& ports,
                                              std::vector<std::string>& ifaces) {
  if (mode_ == HFSAT::kModeMax) {
    return;  // dummy
  }
  std::ostringstream host_bse_tbt_filename;

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_CHANNELS;
  std::string mcast_file_name = host_bse_tbt_filename.str();



//  std::string mcast_file_name = full_mode_ ? DEF_EOBI_FULL_MCASTS_ : DEF_EOBI_MCASTS_;

  std::fstream file(mcast_file_name.c_str(), std::ofstream::in);
  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in BSERawMDHandler::getChannelInfo", mcast_file_name.c_str());
    exit(-1);
  }

  char line[1024];
  char* ticker;
  char* stype;
  memset(line, 0, sizeof(line));

  // kExchSourceEUREX instead of kExchSourceEOBI
  bool isPrimary = true; //HFSAT::TradingLocationUtils::IsCurrentLocationPrimary(kExchSourceEUREX);

  is_local_data_ = isPrimary;

  while (file.getline(line, sizeof(line))) {
    ticker = NULL;
    ticker = strtok(line, "\n\t ");
    std::cout << "line: " << line << std::endl;
    if(ticker[0] == '#') continue;

    std::cout << "Ticker: " << ticker << std::endl;
    // In ComShm mode, do not store addr/port/interface for products that are not in our security def.
    if (mode_ == HFSAT::kComShm && !sec_name_indexer_.HasSecNamePrefix(std::string(ticker))) {
      memset(line, 0, sizeof(line));
      continue;
    }

    if (ticker && (mode_ != HFSAT::kRaw || sec_name_indexer_.HasShortCodePrefix(ticker))) {
      stype = strtok(NULL, "\n\t ");
      std::cout << "Stype: " << stype << std::endl;
      if (stype[0] == '0')  // Incremental channels
      {
        char* str_service = strtok(NULL, "\n\t ");
        std::cout << "str_service: " << str_service  << " Primary: " << isPrimary << std::endl;
        // listen to only stream A at non primary locations
        if (isPrimary || str_service[0] == 'B') {
          std::string ip_ = strtok(NULL, "\n\t ");
          int port_ = atoi(strtok(NULL, "\n\t "));
          std::cout << "ip:port :: " << ip_ << " : " << port_ << std::endl;
          //int mkt_seg_id_ = atoi(strtok(NULL, "\n\t "));

          //application_seq_num_[mkt_seg_id_] = 0;

          // remove duplicates if 2 products have same ip:port
          bool duplicate = false;
          for (int i = 0; i < (int)addr.size() && !duplicate; ++i) {
            if (addr[i] == ip_ && ports[i] == port_) {
              duplicate = true;
            }
          }

   	  std::cout << "duplicate: " << duplicate << std::endl;
          if (!duplicate) {
	    std::cout << "not duplicate: " << duplicate << std::endl;
            if (str_service[0] == 'A') {
	      std::cout << "str_serviceA: " << str_service << std::endl;
              addr.push_back(ip_);
              ports.push_back(port_);
              ifaces.push_back(interface_a_);
              std::cout << ip_ << " " << port_ << " " << interface_a_ << std::endl;
            } else if (str_service[0] == 'B') {
	      std::cout << "str_serviceB: " << str_service << std::endl;
              addr.push_back(ip_);
              ports.push_back(port_);
              ifaces.push_back(interface_b_);
              std::cout << ip_ << " " << port_ << " " << interface_b_ << std::endl;
            } else {  // Unexpected
	      std::cout << "Stream Type Unexpected : str_service: " << str_service << std::endl;
              std::cerr << " Stream Type Unexpected : " << str_service[0] << "\n";
              ifaces.push_back(interface_b_);
            }
          }
        }
      } else if (stype[0] == '1')  // Snapshot channels
      {
	std::cout << "Snapshot channels: " << stype << std::endl;
        char* str_service = strtok(NULL, "\n\t ");

        /// only use one channel for snapshot, irrespective of primary-location or not
        if (str_service[0] == 'B') {
          prod2snapaddr_[ticker] = strtok(NULL, "\n\t ");
          prod2snapport_[ticker] = atoi(strtok(NULL, "\n\t "));
        }
      }
    }
    memset(line, 0, sizeof(line));
  }

  file.close();
  
}

void BSERawMDHandler::createSockets(std::vector<std::string>& addresses, std::vector<int>& ports,
                                             std::vector<std::string>& ifaces) {
  size_t num_sockets = addresses.size();
  HFSAT::MulticastReceiverSocket* sock = NULL;

  std::cout << "num_sockets: " << num_sockets << std::endl;
  for (size_t i = 0; i < num_sockets; i++) {
    std::cout << "ports: " << ports[i] << std::endl;
    if (port2sock_.find(ports[i]) == port2sock_.end()) {
      std::cout << "port not present " << ports[i] << std::endl;
      sock = new HFSAT::MulticastReceiverSocket(addresses[i], ports[i], ifaces[i]);
      sock->setBufferSize(4000000);
      sock->SetNonBlocking();
      sockets.push_back(sock);

      // Priority to Local Sockets
      bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourceBSE) ==
                       TradingLocationUtils::GetTradingLocationFromHostname();
      std::cout << "isPrimary: " << isPrimary << std::endl;
      simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock->socket_file_descriptor(), isPrimary);
      fd2sock_[sock->socket_file_descriptor()] = sock;
      port2sock_[ports[i]] = sock;
    } else {
      std::cout << "port present " << ports[i] << std::endl;
      sock = port2sock_[ports[i]];
      sock->McastJoin(addresses[i], ifaces[i]);
    }
  }
}

void BSERawMDHandler::startRecovery(const char* p_prod_) {
  // In ComShm mode, do not start recovery for products that are not in our security def.
  if (mode_ == HFSAT::kComShm && !sec_name_indexer_.HasSecNamePrefix(std::string(p_prod_))) {
    return;
  }

  char basename[5] = {'\0'};
  strncpy(basename, p_prod_, 4);
  int recovery_port = prod2snapport_[basename];

  // Check if the recovery_port is free to start the recovery process
  if (recovery_port_ctr_.find(recovery_port) == recovery_port_ctr_.end() || recovery_port_ctr_[recovery_port] == 0) {
    HFSAT::MulticastReceiverSocket* sock =
        new HFSAT::MulticastReceiverSocket(prod2snapaddr_[basename], recovery_port, interface_b_);
    sock->setBufferSize(4000000);
    sock->SetNonBlocking();

    recovery_sockets_.push_back(sock);
    fd2sock_[sock->socket_file_descriptor()] = sock;

    simple_live_dispatcher->AddSimpleExternalDataLiveListenerSocket(this, sock->socket_file_descriptor());

    recovery_port_ctr_[recovery_port] = 1;
    recovery_mcast_ctr_[prod2snapaddr_[basename]] = 1;
    port2rsock_[recovery_port] = sock;
  } else {
    // Update the recovery count
    recovery_port_ctr_[recovery_port] += 1;

    if (recovery_mcast_ctr_.find(prod2snapaddr_[basename]) == recovery_mcast_ctr_.end() ||
        recovery_mcast_ctr_[prod2snapaddr_[basename]] == 0) {
      HFSAT::MulticastReceiverSocket* sock = port2rsock_[recovery_port];
      assert(sock != NULL);

      sock->McastJoin(prod2snapaddr_[basename], interface_b_);
      recovery_mcast_ctr_[prod2snapaddr_[basename]] = 1;
    } else {
      recovery_mcast_ctr_[prod2snapaddr_[basename]] += 1;
    }
  }
}

void BSERawMDHandler::endRecovery(const char* p_prod_) {
  char basename[5] = {'\0'};
  strncpy(basename, p_prod_, 4);
  int recovery_port = prod2snapport_[basename];

  if (recovery_mcast_ctr_[prod2snapaddr_[basename]] == 1) {
    if (port2rsock_.find(recovery_port) == port2rsock_.end()) {
      return;
    }
    if (!(port2rsock_.find(recovery_port)->second)) {
      return;
    }

    port2rsock_[recovery_port]->McastDrop(prod2snapaddr_[basename]);
  }

  if (recovery_port_ctr_[recovery_port] == 1) {
    std::vector<HFSAT::MulticastReceiverSocket*>::iterator sockiter =
        std::find(recovery_sockets_.begin(), recovery_sockets_.end(), port2rsock_[recovery_port]);
    fd2sock_.erase((*sockiter)->socket_file_descriptor());
    simple_live_dispatcher->RemoveSimpleExternalDataLiveListenerSocket(this, (*sockiter)->socket_file_descriptor());
    recovery_sockets_.erase(sockiter);

    delete (port2rsock_[recovery_port]);
    port2rsock_.erase(recovery_port);
  }

  recovery_port_ctr_[recovery_port] -= 1;
  recovery_mcast_ctr_[prod2snapaddr_[basename]] -= 1;
}

void BSERawMDHandler::endRecoveryForAllProducts() {
  for (std::map<std::string, int>::iterator _itr_ = prod2snapport_.begin(); _itr_ != prod2snapport_.end(); _itr_++) {
    std::vector<HFSAT::MulticastReceiverSocket*>::iterator sock_itr_ =
        std::find(recovery_sockets_.begin(), recovery_sockets_.end(), port2rsock_[_itr_->second]);
    if (sock_itr_ != recovery_sockets_.end()) {
      fd2sock_.erase((*sock_itr_)->socket_file_descriptor());
      simple_live_dispatcher->RemoveSimpleExternalDataLiveListenerSocket(this, (*sock_itr_)->socket_file_descriptor());
      recovery_sockets_.erase(sock_itr_);

      delete (port2rsock_[_itr_->second]);
      port2rsock_.erase(_itr_->second);
    }
  }
}

void BSERawMDHandler::startRecoveryForAllProducts() {}

// sock_handler default true to broadcast & false to use rawhandler
void BSERawMDHandler::start(bool enable_recovery) {
  /// actual multicast data handling ..
  std::vector<std::string> maddresses;
  std::vector<int> mports;

  // Each Socket to join on specific interface
  std::vector<std::string> minterfaces;

  maddresses.clear();
  mports.clear();
  minterfaces.clear();

  getChannelInfo(maddresses, mports, minterfaces);
  /// init sockets
  createSockets(maddresses, mports, minterfaces);

#ifdef ENABLE_RECOVERY_IN_PRIMARY_LOCATION
  bool isPrimary = TradingLocationUtils::GetTradingLocationExch(kExchSourceBSE) ==
                   TradingLocationUtils::GetTradingLocationFromHostname();
  if (isPrimary) enable_recovery = true;  // always recover at primary locations
  std::cout << "isPrimary, enable_recovery :: " << isPrimary << " , " << enable_recovery << std::endl;
#endif
  switch (mode_) {
    case kMcast:
      if (mcast_sender_socket_ == NULL) {
        fprintf(stderr, "mcast sock is null... exiting \n");
        exit(-1);
      }
      BSE_MD_Processor::BSEMDProcessor::SetInstance(mcast_sender_socket_, this, mode_, enable_recovery, full_mode_);
      break;
    case kProShm:
      BSE_MD_Processor::BSEMDProcessor::SetInstance(NULL, this, mode_, enable_recovery, full_mode_);
      break;
    case kComShm:
      BSE_MD_Processor::BSEMDProcessor::SetInstance(NULL, this, mode_, enable_recovery, full_mode_);
      break;
    case kLogger:
      BSE_MD_Processor::BSEMDProcessor::SetInstance(NULL, this, mode_, enable_recovery, full_mode_);
      break;
    case kPriceFeedLogger:
      BSE_MD_Processor::BSEMDProcessor::SetInstance(NULL, this, mode_, enable_recovery, full_mode_);
      break;
    default:
      fprintf(stderr, "not implemented, wrong mode\n");
      exit(-1);
  }
}

inline void BSERawMDHandler::ProcessAllEvents(int soc_fd_) {
#if IS_LOGGING_ENABLE
  std::cout << "BSERawMDHandler::ProcessAllEvents: " << soc_fd_ << std::endl;
#endif
  if (fd2sock_.find(soc_fd_) == fd2sock_.end()){
#if IS_LOGGING_ENABLE
    std::cout << "soc_fd: " << soc_fd_ << " not present in fd2sock_" << std::endl;
#endif
    return;
  }
  HFSAT::MulticastReceiverSocket* socket_to_process = fd2sock_[soc_fd_];

  uint32_t msg_len = socket_to_process->ReadN(MAX_UDP_MSG_LEN, (void*)msg_buf);
  if (is_local_data_) {
  }
  assert(msg_len > 0);
    // Discard emdi messages. Compare the template id of eobi packet header to achieve that
    // Unique identifier for a T7 6.0 EOBI message layout.
    // Value: 13002 (MarketDataReport, MsgType = U20)
    uint16_t template_id_ = *(uint16_t*)(msg_buf + 2);
    if (template_id_ != 13002) {
      return;
    }

#if IS_LOGGING_ENABLE
    std::cout << "BSERawMDHandler::ProcessAllEvents: template_id_: " << template_id_ << std::endl;
#endif
    int32_t mkt_seg_id_ = *(int32_t*)(msg_buf + 12);
#if IS_LOGGING_ENABLE
    std::cout << "mkt_seg_id_: " << mkt_seg_id_ << std::endl;
#endif
    if (application_seq_num_.find(mkt_seg_id_) == application_seq_num_.end()) {
      application_seq_num_[mkt_seg_id_] = 0;
      //return;
    }
    int port_ = socket_to_process->GetPort();
#if IS_LOGGING_ENABLE
    std::cout << "port: " << port_ << std::endl;
#endif
    // Snapshot channel ports are even numbers
    // Since we listen on only B stream for snapshot messages, we won't check for application sequence number here
    if (port_ % 2 == 0) {
      eobi_decoder_.Decode(msg_buf, msg_len);
      return;
    }
    uint32_t appl_seq_num_ = *(uint32_t*)(msg_buf + 8);
    // Skip the packet if application sequence number is less than the latest processed packet for this key_
    uint32_t prev_appl_seq_num_ = application_seq_num_[mkt_seg_id_];
    if (prev_appl_seq_num_ >= appl_seq_num_) {
      return;
    }
    application_seq_num_[mkt_seg_id_] = appl_seq_num_;
    eobi_decoder_.Decode(msg_buf, msg_len);
}
}
