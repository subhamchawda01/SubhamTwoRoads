// =====================================================================================
//
//       Filename:  trade_time_manager_test.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Friday 18 August 2017 02:09:09  UTC
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
/**
   \file Tests/dvctrade/OrderRouting/om_tests.hpp
   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"

namespace HFTEST {

using namespace HFSAT;

/*
 * The Test class in which we write the test cases.
 */
class TradeTimeManagerTest : public CppUnit::TestFixture {
  //  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(TradeTimeManagerTest);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(parseTimeFileTest);
  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.

  std::vector<std::string> filevec_;
  std::vector<std::string> shortcode_vec_;
  std::vector<int> test_start_time_;
  std::vector<int> test_end_time_;
  HFSAT::SecurityNameIndexer* sec_indexer_;
  HFSAT::TradeTimeManager* trade_time_manager_;
  int** break_start_times_;
  int** break_end_times_;

  void setUp() {
    // initialize trade time manager and sec indexer

    sec_indexer_ = &HFSAT::SecurityNameIndexer::GetUniqueInstance();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(20170609);

    filevec_.push_back((std::string("/spare/local/files/HKEX/hkex-trd-hours.txt")));
    filevec_.push_back((std::string("/spare/local/files/LIFFE/liffe-trd-hours.txt")));
    filevec_.push_back((std::string("/spare/local/files/BMF/bmf-trd-hours.txt")));
    filevec_.push_back((std::string("/spare/local/files/ASX/asx-trd-hours.txt")));
    filevec_.push_back((std::string("/spare/local/files/OSE/ose-trd-hours.txt")));
    filevec_.push_back((std::string("/spare/local/files/CME/cme-trd-hours.txt")));

    const std::string hhi("HHI_0");
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(hhi), hhi);
    shortcode_vec_.push_back(hhi);
    test_start_time_.push_back(14400);
    test_end_time_.push_back(18000);

    const std::string lfz = "LFZ_0";
    shortcode_vec_.push_back(lfz);
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(lfz), lfz);
    test_start_time_.push_back(0);
    test_end_time_.push_back(0);

    const std::string ciel = "CIEL3";
    shortcode_vec_.push_back(ciel);
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(ciel), ciel);
    test_start_time_.push_back(0);
    test_end_time_.push_back(0);

    const std::string xt = "XT_0";
    shortcode_vec_.push_back(xt);
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(xt), xt);
    test_start_time_.push_back(23400);
    test_end_time_.push_back(25920);

    const std::string nk = "NK_0";
    shortcode_vec_.push_back(nk);
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(nk), nk);
    test_start_time_.push_back(22140);
    test_end_time_.push_back(27060);

    const std::string zc = "ZC_0";
    shortcode_vec_.push_back(zc);
    sec_indexer_->AddString(ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(zc), zc);
    test_start_time_.push_back(62400);
    test_end_time_.push_back(82800);

    // Doing it in end so sec_name_indexer has meaningful values
    trade_time_manager_ = new TradeTimeManager(*sec_indexer_, 20170609);

    // not needed as we get pointers from trade_time_manager
    // break_start_times_ = (int**)calloc(sec_indexer_->NumSecurityId(), sizeof(int*));
    // break_end_times_ = (int**)calloc(sec_indexer_->NumSecurityId(), sizeof(int*));

    break_start_times_ = trade_time_manager_->get_break_start_times_();
    break_end_times_ = trade_time_manager_->get_break_end_times_();
  }

  void tearDown() {
    HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
    HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
    delete trade_time_manager_;
    trade_time_manager_ = nullptr;
  };

  void parseTimeFileTest() {
    int sec_id = 0;
    for (unsigned i = 0; i < filevec_.size(); i++) {
      trade_time_manager_->parseTimeFile(*sec_indexer_, filevec_[i].c_str());
      // test that the time assigned by the setBreak is correct

      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_vec_[i]);
      sec_id = sec_indexer_->GetIdFromSecname(exchange_symbol_);
      // check the break for the given shortcodes the

      // check the break start time
      CPPUNIT_ASSERT_EQUAL(break_start_times_[sec_id][0], test_start_time_[i]);
      CPPUNIT_ASSERT_EQUAL(break_end_times_[sec_id][0], test_end_time_[i]);
    }
  }
};
}
