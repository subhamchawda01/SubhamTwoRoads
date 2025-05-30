// =====================================================================================
//
//       Filename:  dvctrade/Tests/BaseTrading/base_trading_tests.cpp
//
//       Description:  Tests for Collect Shortcode, OnPosition Change,Get Flat, Allowed Eco Events of BaseTrading class
//
//        Version:  1.0
//        Created:  08/02/2017 04:31:28 PM
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

#include <cstdio>
#include <ctime>
#include <stdlib.h>

#include "dvctrade/Tests/BaseTrading/base_trading_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void BaseTradingTests::setUp(void) {
  date_to_run = 20170315;
  std::string shortcode = "FGBL_0";
  std::vector<std::string> shortcode_list = {shortcode};
  common_smv_source_ = new CommonSMVSource(shortcode_list, date_to_run);
  common_smv_source_->SetDepShortcodeVector(shortcode_list);
  common_smv_source_->SetSourceShortcodes(shortcode_list);
  common_smv_source_->InitializeVariables();

  // Set indicator list map.
  HFSAT::SetIndicatorListMap();

  watch_ = &common_smv_source_->getWatch();

  std::string account = "12";
  bool livetrading = false;



  //sim_time_series_info_ptr_ =
  sim_time_series_info_ptr_->sid_to_sim_config_.resize(sec_name_indexer.NumSecurityId());

  auto sec_id = sec_name_indexer.GetIdFromString(shortcode);
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  auto smv = sec_id_smv_map[sec_id];

  book_info_manager_ = BookInfoManager::GetUniqueInstance(common_smv_source_->getLogger(), *watch_, *smv);
  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_;
  smm_ = SimMarketMakerHelper::GetSimMarketMaker(
      common_smv_source_->getLogger(), *watch_, smv, smv, 1, *sim_time_series_info_ptr_, sid_to_mov_ptr_map_,
      common_smv_source_->getHistoricalDispatcher(), common_smv_source_, false);

  sim_trader = SimTraderHelper::GetSimTrader(account.c_str(), smm_);

  int first_caos = 0;
  om_ = new SmartOrderManager(common_smv_source_->getLogger(), *watch_, sec_name_indexer, *sim_trader, *smv, 11,
                              livetrading, first_caos);

  secid_ = sec_id;

  trades_writer_ = new HFSAT::BulkFileWriter(1024 * 1024);

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "~/dvctrade/Tests/data/"
              << "/trades." << date_to_run << "." << 11;
  std::string tradesfilename = t_temp_oss_.str();

  // open trades file in append mode in livetrading_
  trades_writer_->Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));

  HFSAT::SimBasePNL* sim_base_pnl = NULL;
  sim_base_pnl = new HFSAT::SimBasePNL(common_smv_source_->getLogger(), *watch_, *smv, 11, *trades_writer_);
  om_->SetBasePNL(sim_base_pnl);

  std::string paramfile_path_ = GetTestDataFullPath("param_BaseTradingTests", "dvctrade");

  economic_events_manager_ = new HFSAT::EconomicEventsManager(common_smv_source_->getLogger(), *watch_);

  std::vector<std::string>
      _this_model_source_shortcode_vec_;  ///< vector of all sources which we need data for or are trading
  std::vector<std::string>
      _this_model_ors_shortcode_vec_;  ///< vector of all sources which we need ORS messages for, to build indicators

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), "DirectionalAggressiveTrading", shortcode,
                                           _this_model_source_shortcode_vec_, _this_model_ors_shortcode_vec_);

  bt_ = new HFSAT::PriceBasedAggressiveTrading(common_smv_source_->getLogger(), *watch_, *smv, *om_, paramfile_path_,
                                               false, nullptr, *economic_events_manager_, 32400200, 43210200, 23432,
                                               _this_model_source_shortcode_vec_);

  // Set start trading = True
  ((ExecInterface*)bt_)->SetStartTrading(true);

  // Set the model math component here. This requires a model file
  std::string modelfilepath_ = GetTestDataFullPath("model_BaseTradingTests", "dvctrade");
  base_model_math_ = HFSAT::ModelCreator::CreateModelMathComponent(common_smv_source_->getLogger(), *watch_,
                                                                   modelfilepath_, nullptr, 32400200, 43210200, 23432);
  ((ExecInterface*)bt_)->SetModelMathComponent(base_model_math_);

}

