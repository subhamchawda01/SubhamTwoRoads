/**
    \file dvccode/TradingInfo/network_account_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_INITCOMMON_NETWORK_ACCOUNT_INFO_MANAGER_H
#define BASE_INITCOMMON_NETWORK_ACCOUNT_INFO_MANAGER_H

#include <string>
#include <map>
#include "dvccode/CDef/defines.hpp"

#define DEF_NW_ACCOUNT_INFO_FILENAME "network_account_info_filename.cfg"
#define FILTER_STR "FILTER"

namespace HFSAT {

class NetworkAccountInfoManager {
 protected:
  std::string network_account_info_filename_;

  DataInfo def_data_info_;    ///< only used for returningdummy values
  TradeInfo def_trade_info_;  ///< only used for returningdummy values

  typedef std::map<ExchSource_t, DataInfo> ExchDataInfoMap;
  typedef std::map<ExchSource_t, DataInfo>::const_iterator ExchDataInfoMapCIter_t;
  typedef std::map<std::string, DataInfo> StrExchDataInfoMap;
  typedef std::map<std::string, DataInfo>::const_iterator StrExchDataInfoMapCIter_t;
  typedef std::map<std::string, DataInfo> SrcDataInfoMap;
  typedef std::map<std::string, DataInfo>::const_iterator SrcDataInfoMapCIter_t;

  ExchDataInfoMap exch_data_map_;
  StrExchDataInfoMap str_exch_data_map_;
  SrcDataInfoMap src_data_map_;
  std::map<ExchSource_t, TradeInfo> exch_trade_map_;
  std::map<std::string, TradeInfo> dep_trade_map_;

  /// for margin stuff
  typedef std::map<std::string, DataInfo> P2DInfoMap;
  typedef std::map<std::string, DataInfo>::const_iterator P2DInfoMapCIter_t;
  typedef std::map<ExchSource_t, P2DInfoMap> Ex2P2DInfoMap;
  typedef std::map<ExchSource_t, P2DInfoMap>::const_iterator Ex2P2DInfoMapCIter_t;

  Ex2P2DInfoMap exch_mcontrol_map_;
  DataInfo param_send_data_info_;
  DataInfo trade_control_recv_data_info_;
  DataInfo combined_control_data_info_;

  typedef std::map<std::string, DataInfo> RetailDataInfoMap;
  typedef std::map<std::string, DataInfo>::const_iterator RetailDataInfoMapCIter_t;

  RetailDataInfoMap retail_source_data_map_;

 public:
  NetworkAccountInfoManager();

  const DataInfo& GetSrcDataInfo(ExchSource_t exch_source_, const std::string& src_shortcode_);
  const std::string& GetSrcDataIp(ExchSource_t exch_source_, const std::string& src_shortcode_);
  const int GetSrcDataPort(ExchSource_t exch_source_, const std::string& src_shortcode_);

  const DataInfo& GetSrcDataInfo(ExchSource_t exch_source_);
  const std::string& GetSrcDataIp(ExchSource_t exch_source_);
  const int GetSrcDataPort(ExchSource_t exch_source_);

  const DataInfo& GetSrcDataInfo(std::string exch_source);

  const DataInfo GetDepDataInfo(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const TradeInfo& GetDepTradeInfo(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const std::string GetDepTradeAccount(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const std::string GetDepTradeHostIp(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const int GetDepTradeHostPort(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const std::string GetDepTradeBcastIp(ExchSource_t exch_source_, const std::string& dep_shortcode_);
  const int GetDepTradeBcastPort(ExchSource_t exch_source_, const std::string& dep_shortcode_);

  /// returns the < bcast_ip_, bcast_port_ > on which status messages are to be sent to the Trading Frontend
  const DataInfo GetParamSendDataInfo() const;

  /// returns the < bcast_ip_, bcast_port_ > on which control messages from Trading Frontend are to be received.
  const DataInfo GetControlRecvDataInfo() const;

  /// returns the < bcast_ip_, bcast_port_ > on which combined writer can be sent command messages
  const DataInfo GetCombControlDataInfo() const;

  const DataInfo& GetMarginControlDataInfo(ExchSource_t exch_source_, std::string profile_) const;
  const DataInfo& GetRetailDataInfoFromSourceType(const std::string& _retail_source_type_);

  void CleanUp() {
    exch_data_map_.clear();
    src_data_map_.clear();
    exch_trade_map_.clear();
    dep_trade_map_.clear();
    exch_mcontrol_map_.clear();
    retail_source_data_map_.clear();
  }

 protected:
  void LoadInfoFile();
};
}

#endif  // BASE_INITCOMMON_NETWORK_ACCOUNT_INFO_MANAGER_H
