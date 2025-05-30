/**
   \file Tests/dvctrade/MarketAdapter/security_market_view_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/MarketAdapter/security_market_view_tests.hpp"

namespace HFTEST {

void SecurityMarketViewTests::setUp(void) {
  std::vector<std::string> shortcode_list = {"FGBM_0"};

  common_smv_source_ = new CommonSMVSource(shortcode_list, 20160310);
  common_smv_source_->SetDepShortcodeVector(shortcode_list);
  common_smv_source_->SetSourceShortcodes(shortcode_list);

  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
}

/**
 * Test the computation of mid-price
 */
void SecurityMarketViewTests::TestMidPrice(void) {
  // setup the variable
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // Set compute mid-price
  smv->ComputeMidPrice();

  // Set the best-ask best-bid value
  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_price_ = 131.10;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // update the prices
  smv->is_ready_ = true;
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(smv->market_update_info().mid_price_, 131.095, DOUBLE_ASSERT_PRECISION);
}

/**
 * Test computation of MktSizeWPrice
 */
void SecurityMarketViewTests::TestMktPrice(void) {
  // setup the variable

  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of mkt-price
  smv->ComputeMktPrice();

  // set best-bid, best-ask variables

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;
  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);
  smv->market_update_info().bestbid_size_ = 100;
  smv->market_update_info().bestask_size_ = 900;

  // update the prices
  smv->is_ready_ = true;
  smv->UpdateL1Prices();

  // px = (bid_px*ask_size + ask_px*bid_size)/ ( bid_size + ask_size)

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.091, smv->market_update_info().mkt_size_weighted_price_, DOUBLE_ASSERT_PRECISION);

  smv->ComputeMidPrice();  // mid-price is required when spread > 1

  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // In this case the price should be just mid-price
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.096, smv->market_update_info().mkt_size_weighted_price_, DOUBLE_ASSERT_PRECISION);
}

void SecurityMarketViewTests::TestOrderSizeWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of OrderSizeWPrice
  smv->ComputeOrderSizeWPrice();

  // set best-bid, best-ask variables

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;
  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);
  smv->market_update_info().bestbid_size_ = 100;
  smv->market_update_info().bestask_size_ = 900;
  smv->market_update_info().bestbid_ordercount_ = 1;
  smv->market_update_info().bestask_ordercount_ = 9;

  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.095, smv->market_update_info().order_size_weighted_price_, DOUBLE_ASSERT_PRECISION);

  //  needed for spread greater than 1
  smv->ComputeMidPrice();

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.10, smv->market_update_info().order_size_weighted_price_, DOUBLE_ASSERT_PRECISION);
}

void SecurityMarketViewTests::TestOrderWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of OrderWPrice
  smv->ComputeOrderWPrice();
  smv->ComputeMidPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_ordercount_ = 9;
  smv->market_update_info().bestask_ordercount_ = 1;

  smv->is_ready_ = true;

  smv->UpdateL1Prices();
  // px = (bid_px*ask_ordercount + ask_px*bid_ordercount)/ ( ask_ordercount + bid_ordercount)

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.099, smv->market_update_info().order_weighted_price_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  // the price now should be just mid-price
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.10, smv->market_update_info().order_weighted_price_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Test TradeWPrice
 */
void SecurityMarketViewTests::TestTradeWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // Set computation of TradeWPrice

  smv->ComputeTradeWPrice();
  smv->ComputeMidPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;

  smv->is_ready_ = true;

  smv->UpdateL1Prices();

  // We are computing the trade price here
  smv->NotifyTradeListeners();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.0925, smv->market_update_info().trade_weighted_price_, DOUBLE_ASSERT_PRECISION);
}

void SecurityMarketViewTests::TestMktSinuSoidal(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of MktSinusoidal

  smv->ComputeMktSinPrice();
  smv->ComputeMidPrice();
  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->is_ready_ = true;
  // Compute the price here
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.092261457551, smv->market_update_info().mkt_sinusoidal_price_,
                               DOUBLE_ASSERT_PRECISION);

  // check for spread > 1
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->UpdateL1Prices();
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.10, smv->market_update_info().mkt_sinusoidal_price_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Test OfflineMixMMS Price
 */
