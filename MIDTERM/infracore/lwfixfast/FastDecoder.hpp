/**
    \file Decoder.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <inttypes.h>
#include <map>
#include "infracore/lwfixfast/FFUtils.hpp"
#include "infracore/lwfixfast/fields/ff_includes.hpp"
#include "infracore/lwfixfast2/fields/ff_includes.hpp"
#include "dvccode/CDef/refdata_locator.hpp"

class FastDecoder {
 public:
  virtual ~FastDecoder() {}
  virtual void decode(FFUtils::ByteStreamReader& input, FFUtils::PMap& pmap) = 0;
  virtual void reset() = 0;
  void process() {}
};
