// =====================================================================================
//
//       Filename:  generic_live_source_shm_data_logger.cpp
//
//    Description:  Only Meant For EUREX LIVE SHM SOURCE With LOW BW Structs
//
//        Version:  1.0
//        Created:  11/08/2012 08:25:34 AM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <map>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/load_low_bandwidth_code_mapping.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CommonDataStructures/enhanced_security_name_indexer.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"
#include "dvccode/Utils/combined_shm_reader.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

#define FILTERED_SECURITIES_FOR_MULTICAST_FILE "/home/pengine/prod/live_configs/filtered_security_for_multicast.txt"

void print_usage(const char *prg_name) {
  printf(" This is the Combined SHM Multicaster Executable \n");
  printf(" Usage:%s --exchange <exch> [--ip IP --port PORT --iface INTERFACE --products_filter FILTER_STR ] \n",
         prg_name);
  printf(
      " --exchange Exchange Name (accepted values CFE, MICEX_RTS, CME, EUREX_ICE, TMX_OBF_LS, CME_LS_NSE, CME_LS_ASX, "
      "EOBI_LS_NSE,"
      "EOBI_LS_ASX, "
      "HKEX_LS_ASX, OSE_LS_FILTERED, NTP_LS_CME ) \n");

  printf(" Alternate Usage:%s --config <config_file> \n", prg_name);
}

static struct option data_options[] = {
    {"help", no_argument, 0, 'h'},         {"exchange", required_argument, 0, 'a'},
    {"ip", required_argument, 0, 'b'},     {"port", required_argument, 0, 'c'},
    {"iface", required_argument, 0, 'd'},  {"products_filter", required_argument, 0, 'e'},
    {"config", required_argument, 0, 'f'}, {0, 0, 0, 0}};
namespace HFSAT {
class CombinedShmMulticaster : public CombinedShmReaderListener {
 protected:
  std::string exch_;
  HFSAT::MulticastSenderSocket *sender_socket_;
  HFSAT::MulticastSenderSocket *sender_socket_1_;

  std::map<std::string, int> eobi_ls_shc_to_code_map_;
  std::map<std::string, int> cme_ls_shc_to_code_map_;

  OSE_MDS::OSEPLCommonStruct ose_l1_event_;

  HFSAT::EnhancedSecurityNameIndexer sec_name_indexer_;

  // this, if provided, should match the 2nd coulumn of FILTERED_SECURITIES_FOR_ASX_FILE. Only those products data
  // will
  // be multicasted
  std::string products_filter_str_;
  unsigned int send_sequence_;
  HFSAT::ShortcodeRequestHelper shc_request_helper;
  std::vector<std::string> shc_list;

