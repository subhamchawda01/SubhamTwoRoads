#include "infracore/Tests/PositionMgr/position_mgr_tests_helper.hpp"

namespace HFTEST {

PositionMgrTestsHelper::PositionMgrTestsHelper() : position_mgr_(HFSAT::ORS::PositionManager::GetUniqueInstance()) {}

void PositionMgrTestsHelper::AddOrderInfo(const OrderStruct& order) {
  switch (order.msg_type_) {
    case DELTAINC: {
      if (order.side_ == HFSAT::kTradeTypeBuy) {
        position_mgr_.AddBestBidMap(order.security_id_, order.price_, order.size_);
        position_mgr_.AddBidSize(order.security_id_, order.size_, 0, 0);
      } else if (order.side_ == HFSAT::kTradeTypeSell) {
        position_mgr_.AddBestAskMap(order.security_id_, order.price_, order.size_);
        position_mgr_.AddAskSize(order.security_id_, order.size_, 0, 0);
      }
    } break;
    case DELTADEC: {
      if (order.side_ == HFSAT::kTradeTypeBuy) {
        position_mgr_.DecBestBidMap(order.security_id_, order.price_, order.size_);
        position_mgr_.DecBidSize(order.security_id_, order.size_, 0, 0);
      } else if (order.side_ == HFSAT::kTradeTypeSell) {
        position_mgr_.DecBestAskMap(order.security_id_, order.price_, order.size_);
        position_mgr_.DecAskSize(order.security_id_, order.size_, 0, 0);
      }
    } break;
    case TRADE: {
      if (order.side_ == HFSAT::kTradeTypeBuy) {
        position_mgr_.AddBuyTrade(order.security_id_, order.saci_, order.size_, false);
      } else if (order.side_ == HFSAT::kTradeTypeSell) {
        position_mgr_.AddSellTrade(order.security_id_, order.saci_, order.size_, false);
      }
    } break;
    case INTERNAl: {
      if (order.side_ == HFSAT::kTradeTypeBuy) {
        position_mgr_.AddInternalBuyTrade(order.saci_, order.size_);
      } else if (order.side_ == HFSAT::kTradeTypeSell) {
        position_mgr_.AddInternalSellTrade(order.saci_, order.size_);
      }
    } break;
    default: { std::cerr << "Order message type unknown \n"; } break;
  }
}

void PositionMgrTestsHelper::AddOrderInfo(const OrderStruct& order, const int& combined_security_id,
                                          const double& security_weight) {
  position_mgr_.AddSecurityToCombinedPool(order.security_id_, combined_security_id, security_weight, false);
  AddOrderInfo(order);
}
}
