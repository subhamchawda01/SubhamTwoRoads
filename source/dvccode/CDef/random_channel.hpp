/**
    \file dvccode/CDef/random_channel.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_RANDOM_CHANNEL_H
#define BASE_CDEF_RANDOM_CHANNEL_H

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include "dvccode/CDef/random_number_generator.hpp"

namespace HFSAT {

/// Class to simulate an unreliable channel
/// Or UDP communication, as in use this in SimTrader
/// to see how the logic holds up in replies are missed
class RandomChannel {
 protected:
  const double prob_success_;

 public:
  RandomChannel(double _prob_success_) : prob_success_((std::max(0.00, std::min(1.00, _prob_success_)))) {}

  inline bool GetNextBool() const { return (HFSAT::RandomNumberGenerator::GetSuccess(prob_success_)); }
};
}

#endif  // BASE_CDEF_RANDOM_CHANNEL_H