 public:
  CombinedShmMulticaster(std::string exch, std::string ip, int port, std::string iface, std::string products_filter_str)
      : exch_(exch),
        eobi_ls_shc_to_code_map_(),
        cme_ls_shc_to_code_map_(),
        products_filter_str_(products_filter_str),
        send_sequence_(1),
        shc_request_helper(-500),
        shc_list() {
    HFSAT::NetworkAccountInfoManager network_account_info_manager_;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    LoadLiveSourceProductCodes(eobi_ls_shc_to_code_map_, DEF_LS_PRODUCTCODE_SHORTCODE_);
    LoadLiveSourceProductCodes(cme_ls_shc_to_code_map_, DEF_CME_LS_PRODUCTCODE_SHORTCODE_);

    bool should_load = false;

    if (exch_ == "CFE") {
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceCFE, "VX_0");

      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCFE, HFSAT::k_MktDataMcast));
    } else if (exch_ == "CME") {
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceCME, "ZF_0");

      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));
    } else if (exch_ == "EUREX_ICE")  //  NY14->TOR12 for FGBL and LFR
    {
      std::string shortcode = "LFR_0";
      shc_list.push_back(shortcode);
      const char *exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
      sec_name_indexer_.AddString(exch_symbol, shortcode);

      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceEUREX, "FGBL_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          "239.23.0.34", 31712,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceICE, HFSAT::k_MktDataLive));

      data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceICE, "LFR_0");
      sender_socket_1_ = new HFSAT::MulticastSenderSocket(
          "239.23.0.33", 31913,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceICE, HFSAT::k_MktDataLive));

    } else if ("TMX_OBF_LS" == exch_) {
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceTMX, "CGB_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceTMX, HFSAT::k_MktDataLive));

    } else if ("CME_LS_ASX" == exch_ || "CME_LS_MOS" == exch_ || "CME_LS_HK" == exch_ || "CME_LS_FILTER1" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(exch_);
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));

      std::cout << exch_ << ". IP: " << data_info_mkt_data.bcast_ip_ << " Port: " << data_info_mkt_data.bcast_port_
                << "\n";

    } else if ("CME_LS_NSE" == exch_) {
      should_load = true;
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          "239.23.0.46", 50999,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));

    } else if ("EOBI_LS_ASX" == exch_) {
      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceEOBI, "FGBM_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_ + 2,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEOBI, HFSAT::k_MktDataLive));

    } else if ("EOBI_LS_NSE" == exch_) {
      should_load = true;
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          "239.23.0.78", 51999,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEOBI, HFSAT::k_MktDataLive));

    } else if ("SGX_LS_NSE" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceSGX, "SGX_NK_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceSGX, HFSAT::k_MktDataLive));
    } else if ("OSE_LS_FILTERED" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceJPY, "JGBL_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceJPY, HFSAT::k_MktDataLive));
    } else if ("HKEX_LS_ASX" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceHKOMDCPF, "HHI_0");
      sender_socket_ =
          new HFSAT::MulticastSenderSocket(data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_ + 13,
                                           HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                               HFSAT::kExchSourceHKOMD, HFSAT::k_MktDataLive));
      std::cout << "IP: " << data_info_mkt_data.bcast_ip_ << " Port: " << data_info_mkt_data.bcast_port_ + 13 << "\n";

    } else if ("HKEX_LS_FILTER1" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data =
          network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceHKOMDCPF, "HHI_0");
      sender_socket_ = new HFSAT::MulticastSenderSocket(data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
                                                        HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                                            HFSAT::kExchSourceHKOMD, HFSAT::k_MktDataLive));
      std::cout << "HKEX_LS_FILTER1. IP: " << data_info_mkt_data.bcast_ip_
                << " Port: " << data_info_mkt_data.bcast_port_ << "\n";

    } else if ("NTP_LS_CME" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceNTP);
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataLive));

      std::cout << "IP: " << data_info_mkt_data.bcast_ip_ << " Port: " << data_info_mkt_data.bcast_port_ << std::endl;
    } else if ("CME_LS_NTP" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(exch_);
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCME, HFSAT::k_MktDataMcast));

      std::cout << "IP: " << data_info_mkt_data.bcast_ip_ << " Port: " << data_info_mkt_data.bcast_port_ << "\n";
    } else if ("EOBI_LS_NTP" == exch_ || "EOBI_LS_FILTER1" == exch_ || "EOBI_LS_FILTER2" == exch_) {
      should_load = true;
      HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(exch_);
      sender_socket_ = new HFSAT::MulticastSenderSocket(
          data_info_mkt_data.bcast_ip_, data_info_mkt_data.bcast_port_,
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEOBI, HFSAT::k_MktDataLive));

      std::cout << "EUREX filter IP: " << data_info_mkt_data.bcast_ip_ << " Port: " << data_info_mkt_data.bcast_port_
                << "\n";
    } else {
      std::cerr << " Not Implemented For This Exchange : " << exch_ << " Exiting \n";
      exit(-1);
    }

    if (should_load) {
      LoadFilteredSecurities();
    }

    shc_request_helper.AddShortcodeListToListen(shc_list);
  }

  ~CombinedShmMulticaster() { shc_request_helper.RemoveAllShortcodesToListen(); }

  void LoadLiveSourceProductCodes(std::map<std::string, int> &shc_to_code_map, const char *file_name) {
    std::ifstream shc_code_file;
    shc_code_file.open(file_name, std::ifstream::in);

    if (!shc_code_file.is_open()) {
      std::cerr << "Could not open " << file_name << std::endl;
      exit(-1);
    }
    if (shc_code_file.is_open()) {
      char line[1024];
      while (!shc_code_file.eof()) {
        bzero(line, 1024);
        shc_code_file.getline(line, 1024);
        if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
        HFSAT::PerishableStringTokenizer st_(line, 1024);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 2) {
          std::cerr << "Malformatted line in " << file_name << std::endl;
          exit(-1);
        }

        std::string shc = tokens_[1];
        int code = atoi(tokens_[0]);

        std::cout << "SHC: " << shc << " Code: " << code << "\n";

        shc_to_code_map[shc] = code;
      }
      shc_code_file.close();
    }
  }

  void LoadFilteredSecurities() {
    std::ifstream needed_products_file_;
    needed_products_file_.open(FILTERED_SECURITIES_FOR_MULTICAST_FILE, std::ifstream::in);

    if (!needed_products_file_.is_open()) {
      std::cerr << "Could not open " << FILTERED_SECURITIES_FOR_MULTICAST_FILE << std::endl;
      exit(-1);
    }
    if (needed_products_file_.is_open()) {
      char line[1024];
      while (!needed_products_file_.eof()) {
        bzero(line, 1024);
        needed_products_file_.getline(line, 1024);
        if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
        HFSAT::PerishableStringTokenizer st_(line, 1024);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 2) {
          std::cerr << "Malformatted line in " << FILTERED_SECURITIES_FOR_MULTICAST_FILE << std::endl;
          exit(-1);
        }

        std::string shortcode = tokens_[0];
        std::string exch = tokens_[1];
        if (products_filter_str_ != "") {
          if (products_filter_str_ == exch) {
            const char *exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
            sec_name_indexer_.AddString(exch_symbol, shortcode);
            shc_list.push_back(shortcode);
            std::cout << "Multicasting data for exch :  " << exch << " : " << shortcode << std::endl;
          }
        } else {
          const char *exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
          sec_name_indexer_.AddString(exch_symbol, shortcode);
          shc_list.push_back(shortcode);
          std::cout << "Multicasting data for " << shortcode << std::endl;
        }
      }
      needed_products_file_.close();
    }
  }

  bool ConvertToCMELSStruct(HFSAT::MDS_MSG::GenericMDSMessage &cstr_, int security_id) {
    const char *shc_str = sec_name_indexer_.GetShortcodeFromId(security_id);
    if (security_id < 0 || shc_str == nullptr) return false;
    std::string shc(shc_str);
    if (cme_ls_shc_to_code_map_.find(shc) == cme_ls_shc_to_code_map_.end()) {
      std::cerr << "SHC: " << shc << " not found in CME livesource -> contract code map\n";
      return false;
    }

    int contract_code = cme_ls_shc_to_code_map_[shc];
    CME_MDS::CMECommonStruct &cme_common_struct = cstr_.generic_data_.cme_data_;
    CME_MDS::CMELSCommonStruct &cme_ls_common_struct = cstr_.generic_data_.cme_ls_data_;

    if (cstr_.generic_data_.cme_data_.msg_ == CME_MDS::CME_DELTA) {
      uint32_t level = cme_common_struct.data_.cme_dels_.level_;
      int32_t size = cme_common_struct.data_.cme_dels_.size_;
      uint32_t orders = cme_common_struct.data_.cme_dels_.num_ords_;
      double price = cme_common_struct.data_.cme_dels_.price_;
      char type = cme_common_struct.data_.cme_dels_.type_;
      char action = cme_common_struct.data_.cme_dels_.action_;
      bool intermediate = cme_common_struct.data_.cme_dels_.intermediate_;

      cme_ls_common_struct.msg_ = CME_MDS::CME_DELTA;
      cme_ls_common_struct.data_.cme_dels_.price_ = price;
      cme_ls_common_struct.data_.cme_dels_.size_ = size;
      cme_ls_common_struct.data_.cme_dels_.num_ords_ = orders;
      cme_ls_common_struct.data_.cme_dels_.contract_code_ = (uint8_t)contract_code;
      cme_ls_common_struct.data_.cme_dels_.level_ = level;
      cme_ls_common_struct.data_.cme_dels_.type_ = type;
      cme_ls_common_struct.data_.cme_dels_.action_ = action;
      cme_ls_common_struct.data_.cme_dels_.intermediate_ = intermediate;

    } else if (cstr_.generic_data_.cme_data_.msg_ == CME_MDS::CME_TRADE) {
      double price = cme_common_struct.data_.cme_trds_.trd_px_;
      uint32_t size = cme_common_struct.data_.cme_trds_.trd_qty_;
      uint32_t agg_side = cme_common_struct.data_.cme_trds_.agg_side_;

      cme_ls_common_struct.msg_ = CME_MDS::CME_TRADE;
      cme_ls_common_struct.data_.cme_trds_.trd_px_ = price;
      cme_ls_common_struct.data_.cme_trds_.trd_qty_ = size;
      cme_ls_common_struct.data_.cme_trds_.contract_code_ = contract_code;
      cme_ls_common_struct.data_.cme_trds_.agg_side_ = agg_side;
    }

    cstr_.mds_msg_exch_ = HFSAT::MDS_MSG::CME_LS;

    return true;
  }

  bool ConvertToEOBILSStruct(HFSAT::MDS_MSG::GenericMDSMessage &cstr_, int security_id) {
    const char *shc_str = sec_name_indexer_.GetShortcodeFromId(security_id);
    if (security_id < 0 || shc_str == nullptr) return false;

    std::string shc(shc_str);
    if (eobi_ls_shc_to_code_map_.find(shc) == eobi_ls_shc_to_code_map_.end()) {
      std::cerr << "SHC: " << shc << " not found in EOBI livesource -> contract code map\n";
      return false;
    }

    int contract_code = eobi_ls_shc_to_code_map_[shc];
    EUREX_MDS::EUREXCommonStruct &eurex_common_struct = cstr_.generic_data_.eobi_pf_data_;
    EUREX_MDS::EUREXLSCommonStruct &eurex_ls_common_struct = cstr_.generic_data_.eurex_ls_data_;

    if (cstr_.generic_data_.eobi_pf_data_.msg_ == EUREX_MDS::EUREX_DELTA) {
      uint32_t level = eurex_common_struct.data_.eurex_dels_.level_;
      int32_t size = eurex_common_struct.data_.eurex_dels_.size_;
      uint32_t orders = eurex_common_struct.data_.eurex_dels_.num_ords_;
      double price = eurex_common_struct.data_.eurex_dels_.price_;
      char type = eurex_common_struct.data_.eurex_dels_.type_;
      char action = eurex_common_struct.data_.eurex_dels_.action_;
      bool intermediate = eurex_common_struct.data_.eurex_dels_.intermediate_;

      eurex_ls_common_struct.msg_ = EUREX_MDS::EUREX_DELTA;
      eurex_ls_common_struct.data_.eurex_dels_.price_ = price;
      eurex_ls_common_struct.data_.eurex_dels_.size_ = size;
      eurex_ls_common_struct.data_.eurex_dels_.num_ords_ = orders;
      eurex_ls_common_struct.data_.eurex_dels_.contract_code_ = (uint8_t)contract_code;
      eurex_ls_common_struct.data_.eurex_dels_.level_ = level;
      eurex_ls_common_struct.data_.eurex_dels_.type_ = type;
      eurex_ls_common_struct.data_.eurex_dels_.action_ = action;
      eurex_ls_common_struct.data_.eurex_dels_.intermediate_ = intermediate;

    } else if (cstr_.generic_data_.eobi_pf_data_.msg_ == EUREX_MDS::EUREX_TRADE) {
      double price = eurex_common_struct.data_.eurex_trds_.trd_px_;
      uint32_t size = eurex_common_struct.data_.eurex_trds_.trd_qty_;
      uint32_t agg_side = eurex_common_struct.data_.eurex_trds_.agg_side_;

      eurex_ls_common_struct.msg_ = EUREX_MDS::EUREX_TRADE;
      eurex_ls_common_struct.data_.eurex_trds_.trd_px_ = price;
      eurex_ls_common_struct.data_.eurex_trds_.trd_qty_ = size;
      eurex_ls_common_struct.data_.eurex_trds_.contract_code_ = contract_code;
      eurex_ls_common_struct.data_.eurex_trds_.agg_side_ = agg_side;
    }

    eurex_ls_common_struct.msg_sequence_ = send_sequence_;
    send_sequence_++;

    cstr_.mds_msg_exch_ = HFSAT::MDS_MSG::EUREX_LS;

    return true;
  }

  void OnShmRead(HFSAT::MDS_MSG::GenericMDSMessage cstr_) {
    // Lot of easy optimizations could be done here. We are doing string matching every time for exchange. Instead
    // we can use function pointers, assign them in constructors.

    if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::CSM && exch_ == "CFE") {
      sender_socket_->WriteN(sizeof(CSM_MDS::CSMCommonStruct), &(cstr_.generic_data_.csm_data_));

    } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::CME && exch_ == "CME") {
      sender_socket_->WriteN(sizeof(CME_MDS::CMECommonStruct), &(cstr_.generic_data_.cme_data_));

    } else if ((cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::EOBI_LS || cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::ICE) &&
               exch_ == "EUREX_ICE") {
      if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::EOBI_LS) {
        if (cstr_.generic_data_.eurex_ls_data_.getContractCode() == 2)  // FGBL
        {
          sender_socket_->WriteN(sizeof(EUREX_MDS::EUREXLSCommonStruct), &(cstr_.generic_data_.eurex_ls_data_));
        }
      }

      if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::ICE) {
        int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.ice_data_.getContract());
        if (security_id >= 0) {
          sender_socket_1_->WriteN(sizeof(ICE_MDS::ICECommonStructLive), &(cstr_.generic_data_.ice_data_));
        }
      }
    } else if (exch_ == "TMX_OBF_LS" && HFSAT::MDS_MSG::TMX_OBF == cstr_.mds_msg_exch_) {
      sender_socket_->WriteN(sizeof(TMX_OBF_MDS::TMXPFCommonStruct), &(cstr_.generic_data_.tmx_obf_data_));
    } else if ((exch_ == "CME_LS_ASX" || "CME_LS_MOS" == exch_ || "CME_LS_HK" == exch_ || "CME_LS_FILTER1" == exch_) &&
               HFSAT::MDS_MSG::CME == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.cme_data_.getContract());
      // Will only send the data for products which we have added in sec_name_indexer ( read from a file )
      if (security_id >= 0 && ConvertToCMELSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(CME_MDS::CMELSCommonStruct), &(cstr_.generic_data_.cme_ls_data_));
      }
    } else if (exch_ == "CME_LS_NSE" && HFSAT::MDS_MSG::CME == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.cme_data_.getContract());
      // Will only send the data for products which we have added in sec_name_indexer ( read from a file )
      if (security_id >= 0 && ConvertToCMELSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(CME_MDS::CMELSCommonStruct), &(cstr_.generic_data_.cme_ls_data_));
      }
    } else if (exch_ == "EOBI_LS_ASX" && HFSAT::MDS_MSG::EOBI_PF == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.eobi_pf_data_.getContract());
      if (security_id >= 0 && ConvertToEOBILSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(EUREX_MDS::EUREXLSCommonStruct), &(cstr_.generic_data_.eurex_ls_data_));
      }
    } else if (exch_ == "EOBI_LS_NSE" && HFSAT::MDS_MSG::EOBI_PF == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.eobi_pf_data_.getContract());
      if (security_id >= 0 && ConvertToEOBILSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(EUREX_MDS::EUREXLSCommonStruct), &(cstr_.generic_data_.eurex_ls_data_));
      }
    } else if (exch_ == "SGX_LS_NSE" && HFSAT::MDS_MSG::SGX == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.sgx_data_.getContract());
      if (security_id >= 0) {
        sender_socket_->WriteN(sizeof(SGX_MDS::SGXPFCommonStruct), &(cstr_.generic_data_.sgx_data_));
      }
    } else if (exch_ == "OSE_LS_FILTERED" && HFSAT::MDS_MSG::OSE_ITCH_PF == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.ose_itch_pf_data_.getContract());

      if (security_id >= 0) {
        sender_socket_->WriteN(sizeof(OSE_ITCH_MDS::OSEPFCommonStruct), &(cstr_.generic_data_.ose_itch_pf_data_));
      }
    } else if ((exch_ == "HKEX_LS_ASX" || exch_ == "HKEX_LS_FILTER1") &&
               HFSAT::MDS_MSG::HKOMDPF == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.hkomd_pf_data_.getContract());
      if (security_id >= 0) {
        sender_socket_->WriteN(sizeof(HKOMD_MDS::HKOMDPFCommonStruct), &(cstr_.generic_data_.hkomd_pf_data_));
      }
    } else if (exch_ == "NTP_LS_CME" && HFSAT::MDS_MSG::NTP == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.ntp_data_.getContract());
      if (security_id >= 0) {
        sender_socket_->WriteN(sizeof(NTP_MDS::NTPCommonStruct), &(cstr_.generic_data_.ntp_data_));
      }
    } else if (exch_ == "CME_LS_NTP" && HFSAT::MDS_MSG::CME == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.cme_data_.getContract());
      // Will only send the data for products which we have added in sec_name_indexer ( read from a file )
      if (security_id >= 0 && ConvertToCMELSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(CME_MDS::CMELSCommonStruct), &(cstr_.generic_data_.cme_ls_data_));
      }
    } else if ((exch_ == "EOBI_LS_NTP" || "EOBI_LS_FILTER1" == exch_ || "EOBI_LS_FILTER2" == exch_) &&
               HFSAT::MDS_MSG::EOBI_PF == cstr_.mds_msg_exch_) {
      int security_id = sec_name_indexer_.GetIdFromSecname(cstr_.generic_data_.eobi_pf_data_.getContract());
      if (security_id >= 0 && ConvertToEOBILSStruct(cstr_, security_id)) {
        sender_socket_->WriteN(sizeof(EUREX_MDS::EUREXLSCommonStruct), &(cstr_.generic_data_.eurex_ls_data_));
      }
    }
  }
};
}

