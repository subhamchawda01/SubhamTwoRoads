#include "infracore/Tests/PlaybackMgr/bmf_playback_manager_tests.hpp"

#include "infracore/BasicOrderRoutingServer/exchange_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_manager.hpp"
#include "infracore/BMFEP/bmf_playback_defines.hpp"

#include <map>
#include <vector>

namespace HFTEST {
using namespace HFSAT::ORS;

bool BMFPlaybackManagerTests::IsOrderValid(BMFPlaybackStruct order, int side, int size, unsigned long long id) {
  return (order.side == side && order.size == size && order.origClOrdID == id);
}

// New Order, Partial Execute
void BMFPlaybackManagerTests::Test1() {
  int sec_id = 123, size = 10, side = 1, id = 15, exec_size = 3;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], side, size - exec_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, No Execute
void BMFPlaybackManagerTests::Test2() {
  int sec_id = 123, size = 10, side = 1, id = 15, exec_size = 0;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], side, size - exec_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Full Execute
void BMFPlaybackManagerTests::Test3() {
  int sec_id = 123, size = 10, side = 1, id = 15, exec_size = size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Cancel Order
void BMFPlaybackManagerTests::Test4() {
  int sec_id = 123, size = 10, side = 1, id = 15;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct cancel_order(side, size, id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == 0 && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Partial Execution, Cancel
void BMFPlaybackManagerTests::Test5() {
  int sec_id = 123, size = 10, side = 1, id = 15, exec_size = 3;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);
  BMFPlaybackStruct cancel_order(side, size - exec_size, id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Partial Execute
void BMFPlaybackManagerTests::Test6() {
  int sec_id = 123, size = 10, side = 2, id = 15, exec_size = 3;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == -exec_size && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], side, size - exec_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, No Execute
void BMFPlaybackManagerTests::Test7() {
  int sec_id = 123, size = 10, side = 2, id = 15, exec_size = 0;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == -exec_size && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], side, size - exec_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Full Execute
void BMFPlaybackManagerTests::Test8() {
  int sec_id = 123, size = 10, side = 2, id = 15, exec_size = size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == -exec_size && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Cancel Order
void BMFPlaybackManagerTests::Test9() {
  int sec_id = 123, size = 10, side = 2, id = 15;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct cancel_order(side, size, id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == 0 && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Partial Execution, Cancel
void BMFPlaybackManagerTests::Test10() {
  int sec_id = 123, size = 10, side = 2, id = 15, exec_size = 3;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);
  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);
  BMFPlaybackStruct cancel_order(side, size - exec_size, id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == -exec_size && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, No Exeution, No Cancel
void BMFPlaybackManagerTests::Test11() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == 0 && (live_orders.size() == 2) &&
                  (IsOrderValid(live_orders[0], buy_side, buy_size, buy_id) ||
                   IsOrderValid(live_orders[0], sell_side, sell_size, sell_id)) &&
                  (IsOrderValid(live_orders[1], buy_side, buy_size, buy_id) ||
                   IsOrderValid(live_orders[1], sell_side, sell_size, sell_id)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Full Execution Both Sides
void BMFPlaybackManagerTests::Test12() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = buy_size, sell_size_exec = sell_size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (buy_size_exec - sell_size_exec) && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Full Execution Buy side, No Execution Other
void BMFPlaybackManagerTests::Test13() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = buy_size, sell_size_exec = 0;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (buy_size_exec - sell_size_exec) && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], sell_side, sell_size - sell_size_exec, sell_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Full Execution Sell side, No Execution Other
void BMFPlaybackManagerTests::Test14() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = 0, sell_size_exec = sell_size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (buy_size_exec - sell_size_exec) && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], buy_side, buy_size - buy_size_exec, buy_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Partial Execution Both Sides
void BMFPlaybackManagerTests::Test15() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = buy_size - buy_size / 2, sell_size_exec = sell_size - sell_size / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (buy_size_exec - sell_size_exec) && (live_orders.size() == 2) &&
                  (IsOrderValid(live_orders[0], buy_side, buy_size - buy_size_exec, buy_id) ||
                   IsOrderValid(live_orders[0], sell_side, sell_size - sell_size_exec, sell_id)) &&
                  (IsOrderValid(live_orders[1], buy_side, buy_size - buy_size_exec, buy_id) ||
                   IsOrderValid(live_orders[1], sell_side, sell_size - sell_size_exec, sell_id)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel both
void BMFPlaybackManagerTests::Test16() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct cancel_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_buy);
  BMFPlaybackStruct cancel_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == 0 && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Buy Side Order
void BMFPlaybackManagerTests::Test17() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct cancel_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_buy);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], sell_side, sell_size, sell_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Sell Side Order
void BMFPlaybackManagerTests::Test18() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct cancel_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], buy_side, buy_size, buy_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Sell Side Order, Full Execute Buy Side Order
void BMFPlaybackManagerTests::Test19() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = buy_size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct cancel_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == buy_size_exec && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Sell Side Order, Partial Execute Buy Side Order
void BMFPlaybackManagerTests::Test20() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      buy_size_exec = buy_size - buy_size / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct exec_order_buy(buy_side, buy_size_exec, buy_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_buy);
  BMFPlaybackStruct cancel_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == buy_size_exec && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], buy_side, buy_size - buy_size_exec, buy_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Buy Side Order, Full Execute Sell Side Order
void BMFPlaybackManagerTests::Test21() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      sell_size_exec = sell_size;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct cancel_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (-sell_size_exec) && (live_orders.empty()));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// New Order Buy and Sell Side, Cancel Buy Side Order, Partial Execute Sell Side Order
void BMFPlaybackManagerTests::Test22() {
  int sec_id = 123, buy_size = 10, sell_size = 13, buy_side = 1, sell_side = 2, buy_id = 15, sell_id = 25,
      sell_size_exec = sell_size - sell_size / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_buy);
  BMFPlaybackStruct new_order_sell(sell_side, sell_size, sell_id);
  exch_pb_mgr.OrderNew(sec_id, new_order_sell);
  BMFPlaybackStruct cancel_order_buy(buy_side, buy_size, buy_id);
  exch_pb_mgr.OrderCancel(sec_id, cancel_order_buy);
  BMFPlaybackStruct exec_order_sell(sell_side, sell_size_exec, sell_id);
  exch_pb_mgr.OrderExec(sec_id, exec_order_sell);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == (-sell_size_exec) && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], sell_side, sell_size - sell_size_exec, sell_id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New buy Side Orders
void BMFPlaybackManagerTests::Test23() {
  int sec_id = 123, size_1 = 10, size_2 = 13, side = 1, id_1 = 15, id_2 = 23;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 2) &&
       (IsOrderValid(live_orders[0], side, size_1, id_1) || IsOrderValid(live_orders[0], side, size_2, id_2)) &&
       (IsOrderValid(live_orders[1], side, size_1, id_1) || IsOrderValid(live_orders[1], side, size_2, id_2)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New Sell Side Orders
void BMFPlaybackManagerTests::Test24() {
  int sec_id = 123, size_1 = 10, size_2 = 13, side = 2, id_1 = 15, id_2 = 23;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 2) &&
       (IsOrderValid(live_orders[0], side, size_1, id_1) || IsOrderValid(live_orders[0], side, size_2, id_2)) &&
       (IsOrderValid(live_orders[1], side, size_1, id_1) || IsOrderValid(live_orders[1], side, size_2, id_2)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New buy Side Orders, Full Execute One
void BMFPlaybackManagerTests::Test25() {
  int sec_id = 123, size_1 = 10, size_2 = 13, side = 1, id_1 = 15, id_2 = 23;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);
  BMFPlaybackStruct exec_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderExec(sec_id, exec_order_1);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == size_1 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], side, size_2, id_2));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New buy Side Orders, Partial Execute One
