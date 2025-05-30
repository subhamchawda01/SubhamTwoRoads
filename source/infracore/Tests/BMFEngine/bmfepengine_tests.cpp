#include "infracore/Tests/BMFEngine/bmfepengine_tests.hpp"
#include "dvccode/CDef/order.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPCancelReplace.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPNewOrder.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPCancelOrder.hpp"

namespace HFTEST {
BMFEngineTests::BMFEngineTests() {}

void BMFEngineTests::CheckBuySetDynamicSendOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->server_assigned_order_sequence_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeBuy;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;

  HFSAT::ORS::BMFEPFIX::NewOrder* order = new HFSAT::ORS::BMFEPFIX::NewOrder(
      "DBRP", "DCD", "4505", "COLO", "DVC", "120", "SEC", "SYM", true, last_send_time_, false);

  order->SetDynamicSendOrderFieldsUsingOrderStruct(internalOrder, last_send_time_, last_seq_num_);

  std::string del(1, BMFEP_Delimiter_SOH_);
  std::string message("8=FIX.4.4" + del + "9=000226" + del + "35=D" + del + "34=000000124" + del + "49=DBRP" + del +
                      "52=20170907-05:31:19.000" + del + "56=DCD" + del + "1=4505" + del + "11=000000019" + del +
                      "38=000000200" + del + "40=2" + del + "44=0000070.0000000" + del + "54=1" + del + "55=SYM" + del +
                      "60=20170907-05:31:19.000" + del + "453=3" + del + "448=COLO" + del + "447=D" + del + "452=54" +
                      del + "448=DVC" + del + "447=D" + del + "452=36" + del + "448=120" + del + "447=D" + del +
                      "452=7" + del + "59=0" + del + "10=193" + del);
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void BMFEngineTests::CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->exch_assigned_order_sequence_length_ = 19;
  internalOrder->server_assigned_order_sequence_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeBuy;
  internalOrder->size_executed_ = 5;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  HFSAT::ORS::BMFEPFIX::CancelOrder* order = new HFSAT::ORS::BMFEPFIX::CancelOrder(
      "DBRP", "DCD", "4505", "COLO", "DVC", "120", "SEC", "SYM", true, last_send_time_);
  order->SetDynamicCancelOrderFieldsUsingOrderStruct(internalOrder, last_send_time_, last_seq_num_);
  std::string del(1, BMFEP_Delimiter_SOH_);
  std::string message("8=FIX.4.4" + del + "9=000203" + del + "35=F" + del + "34=000000124" + del + "49=DBRP" + del +
                      "52=20170907-05:31:19.000" + del + "56=DCD" + del + "11=000000019" + del + "41=000000019" + del +
                      "54=1" + del + "55=SYM" + del + "38=000000200" + del + "60=20170907-05:31:19.000" + del +
                      "453=3" + del + "448=COLO" + del + "447=D" + del + "452=54" + del + "448=DVC" + del + "447=D" +
                      del + "452=36" + del + "448=120" + del + "447=D" + del + "452=7" + del + "10=179" + del);
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void BMFEngineTests::CheckBuySetDynamicCancelReplaceOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->server_assigned_order_sequence_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeSell;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  HFSAT::ORS::BMFEPFIX::CancelReplace* order = new HFSAT::ORS::BMFEPFIX::CancelReplace(
      "DBRP", "DCD", "4505", "COLO", "DVC", "120", "SEC", "SYM", true, last_send_time_);
  order->SetDynamicCancelReplaceOrderFieldsUsingOrderStruct(internalOrder, last_send_time_, last_seq_num_);
  std::string del(1, BMFEP_Delimiter_SOH_);
  std::string message("8=FIX.4.4" + del + "9=000244" + del + "35=G" + del + "34=000000124" + del + "49=DBRP" + del +
                      "52=20170907-05:31:19.000" + del + "56=DCD" + del + "41=000000019" + del + "11=000000019" + del +
                      "55=SYM" + del + "54=2" + del + "38=000000200" + del + "40=2" + del + "44=0000070.0000000" + del +
                      "78=1" + del + "79=1" + del + "661=99" + del + "453=3" + del + "448=COLO" + del + "447=D" + del +
                      "452=54" + del + "448=DVC" + del + "447=D" + del + "452=36" + del + "448=120" + del + "447=D" +
                      del + "452=7" + del + "60=20170907-05:31:19.000" + del + "10=019" + del);
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

BMFEngineTests::~BMFEngineTests() {}
}
