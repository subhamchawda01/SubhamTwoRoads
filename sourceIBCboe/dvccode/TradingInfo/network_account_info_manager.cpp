/**
   \file dvccode/TradingInfo/network_account_info_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

namespace HFSAT {

NetworkAccountInfoManager::NetworkAccountInfoManager()
    : network_account_info_filename_(""),
      def_data_info_(),
      def_trade_info_(),
      exch_data_map_(),
      src_data_map_(),
      exch_trade_map_(),
      dep_trade_map_(),
      exch_mcontrol_map_(),
      param_send_data_info_("225.2.2.1", 12221),                     /// error value
      trade_control_recv_data_info_("225.2.2.1", 12122),             /// error value
      trade_control_recv_udp_direct_data_info_("225.2.2.1", 12122),  /// error value
      trade_signal_recv_udp_direct_data_info_("225.2.2.1", 12134),  /// error value
      combined_control_data_info_("225.2.2.1", 12123),               /// error value
      combined_control_udp_direct_data_info_("225.2.2.1", 12123),    /// error value
      retail_source_data_map_() {
  //=============================   Trying to make things robust against incorrect filesync etc
  //============================= //

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::ostringstream host_nw_account_info_config_file;
  if (strncmp(hostname, "ip-10-0-1", 9) == 0) {
    host_nw_account_info_config_file
        << getenv("HOME")
        << "/infracore_install/SysInfo/dvccode/TradingInfo/NetworkInfo/network_account_info_filename.txt";
  } else {
    host_nw_account_info_config_file << PROD_CONFIGS_DIR << hostname << "_" << DEF_NW_ACCOUNT_INFO_FILENAME;
  }
  network_account_info_filename_ = host_nw_account_info_config_file.str();

  //  std::cerr << " Loaded Network Account Info File : " << network_account_info_filename_ << "\n";

  //=========================================================================================================================
  ////

  LoadInfoFile();
}

const DataInfo& NetworkAccountInfoManager::GetSrcDataInfo(ExchSource_t exch_source_,
                                                          const std::string& src_shortcode_) {
  if (src_data_map_.find(src_shortcode_) == src_data_map_.end()) {
    if (exch_source_ == kExchSourceInvalid) {
      exch_source_ =
          SecurityDefinitions::GetContractExchSource(src_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal());
    }

    if (exch_data_map_.find(exch_source_) != exch_data_map_.end()) {
      return exch_data_map_[exch_source_];
    } else {
      std::cerr << " NetworkAccountInfoManager::GetSrcDataInfo ( " << ExchSourceStringForm(exch_source_) << ", "
                << src_shortcode_ << " ) Should not be here ! " << std::endl;
      ExitVerbose(kExitErrorCodeNetworkAccountInfoManager);
      return def_data_info_;
    }
  } else {
    return src_data_map_[src_shortcode_];
  }
}

const DataInfo& NetworkAccountInfoManager::GetSrcDataInfo(ExchSource_t exch_source_) {
  if (exch_data_map_.find(exch_source_) != exch_data_map_.end()) {
    return exch_data_map_[exch_source_];
  } else {
    std::cerr << " NetworkAccountInfoManager::GetSrcDataInfo ( " << ExchSourceStringForm(exch_source_)
              << " ) Should not be here ! " << std::endl;
    ExitVerbose(kExitErrorCodeNetworkAccountInfoManager);
    return def_data_info_;
  }
}

const DataInfo& NetworkAccountInfoManager::GetSrcDataInfo(std::string exch_source) {
  if (str_exch_data_map_.find(exch_source) != str_exch_data_map_.end()) {
    return str_exch_data_map_[exch_source];
  } else {
    std::cerr << " NetworkAccountInfoManager::GetSrcDataInfo ( " << exch_source << " ) Should not be here ! "
              << std::endl;
    ExitVerbose(kExitErrorCodeNetworkAccountInfoManager);
    return def_data_info_;
  }
}

const DataInfo& NetworkAccountInfoManager::GetRetailDataInfoFromSourceType(const std::string& _retail_source_type_) {
  if (retail_source_data_map_.find(_retail_source_type_) == retail_source_data_map_.end()) {
    std::cerr << " SHOULD NOT REACH HERE : "
              << " COULDN'T FIND RETAIL SOURCE DATA INFO : " << _retail_source_type_ << "\n";
    ExitVerbose(kExitErrorCodeNetworkAccountInfoManager);
    return def_data_info_;
  }

  return retail_source_data_map_[_retail_source_type_];
}

const std::string& NetworkAccountInfoManager::GetSrcDataIp(ExchSource_t exch_source_,
                                                           const std::string& src_shortcode_) {
  return GetSrcDataInfo(exch_source_, src_shortcode_).bcast_ip_;
}

const int NetworkAccountInfoManager::GetSrcDataPort(ExchSource_t exch_source_, const std::string& src_shortcode_) {
  return GetSrcDataInfo(exch_source_, src_shortcode_).bcast_port_;
}

const DataInfo NetworkAccountInfoManager::GetDepDataInfo(ExchSource_t exch_source_, const std::string& dep_shortcode_) {
  return (GetDepTradeInfo(exch_source_, dep_shortcode_)).GetBcastDataInfo();
}

const TradeInfo& NetworkAccountInfoManager::GetDepTradeInfo(ExchSource_t exch_source_,
                                                            const std::string& dep_shortcode_) {
  if (exch_source_ == kExchSourceMICEX_EQ || exch_source_ == kExchSourceMICEX_CR) {
    exch_source_ = kExchSourceMICEX;
  }
  if (dep_trade_map_.find(dep_shortcode_) == dep_trade_map_.end()) {
    if (exch_source_ == kExchSourceInvalid) {
      exch_source_ =
          SecurityDefinitions::GetContractExchSource(dep_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal());
    }
    if (exch_trade_map_.find(exch_source_) != exch_trade_map_.end()) {
      return exch_trade_map_[exch_source_];
    } else {
      std::cerr << " NetworkAccountInfoManager::GetDepTradeInfo ( " << exch_source_ << ", " << dep_shortcode_
                << " ) Should not be here ! " << std::endl;
      ExitVerbose(kExitErrorCodeNetworkAccountInfoManager); 
      return def_trade_info_;
    }
  } else {
    return dep_trade_map_[dep_shortcode_];
  }
}

const std::string NetworkAccountInfoManager::GetDepTradeAccount(ExchSource_t exch_source_,
                                                                const std::string& dep_shortcode_) {
  return GetDepTradeInfo(exch_source_, dep_shortcode_).account_;
}
const std::string NetworkAccountInfoManager::GetDepTradeHostIp(ExchSource_t exch_source_,
                                                               const std::string& dep_shortcode_) {
  return GetDepTradeInfo(exch_source_, dep_shortcode_).host_ip_;
}
const int NetworkAccountInfoManager::GetDepTradeHostPort(ExchSource_t exch_source_, const std::string& dep_shortcode_) {
  return GetDepTradeInfo(exch_source_, dep_shortcode_).host_port_;
}
const std::string NetworkAccountInfoManager::GetDepTradeBcastIp(ExchSource_t exch_source_,
                                                                const std::string& dep_shortcode_) {
  return GetDepTradeInfo(exch_source_, dep_shortcode_).bcast_ip_;
}
const int NetworkAccountInfoManager::GetDepTradeBcastPort(ExchSource_t exch_source_,
                                                          const std::string& dep_shortcode_) {
  return GetDepTradeInfo(exch_source_, dep_shortcode_).bcast_port_;
}

void NetworkAccountInfoManager::LoadInfoFile() {
  std::ifstream network_account_info_file_;
  network_account_info_file_.open(network_account_info_filename_.c_str(), std::ifstream::in);
  if (network_account_info_file_.is_open()) {
    const int kNetworkAccountInfoFileLineBufferLen = 1024;
    char readline_buffer_[kNetworkAccountInfoFileLineBufferLen];
    bzero(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);

    while (network_account_info_file_.good()) {
      bzero(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      network_account_info_file_.getline(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kNetworkAccountInfoFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 3) {
        if (strcmp(tokens_[0], "EXCHDATA") == 0) {  // "EXCHDATA" EXCH["MDS"] IP[MDSBCASTIP] PORT[MDSBCASTIP]
          // this <ip,port> combination refers to receiving mcast data from the exchange
          if (tokens_.size() >= 4) {
            if (exch_data_map_.find(StringToExchSource(tokens_[1])) == exch_data_map_.end()) {
              exch_data_map_[StringToExchSource(tokens_[1])] = DataInfo(std::string(tokens_[2]), atoi(tokens_[3]));
            }
          }
        } else if (strcmp(tokens_[0], "EXCHDATA2") == 0) {  // "EXCHDATA2" EXCH["MDS"] IP[MDSBCASTIP] PORT[MDSBCASTIP]
          // "EXCHDATA2" should always  be used to override EXCHDATA
          if (tokens_.size() >= 4) {
            char hostname[100] = {'\0'};
            int value = gethostname(hostname, 100);
            if (value == 0)  // success
            {
              if (strcmp(hostname, tokens_[2]) == 0) {
                if (StringToExchSource(tokens_[1]) != HFSAT::kExchSourceInvalid) {
                  exch_data_map_[StringToExchSource(tokens_[1])] = DataInfo(std::string(tokens_[3]), atoi(tokens_[4]));
                }
                str_exch_data_map_[std::string(tokens_[1])] = DataInfo(std::string(tokens_[3]), atoi(tokens_[4]));
              }
            }
          }
        }

        else if (strcmp(tokens_[0], "SRCDATA") == 0) {  // "SRCDATA" SRC["ZN_0"] IP[MDSBCASTIP] PORT[MDSBCASTIP]
          // Sometimes the data of an exchange could be divided into multiple channels.
          // TODO
        } else if (strcmp(tokens_[0], "EXCHTRADE") ==
                   0) {  // "EXCHTRADE" EXCH["EUREX"] ACCOUNT["57E57416"] ORS_CLIENT_IP[EUREX_TRADE_IP]
                         // ORS_CLIENT_PORT[EUREX_TRADE_PORT] IP[ORS_BCAST_IP] PORT[ORS_BCAST_PORT]
          // EXCHTRADE exch account_, host_ip_, host_port_, bast_ip_, bcast_port_
          //
          // PLEASE NOTE THAT IN THE NORMAL CASE THIS LINE SHOULD NOT BE USED
          // SRCTRADE SHOULD BE USED
          if (tokens_.size() >= 7) {
            exch_trade_map_[StringToExchSource(tokens_[1])] =
                TradeInfo(std::string(tokens_[2]), std::string(tokens_[3]), atoi(tokens_[4]), std::string(tokens_[5]),
                          atoi(tokens_[6]));
          }
        } else if (strcmp(tokens_[0], "SRCTRADE") == 0) {  // For specific product the ORS might be ( and is commonly )
          // differnt the default EXCHTRADE line above
          // in practise we don't expect "EXCHTRADE" line to be used
          // TODO
        } else if (strcmp(tokens_[0], "MARGINCONTROL") ==
                   0) {  // MARGINCONTROL EXCH PROFILE MARGIN_BCAST_IP MARGIN_BCAST_PORT
          if (tokens_.size() >= 5) {
            (exch_mcontrol_map_[StringToExchSource(tokens_[1])])[tokens_[2]] =
                DataInfo(std::string(tokens_[3]), atoi(tokens_[4]));
          }
        } else if (strcmp(tokens_[0], "TRADECONTROLRECV") == 0) {  // TRADECONTROLRECV TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            trade_control_recv_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "TRADECONTROLRECVUDPDIRECT") ==
                   0) {  // TRADECONTROLRECV TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            trade_control_recv_udp_direct_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "TRADESIGNALRECVUDPDIRECT") ==
                   0) {  // TRADECONTROLRECV TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            trade_signal_recv_udp_direct_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "COMBINEDCONTROL") == 0) {  // COMBINEDCONTROL TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            combined_control_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "COMBINEDCONTROLUDPDIRECT") == 0) {  // COMBINEDCONTROL TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            combined_control_udp_direct_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "PARAMSEND") == 0) {  // PARAMSEND PS_MCAST_IP PS_MCAST_PORT
          if (tokens_.size() >= 3) {
            param_send_data_info_ = DataInfo(std::string(tokens_[1]), atoi(tokens_[2]));
          }
        } else if (strcmp(tokens_[0], "RETAILSOURCETYPE") == 0) {  // TRADECONTROLRECV TC_MCAST_IP TC_MCAST_PORT
          if (tokens_.size() >= 3) {
            if (retail_source_data_map_.find(tokens_[1]) == retail_source_data_map_.end()) {
              retail_source_data_map_[tokens_[1]] = DataInfo(std::string(tokens_[2]), atoi(tokens_[3]));
            }
          }
        } else {  // IGNORE might be comments
        }
      }
    }
  }
}

const DataInfo NetworkAccountInfoManager::GetParamSendDataInfo() const { return param_send_data_info_; }

const DataInfo NetworkAccountInfoManager::GetControlRecvDataInfo() const { return trade_control_recv_data_info_; }
const DataInfo NetworkAccountInfoManager::GetControlRecvUDPDirectDataInfo() const {
  return trade_control_recv_udp_direct_data_info_;
}

const DataInfo NetworkAccountInfoManager::GetSignalRecvUDPDirectDataInfo() const {
  return trade_signal_recv_udp_direct_data_info_;
}

const DataInfo NetworkAccountInfoManager::GetCombControlDataInfo() const { return combined_control_data_info_; }
const DataInfo NetworkAccountInfoManager::GetCombControlUDPDirectDataInfo() const {
  return combined_control_udp_direct_data_info_;
}

const DataInfo& NetworkAccountInfoManager::GetMarginControlDataInfo(ExchSource_t exch_source_,
                                                                    std::string profile_) const {
  Ex2P2DInfoMapCIter_t citer_ = exch_mcontrol_map_.find(exch_source_);
  if (citer_ == exch_mcontrol_map_.end()) return def_data_info_;

  const P2DInfoMap& p2dinfo_map_ = citer_->second;
  P2DInfoMapCIter_t pciter_ = p2dinfo_map_.find(profile_);
  if (pciter_ == p2dinfo_map_.end()) return def_data_info_;

  return pciter_->second;
  // return (exch_mcontrol_map_[ exch_source_ ])[profile_];
}
}
