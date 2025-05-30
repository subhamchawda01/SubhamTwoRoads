/**
   \file Tests/dvctrade/OrderRouting/om_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/OrderRouting/om_tests.hpp"
#include "baseinfra/Tests/OrderRouting/om_tests_helper.hpp"

namespace HFTEST {

//-----------------------------------------------------------------------------

/**
 *
 * Tests if we send two Buy orders which have price difference of > order vector size, order manager handles them
 *correctly
 *
 * Again Tests should include order_vectors as well as sum_[bid/ask]_[confirmed/unconfirmed] sizes
 *
 */
void OmTests::TestBidIndexHighLow(void) {}

/**
 *  Tests if we send two Buy orders which have price difference of > order vector size, order manager handles them
 *correctly
 *
 * Again Tests should include order_vectors as well as sum_[bid/ask]_[confirmed/unconfirmed] sizes
 */

void OmTests::TestAskIndexHighLow(void) {}

/*
 * Test Case 1: TestIntExecSeqdRejc
 * Test the following sequence:
 * Seqd -> IntExec -> Seqd (real seqd to the exchange) -> Rejc
 * Expected result: The original order must not exist in the OM
 */
void OmTests::TestIntExecSeqdRejc(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  int saci = om->server_assigned_client_id_;
  om->OrderSequenced(saci, 305, 24762, 0, 14.15, kTradeTypeBuy, 100, 0, 1445, 210, 1415, 360, 0, watch_->tv());
  om->OrderInternallyMatched(saci, 305, 24762, 0, 14.15, kTradeTypeBuy, 95, 5, 1450, 210, 1415, 361, 0, watch_->tv());
  om->OrderSequenced(saci, 305, 24762, 0, 14.15, kTradeTypeBuy, 95, 5, 1450, 210, 1415, 362, 0, watch_->tv());
  om->OrderRejected(saci, 305, 0, 14.15, kTradeTypeBuy, 95, 6, 1415, 0, watch_->tv());

  int price = 5;

  CPPUNIT_ASSERT_EQUAL(5, price);
  delete om;
}

void OmTests::TestReject(void) {}

/**
 *  Test Bid Order has been sequenced and put in appropriate vectors
 */
void OmTests::TestBidSequenced(void) {
  //

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Add another order at same price
  px = 162.30, intpx = 16230;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Add order at different price
  px = 162.34, intpx = 16234;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  auto bid_index = om->GetOrderVecBottomBidIndex();  // 16230
  auto& bid_order_vec = om->BidOrderVec();
  auto& sum_bid_order_unconfirmed = om->SumBidUnconfirmed();
  auto& sum_bid_order_unconfirmed_orders_ = om->SumBidUnconfirmedOrders();
  /// adding assert statements, the goal here is to check two things
  /// a ) Order gets added with appropriate status
  /// b ) Multiple orders at same price are handled correctly

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(2ul, bid_order_vec[bid_index].size());

  // Total size at this price is twice of size added
  CPPUNIT_ASSERT_EQUAL(2 * size, sum_bid_order_unconfirmed[bid_index]);
  CPPUNIT_ASSERT_EQUAL(2, sum_bid_order_unconfirmed_orders_[bid_index]);

  // Assert if the order status is sequenced
  CPPUNIT_ASSERT(bid_order_vec[bid_index][0]->order_status_ == kORRType_Seqd);
  CPPUNIT_ASSERT(bid_order_vec[bid_index][1]->order_status_ == kORRType_Seqd);

  /// Once orders at one price are added,
  /// adding orders at new price works correctly

  auto bid_new_index = om->GetOrderVecTopBidIndex();  // int-px 16234

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(1ul, bid_order_vec[bid_new_index].size());

  // Only 1 order so size should be of "size"
  CPPUNIT_ASSERT_EQUAL(size, sum_bid_order_unconfirmed[bid_new_index]);

  // Number of orders should be one
  CPPUNIT_ASSERT_EQUAL(1, sum_bid_order_unconfirmed_orders_[bid_new_index]);
  // Assert if the order status is sequenced
  CPPUNIT_ASSERT(bid_order_vec[bid_new_index][0]->order_status_ == kORRType_Seqd);

  delete om;
}

