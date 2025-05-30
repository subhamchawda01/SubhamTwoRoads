#include "infracore/Tests/ASXEngine/asxengine_tests.hpp"
#include "dvccode/CDef/order.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/ASXT/ASXFIX/ASXNewOrder.hpp"
#include "infracore/ASXT/ASXFIX/ASXCancelOrder.hpp"
#include "infracore/ASXT/ASXFIX/ASXModifyOrder.hpp"
#include <ctime>

namespace HFTEST {
ASXEngineTests::ASXEngineTests() {}

void ASXEngineTests::CheckBuySetDynamicSendOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->server_assigned_order_sequence_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeBuy;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  HFSAT::ORS::ASXFIX::NewOrder* order =
      new HFSAT::ORS::ASXFIX::NewOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL", true,
                                       "DVC_GRP", true, last_send_time_);
  order->SetDynamicSendOrderFieldsUsingOrderStruct(internalOrder, last_send_time_, last_seq_num_);
  char soas[11];
  char buysell[5];
  char sizeremaining[9];
  char price[19];
  memcpy(soas, order->Order_Tag_11_, ASX_FIX_Tag_11_Width_ + 3);
  soas[10] = '\0';
  memcpy(buysell, order->Order_Tag_54_, 4);
  buysell[4] = '\0';
  memcpy(sizeremaining, order->Order_Tag_38_, ASX_FIX_Tag_38_Width_ + 3);
  sizeremaining[8] = '\0';
  memcpy(price, order->Order_Tag_44_, ASX_FIX_Tag_44_Width_ + 3);
  price[18] = '\0';
  CPPUNIT_ASSERT(strcmp(soas, "11=0000019") == 0);
  CPPUNIT_ASSERT(strcmp(buysell, "54=1") == 0);
  CPPUNIT_ASSERT(strcmp(sizeremaining, "38=00200") == 0);
  CPPUNIT_ASSERT(strcmp(price, "44=0000070.0000000") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0218" + del + "35=D" + del + "34=0000124" + del + "43=N" + del +
                      "49=DVC" + del + "56=ASX" + del + "52=20170907-05:31:19" + del + "122=20170907-05:31:19" + del +
                      "1=1983H" + del + "581=2" + del + "60=20170907-05:31:19" + del + "11=0000019" + del + "38=00200" +
                      del + "40=2" + del + "44=0000070.0000000" + del + "54=1" + del + "55=SYMBOL" + del + "18=o" +
                      del + "453=1" + del + "448=DVC_GRP" + del + "447=D" + del + "452=24" + del + "58=NewOrd" + del +
                      "10=095" + del);
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void ASXEngineTests::CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->exch_assigned_order_sequence_length_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeBuy;
  internalOrder->size_executed_ = 5;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  int new_client_orderid = 1234;
  HFSAT::ORS::ASXFIX::CancelOrder* order =
      new HFSAT::ORS::ASXFIX::CancelOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL",
                                          true, "DVC_GRP", true, last_send_time_);
  order->SetDynamicCancelOrderFieldsUsingOrderStruct(internalOrder, new_client_orderid, last_send_time_, last_seq_num_);
  char OrderCanceTag_11[11];
  char OrderCanceTag_52[5];
  char OrderCanceTag_38[9];
  memcpy(OrderCanceTag_11, order->OrderCancel_Tag_11_, ASX_FIX_Tag_11_Width_ + 3);
  OrderCanceTag_11[10] = '\0';
  memcpy(OrderCanceTag_52, order->OrderCancel_Tag_54_, 4);
  OrderCanceTag_52[4] = '\0';
  memcpy(OrderCanceTag_38, order->OrderCancel_Tag_38_, 8);
  OrderCanceTag_38[8] = '\0';
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_11, "11=0001234") == 0);
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_52, "54=1") == 0);
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_38, "38=00205") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0161" + del + "35=F" + del + "34=0000124" + del + "49=DVC" + del +
                      "56=ASX" + del + "52=20170907-05:31:19" + del + "11=0001234" + del + "54=1" + del +
                      "60=20170907-05:31:19" + del + "55=SYMBOL" + del + "453=1" + del + "448=DVC_GRP" + del + "447=D" +
                      del + "452=24" + del + "38=00205" + del + "37=");
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void ASXEngineTests::CheckBuySetDynamicModifyOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->exch_assigned_order_sequence_length_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeBuy;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  int new_client_orderid = 1234;
  HFSAT::ORS::ASXFIX::ModifyOrder* order =
      new HFSAT::ORS::ASXFIX::ModifyOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL",
                                          true, "DVC_GRP", true, last_send_time_);
  order->SetDynamicCancelReplaceOrderFieldsUsingOrderStruct(internalOrder, new_client_orderid, last_send_time_,
                                                            last_seq_num_);
  char Order_Tag_11_[11];
  char Order_Tag_38_[9];
  char Order_Tag_44_[19];
  char Order_Tag_54_[5];
  memcpy(Order_Tag_11_, order->Order_Tag_11_, 10);
  Order_Tag_11_[10] = '\0';
  memcpy(Order_Tag_38_, order->Order_Tag_38_, 9);
  Order_Tag_38_[8] = '\0';
  memcpy(Order_Tag_44_, order->Order_Tag_44_, 18);
  Order_Tag_44_[18] = '\0';
  memcpy(Order_Tag_54_, order->Order_Tag_54_, 4);
  Order_Tag_54_[4] = '\0';
  CPPUNIT_ASSERT(strcmp(Order_Tag_11_, "11=0001234") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_38_, "38=00200") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_44_, "44=0000070.0000000") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_54_, "54=1") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0226" + del + "35=G" + del + "34=0000124" + del + "43=N" + del +
                      "49=DVC" + del + "56=ASX" + del + "52=20170907-05:31:19" + del + "122=20170907-05:31:19" + del +
                      "11=0001234" + del + "38=00200" + del + "44=0000070.0000000" + del + "54=1" + del + "55=SYMBOL" +
                      del + "1=1983H" + del + "60=20170907-05:31:19" + del + "581=2" + del + "453=1" + del +
                      "448=DVC_GRP" + del + "447=D" + del + "452=24" + del + "40=2" + del + "37=");
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void ASXEngineTests::CheckSelSetDynamicSendOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->server_assigned_order_sequence_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeSell;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time = 1504762279;
  int last_seq_num_ = 124;
  HFSAT::ORS::ASXFIX::NewOrder* order =
      new HFSAT::ORS::ASXFIX::NewOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL", true,
                                       "DVC_GRP", true, last_send_time);
  order->SetDynamicSendOrderFieldsUsingOrderStruct(internalOrder, last_send_time, last_seq_num_);
  char soas[11];
  char buysell[5];
  char sizeremaining[9];
  char price[19];
  memcpy(soas, order->Order_Tag_11_, ASX_FIX_Tag_11_Width_ + 3);
  soas[10] = '\0';
  memcpy(buysell, order->Order_Tag_54_, 4);
  buysell[4] = '\0';
  memcpy(sizeremaining, order->Order_Tag_38_, ASX_FIX_Tag_38_Width_ + 3);
  sizeremaining[8] = '\0';
  memcpy(price, order->Order_Tag_44_, ASX_FIX_Tag_44_Width_ + 3);
  price[18] = '\0';
  CPPUNIT_ASSERT(strcmp(soas, "11=0000019") == 0);
  CPPUNIT_ASSERT(strcmp(buysell, "54=2") == 0);
  CPPUNIT_ASSERT(strcmp(sizeremaining, "38=00200") == 0);
  CPPUNIT_ASSERT(strcmp(price, "44=0000070.0000000") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0218" + del + "35=D" + del + "34=0000124" + del + "43=N" + del +
                      "49=DVC" + del + "56=ASX" + del + "52=20170907-05:31:19" + del + "122=20170907-05:31:19" + del +
                      "1=1983H" + del + "581=2" + del + "60=20170907-05:31:19" + del + "11=0000019" + del + "38=00200" +
                      del + "40=2" + del + "44=0000070.0000000" + del + "54=2" + del + "55=SYMBOL" + del + "18=o" +
                      del + "453=1" + del + "448=DVC_GRP" + del + "447=D" + del + "452=24" + del + "58=NewOrd" + del +
                      "10=096" + del);
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void ASXEngineTests::CheckSelSetDynamicCancelOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->exch_assigned_order_sequence_length_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeSell;
  internalOrder->size_executed_ = 5;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  int new_client_orderid = 1234;
  HFSAT::ORS::ASXFIX::CancelOrder* order =
      new HFSAT::ORS::ASXFIX::CancelOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL",
                                          true, "DVC_GRP", true, last_send_time_);
  order->SetDynamicCancelOrderFieldsUsingOrderStruct(internalOrder, new_client_orderid, last_send_time_, last_seq_num_);
  char OrderCanceTag_11[11];
  char OrderCanceTag_52[5];
  char OrderCanceTag_38[9];
  memcpy(OrderCanceTag_11, order->OrderCancel_Tag_11_, ASX_FIX_Tag_11_Width_ + 3);
  OrderCanceTag_11[10] = '\0';
  memcpy(OrderCanceTag_52, order->OrderCancel_Tag_54_, 4);
  OrderCanceTag_52[4] = '\0';
  memcpy(OrderCanceTag_38, order->OrderCancel_Tag_38_, 8);
  OrderCanceTag_38[8] = '\0';
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_11, "11=0001234") == 0);
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_52, "54=2") == 0);
  CPPUNIT_ASSERT(strcmp(OrderCanceTag_38, "38=00205") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0161" + del + "35=F" + del + "34=0000124" + del + "49=DVC" + del +
                      "56=ASX" + del + "52=20170907-05:31:19" + del + "11=0001234" + del + "54=2" + del +
                      "60=20170907-05:31:19" + del + "55=SYMBOL" + del + "453=1" + del + "448=DVC_GRP" + del + "447=D" +
                      del + "452=24" + del + "38=00205" + del + "37=");
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

