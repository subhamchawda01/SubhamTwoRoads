#ifndef BASE_BASICORDERROUTINGSERVER_ENGINELISTENER_H
#define BASE_BASICORDERROUTINGSERVER_ENGINELISTENER_H

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "infracore/BasicOrderRoutingServer/order.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

namespace HFSAT {
namespace ORS {

/// Typically we expect these to remain the same even
/// if the formatting engine changes for an exchange
class EngineListener {
 public:
  virtual ~EngineListener() {}

  /// technical connection based callbacks
  virtual void OnConnect(bool success) = 0;
  virtual void OnLogin(bool success) = 0;
  virtual void OnLogout() = 0;
  virtual void OnDisconnect() = 0;

  /// order management related callbacks
  virtual void OnOrderConf(Order* ord) = 0;
  virtual void OnOrderCxl(Order* ord) = 0;
  virtual void OnOrderExec(Order* ord) = 0;
  virtual void OnOrderMod(Order* ord) = 0;
  /// new order submission reject
  virtual void OnReject(Order* ord) = 0;
  /// cancel/modify request reject
  virtual void OnCxlReject(
      Order* ord, CxlRejectReason_t _cxl_reject_reason) = 0;  // Added a reason parameter. See "CDef/defines.hpp" -- .
  virtual void OnCxlReject(int server_assigned_seqnum, CxlRejectReason_t _cxl_reject_reason_,
                           uint64_t _exch_seq_ = 0) = 0;

  /// callbacks which are potentially faster for the Engine
  virtual void OnReject(int server_assigned_sequence_number,
                        const ORSRejectionReason_t rejection_reason_ = kExchOrderReject, uint64_t _exch_seq_ = 0, uint64_t min_throttle_wait_ = 0) {}

  virtual void OnORSReject(Order* ord, const ORSRejectionReason_t rejection_reason_ = kExchOrderReject, uint64_t min_throttle_wait_ = 0) {}
  /// callbacks which are potentially faster for the Engine
  virtual void OnOrderModReject(int server_assigned_sequence_number,
                                const CxlReplaceRejectReason_t rejection_reason_ = kExchCancelReplaceReject) {}

  virtual void OnOrderConf(int server_assigned_sequence_number, const char* exch_assigned_sequence_number, double price,
                           int size, int exch_assigned_sequence_number_length_ = 0, uint64_t _exch_seq_ = 0,
                           int32_t entry_dt = 0, int64_t last_activity_ref = 0) {}
  virtual void OnOrderConf(int server_assigned_sequence_number, const char* exch_assigned_sequence_number,
                           int exch_assigned_sequence_number_length_ = 0, uint64_t _exch_seq_ = 0) {}
  virtual void OnOrderCxl(int server_assigned_sequence_number, uint64_t _exch_seq_ = 0) {}
  virtual void OnOrderCxlETS(int server_assigned_sequence_number, int cum_executed_qty_, int actual_del_qty_) {}
  virtual void OnOrderCxlCMI(const int32_t& _server_assigned_seqnum_, const uint32_t& _leaves_qty_,
                             const uint32_t& _cumulated_traded_qty_, const uint32_t& _cumulated_cxl_qty_) {}
  virtual void OnOrderCxlHK(int server_assigned_sequence_number, int actual_del_qty_, uint64_t _exch_seq_ = 0) {}
  virtual void OnOrderCxlOSE(int server_assigned_sequence_number, int actual_del_qty_, uint64_t _exch_seq_ = 0) {}

  virtual void OnOrderMod(int server_assigned_sequence_number, const char* exch_assigned_sequence_number, double price,
                          int size) {}

  virtual void OnOrderExec(int server_assigned_sequence_number, const char* symbol, TradeType_t trade_type,
                           double price, int size_executed, int size_remaining, uint64_t _exch_seq_ = 0,
                           int32_t last_mod_dt = 0, int64_t last_activity_ref = 0) {}

  virtual void OnOrderExecCMI(int server_assigned_sequence_number, const char* symbol, TradeType_t trade_type,
                              double price, int size_executed, int size_remaining, uint64_t _exch_seq_ = 0) {}

  virtual void OnOrderConfBMF(int server_assigned_seqnum, uint64_t _exch_seq_ = 0) {}
  virtual void OnOrderCxlBMF(int server_assigned_seqnum) {}
  virtual void OnOrderExecBMF(int server_assigned_seqnum, double price, int size_executed, int size_remaining) {}
  virtual void OnOrderCancelReplacedBMF(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num, double price,
                                        int32_t size, std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_) {
  }
  virtual void OnOrderCancelReplaced(int32_t server_assigned_seq_num, uint64_t exch_assigned_seq_num, double price,
                                     int32_t size, std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_,
                                     int32_t last_mod_dt = 0, int64_t last_activity_ref = 0) {}
  virtual void OnOrderCancelReplacedRTS(int32_t server_assigned_seq_num, const char* exch_assigned_sequence_number,
                                        double price, int32_t size, int new_saos,
                                        std::vector<HFSAT::FastPriceConvertor*>& fast_px_convertor_vec_) {}
  virtual void CancelAllPendingOrders() {}
  virtual void OnBatchCxlAlert(int32_t user_id_) {} 
  virtual void ForceBatchCancelBroadcastSAOS(char const * saos) {} 
  virtual void ForceBatchCancelBroadcastSymbol(char const * symbol) {}
  virtual void ForceBatchCancelBroadcastAll(bool isMarginBreached = false) {}
  virtual void ForceBatchCancelBroadcastAllOrderRemoval(){}
  virtual void KillSwitch() {}
  virtual void DumpSettingsToFile(){}
  virtual void SetMarginValue(double margin) {}
  virtual void FetchMarginUsage() {};
  virtual void RequestExecutedOrders() {};
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_ENGINELISTENER_H
