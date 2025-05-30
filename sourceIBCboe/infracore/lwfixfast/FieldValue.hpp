/**
    \file FieldValue.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

     Created on: Nov 3, 2011

*/
#pragma once

#include <inttypes.h>

struct I32Val {
  int32_t val;
  bool isNull;
};

struct U32Val {
  uint32_t val;
  bool isNull;
};

struct I64Val {
  int64_t val;
  bool isNull;
};

struct U64Val {
  uint64_t val;
  bool isNull;
};

struct StringVal {
  char* buf;
  int len;
  bool isNull;
};

struct DecimalVal {
  int64_t _mantissa;
  int32_t _exponent;
  bool isNull;
};