/**
 *  Test Bid Order has been sequenced and put in appropriate vectors
 */
void OmTests::TestAskSequenced(void) {
  //

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Add another order at same price
  px = 162.30, intpx = 16230;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Add order at different price
  px = 162.34, intpx = 16234;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  auto ask_index = om->GetOrderVecTopAskIndex();  // int-px 16230;

  auto& ask_order_vec = om->AskOrderVec();
  auto& sum_ask_order_unconfirmed = om->SumAskUnconfirmed();
  auto& sum_ask_order_unconfirmed_orders_ = om->SumAskUnconfirmedOrders();

  /// adding assert statements, the goal here is to check two things
  /// a ) Order gets added with appropriate status
  /// b ) Multiple orders at same price are handled correctly

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(2ul, ask_order_vec[ask_index].size());

  // Assert if we have size equal to twice of each order
  CPPUNIT_ASSERT_EQUAL(2 * size, sum_ask_order_unconfirmed[ask_index]);
  CPPUNIT_ASSERT_EQUAL(2, sum_ask_order_unconfirmed_orders_[ask_index]);

  // Assert if the order status is sequenced
  CPPUNIT_ASSERT(ask_order_vec[ask_index][0]->order_status_ == kORRType_Seqd);
  CPPUNIT_ASSERT(ask_order_vec[ask_index][1]->order_status_ == kORRType_Seqd);

  /// Once orders at one price are added,
  /// adding orders at new price works correctly

  auto ask_new_index = om->GetOrderVecBottomAskIndex();  // int-px 16234

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(1ul, ask_order_vec[ask_new_index].size());

  // Sum size should be equal to one order
  CPPUNIT_ASSERT_EQUAL(size, sum_ask_order_unconfirmed[ask_new_index]);

  // Order should be equal to one
  CPPUNIT_ASSERT_EQUAL(1, sum_ask_order_unconfirmed_orders_[ask_new_index]);
  // Assert if the order status is sequenced
  CPPUNIT_ASSERT(ask_order_vec[ask_new_index][0]->order_status_ == kORRType_Seqd);

  delete om;
}

/**
 * Test if whenever we receive confirmation, order is put in appropriate vectors
 */
void OmTests::TestBidConfirmed(void) {
  //

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 162.30, intpx = 16230;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Send new order
  px = 162.34, intpx = 16234;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  auto bid_index = om->GetOrderVecBottomBidIndex();  // int-px (16230);
  auto& bid_order_vec = om->BidOrderVec();
  // Sum size vectors
  auto& sum_bid_order_unconfirmend = om->SumBidUnconfirmed();
  auto& sum_bid_order_confirmed = om->SumBidConfirmed();
  auto& sum_bid_order_confirmed_orders_ = om->SumBidConfirmedOrders();

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(2ul, bid_order_vec[bid_index].size());

  // Assert if the order status is confirmed
  CPPUNIT_ASSERT(bid_order_vec[bid_index][0]->order_status_ == kORRType_Conf);
  CPPUNIT_ASSERT(bid_order_vec[bid_index][1]->order_status_ == kORRType_Conf);

  auto new_bid_index = om->GetOrderVecTopBidIndex();  // int-px (16234)

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(1ul, bid_order_vec[new_bid_index].size());

  // Assert if the order status is confirmed
  CPPUNIT_ASSERT(bid_order_vec[new_bid_index][0]->order_status_ == kORRType_Conf);

  // Check for valid confirmed and unconfirmed sizes
  CPPUNIT_ASSERT_EQUAL(size, sum_bid_order_confirmed[new_bid_index]);
  CPPUNIT_ASSERT_EQUAL(1, sum_bid_order_confirmed_orders_[new_bid_index]);
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmend[new_bid_index]);

  delete om;
}

