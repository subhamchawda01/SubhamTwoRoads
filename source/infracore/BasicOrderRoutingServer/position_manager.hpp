/**
   \file BasicOrderRoutingServer/position_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_POSITIONMANAGER_H
#define BASE_BASICORDERROUTINGSERVER_POSITIONMANAGER_H

//#include <iostream>

#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/lock.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/common_modules.hpp"
#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"

#if USE_ORS_MARKET_DATA_THREAD

#include "infracore/BasicOrderRoutingServer/market_data_thread.hpp"

#endif

namespace HFSAT {
namespace ORS {

/// Class that is used by MarginChecker to make sure that
/// positions never exceed MaxPosition and WorstCaseMaxPos
class PositionManager {
 private:
  PositionManager(const PositionManager&);

 public:
  /// Makes sure that there is only one instance of PositionManager
  static PositionManager& GetUniqueInstance() {
    static PositionManager uniqueinstance_;
    return uniqueinstance_;
  }

  /// Returns global position of this security
  inline const PositionInfo* GetPositionInfoStruct(unsigned int security_id) const {
    return &security_id_to_pos_info_[security_id];
  }

  /// Returns global position of this security
  inline int GetGlobalPosition(unsigned int _security_id_) const {
    return security_id_to_pos_info_[_security_id_].position;
  }

  /// Returns global position of this security
  inline int GetGlobalBidSize(unsigned int _security_id_) const {
    return security_id_to_pos_info_[_security_id_].bid_size;
  }
  /// Returns global position of this security
  inline int GetGlobalAskSize(unsigned int _security_id_) const {
    return security_id_to_pos_info_[_security_id_].ask_size;
  }

  /// Returns global position of this security as it would be if all the bids are filled at all prices for this security
  inline int GetGlobalWorstCaseBidPosition(unsigned int _security_id_) const {
    return std::max(
        0, security_id_to_pos_info_[_security_id_].position + security_id_to_pos_info_[_security_id_].bid_size);
  }

  /// Returns global position of this security as it would be if all the bids are filled at all prices for this security
  inline int GetGlobalWorstCaseAskPosition(unsigned int _security_id_) const {
    int retval = std::max(
        0, security_id_to_pos_info_[_security_id_].ask_size - security_id_to_pos_info_[_security_id_].position);
    return retval;
  }

  /// Returns the position per SACI
  inline int GetClientPosition(int _server_assigned_client_id_) const {
    return saci_to_position_info_[SACItoKey(_server_assigned_client_id_)].position;
  }

  inline bool CheckifAnyLongClient(unsigned int security_id) const {
    if (security_id < 0) return false;

    if (GetGlobalPosition(security_id) > 0) {
      return true;
    }

    return false;
  }

  inline bool CheckifAnyShortClient(unsigned int security_id) const {
    if (security_id < 0) return false;

    if (GetGlobalPosition(security_id) < 0) {
      return true;
    }

    return false;
  }

  inline bool CheckifAnyLiveBuyOrders(unsigned int security_id) const {
    return (security_id_to_pos_info_[security_id].bid_size != 0);
  }

  inline bool CheckifAnyLiveSellOrders(unsigned int security_id) const {
    return (security_id_to_pos_info_[security_id].ask_size != 0);
  }
  /// Adds a Buy that was executed in the real exchange
  inline void AddBuyTrade(const unsigned int _security_id_, const int _server_assigned_client_id_,
                          const int _bid_trade_size_) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_add_and_fetch(&pos_info.position, _bid_trade_size_);

    if (_server_assigned_client_id_ >= 0) {
      __sync_add_and_fetch((&(saci_to_position_info_[SACItoKey(_server_assigned_client_id_)].position)),
                           _bid_trade_size_);  // TODO ... test if this can be made more elegant as (
                                               // saci_to_position_map_ + SACItoKey ( _server_assigned_client_id_ ) )
    }
  }

  /// Adds a Buy that was executed internally at ORS level
  // Update client positions only... global position prone to error
  inline void AddInternalBuyTrade(const int _server_assigned_client_id_, const int _bid_trade_size_) {
    if (_server_assigned_client_id_ >= 0) {
      __sync_add_and_fetch((&(saci_to_position_info_[SACItoKey(_server_assigned_client_id_)].position)),
                           _bid_trade_size_);  // TODO ... test if this can be made more elegant as (
                                               // saci_to_position_map_ + SACItoKey ( _server_assigned_client_id_ ) )
    }
  }

  /// Adds a Sell that was executed in the real exchange
  inline void AddSellTrade(const unsigned int _security_id_, const int _server_assigned_client_id_,
                           const int _ask_trade_size_) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_sub_and_fetch(&pos_info.position, _ask_trade_size_);

    if (_server_assigned_client_id_ >= 0) {
      __sync_sub_and_fetch((&(saci_to_position_info_[SACItoKey(_server_assigned_client_id_)].position)),
                           _ask_trade_size_);  // TODO .. .test if this can be made more elegant as (
                                               // saci_to_position_map_ + SACItoKey ( _server_assigned_client_id_ ) )
    }
  }

  /// Adds a Sell that was executed internally at ORS level
  // Update client positions only... global position prone to error
  inline void AddInternalSellTrade(const int _server_assigned_client_id_, const int _ask_trade_size_) {
    if (_server_assigned_client_id_ >= 0) {
      __sync_sub_and_fetch((&(saci_to_position_info_[SACItoKey(_server_assigned_client_id_)].position)),
                           _ask_trade_size_);  // TODO .. .test if this can be made more elegant as (
                                               // saci_to_position_map_ + SACItoKey ( _server_assigned_client_id_ ) )
    }
  }

  inline void AddBidSize(const unsigned int _security_id_, const int _additional_bid_size_, int ordered_vol,
                         int ordered_count) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_add_and_fetch(&pos_info.bid_size, _additional_bid_size_);
  }

  inline void DecBidSize(const unsigned int _security_id_, const int _reduced_bid_size_, int ordered_vol,
                         int ordered_count) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_sub_and_fetch(&pos_info.bid_size, _reduced_bid_size_);
  }

  inline void AddAskSize(const unsigned int _security_id_, const int _additional_ask_size_, int ordered_vol,
                         int ordered_count) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_add_and_fetch(&pos_info.ask_size, _additional_ask_size_);
  }

  inline void DecAskSize(const unsigned int _security_id_, const int _reduced_ask_size_, int ordered_vol,
                         int ordered_count) {
    PositionInfo& pos_info = security_id_to_pos_info_[_security_id_];
    __sync_sub_and_fetch(&pos_info.ask_size, _reduced_ask_size_);
  }

  std::string DumpPMState() const;
  std::string DumpSACIPosition() const;
  std::string DumpSecurityPositions() const;
  void ClearPositions();
  /// Setup the recovery file for the position manager to write to
  void SetRecoveryFile(std::string _position_filename_);

  void SetBaseWriterId(int client_base);

#if USE_ORS_MARKET_DATA_THREAD

  inline void SetMarketDataThread(HFSAT::ORS::MarketDataThread* _market_data_thread_) {
    market_data_thread_ = _market_data_thread_;
  }
#endif

#if USE_ORS_MARKET_DATA_THREAD

  inline void SetDebugLogger(HFSAT::DebugLogger* _dbglogger_) { dbglogger_ = _dbglogger_; }
#endif

  /// Write the position manager's state to the specified file, to be used for recovery at next startup --
  void DumpPMRecovery();

 private:
  PositionManager() : position_filename_(""), order_manager_(OrderManager::GetUniqueInstance()) {
    for (int i = 0; i < ORS_MAX_NUM_OF_CLIENTS; i++) {
      saci_to_position_info_[i].position = 0;
    }
    for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
      security_id_to_pos_info_[i].position = 0;
      security_id_to_pos_info_[i].bid_size = 0;
      security_id_to_pos_info_[i].ask_size = 0;
      security_id_to_pos_info_[i].ordered_vol = 0;
      security_id_to_pos_info_[i].ordered_count = 0;
      security_id_to_pos_info_[i].traded_vol_ = 0;
      security_id_to_pos_info_[i].traded_count_ = 0;
    }
  }

  /// map from SACI to postion
  SACIPositionInfo saci_to_position_info_[ORS_MAX_NUM_OF_CLIENTS];
  PositionInfo security_id_to_pos_info_[DEF_MAX_SEC_ID];

  std::string position_filename_;
  OrderManager& order_manager_;
  int base_writer_id_;

#if USE_ORS_MARKET_DATA_THREAD

  HFSAT::DebugLogger* dbglogger_;
  HFSAT::ORS::MarketDataThread* market_data_thread_;

#endif
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_POSITIONMANAGER_H
