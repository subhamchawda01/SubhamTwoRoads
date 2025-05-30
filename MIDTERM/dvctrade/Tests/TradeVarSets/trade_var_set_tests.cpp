// =====================================================================================
//
//       Filename:  dvctrade/Tests/TradeVarSets/trade_var_set_tests.hpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
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

#include "dvctrade/Tests/TradeVarSets/trade_var_set_tests.hpp"

namespace HFTEST {
using namespace HFSAT;

void TradeVarSetTests::setUp(void) {
  shortcode_vec_.push_back("USD000UTSTOM");
  tradingdate_ = 20170123;

  common_smv_source_ = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
  common_smv_source_->InitializeVariables();
  watch_ = &common_smv_source_->getWatch();
  dbglogger_ = &common_smv_source_->getLogger(); 

  auto& sid_to_smv_map = common_smv_source_->getSMVMap();
  smv = sid_to_smv_map[SecurityNameIndexer::GetUniqueInstance().GetIdFromString("USD000UTSTOM")];

  tradevarset_builder_ = new TradeVarSetBuilder(*dbglogger_, *watch_, false);
  P2TV_zero_idx_ = MAX_POS_MAP_SIZE;
  position_tradevarset_map_.resize(2 * MAX_POS_MAP_SIZE + 1);
  map_pos_increment_ = 1;

  // Some basic assumption in this param file : 
  // 1) Everything is specified as unit ratio
  // 2) 0 <= zeropos <= highpos <= mur (This can be ensured manually also)
  std::string paramfile_path_ = GetTestDataFullPath("param_tradevarset_test","dvctrade");
  param_ = new ParamSet(paramfile_path_,tradingdate_,smv->shortcode());

  tradevarset_builder_->BuildPositionTradeVarSetMap(param_,smv,position_tradevarset_map_,map_pos_increment_,P2TV_zero_idx_,false);

}

void TradeVarSetTests::TestTradeVarsValue(void) {

  double zeropos_limit_position_ = param_->zeropos_limits_unit_ratio_*(double)param_->unit_trade_size_ ; 
  double highpos_limit_position_ = param_->highpos_limits_unit_ratio_*(double)param_->unit_trade_size_ ; 

  // Manually calculating thresholds on half of zeropos_limit_position 
  double place_thresh_  = ((param_->increase_place_ + param_->zeropos_place_)/2)*smv->min_price_increment();
  double keep_thresh_ = ((param_->increase_keep_ + param_->zeropos_keep_)/2)*smv->min_price_increment();
  double opp_place_thresh_ = ((param_->decrease_place_ + param_->zeropos_place_)/2)*smv->min_price_increment();
  int index_ = zeropos_limit_position_/(2*map_pos_increment_) + P2TV_zero_idx_;

  TradeVars_t actual_tradevars_ = position_tradevarset_map_[index_];

  CPPUNIT_ASSERT_DOUBLES_EQUAL(place_thresh_,actual_tradevars_.l1bid_place_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(keep_thresh_,actual_tradevars_.l1bid_keep_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(opp_place_thresh_,actual_tradevars_.l1ask_place_,DOUBLE_ASSERT_PRECISION);

  // Manually calculating thresholds between negative zeropos and highpos 
  place_thresh_  = param_->increase_place_ *smv->min_price_increment();
  double agg_thresh_ =  (param_->increase_place_+ param_->aggressive_)*smv->min_price_increment();
  index_ = -(highpos_limit_position_ + zeropos_limit_position_)/(2*map_pos_increment_) + P2TV_zero_idx_;

  actual_tradevars_ = position_tradevarset_map_[index_];

  CPPUNIT_ASSERT_DOUBLES_EQUAL(place_thresh_,actual_tradevars_.l1ask_place_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(agg_thresh_,actual_tradevars_.l1ask_aggressive_,DOUBLE_ASSERT_PRECISION);

}

void TradeVarSetTests::TestHighPosTradeVarsValue(void) {

  double highpos_limit_position_ = param_->highpos_limits_unit_ratio_*(double)param_->unit_trade_size_ ; 
  double max_limit_position_ = param_->max_unit_ratio_*(double)param_->unit_trade_size_ ; 

  // Manually calculating thresholds after highpos 
  int position_ = (highpos_limit_position_ + max_limit_position_)/2;
  int index_ = position_/(map_pos_increment_) + P2TV_zero_idx_;
  double highpos_fraction_ = (double)position_/highpos_limit_position_;
  double place_thresh_ = (param_->increase_place_ + highpos_fraction_*param_->highpos_thresh_factor_)*smv->min_price_increment();
  double opp_place_thresh_ = (param_->decrease_place_ - highpos_fraction_*param_->highpos_thresh_decrease_)*smv->min_price_increment();
  double increase_size_ = MathUtils::GetFlooredMultipleOf( std::max(0 , std::min(param_->unit_trade_size_, param_->max_position_ - position_)) , smv->min_order_size());
  double decrease_size_ = MathUtils::GetFlooredMultipleOf((int)round((double)param_->unit_trade_size_ * (1 + param_->highpos_size_factor_)), smv->min_order_size());

  TradeVars_t actual_tradevars_ = position_tradevarset_map_[index_];
  
  CPPUNIT_ASSERT_DOUBLES_EQUAL(place_thresh_,actual_tradevars_.l1bid_place_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(opp_place_thresh_,actual_tradevars_.l1ask_place_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(increase_size_,actual_tradevars_.l1bid_trade_size_,DOUBLE_ASSERT_PRECISION);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(decrease_size_,actual_tradevars_.l1ask_trade_size_,DOUBLE_ASSERT_PRECISION);

}

void TradeVarSetTests::tearDown(void) {
  // Remove the unique instances

  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();
  HFSAT::SecurityNameIndexer::GetUniqueInstance().Clear();
  HFSAT::CurrencyConvertor::RemoveInstance();

  // deallocate the heap variables
  delete common_smv_source_;
//  delete tradevarset_builder_;  // No destructor for tradevarset available
}
}