void BMFPlaybackManagerTests::Test26() {
  int sec_id = 123, size_1 = 13, size_2 = 13, side = 1, id_1 = 15, id_2 = 23, exec_size_1 = size_1 - size_1 / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);
  BMFPlaybackStruct exec_order_1(side, exec_size_1, id_1);
  exch_pb_mgr.OrderExec(sec_id, exec_order_1);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size_1 && (live_orders.size() == 2) &&
                  (IsOrderValid(live_orders[0], side, size_2, id_2) ||
                   IsOrderValid(live_orders[0], side, size_1 - exec_size_1, id_1)) &&
                  (IsOrderValid(live_orders[1], side, size_1 - exec_size_1, id_1) ||
                   IsOrderValid(live_orders[1], side, size_2, id_2)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New Sell Side Orders, Full Execute One
void BMFPlaybackManagerTests::Test27() {
  int sec_id = 123, size_1 = 10, size_2 = 13, side = 2, id_1 = 15, id_2 = 23;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);
  BMFPlaybackStruct exec_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderExec(sec_id, exec_order_1);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == -size_1 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], side, size_2, id_2));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
// Multiple New Sell Side Orders, Partial Execute One
void BMFPlaybackManagerTests::Test28() {
  int sec_id = 123, size_1 = 13, size_2 = 13, side = 2, id_1 = 15, id_2 = 23, exec_size_1 = size_1 - size_1 / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order_1(side, size_1, id_1);
  exch_pb_mgr.OrderNew(sec_id, new_order_1);
  BMFPlaybackStruct new_order_2(side, size_2, id_2);
  exch_pb_mgr.OrderNew(sec_id, new_order_2);
  BMFPlaybackStruct exec_order_1(side, exec_size_1, id_1);
  exch_pb_mgr.OrderExec(sec_id, exec_order_1);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == -exec_size_1 && (live_orders.size() == 2) &&
                  (IsOrderValid(live_orders[0], side, size_2, id_2) ||
                   IsOrderValid(live_orders[0], side, size_1 - exec_size_1, id_1)) &&
                  (IsOrderValid(live_orders[1], side, size_1 - exec_size_1, id_1) ||
                   IsOrderValid(live_orders[1], side, size_2, id_2)));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// Test GetAllLivePositions() and GetAllLiveOrders() of Exchange Playback manager Class