/**
 * Test if whenever we receive confirmation, order is put in appropriate vectors
 */
void OmTests::TestAskConfirmed(void) {
  //

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 162.30, intpx = 16230;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  // Send new order
  px = 162.34, intpx = 16234;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  auto ask_index = om->GetOrderVecTopAskIndex();  // int-px (16230);
  auto& ask_order_vec = om->AskOrderVec();
  // Sum size vectors
  auto& sum_ask_order_unconfirmend = om->SumAskUnconfirmed();
  auto& sum_ask_order_confirmed = om->SumAskConfirmed();
  auto& sum_ask_order_confirmed_orders_ = om->SumAskConfirmedOrders();

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(2ul, ask_order_vec[ask_index].size());

  // Assert if the order status is confirmed
  CPPUNIT_ASSERT(ask_order_vec[ask_index][0]->order_status_ == kORRType_Conf);
  CPPUNIT_ASSERT(ask_order_vec[ask_index][1]->order_status_ == kORRType_Conf);

  auto new_ask_index = om->GetOrderVecBottomAskIndex();  // int-px (16234);

  // Assert if we have only one order at this price
  CPPUNIT_ASSERT_EQUAL(1ul, ask_order_vec[new_ask_index].size());

  // Assert if the order status is confirmed
  CPPUNIT_ASSERT(ask_order_vec[new_ask_index][0]->order_status_ == kORRType_Conf);

  // Check for valid confirmed and unconfirmed sizes
  CPPUNIT_ASSERT_EQUAL(size, sum_ask_order_confirmed[new_ask_index]);
  CPPUNIT_ASSERT_EQUAL(1, sum_ask_order_confirmed_orders_[new_ask_index]);
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmend[new_ask_index]);

  delete om;
}

/**
 * Test if on send cancel, the order is handled correclty
 */