void ParseConfigFile(std::string config_file,
                     std::vector<std::pair<std::string, std::string>> &exch_product_filter_vec_) {
  exch_product_filter_vec_.clear();

  std::ifstream config_stream_;
  config_stream_.open(config_file.c_str(), std::ifstream::in);

  if (config_stream_.is_open()) {
    const int config_len_ = 1024;
    char readline_buffer_[config_len_];
    bzero(readline_buffer_, config_len_);

    while (config_stream_.good()) {
      bzero(readline_buffer_, config_len_);
      config_stream_.getline(readline_buffer_, config_len_);

      // Check for commented configs
      int iter = 0;
      while (readline_buffer_[iter] == ' ') {
        iter++;
      }
      if (readline_buffer_[iter] == '#') {
        continue;
      }

      HFSAT::PerishableStringTokenizer st_(readline_buffer_, config_len_);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.empty()) {
        continue;
      } else if (tokens_.size() >= 2) {
        std::string exch(tokens_[0]);
        std::string prod_filter(tokens_[1]);
        exch_product_filter_vec_.push_back(std::make_pair(exch, prod_filter));
      } else {
        std::string exch(tokens_[0]);
        exch_product_filter_vec_.push_back(std::make_pair(exch, ""));
      }
    }
    config_stream_.close();
  } else {
    std::cerr << "ConfigFile: " << config_file << std::endl;
    std::cerr << "Exiting." << std::endl;
    exit(-1);
  }
  return;
}

