/**
    \file dvccode/CommonTradeUtils/rollover_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <string>

namespace HFSAT {
namespace RollOverUtils {

// returns the major maturity for the shortcode
std::string GetNearestMajorExpiry(const std::string& shortcode_);
}
}