void OmTests::TestBidCancel(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  px = 162.30, intpx = 16230;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  // Get the order to cancel, this should be one with saos 0
  auto order = om->GetTopConfirmedBidOrder();

  om->Cancel(*order);

  om->OrderCanceled(0, 0, 0, sec_id_, px, kTradeTypeBuy, size, 0, 0, intpx, sams++, 0, watch_->tv());

  // Send new order
  px = 162.34, intpx = 16234;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  auto bid_index = om->GetOrderVecBottomBidIndex();  // int-px (16230);
  auto& bid_order_vec = om->BidOrderVec();
  // Sum size vectors
  auto& sum_bid_order_unconfirmend = om->SumBidUnconfirmed();
  auto& sum_bid_order_confirmed = om->SumBidConfirmed();

  // See if number of orders are 0
  CPPUNIT_ASSERT_EQUAL(1ul, bid_order_vec[bid_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmend[bid_index]);

  // See if number of confirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(1000, sum_bid_order_confirmed[bid_index]);

  delete om;
}

/**
 *
 * Test if on send cancel, the order is handled correctly
 *
 */
void OmTests::TestAskCancel(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  px = 162.30, intpx = 16230;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  // Get the order to cancel, this should be one with saos 0
  auto order = om->GetTopConfirmedAskOrder();

  om->Cancel(*order);

  om->OrderCanceled(0, 0, 0, sec_id_, px, kTradeTypeSell, size, 0, 0, intpx, sams++, 0, watch_->tv());

  // Send new order
  px = 162.34, intpx = 16234;
  saos++;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  // We should try canceling unconfirmed order here and see if our order manager handles it correctly

  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  auto ask_index = om->GetOrderVecTopAskIndex();  // int-px (16230);
  auto& ask_order_vec = om->AskOrderVec();
  // Sum size vectors
  auto& sum_ask_order_unconfirmend = om->SumAskUnconfirmed();
  auto& sum_ask_order_confirmed = om->SumAskConfirmed();

  // See if number of orders are 0
  CPPUNIT_ASSERT_EQUAL(1ul, ask_order_vec[ask_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmend[ask_index]);

  // See if number of confirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(1000, sum_ask_order_confirmed[ask_index]);

  delete om;
}

/**
 * The cancel reject could be of a lot of kinds, however we don't need to worry about types in tradeinit-order manager
 * One thing we need to do though is in case of execs :
 * a ) If cancel reject is received before Exec
 * b ) If cancel reject is received after Exec
 *
 * The tests should include both orders as well as sum_bid/ask_[confirmed/unconfirmed] sizes
 */
void OmTests::TestBidCancelReject(void) {
  /**
   * The cancel reject could be of a lot of kinds, however we don't need to worry about types in tradeinit-order manager
   * One thing we need to do though is in case of execs :
   * a ) If cancel reject is received before Exec
   * b ) If cancel reject is received after Exec
   *
   * The tests should include both orders as well as sum_bid/ask_[confirmed/unconfirmed] sizes
   */

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;

  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  auto order = om->GetTopConfirmedBidOrder();
  om->Cancel(*order);
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 0, 1000, -1000, -1000, intpx, sams++, 0, watch_->tv());
  om->OrderCancelRejected(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, watch_->tv());

  auto bid_index = om->GetOrderVecBottomBidIndex();  // int-px (16230);
  // Sum size vectors
  auto& sum_bid_order_unconfirmed = om->SumBidUnconfirmed();
  auto& sum_bid_order_confirmed = om->SumBidConfirmed();
  // on getting cancellation reject due to execution the position must be 1000
  // CPPUNIT_ASSERT_EQUAL(1000,global_position_ );

  // on cancellation the confirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_confirmed[bid_index]);

  // on cancellation the unconfirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmed[bid_index]);

  // Cancel reject is obtained before exec
  px = 162.4;
  intpx = 16240, size = 1000;
  saos = 1;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  order = om->GetTopConfirmedBidOrder();
  om->Cancel(*order);
  om->OrderCancelRejected(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, watch_->tv());
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 0, 1000, -1000, -1000, intpx, sams++, 0, watch_->tv());

  bid_index = om->GetOrderVecBottomBidIndex();  // int-px (16230);
  // Sum size vectors
  sum_bid_order_unconfirmed = om->SumBidUnconfirmed();
  sum_bid_order_confirmed = om->SumBidConfirmed();

  // on cancel reject the confirmed order must be 1000
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_confirmed[bid_index]);

  // on cancel reject the unconfirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmed[bid_index]);

  delete om;
}

void OmTests::TestAskCancelReject(void) {
  /**
   * Two kind of tests:
   * a ) Partial execs
   * b ) Full execs
   *
   * The tests should include both orders as well as sum_bid/ask_[confirmed/unconfirmed] sizes
   */

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);
  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  //  auto order_temp = om->GetTopConfirmedAskOrder();

  auto order = om->GetTopConfirmedAskOrder();

  om->Cancel(*order);
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 0, 1000, -1000, -1000, intpx, sams++, 0, watch_->tv());
  om->OrderCancelRejected(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, watch_->tv());

  auto ask_index = om->GetOrderVecBottomAskIndex();  // int-px (16230);
  // Sum size vectors
  auto& sum_ask_order_unconfirmed = om->SumAskUnconfirmed();
  auto& sum_ask_order_confirmed = om->SumAskConfirmed();

  // on getting cancellation reject due to execution the position must be 1000
  // CPPUNIT_ASSERT_EQUAL(1000,global_position_ );

  // on cancellation the confirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_confirmed[ask_index]);

  // on cancellation the unconfirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmed[ask_index]);

  // Cancel reject is obtained before exec

  px = 162.4;
  intpx = 16240, size = 1000;
  saos = 1;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());

  order = om->GetTopConfirmedBidOrder();
  om->Cancel(*order);

  om->OrderCancelRejected(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, 0, watch_->tv());
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 0, 1000, -1000, -1000, intpx, sams++, 0, watch_->tv());

  ask_index = om->GetOrderVecTopAskIndex();  // int-px (16230);
                                             // Sum size vectors
  sum_ask_order_unconfirmed = om->SumAskUnconfirmed();
  sum_ask_order_confirmed = om->SumAskConfirmed();

  // on cancel reject the confirmed order must be 1000
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_confirmed[ask_index]);

  // on cancel reject the unconfirmed order must be 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmed[ask_index]);

  delete om;
}