/// signal handling
void sighandler(int signum) {
  std::cerr << " Received Termination Signal \n";
  exit(0);
}

int main(int argc, char **argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  int c;
  int hflag = 0;
  /// input arguments
  std::string exch_;
  std::string ip_ = "";
  std::string iface_ = "";
  std::string products_filter = "";
  int port_ = -1;

  std::vector<std::string> exch_source_vec_;

  std::string config_file_ = "";
  std::vector<std::pair<std::string, std::string>> exch_product_filter_vec_;

  std::cout << " going For arg \n";

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'h':
        hflag = 1;
        break;
      case 'a':
        exch_ = optarg;
        exch_source_vec_.push_back(exch_);
        break;
      case 'b':
        ip_ = optarg;
        break;
      case 'c':
        port_ = atoi(optarg);
        break;
      case 'd':
        iface_ = optarg;
        break;
      case 'e':
        products_filter = optarg;
        break;
      case 'f':
        config_file_ = optarg;
        break;
      case '?':
        if (optopt == 'a') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  std::cout << " Arg Taken \n ";

  if (hflag || (exch_source_vec_.size() == 0 && config_file_ == "")) {
    print_usage(argv[0]);
    exit(-1);
  }

  exch_product_filter_vec_.push_back(std::make_pair(exch_, products_filter));

  if (config_file_ != "") {
    std::cout << "Parsing config file: " << config_file_ << std::endl;
    ParseConfigFile(config_file_, exch_product_filter_vec_);
  }

  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("CombinedShmMulticaster");

  for (auto exch_prod_filter : exch_product_filter_vec_) {
    std::cout << "Config EXCH : " << exch_prod_filter.first << " PROD_FILTER_STR : " << exch_prod_filter.second
              << std::endl;
    HFSAT::CombinedShmReader::GetUniqueInstance().AddListener(
        new HFSAT::CombinedShmMulticaster(exch_prod_filter.first, ip_, port_, iface_, exch_prod_filter.second));
  }

  HFSAT::CombinedShmReader::GetUniqueInstance().StartReading();

  return 0;
}
