/**
    \file ModelMathCode/md_model_aggregator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvctrade/ModelMath/md_model_aggregator.hpp"

namespace HFSAT {
MDModelAggregator::MDModelAggregator(DebugLogger& r_dbg_logger_, const Watch& r_watch_,
                                     const std::string& rs_model_filename_, SecurityMarketView& _dep_market_view_,
                                     PriceType_t _base_pricetype_)
    : BaseModelMath(r_dbg_logger_, r_watch_, rs_model_filename_), dep_market_view_(_dep_market_view_) {}

void MDModelAggregator::SetModelWeightIndicator(CommonIndicator* p_greek_indicator_, bool is_siglr_) {}

void MDModelAggregator::SetSubModelMath(BaseModelMath* p_greek_modelmath_) {}

void MDModelAggregator::OnIndicatorUpdate(const unsigned int& this_indicator_index_,
                                          const double& _this_indicator_value_) {}

void MDModelAggregator::set_basepx_pxtype() {}

bool MDModelAggregator::AreAllReady() { return false; }

void MDModelAggregator::ForceIndicatorReady() {}

void MDModelAggregator::OnControlUpdate() {}

void MDModelAggregator::AddListener() {}

void MDModelAggregator::SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {}

void MDModelAggregator::SMVOnReady() {}

void MDModelAggregator::ShowIndicatorValues() {}

void MDModelAggregator::DumpIndicatorValues() {}
}