void BMFPlaybackManagerTests::Test29() {
  int sec_id_1 = 123, sec_id_2 = 234, size = 13, side = 1, id = 15;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;
  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id_1, new_order);
  exch_pb_mgr.OrderNew(sec_id_2, new_order);

  const std::map<int, std::vector<BMFPlaybackStruct> >& sec_id_2_live_orders = exch_pb_mgr.GetAllLiveOrders();
  bool success = (sec_id_2_live_orders.size() == 2);
  for (auto iter = sec_id_2_live_orders.begin(); iter != sec_id_2_live_orders.end(); iter++) {
    success = (success && ((iter->second.size() == 1) && IsOrderValid(iter->second[0], side, size, id)));
  }
  const std::map<int, int>& sec_id_2_live_positions = exch_pb_mgr.GetAllLivePositions();
  for (auto iter = sec_id_2_live_positions.begin(); iter != sec_id_2_live_positions.end(); iter++) {
    success = (success && (iter->second == 0));
  }
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetAllInstances();
}

// New Order, Cancel Replace
void BMFPlaybackManagerTests::Test30() {
  int sec_id = 123, size = 13, side = 1, id = 15, replace_size = 21;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);

  BMFPlaybackStruct cancel_replace_order(side, replace_size, id);
  exch_pb_mgr.OrderCancelReplace(sec_id, cancel_replace_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], side, replace_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Partial Execute, Cancel Replace
void BMFPlaybackManagerTests::Test31() {
  int sec_id = 123, size = 13, side = 1, id = 15, replace_size = 21, exec_size = size - size / 2;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);

  BMFPlaybackStruct exec_order(side, exec_size, id);
  exch_pb_mgr.OrderExec(sec_id, exec_order);

  BMFPlaybackStruct cancel_replace_order(side, replace_size, id);
  exch_pb_mgr.OrderCancelReplace(sec_id, cancel_replace_order);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success = (live_positions == exec_size && (live_orders.size() == 1) &&
                  IsOrderValid(live_orders[0], side, replace_size, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}

// New Order, Cancel Replace, Cancel Replace
void BMFPlaybackManagerTests::Test32() {
  int sec_id = 123, size = 13, side = 1, id = 15, replace_size_1 = 21, replace_size_2 = 31;
  ExchangePlaybackManagerSet<BMFPlaybackManager, BMFPlaybackStruct> exch_pb_mgr;

  BMFPlaybackStruct new_order(side, size, id);
  exch_pb_mgr.OrderNew(sec_id, new_order);

  BMFPlaybackStruct cancel_replace_order_1(side, replace_size_1, id);
  exch_pb_mgr.OrderCancelReplace(sec_id, cancel_replace_order_1);

  BMFPlaybackStruct cancel_replace_order_2(side, replace_size_2, id);
  exch_pb_mgr.OrderCancelReplace(sec_id, cancel_replace_order_2);

  const int live_positions = exch_pb_mgr.GetLivePositions(sec_id);
  const std::vector<BMFPlaybackStruct> live_orders = exch_pb_mgr.GetLiveOrders(sec_id);
  bool success =
      (live_positions == 0 && (live_orders.size() == 1) && IsOrderValid(live_orders[0], side, replace_size_2, id));
  CPPUNIT_ASSERT(success);
  exch_pb_mgr.ResetInstance(sec_id);
}
}
