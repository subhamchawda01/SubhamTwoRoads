// =====================================================================================
//
//       Filename:  order_modify_tests.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/16/2015 05:04:13 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "baseinfra/Tests/OrderRouting/order_modify_tests.hpp"

namespace HFTEST {

void OrderModifyTests::TestPriceModify(void) {
  double px = 12.3;
  int intpx = 1230, size = 1000;
  int saos = 0;

  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 12.31, intpx = 1231;
  saos++;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 12.34, intpx = 1234;
  saos++;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 12.36, intpx = 1236;
  saos++;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  std::vector<double> px_vec = {12.33, 12.35};
  std::vector<int> intpx_vec = {1233, 1235};
  std::vector<int> size_vec = {500, 500};
  std::vector<char> oli_vec = {'B', 'B'};

  // om_->CancelReplaceBidOrdersEqAboveAndEqBelowIntPrice( 1236, 1231, px_vec_, intpx_vec_, size_vec_, oli_vec_,
  // kOrderDay );

  BaseOrder* order = om_->GetTopConfirmedBidOrder();
  px = 12.35, intpx = 1235, size = 100;

  om_->Modify(order, px, intpx, size);
  om_->OrderConfCxlReplaced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  order = om_->GetTopConfirmedBidOrder();
  int size1 = om_->SumBidSizeConfirmedAboveIntPrice(1234);
  CPPUNIT_ASSERT(size1 == 100);
  CPPUNIT_ASSERT(order->int_price_ == 1235);
}

void OrderModifyTests::TestSizeModify(void) {
  double px = 12.3;
  int intpx = 1230, size = 1000;
  int saos = 0;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 12.31, intpx = 1231;
  saos++;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  px = 12.34, intpx = 1234;
  saos++;
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  BaseOrder* order = om_->GetTopConfirmedBidOrder();
  px = 12.34, intpx = 1234, size = 100;
  om_->Modify(order, px, intpx, size);
  om_->OrderConfCxlReplaced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());

  int size1 = om_->SumBidSizeConfirmedAboveIntPrice(1230);
  CPPUNIT_ASSERT(size1 == 1100);
}

void OrderModifyTests::setUp(void) {
  // setup the common smv-source

  std::vector<std::string> shortcode_list = {"VALE5"};
  common_smv_source_ = new CommonSMVSource(shortcode_list, 20150527);
  common_smv_source_->SetDepShortcodeVector(shortcode_list);
  common_smv_source_->SetSourceShortcodes(shortcode_list);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();

  std::string account = "12";
  bool livetrading = false;

  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  SimTimeSeriesInfo sim_time_series_info(sec_name_indexer.NumSecurityId());
  sim_time_series_info.sid_to_sim_config_.resize(sec_name_indexer.NumSecurityId());

  auto sec_id = sec_name_indexer.GetIdFromString("VALE5");
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  auto smv = sec_id_smv_map[sec_id];

  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_;
  // Sim Market Maker
  smm_ = SimMarketMakerHelper::GetSimMarketMaker(
      common_smv_source_->getLogger(), *watch_, smv, smv, 1, sim_time_series_info, sid_to_mov_ptr_map_,
      common_smv_source_->getHistoricalDispatcher(), common_smv_source_, false);

  // Sim Trader
  sim_trader = SimTraderHelper::GetSimTrader(account.c_str(), smm_);

  int first_caos = 0;
  om_ = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader, *smv, 11,
                              livetrading, first_caos);

  secid_ = sec_id;
}

void OrderModifyTests::tearDown(void) {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete common_smv_source_;
  delete om_;
  delete sim_trader;
}
}
