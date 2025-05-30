/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

namespace HFSAT {
class SBERiskReaderListener {
public:
  virtual ~SBERiskReaderListener() {}
  virtual void OnNewOrderFromStrategy(std::string t_instrument_, std::string _order_id_, int _order_lots_, double _ref_px_) = 0;
  virtual void OnNewPositionFromStrategy(std::string t_instrument_, std::string _strat_id_, int _position_lots_, double _new_ref_px_) = 0;
};
}
