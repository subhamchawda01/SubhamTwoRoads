#ifndef POSITION_MGR_TESTS_HELPER
#define POSITION_MGR_TESTS_HELPER

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"

namespace HFTEST {
enum msgType { DELTAINC, DELTADEC, TRADE, INTERNAl, NOINFO };

struct OrderStruct {
  HFSAT::TradeType_t side_;
  int size_;
  int saci_;
  unsigned int security_id_;
  msgType msg_type_;
  int price_;
  OrderStruct()
      : side_(HFSAT::kTradeTypeNoInfo),
        size_(0),
        saci_(-1),
        security_id_(DEF_MAX_SEC_ID),
        msg_type_(NOINFO),
        price_(0) {}
  OrderStruct(HFSAT::TradeType_t side, unsigned int security_id, int size, int saci, msgType msg_type, int price)
      : side_(side), size_(size), saci_(saci), security_id_(security_id), msg_type_(msg_type), price_(price) {}
};

class PositionMgrTestsHelper {
 public:
  PositionMgrTestsHelper();
  void AddOrderInfo(const OrderStruct& order);
  void AddOrderInfo(const OrderStruct& order, const int& _combined_security_id_, const double& _security_weight_);

 private:
  HFSAT::ORS::PositionManager& position_mgr_;
};
}
#endif