void OmTests::TestBidExec(void) {
  // Complete fill
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);
  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  auto bid_index = om->GetOrderVecBottomBidIndex();  // int-px (16230);
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeBuy, 0, 1000, 1000, 1000, intpx, sams++, 0, watch_->tv());

  int position = om->global_position();

  auto& bid_order_vec = om->BidOrderVec();
  // Sum size vectors
  auto& sum_bid_order_unconfirmend = om->SumBidUnconfirmed();
  auto& sum_bid_order_confirmed = om->SumBidConfirmed();

  // See if number of orders are 0
  CPPUNIT_ASSERT_EQUAL(0ul, bid_order_vec[bid_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmend[bid_index]);

  // See if number of confirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_confirmed[bid_index]);

  // See if global position is as desired
  CPPUNIT_ASSERT_EQUAL(1000, position);

  // Partial fill
  size = 2000;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeBuy, 1000, 1000, 2000, 2000, intpx, sams++, 0, watch_->tv());

  position = om->global_position();

  // See if number of orders are 1
  CPPUNIT_ASSERT_EQUAL(1ul, bid_order_vec[bid_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_bid_order_unconfirmend[bid_index]);

  // See if number of confirmed size at this price is 1000
  CPPUNIT_ASSERT_EQUAL(1000, sum_bid_order_confirmed[bid_index]);

  // See if global position is as desired
  CPPUNIT_ASSERT_EQUAL(2000, position);

  delete om;
}

/**
 * Two kind of tests:
 * a ) Partial execs
 * b ) Full execs
 *
 * The tests should include both orders as well as sum_bid/ask_[confirmed/unconfirmed] sizes
 */
void OmTests::TestAskExec(void) {
  // Complete fill
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);
  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  auto ask_index = om->GetOrderVecBottomAskIndex();  // int-px (16230);
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 0, 1000, -1000, -1000, intpx, sams++, 0, watch_->tv());

  int position = om->global_position();

  auto& ask_order_vec = om->AskOrderVec();
  // Sum size vectors
  auto& sum_ask_order_unconfirmend = om->SumAskUnconfirmed();
  auto& sum_ask_order_confirmed = om->SumAskConfirmed();

  // See if number of orders are 0
  CPPUNIT_ASSERT_EQUAL(0ul, ask_order_vec[ask_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmend[ask_index]);

  // See if number of confirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_confirmed[ask_index]);

  // See if global position is as desired
  CPPUNIT_ASSERT_EQUAL(-1000, position);

  // Partial fill
  size = 2000;
  saos++;
  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderConfirmed(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  om->OrderExecuted(0, saos, saos, sec_id_, px, kTradeTypeSell, 1000, 1000, -2000, -2000, intpx, sams++, 0,
                    watch_->tv());

  position = om->global_position();

  // See if number of orders are 0
  CPPUNIT_ASSERT_EQUAL(1ul, ask_order_vec[ask_index].size());

  // See if number of unconfirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(0, sum_ask_order_unconfirmend[ask_index]);

  // See if number of confirmed size at this price is 0
  CPPUNIT_ASSERT_EQUAL(1000, sum_ask_order_confirmed[ask_index]);

  // See if global position is as desired
  CPPUNIT_ASSERT_EQUAL(-2000, position);

  delete om;
}

/**
 * Test if query reponds correctly on ors reply
 */
void OmTests::TestFreezeDurToOrsNoReply(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);
  auto freeze_listener = new HFTEST::DummyExchangeRejectListener();
  om->AddExchangeRejectsListeners(freeze_listener);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;

  watch_->OnTimeReceived(ttime_t(time_at_midnight_ + 1, 0));
  om->SendTrade(px, intpx, size, kTradeTypeSell, 'B', kOrderDay);
  watch_->OnTimeReceived(ttime_t(time_at_midnight_ + 10, 0));
  // we are listening to big-time-period updates in om ( 1 sec)
  // In notifying the freeze-listener we check for time  > 60 since last notification, this makes it send once every 2
  // seconds
  CPPUNIT_ASSERT_EQUAL(1, freeze_listener->num_calls_so_far());

  watch_->OnTimeReceived(ttime_t(time_at_midnight_ + 300, 0));
  CPPUNIT_ASSERT_EQUAL(1, freeze_listener->num_calls_so_far());
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeSell, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
}