void ASXEngineTests::CheckSelSetDynamicModifyOrderFieldsUsingOrderStruct() {
  HFSAT::ORS::Order* internalOrder = new HFSAT::ORS::Order;
  internalOrder->exch_assigned_order_sequence_length_ = 19;
  internalOrder->buysell_ = HFSAT::kTradeTypeSell;
  internalOrder->price_ = 70;
  internalOrder->size_remaining_ = 200;
  time_t last_send_time_ = 1504762279;
  int last_seq_num_ = 124;
  int new_client_orderid = 1234;
  HFSAT::ORS::ASXFIX::ModifyOrder* order =
      new HFSAT::ORS::ASXFIX::ModifyOrder("DVC", "ASX", "ASX", "DVA", "1983H", "NY4", "ASX", "SECURITY_ID", "SYMBOL",
                                          true, "DVC_GRP", true, last_send_time_);
  order->SetDynamicCancelReplaceOrderFieldsUsingOrderStruct(internalOrder, new_client_orderid, last_send_time_,
                                                            last_seq_num_);
  char Order_Tag_11_[11];
  char Order_Tag_38_[9];
  char Order_Tag_44_[19];
  char Order_Tag_54_[5];
  memcpy(Order_Tag_11_, order->Order_Tag_11_, 10);
  Order_Tag_11_[10] = '\0';
  memcpy(Order_Tag_38_, order->Order_Tag_38_, 9);
  Order_Tag_38_[8] = '\0';
  memcpy(Order_Tag_44_, order->Order_Tag_44_, 18);
  Order_Tag_44_[18] = '\0';
  memcpy(Order_Tag_54_, order->Order_Tag_54_, 4);
  Order_Tag_54_[4] = '\0';
  CPPUNIT_ASSERT(strcmp(Order_Tag_11_, "11=0001234") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_38_, "38=00200") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_44_, "44=0000070.0000000") == 0);
  CPPUNIT_ASSERT(strcmp(Order_Tag_54_, "54=2") == 0);
  std::string del(1, ASX_Delimiter_SOH_);
  std::string message("8=FIXT.1.1" + del + "9=0226" + del + "35=G" + del + "34=0000124" + del + "43=N" + del +
                      "49=DVC" + del + "56=ASX" + del + "52=20170907-05:31:19" + del + "122=20170907-05:31:19" + del +
                      "11=0001234" + del + "38=00200" + del + "44=0000070.0000000" + del + "54=2" + del + "55=SYMBOL" +
                      del + "1=1983H" + del + "60=20170907-05:31:19" + del + "581=2" + del + "453=1" + del +
                      "448=DVC_GRP" + del + "447=D" + del + "452=24" + del + "40=2" + del + "37=");
  CPPUNIT_ASSERT(strcmp(order->msg_char_buf_, message.c_str()) == 0);
}

ASXEngineTests::~ASXEngineTests() {}
}