void SecurityMarketViewTests::TestOfflineMixMMS(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // Set computation of OmixPrice

  smv->ComputeMidPrice();
  smv->ComputeMktPrice();
  smv->ComputeMktSinPrice();
  smv->ComputeOrderWPrice();
  smv->ComputeTradeWPrice();

  // Not good way of doing it
  smv->price_type_subscribed_[kPriceTypeOfflineMixMMS] = true;

  // set all variables required by all given price-types

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->market_update_info().bestbid_ordercount_ = 3;
  smv->market_update_info().bestask_ordercount_ = 30;

  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;

  smv->is_ready_ = true;
  // We are computing the trade price here
  smv->NotifyTradeListeners();

  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.09090909, smv->market_update_info().offline_mix_mms_price_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Tests the ValidLevelPrice,
 */
void SecurityMarketViewTests::TestValidLevelPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of OrderWPrice
  smv->ComputeMidPrice();
  smv->ComputeValidLevelMidPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->is_ready_ = true;

  smv->UpdateL1Prices();
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.095, smv->market_update_info().valid_level_mid_price_, DOUBLE_ASSERT_PRECISION);
}
/**
 * Tests the TradeOrderWlPrice,
 */
void SecurityMarketViewTests::TestTradeOrderWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // resetting the watch so the msecs from midnight is set, else it will give segmentation fault
  watch_->ResetWatch(20170501);

  // set computation of Prices
  smv->ComputeOrderWPrice();
  smv->ComputeMidPrice();
  smv->ComputeTradeWPrice();
  smv->ComputeTradeOrderWPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_ordercount_ = 10;
  smv->market_update_info().bestask_ordercount_ = 100;
  smv->is_ready_ = true;

  // updating the current time twice so that we have curr - last_updated secs
  auto current_time = ttime_t(1400000001, 00000);
  watch_->OnTimeReceived(current_time);
  current_time = ttime_t(1400000001, 1000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  // doing a trade and updating the trade prices
  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;
  smv->SetTradeVarsForIndicatorsIfRequired();
  smv->NotifyTradeListeners();

  current_time = ttime_t(1400000001, 2000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090909090909, smv->market_update_info().trade_orderw_px_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.09000335350132, smv->market_update_info().trade_orderw_px_, DOUBLE_ASSERT_PRECISION);

  // check for constants read from file
  double* result =
      HFSAT::SMVUtils::SMVUtils::GetTradeBasePriceParams("/spare/local/tradeinfo/param_trade_basepx.txt", "FGBM_0");

  CPPUNIT_ASSERT_DOUBLES_EQUAL(20, result[0], DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(10, result[1], DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(100, result[2], DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(50, result[3], DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(5, result[4], DOUBLE_ASSERT_PRECISION);
}

/**
 * Tests the TradeMktSizeWPrice,
 */
void SecurityMarketViewTests::TestTradeMktSizeWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // resetting the watch so the msecs from midnight is set, else it will give segmentation fault
  watch_->ResetWatch(20170501);

  // set computation of Prices
  smv->ComputeOrderWPrice();
  smv->ComputeMktPrice();
  smv->ComputeTradeWPrice();
  smv->ComputeMidPrice();
  smv->ComputeTradeMktSizePrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->is_ready_ = true;
  smv->market_update_info().bestbid_size_ = 9;
  smv->market_update_info().bestask_size_ = 1;

  // updating the current time twice so that we have curr - last_updated secs
  auto current_time = ttime_t(1400000001, 00000);
  watch_->OnTimeReceived(current_time);
  current_time = ttime_t(1400000001, 1000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  // doing a trade and updating the trade prices
  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;
  current_time = ttime_t(1400000001, 2000);
  watch_->OnTimeReceived(current_time);
  smv->SetTradeVarsForIndicatorsIfRequired();
  smv->NotifyTradeListeners();
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.098999999, smv->market_update_info().trade_mktsz_px_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090004694902, smv->market_update_info().trade_mktsz_px_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Tests the TradeMktSinPrice,
 */
void SecurityMarketViewTests::TestTradeMktSinPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // resetting the watch so the msecs from midnight is set, else it will give segmentation fault
  watch_->ResetWatch(20170501);

  // set computation of Prices
  smv->ComputeOrderWPrice();
  smv->ComputeMidPrice();
  smv->ComputeTradeWPrice();
  smv->ComputeMktSinPrice();
  smv->ComputeTradeMktSinPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->is_ready_ = true;

  // updating the current time twice so that we have curr - last_updated secs
  auto current_time = ttime_t(1400000001, 00000);
  watch_->OnTimeReceived(current_time);
  current_time = ttime_t(1400000001, 1000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  // doing a trade and updating the trade prices
  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;
  current_time = ttime_t(1400000001, 2000);
  watch_->OnTimeReceived(current_time);
  smv->SetTradeVarsForIndicatorsIfRequired();
  smv->NotifyTradeListeners();
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.092261457551, smv->market_update_info().trade_mktsin_px_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090003353501, smv->market_update_info().trade_mktsin_px_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Tests the TradeTradePrice,
 */
void SecurityMarketViewTests::TestTradeTradeWPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // resetting the watch so the msecs from midnight is set, else it will give segmentation fault
  watch_->ResetWatch(20170501);

  // set computation of Prices
  smv->ComputeOrderWPrice();
  smv->ComputeMidPrice();
  smv->ComputeTradeWPrice();
  smv->ComputeTradeTradeWPrice();
  smv->ComputeTradeMktSinPrice();

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->is_ready_ = true;

  // updating the current time twice so that we have curr - last_updated secs
  auto current_time = ttime_t(1400000001, 00000);
  watch_->OnTimeReceived(current_time);
  current_time = ttime_t(1400000001, 1000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  // doing a trade and updating the trade prices
  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;
  current_time = ttime_t(1400000001, 2000);
  watch_->OnTimeReceived(current_time);
  smv->SetTradeVarsForIndicatorsIfRequired();
  smv->NotifyTradeListeners();
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.0925, smv->market_update_info().trade_tradew_px_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090000838375, smv->market_update_info().trade_tradew_px_, DOUBLE_ASSERT_PRECISION);
}

/**
 * Test for TradeOmixPrice
 */

void SecurityMarketViewTests::TestTradeOmixPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // resetting the watch so the msecs from midnight is set, else it will give segmentation fault
  watch_->ResetWatch(20170501);

  // set computation of Prices
  smv->ComputeMidPrice();
  smv->ComputeMktPrice();
  smv->ComputeMktSinPrice();
  smv->ComputeTradeOmixPrice();
  smv->ComputeOrderWPrice();
  smv->ComputeTradeWPrice();

  // Not good way of doing it
  smv->price_type_subscribed_[kPriceTypeOfflineMixMMS] = true;

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->market_update_info().bestbid_ordercount_ = 2;
  smv->market_update_info().bestask_ordercount_ = 10;

  smv->is_ready_ = true;

  // updating the current time twice so that we have curr - last_updated secs
  auto current_time = ttime_t(1400000001, 00000);
  watch_->OnTimeReceived(current_time);
  current_time = ttime_t(1400000001, 1000);
  watch_->OnTimeReceived(current_time);
  smv->UpdateL1Prices();

  // doing a trade and updating the trade prices
  smv->trade_print_info().size_traded_ = 100;
  smv->trade_print_info().trade_price_ = 131.09;
  current_time = ttime_t(1400000001, 2000);
  watch_->OnTimeReceived(current_time);
  smv->SetTradeVarsForIndicatorsIfRequired();
  smv->NotifyTradeListeners();
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.09090909090909, smv->market_update_info().trade_omix_px_, DOUBLE_ASSERT_PRECISION);

  // Set the spread to two ticks
  smv->market_update_info().bestask_price_ = 131.11;
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  // recompute the prices
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090001981614, smv->market_update_info().trade_omix_px_, DOUBLE_ASSERT_PRECISION);
}
/**
 * Tests the OnlineMixPrice,
 */
void SecurityMarketViewTests::TestOnlineMixPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // Set computation of OnlinemixPrice

  smv->ComputeMidPrice();
  smv->ComputeMktPrice();
  smv->ComputeMktSinPrice();
  smv->ComputeOrderWPrice();
  smv->ComputeTradeWPrice();
  smv->ComputeOnlineMixPrice();

  // Not good way of doing it
  smv->price_type_subscribed_[kPriceTypeOfflineMixMMS] = true;

  // set all variables required by all given price-types

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;

  smv->market_update_info().bestbid_ordercount_ = 2;
  smv->market_update_info().bestask_ordercount_ = 10;

  // We are computing the trade price here
  smv->NotifyTradeListeners();

  smv->UpdateL1Prices();

  // checking for the parameters
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, smv->online_const_c, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, smv->online_const_k, DOUBLE_ASSERT_PRECISION);
  // test for AS with no file
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.090909090909, smv->market_update_info().online_mix_price_, DOUBLE_ASSERT_PRECISION);
  // checking only for AS time now
  // watch_->ResetWatch(20170511);
  // setting EU time and testing for EU
  // auto current_time = ttime_t(1494492845, 00000);
  // watch_->OnTimeReceived(current_time);
  // smv->UpdateL1Prices();
  // CPPUNIT_ASSERT_DOUBLES_EQUAL(131.052556, smv->market_update_info().offline_mix_mms_price_,
  // DOUBLE_ASSERT_PRECISION);
  // setting US time and trying for US
  // current_time = ttime_t(1494507557, 00000);
  // watch_->OnTimeReceived(current_time);
  // smv->UpdateL1Prices();
  // CPPUNIT_ASSERT_DOUBLES_EQUAL(131.070091, smv->market_update_info().offline_mix_mms_price_,
  // DOUBLE_ASSERT_PRECISION);
}