void BaseTradingTests::TestCollectORSShortCodes(void) {
  // Function populates the vector source_shortcode_vec_ and ors_needed_by_indicators_vec_. Check if all the required
  // shortcodes are present in them for different stratnames.
  std::string stratname = "DirectionalAggressiveTrading";
  std::string dep_shc = "ZN_0";
  std::string ors_source_needed = "ZN_0";
  std::vector<std::string> source_shortcode_vec_;  ///< vector of all sources which we need data for or are trading
  std::vector<std::string>
      ors_needed_by_indicators_vec_;  ///< vector of all sources which we need ORS messages for, to build indicators

  std::stringstream message;

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), stratname, dep_shc, source_shortcode_vec_,
                                           ors_needed_by_indicators_vec_);

  std::string shc_to_collect = dep_shc;
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  message << "Assesrion Failed : The ors source for shortcode" << ors_source_needed
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(),
                         std::find(ors_needed_by_indicators_vec_.begin(), ors_needed_by_indicators_vec_.end(),
                                   ors_source_needed) != ors_needed_by_indicators_vec_.end());

  stratname = "ProjectedPricePairBasedAggressiveTrading";
  dep_shc = "HHI_0";
  ors_source_needed = "HHI_0";
  source_shortcode_vec_.clear();
  ors_needed_by_indicators_vec_.clear();

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), stratname, dep_shc, source_shortcode_vec_,
                                           ors_needed_by_indicators_vec_);

  shc_to_collect = "HSI_0";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = dep_shc;
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  message << "Assesrion Failed : The ors source for shortcode" << ors_source_needed
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(),
                         std::find(ors_needed_by_indicators_vec_.begin(), ors_needed_by_indicators_vec_.end(),
                                   ors_source_needed) != ors_needed_by_indicators_vec_.end());

  stratname = "ImpliedDirectionalAggressiveTrading";
  dep_shc = "VX_1";
  ors_source_needed = "VX_1";
  source_shortcode_vec_.clear();
  ors_needed_by_indicators_vec_.clear();

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), stratname, dep_shc, source_shortcode_vec_,
                                           ors_needed_by_indicators_vec_);

  shc_to_collect = "SP_VX0_VX1";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "VX_0";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = dep_shc;
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  message << "Assesrion Failed : The ors source for shortcode" << ors_source_needed
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(),
                         std::find(ors_needed_by_indicators_vec_.begin(), ors_needed_by_indicators_vec_.end(),
                                   ors_source_needed) != ors_needed_by_indicators_vec_.end());

  stratname = "EquityTrading2";
  dep_shc = "NSE_NIFTY_FUT0";
  ors_source_needed = "NSE_NIFTY_FUT0";
  source_shortcode_vec_.clear();
  ors_needed_by_indicators_vec_.clear();

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), stratname, dep_shc, source_shortcode_vec_,
                                           ors_needed_by_indicators_vec_);

  shc_to_collect = "NSE_NIFTY_FUT0";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "NSE_BANKNIFTY_FUT0";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = dep_shc;
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  message << "Assesrion Failed : The ors source for shortcode" << ors_source_needed
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(),
                         std::find(ors_needed_by_indicators_vec_.begin(), ors_needed_by_indicators_vec_.end(),
                                   ors_source_needed) != ors_needed_by_indicators_vec_.end());

  stratname = "EquityTrading2";
  dep_shc = "FGBM_0";
  ors_source_needed = "FGBM_0";
  source_shortcode_vec_.clear();
  ors_needed_by_indicators_vec_.clear();

  HFSAT::BaseTrading::CollectORSShortCodes(common_smv_source_->getLogger(), stratname, dep_shc, source_shortcode_vec_,
                                           ors_needed_by_indicators_vec_);

  shc_to_collect = "BR_IND_0";
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = dep_shc;
  message << "Assesrion Failed : The shortocde " << shc_to_collect
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  message << "Assesrion Failed : The ors source for shortcode" << ors_source_needed
          << " is not collected properly for shortcode = " << dep_shc << "and for stratname = " << stratname << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(),
                         std::find(ors_needed_by_indicators_vec_.begin(), ors_needed_by_indicators_vec_.end(),
                                   ors_source_needed) != ors_needed_by_indicators_vec_.end());
}

