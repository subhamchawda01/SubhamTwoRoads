/**
    \file Indicators/recent_simple_volume_measure.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to RecentSimpleRangeMeasure
/// to listen to changes in online computed stdev of the product
class RecentSimpleRangeMeasureListener {
 public:
  virtual ~RecentSimpleRangeMeasureListener(){};
  virtual void OnRangeUpdate(const unsigned int r_security_id_, const double& _new_range_value_) = 0;
};

typedef std::vector<RecentSimpleRangeMeasureListener*> RecentSimpleRangeMeasureListenerPtrVec;
typedef std::vector<RecentSimpleRangeMeasureListener*>::const_iterator RecentSimpleRangeMeasureListenerPtrVecCIter_t;
typedef std::vector<RecentSimpleRangeMeasureListener*>::iterator RecentSimpleRangeMeasureListenerPtrVecIter_t;

/// Indicator that Sum ( tradepx_mktpx_diff_ * size_traded_ )
class RecentSimpleRangeMeasure : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  const unsigned int range_calc_seconds_;
  const double decay_second_factor_;
  double running_max_value_;
  double running_min_value_;

  RecentSimpleRangeMeasureListenerPtrVec recent_range_measure_listener_ptr_vec_;

 protected:
  RecentSimpleRangeMeasure(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_,
                           const SecurityMarketView& _indep_market_view_, unsigned int t_seconds_);

 public:
  // static void CollectShortCodes ( std::vector < std::string > & _shortcodes_affecting_this_indicator_, std::vector <
  // std::string > & _ors_source_needed_vec_, const std::vector < const char * > & _tokens_ ) ;

  // static RecentSimpleRangeMeasure * GetUniqueInstance ( DebugLogger & _dbglogger_, const Watch & _watch_,
  // 						 const std::vector < const char * > & _tokens_ , PriceType_t
  // _basepx_pxtype_
  // )
  // ;

  static RecentSimpleRangeMeasure* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const SecurityMarketView& _indep_market_view_,
                                                     unsigned int t_seconds_);

  ~RecentSimpleRangeMeasure() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);  // only function implemented
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "RecentSimpleRangeMeasure"; }
  inline unsigned int range_calc_seconds() const { return range_calc_seconds_; }
  inline double recent_range() const { return indicator_value_; }

  inline void AddRecentSimpleRangeMeasureListener(RecentSimpleRangeMeasureListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(recent_range_measure_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveRecentSimpleRangeMeasureListener(RecentSimpleRangeMeasureListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(recent_range_measure_listener_ptr_vec_, _new_listener_);
  }
};
}
