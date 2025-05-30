#include <iostream>

#include "dvccode/CDef/security_definitions.hpp"
#include "infracore/Tools/positions_and_orders_calculator.hpp"

namespace HFSAT {

PositionAndOrderCalculator::PositionAndOrderCalculator(int date, std::string shortcode)
    : position_mgr_(HFSAT::ORS::PositionManager::GetUniqueInstance()),
      trading_date_(date),
      shortcode_(shortcode),
      t_exch_source_(HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, date)) {}

PositionAndOrderCalculator::~PositionAndOrderCalculator() {
  for (auto iter = saos_to_exec_size_.begin(); iter != saos_to_exec_size_.end(); iter++) {
    /* Orders which are internally executed need to be excluded. For aggressive orders which are partially executed, we
     don't send the order to Exchange
     Thus, we can only have Msg Sequenced, Internal Exec for these kind of orders & as there is no msg after the
internal exec, these orders will be displayed as incomplete incorrectly. For example :-

     Sym: WDON17 Px: 3274 ST: 1496434324.010370 CT: 1496434324.010350 ORR: Seqd SAMS: 9391 SACI: 5111814 SR: 6 BS: 1
SAOS: 26908 CAOS: 3051 SE: 0 CP: 19 GP: 17 IP: 6548 Seq: 5111814 WBP: 233 WAP: 276 Pos:31
Sym: WDON17 Px: 3274 ST: 1496434324.010372 CT: 1496434324.010350 ORR: IntExec SAMS: 9392 SACI: 5111814 SR: 3 BS: 1 SAOS:
26908 CAOS: 3051 SE: 3 CP: 16 GP: 17 IP: 6548 Seq: 0 WBP: 233 WAP: 276 Pos:31
     */

    if (saos_to_exec_size_[iter->first] != saos_to_total_size_[iter->first] &&
        saos_to_internal_exec_size.find(iter->first) == saos_to_internal_exec_size.end()) {
      std::cerr << "Error: Executed Size and Total Size Mismatch for SAOS:" << iter->first
                << ". Total Size: " << saos_to_total_size_[iter->first]
                << ". Exec Size: " << saos_to_exec_size_[iter->first] << std::endl;
    }
  }
}

void PositionAndOrderCalculator::ORSMessageBegin(const unsigned int _security_id_,
                                                 const GenericORSReplyStruct& ors_reply) {
  ors_reply_ = ors_reply;
  std::string exch_symbol = ors_reply.symbol_;
}

void PositionAndOrderCalculator::ORSMessageEnd(const unsigned int _security_id_,
                                               const GenericORSReplyStruct& ors_reply) {
  std::string ors_reply_to_string = const_cast<GenericORSReplyStruct&>(ors_reply).ToString();
  std::cout << ors_reply_to_string.substr(0, ors_reply_to_string.size() - 1)
            << " WBP: " << position_mgr_.GetGlobalWorstCaseBidPosition(_security_id_)
            << " WAP: " << position_mgr_.GetGlobalWorstCaseAskPosition(_security_id_)
            << " Pos:" << position_mgr_.GetGlobalPosition(_security_id_) << std::endl;
  /*std::cout << static_cast<GenericORSReplyStruct&>(ors_reply).time_set_by_server_.ToString()
            << " Pos: " << position_mgr_.GetGlobalPosition(_security_id_)
            << " BidLiveSize: " << position_mgr_.GetBidLiveSize(_security_id_)
            << " BidWorstPos: " << position_mgr_.GetGlobalWorstCaseBidPosition(_security_id_)
            << " AskLiveSize: " << position_mgr_.GetAskLiveSize(_security_id_)
            << " AskWorstPos: " << position_mgr_.GetGlobalWorstCaseAskPosition(_security_id_) << std::endl;*/
}

void PositionAndOrderCalculator::OrderSequencedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const uint64_t exchange_order_id,
    const ttime_t time_set_by_server) {
  if (r_buysell_ == kTradeTypeBuy) {
    position_mgr_.AddBidSize(_security_id_, _size_remaining_, 0, 0);
  } else if (r_buysell_ == kTradeTypeSell) {
    position_mgr_.AddAskSize(_security_id_, _size_remaining_, 0, 0);
  }
  // In case of Internal Exec, ORS confirmed is not received for one side, so the order needs to be added in the map
  saos_to_total_size_[_server_assigned_order_sequence_] = _size_remaining_;
}

void PositionAndOrderCalculator::OrderORSConfirmed(const int t_server_assigned_client_id_,
                                                   const int _client_assigned_order_sequence_,
                                                   const int _server_assigned_order_sequence_,
                                                   const unsigned int _security_id_, const double _price_,
                                                   const TradeType_t r_buysell_, const int _size_remaining_,
                                                   const int _size_executed_, const int r_int_price_,
                                                   const int32_t server_assigned_message_sequence,
                                                   const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;
  saos_to_total_size_[_server_assigned_order_sequence_] = _size_remaining_ + _size_executed_;
}