void BaseTradingTests::TestCollectShortCodes(void) {
  std::string paramfile_path_ = GetTestDataFullPath("param_BaseTradingTests_CollectShortCodes", "dvctrade");
  std::vector<std::string> source_shortcode_vec_;

  // Shortocdes must be collected properly from the param file
  HFSAT::BaseTrading::CollectShortCodes(common_smv_source_->getLogger(), paramfile_path_, source_shortcode_vec_);

  std::stringstream message;
  std::string shc_to_collect = "ZN_0";
  message << "Assesrion Failed : The shortcode = " << shc_to_collect << " present in file " << paramfile_path_
          << " is not collected properly" << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "FGBM_0";
  message << "Assesrion Failed : The shortcode = " << shc_to_collect << " present in file " << paramfile_path_
          << " is not collected properly" << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "FGBL_0";
  message << "Assesrion Failed : The shortcode = " << shc_to_collect << " present in file " << paramfile_path_
          << " is not collected properly" << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "ZT_0";
  message << "Assesrion Failed : The shortcode = " << shc_to_collect << " present in file " << paramfile_path_
          << " is not collected properly" << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());

  shc_to_collect = "ZF_0";
  message << "Assesrion Failed : The shortcode = " << shc_to_collect << " present in file " << paramfile_path_
          << " is not collected properly" << endl;
  CPPUNIT_ASSERT_MESSAGE(message.str(), std::find(source_shortcode_vec_.begin(), source_shortcode_vec_.end(),
                                                  shc_to_collect) != source_shortcode_vec_.end());
}

void BaseTradingTests::TestsOnPositionChange(void) {
  // make sure the size passed in OnPositionChange is proportionate to UTS present in param file.
  // Also while placing order, the size should not be a relatively large number.

  double px = 12.3;
  int intpx = 1230;
  int size = 10;
  int saos = 0;

  // sending and exectuing a trade for positions
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderExecuted(0, saos, saos, secid_, px, kTradeTypeBuy, 0, size, size, 0, intpx, 0, 0, watch_->tv());

  // Checking the current postion is 10
  CPPUNIT_ASSERT_EQUAL(10, bt_->my_position());

  // Manually change to some other value using OnPositionChange function and check
  bt_->OnPositionChange(20, 38, secid_);
  CPPUNIT_ASSERT_EQUAL(20, bt_->my_position());

  bt_->OnPositionChange(-10, 38, secid_);
  CPPUNIT_ASSERT_EQUAL(-10, bt_->my_position());
}

void BaseTradingTests::TestsGetFlatTradingLogic(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  SimTimeSeriesInfo sim_time_series_info(sec_name_indexer.NumSecurityId());
  sim_time_series_info.sid_to_sim_config_.resize(sec_name_indexer.NumSecurityId());

  auto sec_id = sec_name_indexer.GetIdFromString("FGBL_0");
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  auto smv = sec_id_smv_map[sec_id];

  // Create a dummy book. This has 5 levels in it.
  // Book needs to be created of FGBL only because the price levels are set acording to its min price increment.
  MvmTestsUtils::ConstructDummyBook(smv);

  bt_->OnMarketUpdate(sec_id, smv->market_update_info_);

  // This updates BestAsk/BidLevelInfo structs
  smv->UpdateL1Prices();

  smv->NotifyL2Listeners();

  double px = smv->bestbid_price();
  int intpx = smv->bestbid_int_price();
  int size = 3;
  int saos = 0;

  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 43200, 210 * 1000));
  // Update target once after the start time
  bt_->UpdateTarget(smv->bestbid_price(), 0.1, 0);

  // Placing order at best level and executing it
  om_->SendTrade(px, intpx, size, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderExecuted(0, saos, saos, secid_, px, kTradeTypeBuy, 0, size, size, 0, intpx, 0, 0, watch_->tv());

  int prev_size = size;
  saos++;
  px = 131.08;
  intpx = smv->GetIntPx(131.08);
  size = 1;
  int total_size = size + prev_size;

  // Placing order on non best level
  om_->SendTrade(px, intpx, 1, kTradeTypeBuy, 'B', kOrderDay);
  om_->OrderSequenced(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, 0, 0, intpx, 0, 0, watch_->tv());
  om_->OrderConfirmed(0, saos, saos, secid_, px, kTradeTypeBuy, size, 0, total_size, 0, intpx, 0, 0, watch_->tv());

  // Set the time to something that is beyond the end time of the bt_ object created. To check if it tries to get flat
  // after that.
  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 43210, 700 * 1000));

  bt_->OnMarketUpdate(sec_id, smv->market_update_info_);
  bt_->UpdateTarget(smv->bestask_price(), 0.1, 0);

  // Size at best ask should be equal to the sum of number of executed buys and confirmed buys. Confirming that query is
  // trying to get flat.
  // We do not check if it actually gets the fills/not, but the fact that it places corect number of order at correct
  // price point.
  // After changes for calling TradingLogic, new orders can be sent on OrderConfirmed. Therefore comparing it with 10 moving forward.
  CPPUNIT_ASSERT_EQUAL(10, om_->GetTotalAskSizeOrderedAtIntPx(smv->bestask_int_price()));

  MvmTestsUtils::ClearDummyBook(smv);
}