/**
 * tests stable bid price
 */
void SecurityMarketViewTests::TestStableBidPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of StableBidPrice
  smv->ComputeMidPrice();
  smv->ComputeStableBidPrice();
  smv->is_ready_ = true;

  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(0, smv->min_bid_size_to_consider_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.09, smv->market_update_info().stable_bid_price_, DOUBLE_ASSERT_PRECISION);
}

/**
 * tests stable ask price
 */
void SecurityMarketViewTests::TestStableAskPrice(void) {
  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  auto& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  auto sec_id = sec_name_indexer.GetIdFromString("FGBM_0");
  auto& smv = sid_to_smv_map[sec_id];

  // set computation of StableAskPrice
  smv->ComputeStableAskPrice();
  smv->ComputeMidPrice();

  smv->is_ready_ = true;

  smv->price_type_subscribed_[kPriceTypeStableAskPrice] = true;
  smv->market_update_info().bestbid_price_ = 131.09;
  smv->market_update_info().bestask_price_ = 131.10;

  smv->market_update_info().bestbid_int_price_ = smv->GetIntPx(smv->market_update_info().bestbid_price_);
  smv->market_update_info().bestask_int_price_ = smv->GetIntPx(smv->market_update_info().bestask_price_);

  smv->market_update_info().bestbid_size_ = 10;
  smv->market_update_info().bestask_size_ = 100;
  smv->UpdateL1Prices();

  CPPUNIT_ASSERT_DOUBLES_EQUAL(0, smv->min_ask_size_to_consider_, DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(131.10, smv->market_update_info().stable_ask_price_, DOUBLE_ASSERT_PRECISION);
}

void SecurityMarketViewTests::tearDown(void) {
  delete common_smv_source_;
  common_smv_source_ = nullptr;
}
}
