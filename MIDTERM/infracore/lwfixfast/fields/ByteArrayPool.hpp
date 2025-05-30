/**
    \file ByteArrayPool.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

// A simple class that allocates some static memory which is
// used to store temporary string / byte vectors

class ByteArrayPool {
#define MAX_STRING_SIZE 128
#define MAX_POOL_SIZE 1024
#define POOL_INDEX_SIZE_MASK 1023

 public:
  static char* get() {
    static char* buffer = (char*)calloc(MAX_POOL_SIZE, MAX_STRING_SIZE);
    static int len = 0;
    return buffer + ((++len) & POOL_INDEX_SIZE_MASK) * MAX_STRING_SIZE;
  }
};