void BaseTradingTests::TestsPlaceCancelNonBestLevels(void) {
  auto& sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  SimTimeSeriesInfo sim_time_series_info(sec_name_indexer.NumSecurityId());
  sim_time_series_info.sid_to_sim_config_.resize(sec_name_indexer.NumSecurityId());

  auto sec_id = sec_name_indexer.GetIdFromString("FGBL_0");
  auto& sec_id_smv_map = common_smv_source_->getSMVMap();
  auto smv = sec_id_smv_map[sec_id];

  // Create a dummy book. This has 5 levels in it.
  // Book needs to be created of FGBL only because the price levels are set acording to its min price increment.
  MvmTestsUtils::ConstructDummyBook(smv);

  bt_->OnMarketUpdate(sec_id, smv->market_update_info_);

  // This updates BestAsk/BidLevelInfo structs
  smv->UpdateL1Prices();

  smv->NotifyL2Listeners();

  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 43200, 210 * 1000));
  bt_->UpdateTarget(smv->bestbid_price(), 0.1, 0);

  // Setting some time between the start and end time
  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 43203, 0));
  bt_->UpdateTarget(smv->bestask_price(), 0.1, 0);

  int uts = 10;

  // Check that even if we have not placed any orders here, strategy should place orders on 2 non best levels by itself.
  // Size of this order will be equla to uts mentioned in params.
  // Note that if in params num_non_best_levels_monitored is set to 3, then it places order on 2 non best level ( 1 less
  // than what is set in params)
  // On other lower levels there should be 0 orders.

  CPPUNIT_ASSERT_EQUAL(uts, om_->GetTotalBidSizeOrderedAtIntPx(smv->GetIntPx(smv->bid_price(1))));
  CPPUNIT_ASSERT_EQUAL(uts, om_->GetTotalBidSizeOrderedAtIntPx(smv->GetIntPx(smv->bid_price(2))));
  CPPUNIT_ASSERT_EQUAL(0, om_->GetTotalBidSizeOrderedAtIntPx(smv->GetIntPx(smv->bid_price(3))));
  CPPUNIT_ASSERT_EQUAL(0, om_->GetTotalBidSizeOrderedAtIntPx(smv->GetIntPx(smv->bid_price(4))));

  CPPUNIT_ASSERT_EQUAL(uts, om_->GetTotalAskSizeOrderedAtIntPx(smv->GetIntPx(smv->ask_price(1))));
  CPPUNIT_ASSERT_EQUAL(uts, om_->GetTotalAskSizeOrderedAtIntPx(smv->GetIntPx(smv->ask_price(2))));
  CPPUNIT_ASSERT_EQUAL(0, om_->GetTotalAskSizeOrderedAtIntPx(smv->GetIntPx(smv->ask_price(3))));
  CPPUNIT_ASSERT_EQUAL(0, om_->GetTotalAskSizeOrderedAtIntPx(smv->GetIntPx(smv->ask_price(4))));

  MvmTestsUtils::ClearDummyBook(smv);
}

void BaseTradingTests::TestsProcessAllowedEco(void) {
  // Event start time is 10:00 UTC. So checking the get flat flag for time before the event, during the event and after
  // the event.
  // This test may fail if this particular event is changed in the AllowedEcoEvents file. In that case, please update
  // the test case below and this comment too.

  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 33000, 700 * 1000));
  CPPUNIT_ASSERT(!bt_->get_flag_getflat_due_to_allowed_economic_event());

  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 36000, 700 * 1000));
  CPPUNIT_ASSERT(bt_->get_flag_getflat_due_to_allowed_economic_event());

  watch_->OnTimeReceived(ttime_t(HFSAT::DateTime::GetTimeMidnightUTC(date_to_run) + 38000, 700 * 1000));
  CPPUNIT_ASSERT(!bt_->get_flag_getflat_due_to_allowed_economic_event());
}

void BaseTradingTests::tearDown(void) {
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  delete sim_time_series_info_ptr_;
  delete common_smv_source_;
  delete trades_writer_;
  delete om_;
  delete bt_;
  delete economic_events_manager_;
  common_smv_source_ = nullptr;
  trades_writer_ = nullptr;
  om_ = nullptr;
  bt_ = nullptr;
  economic_events_manager_ = nullptr;
}
}
