/**
    \file Indicators/recent_volume_measure.hpp

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

/// Common interface extended by all classes listening to RecentVolumeMeasure
/// to listen to changes in online computed stdev of the product
class RecentVolumeMeasureListener {
 public:
  virtual ~RecentVolumeMeasureListener(){};
  virtual void OnVolumeUpdate(unsigned int r_security_id_, double _new_volume_value_) = 0;
};

typedef std::vector<RecentVolumeMeasureListener*> RecentVolumeMeasureListenerPtrVec;
typedef std::vector<RecentVolumeMeasureListener*>::const_iterator RecentVolumeMeasureListenerPtrVecCIter_t;
typedef std::vector<RecentVolumeMeasureListener*>::iterator RecentVolumeMeasureListenerPtrVecIter_t;

/// Indicator that Sum ( tradepx_mktpx_diff_ * size_traded_ )
class RecentVolumeMeasure : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;
  double decay_minute_factor_;
  int pages_since_last_decay_;
  RecentVolumeMeasureListenerPtrVec recent_volume_measure_listener_ptr_vec_;

 protected:
  RecentVolumeMeasure(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      const SecurityMarketView& _indep_market_view_, double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RecentVolumeMeasure* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RecentVolumeMeasure* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const SecurityMarketView& _indep_market_view_,
                                                double _fractional_seconds_);

  ~RecentVolumeMeasure() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "RecentVolumeMeasure"; }
  inline double recent_volume() const { return indicator_value_; }

  inline void AddRecentVolumeMeasureListener(RecentVolumeMeasureListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(recent_volume_measure_listener_ptr_vec_, _new_listener_);
  }
  inline void RemoveRecentVolumeMeasureListener(RecentVolumeMeasureListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(recent_volume_measure_listener_ptr_vec_, _new_listener_);
  }
};
}
