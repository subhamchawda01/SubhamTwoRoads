// =====================================================================================
//
//       Filename:  dvccode/Tests/dvccode/CommonTradeUtils/watch_tests.cpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:
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

#include "dvccode/Tests/CommonTradeUtils/watch_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void WatchTests::setUp(void) {
  // Setup the watch and its listener
  dbglogger_ = new DebugLogger(1024 * 1024, 1024 * 32);
  watch_ = new Watch(*dbglogger_, 20170310);
  // Need to have it as watch maintains some variables which are
  // dependent on the tradingdate we are initializing it for
  sec_start_time_ = DateTime::GetTimeMidnightUTC(20170310);
}

/**
 * Tests whenever we call reset on watch, everything gets reset properly.
 * Apparently the reset function doesn't reset tv_, it just resets the date and
 * adjusts the msecs_from_midnight accordingly
 */
void WatchTests::TestResetWatch(void) {
  // Initial time, We need to make this call otherwise ResetWatch doesn't work
  // Though it would have been better if this was better handled in watch

  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  watch_->ResetWatch(20170312);

  // assert for new values,
  // other way of asserting could be with difference from last values
  CPPUNIT_ASSERT_EQUAL(172800000, watch_->msecs_from_midnight());
  CPPUNIT_ASSERT_EQUAL((int64_t)172800000000, watch_->usecs_from_midnight());
  CPPUNIT_ASSERT_EQUAL(1489276800, (int)watch_->last_midnight_sec());
}

void WatchTests::TestMidnightChange(void) {}

/**
 * Tests if a listener is notified at appropriate time and with appropriate parameters
 */
void WatchTests::TestTimePeriodListener(void) {
  dummy_listener_ = new WatchListener();
  // Normal TimePeriod Listener ( 100 msecs )
  watch_->subscribe_TimePeriod(dummy_listener_);

  // Initial time
  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  // New time
  current_time = ttime_t(sec_start_time_, 400000);
  watch_->OnTimeReceived(current_time);

  // The number of calls should be 1 with num_pages 4
  CPPUNIT_ASSERT_EQUAL(1, dummy_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(4, dummy_listener_->LastPageWidth());
}

void WatchTests::TestBigTimePeriodListener(void) {
  dummy_listener_ = new WatchListener();

  watch_->subscribe_BigTimePeriod(dummy_listener_);

  auto dummy_listener2 = new WatchListener();

  watch_->subscribe_TimePeriod(dummy_listener2);

  // Initial time
  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  // New time
  current_time = ttime_t(sec_start_time_ + 50, 400000);
  watch_->OnTimeReceived(current_time);

  // The number of calls should be 1 with num_pages 4
  CPPUNIT_ASSERT_EQUAL(1, dummy_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(50, dummy_listener_->LastPageWidth());

  // The number of calls should be 1 with num_pages 4
  CPPUNIT_ASSERT_EQUAL(1, dummy_listener2->NumCalls());
  CPPUNIT_ASSERT_EQUAL(504, dummy_listener2->LastPageWidth());

  delete dummy_listener2;
}

void WatchTests::TestFifteenSecondsTimePeriodListener(void) {
  auto fifteen_second_listener_ = new WatchListener();

  watch_->subscribe_FifteenSecondPeriod(fifteen_second_listener_);

  auto default_listener_ = new WatchListener();

  watch_->subscribe_TimePeriod(default_listener_);

  // Initial time
  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  // New time
  current_time = ttime_t(sec_start_time_ + 42, 3000000);
  watch_->OnTimeReceived(current_time);

  CPPUNIT_ASSERT_EQUAL(1, fifteen_second_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(3, fifteen_second_listener_->LastPageWidth());

  CPPUNIT_ASSERT_EQUAL(1, default_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(450, default_listener_->LastPageWidth());

  delete fifteen_second_listener_;
  delete default_listener_;
}

void WatchTests::TestOneMinutesTimePeriodListener(void) {
  auto one_minute_listener_ = new WatchListener();

  watch_->subscribe_OneMinutePeriod(one_minute_listener_);

  auto default_listener_ = new WatchListener();

  watch_->subscribe_TimePeriod(default_listener_);

  // Initial time
  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  // New time
  current_time = ttime_t(sec_start_time_ + 117, 3000000);
  watch_->OnTimeReceived(current_time);

  CPPUNIT_ASSERT_EQUAL(1, one_minute_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(2, one_minute_listener_->LastPageWidth());

  CPPUNIT_ASSERT_EQUAL(1, default_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(1200, default_listener_->LastPageWidth());

  delete one_minute_listener_;
  delete default_listener_;
}
void WatchTests::TestFifteenMinutesTimePeriodListener(void) {
  auto fifteen_minutes_listener_ = new WatchListener();

  watch_->subscribe_FifteenMinutesPeriod(fifteen_minutes_listener_);

  auto default_listener_ = new WatchListener();

  watch_->subscribe_TimePeriod(default_listener_);

  // Initial time
  ttime_t current_time(sec_start_time_, 0);
  watch_->OnTimeReceived(current_time);

  // New time
  current_time = ttime_t(sec_start_time_ + 1800, 0);
  watch_->OnTimeReceived(current_time);

  CPPUNIT_ASSERT_EQUAL(1, fifteen_minutes_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(2, fifteen_minutes_listener_->LastPageWidth());

  CPPUNIT_ASSERT_EQUAL(1, default_listener_->NumCalls());
  CPPUNIT_ASSERT_EQUAL(18000, default_listener_->LastPageWidth());

  delete fifteen_minutes_listener_;
  delete default_listener_;
}

void WatchTests::tearDown() {
  delete dbglogger_;
  delete watch_;
  delete dummy_listener_;
}
}