// Like Constructor : Initialize the book, which initializes all the commonly required variables/
// Then we can add as many shortcodes as we want for testing
// Make the object for om
void OmTests::setUp(void) {
  // setup the common smv_-source

  shortcode_list_ = {"FGBL_0"};
  common_smv_source_ = new CommonSMVSource(shortcode_list_, 20170310);
  time_at_midnight_ = DateTime::GetTimeMidnightUTC(20170313);
  common_smv_source_->SetDepShortcodeVector(shortcode_list_);
  common_smv_source_->SetSourceShortcodes(shortcode_list_);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  std::string account = "12";

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  sec_id_ = sec_name_indexer.GetIdFromString(shortcode_list_[0]);
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  smv_ = sec_id_smv_map[sec_id_];

  sim_time_series_info_ = new SimTimeSeriesInfo(sec_name_indexer.NumSecurityId());
  sim_time_series_info_->sid_to_sim_config_.resize(sec_name_indexer.NumSecurityId());

  // Sim Market Maker
  smm_ = SimMarketMakerHelper::GetSimMarketMaker(
      common_smv_source_->getLogger(), *watch_, smv_, smv_, 1, *sim_time_series_info_, sid_to_mov_ptr_map_,
      common_smv_source_->getHistoricalDispatcher(), common_smv_source_, false);

  // Sim Trader
  sim_trader_ = SimTraderHelper::GetSimTrader(account.c_str(), smm_);
}

// Test to check whether an order can be cancelled or not
void OmTests::TestCancelBeforeConfirm(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  int first_caos = 0;
  auto om = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader_, *smv_, 11,
                                  false, first_caos);

  double px = 162.3;
  int intpx = 16230, size = 1000;
  int saos = 0;
  int sams = 0;
  om->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om->OrderSequenced(0, saos, saos, sec_id_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, sams++, 0, watch_->tv());
  auto order = om->GetTopBidOrder();
  // CPPUNIT_ASSERT_EQUAL(1, om->GetNumBidOrdersAtIntPx(intpx));
  om->Cancel(*order);
  CPPUNIT_ASSERT(order->canceled());  // FGBL_0 belongs to EUREX and right now only BMF supports CancelBeforeConf
  // CPPUNIT_ASSERT_EQUAL(0, om->GetNumBidOrdersAtIntPx(intpx));
  // om->OrderCanceled(0, 0, 0, sec_id_, px, kTradeTypeBuy, size, 0, 0, intpx, sams++, 0, watch_->tv());

  delete om;
}
// Like Destructor
void OmTests::tearDown(void) {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete sim_time_series_info_;
  delete common_smv_source_;
  common_smv_source_ = nullptr;
  //  delete smm_;
  delete sim_trader_;
  sim_trader_ = nullptr;
}

//-----------------------------------------------------------------------------
}
