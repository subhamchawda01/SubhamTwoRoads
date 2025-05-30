/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
**/

#pragma once

namespace HFSAT {
  class PortRiskListener {
  public:
    virtual ~PortRiskListener() {}
    virtual void OnNewOrderFromStrategy(std::string t_instrument_, std::string t_strategy_id_, int _size_in_lots_, double _ref_px_ ) = 0;
    // we need to contraint the steps in position ( ? ) to avoid burden on execution_algo
    virtual void OnNewPositionFromStrategy(std::string t_instrument_, std::string t_strategy_id_, int _position_in_lots_, double _ref_px_) = 0;
    virtual void OnNewPortRiskFromStrategy(int _strategy_id_, std::vector<int>& _signal_risk_vec_, std::vector<double>& _ref_px_) = 0;
  };
}