void PositionAndOrderCalculator::OrderConfirmedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const uint64_t exchange_order_id,
    const ttime_t time_set_by_server) {
  saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;
  saos_to_total_size_[_server_assigned_order_sequence_] = _size_remaining_ + _size_executed_;
}

void PositionAndOrderCalculator::OrderCxlSequencedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

void PositionAndOrderCalculator::OrderCanceledAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (t_exch_source_ == HFSAT::kExchSourceCFE || t_exch_source_ == HFSAT::kExchSourceHONGKONG ||
      t_exch_source_ == HFSAT::kExchSourceJPY || t_exch_source_ == HFSAT::kExchSourceEUREX) {
    /* The above four Exchanges support partial cancellation of orders. It can send executions on the orders which are
partially cancelled
     * When the order is partially cancelled, the SR(size remaining) field in ORS msg indicates Qty which can be traded
after partial cancellation. If the order is fully cancelled, SR equals 0
     * Example: -
     * Sym: TOPIXM1706 Px: 161050 ST: 1496391222.022072 CT: 1496391222.022050 ORR: Seqd SAMS: 575 SACI: 1507330 SR: 8
BS: 0 SAOS: 71085 CAOS: 193 SE: 0 CP: -8 GP: -8 IP: 6442 Seq: 22050 WBP:
10 WAP: 18 Pos:-8
Sym: TOPIXM1706 Px: 161050 ST: 1496391222.022892 CT: 1496391222.022050 ORR: Conf SAMS: 580 SACI: 1507330 SR: 8 BS: 0
SAOS: 71085 CAOS: 193 SE: 0 CP: -8 GP: -8 IP: 6442 Seq: 68552318112
12445754 WBP: 0 WAP: 18 Pos:-8
Sym: TOPIXM1706 Px: 161050 ST: 1496391222.022924 CT: 1496391222.022917 ORR: CxlSeqd SAMS: 0 SACI: 1507330 SR: 8 BS: 0
SAOS: 71085 CAOS: 193 SE: 0 CP: -8 GP: -8 IP: 6442 Seq: 6855231811
212445754 WBP: 2 WAP: 18 Pos:-8
Sym: TOPIXM1706 Px: 161050 ST: 1496391222.023554 CT: 1496391222.022917 ORR: Cxld SAMS: 583 SACI: 1507330 SR: 1 BS: 0
SAOS: 71085 CAOS: 193 SE: 0 CP: -8 GP: -8 IP: 6442 Seq: 68552318112
12445754 WBP: 1 WAP: 18 Pos:-8
Sym: TOPIXM1706 Px: 161050 ST: 1496391222.024863 CT: 1496391222.022917 ORR: Exec SAMS: 587 SACI: 1507330 SR: 0 BS: 0
SAOS: 71085 CAOS: 193 SE: 1 CP: -7 GP: -7 IP: 6442 Seq: 68552318112
12445754 WBP: 9 WAP: 17 Pos:-7
     *
     */
    unsigned int quantity_cancelled = saos_to_total_size_[_server_assigned_order_sequence_] - _size_remaining_;
    if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
      quantity_cancelled = quantity_cancelled - saos_to_exec_size_[_server_assigned_order_sequence_];
    }
    if (r_buysell_ == kTradeTypeBuy) {
      position_mgr_.DecBidSize(_security_id_, quantity_cancelled, 0, 0);

    } else if (r_buysell_ == kTradeTypeSell) {
      position_mgr_.DecAskSize(_security_id_, quantity_cancelled, 0, 0);
    }
    saos_to_total_size_[_server_assigned_order_sequence_] =
        saos_to_total_size_[_server_assigned_order_sequence_] - quantity_cancelled;

  } else {
    /* Exchanges other than EUREX, CFE, TK & HK don't support partial order cancellation. When an order is cancelled &
     *Exch sends a Cxld, the SR (size remaining) field in the ORS msg indicates Qty which is cancelled
     *It cannot send further executions on cancelled orders
     *Example :-
     *Sym: AP201706 Px: 5762 ST: 1496650597.837294 CT: 1496650597.837281 ORR: Seqd SAMS: 8 SACI: 2883609 SR: 2 BS: 1
SAOS: 10826 CAOS: 4 SE: 0 CP: 0 GP: 1 IP: 5762 Seq: 143797216
Sym: AP201706 Px: 5762 ST: 1496650597.837722 CT: 1496650597.837281 ORR: Conf SAMS: 9 SACI: 2883609 SR: 2 BS: 1 SAOS:
10826 CAOS: 4 SE: 0 CP: 0 GP: 1 IP: 5762 Seq: 6277407589110136833
Sym: AP201706 Px: 5762 ST: 1496650750.059392 CT: 1496650597.837281 ORR: Exec SAMS: 16 SACI: 2883609 SR: 1 BS: 1 SAOS:
10826 CAOS: 4 SE: 1 CP: -3 GP: -6 IP: 5762 Seq: 0
Sym: AP201706 Px: 5762 ST: 1496650756.297116 CT: 1496650756.297092 ORR: CxlSeqd SAMS: 0 SACI: 2883609 SR: 1 BS: 1 SAOS:
10826 CAOS: 4 SE: 1 CP: -3 GP: -6 IP: 5762 Seq: 0
Sym: AP201706 Px: 5762 ST: 1496650756.297711 CT: 1496650756.297092 ORR: Cxld SAMS: 17 SACI: 2883609 SR: 1 BS: 1 SAOS:
10826 CAOS: 4 SE: 0 CP: -3 GP: -6 IP: 5762 Seq: 0
     *
     *
     */
    unsigned int sr = _size_remaining_;
    if (sr == 0) {
      sr = saos_to_total_size_[_server_assigned_order_sequence_];
      if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
        sr = sr - saos_to_exec_size_[_server_assigned_order_sequence_];
      }
    }
    if (r_buysell_ == kTradeTypeBuy) {
      position_mgr_.DecBidSize(_security_id_, sr, 0, 0);

    } else if (r_buysell_ == kTradeTypeSell) {
      position_mgr_.DecAskSize(_security_id_, sr, 0, 0);
    }
    if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
      if (saos_to_total_size_[_server_assigned_order_sequence_] -
                  saos_to_exec_size_[_server_assigned_order_sequence_] !=
              (int32_t)sr &&
          (saos_to_internal_exec_size.find(_server_assigned_order_sequence_) == saos_to_internal_exec_size.end())) {
        std::cerr << "Error: Cancel Conf : Mismathc b/w remaining size and cancelled size. SAOS : "
                  << _server_assigned_order_sequence_
                  << ". Total Size: " << saos_to_total_size_[_server_assigned_order_sequence_]
                  << ". Exec Size: " << saos_to_exec_size_[_server_assigned_order_sequence_] << std::endl;
      }
      saos_to_exec_size_.erase(_server_assigned_order_sequence_);
      saos_to_total_size_.erase(_server_assigned_order_sequence_);
    }
  }
}

void PositionAndOrderCalculator::OrderCancelRejected(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t t_buysell_, const int _size_remaining_, const int _rejection_reason_,
    const int t_client_position_, const int t_global_position_, const int r_int_price_,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

void PositionAndOrderCalculator::OrderExecuted(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  int size_now_executed = _size_executed_;
  if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
    size_now_executed -= saos_to_exec_size_[_server_assigned_order_sequence_];
  }
  if (r_buysell_ == kTradeTypeBuy) {
    position_mgr_.AddBuyTrade(_security_id_, t_server_assigned_client_id_, size_now_executed);
    position_mgr_.DecBidSize(_security_id_, size_now_executed, 0, 0);
  } else if (r_buysell_ == kTradeTypeSell) {
    position_mgr_.AddSellTrade(_security_id_, t_server_assigned_client_id_, size_now_executed);
    position_mgr_.DecAskSize(_security_id_, size_now_executed, 0, 0);
  }
  saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;
}

void PositionAndOrderCalculator::OrderInternallyMatched(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  int size_now_executed = _size_executed_;
  if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
    size_now_executed -= saos_to_exec_size_[_server_assigned_order_sequence_];
  }
  if (r_buysell_ == kTradeTypeBuy) {
    position_mgr_.AddInternalBuyTrade(t_server_assigned_client_id_, size_now_executed);
  } else if (r_buysell_ == kTradeTypeSell) {
    position_mgr_.AddInternalSellTrade(t_server_assigned_client_id_, size_now_executed);
  }
  saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;
  saos_to_internal_exec_size[_server_assigned_order_sequence_] = _size_executed_;
  if (saos_to_total_size_.find(_server_assigned_order_sequence_) == saos_to_total_size_.end()) {
    // Confirmed not received. Restore Bid, Ask Maps
    if (r_buysell_ == kTradeTypeBuy) {
      position_mgr_.DecBidSize(_security_id_, size_now_executed, 0, 0);

    } else if (r_buysell_ == kTradeTypeSell) {
      position_mgr_.DecAskSize(_security_id_, size_now_executed, 0, 0);
    }
  }
}

void PositionAndOrderCalculator::OrderRejected(const int t_server_assigned_client_id_,
                                               const int _client_assigned_order_sequence_,
                                               const unsigned int _security_id_, const double _price_,
                                               const TradeType_t r_buysell_, const int _size_remaining_,
                                               const int _rejection_reason_, const int r_int_price_,
                                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (ors_reply_.server_assigned_order_sequence_ == 0) {
    return;
  }
  if (r_buysell_ == kTradeTypeBuy) {
    position_mgr_.DecBidSize(_security_id_, _size_remaining_, 0, 0);

  } else if (r_buysell_ == kTradeTypeSell) {
    position_mgr_.DecAskSize(_security_id_, _size_remaining_, 0, 0);
  }
}
}
