//@ramkris: Similar to CMEFIXTypes

#ifndef BMFEP_FIX_TYPES_HPP
#define BMFEP_FIX_TYPES_HPP

#include <ctime>

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

// Field widths.
#define BMFEP_FIX_Tag_10_Width_ 3
#define BMFEP_FIX_Tag_34_Width_ 9
#define BMFEP_FIX_Tag_369_Width_ 9
#define BMFEP_FIX_Tag_11_Width_ 38     // ClOrdID
#define OUR_BMFEP_FIX_Tag_11_Width_ 9  // ClOrdID

#define BMFEP_FIX_Tag_38_Width_ 9
#define BMFEP_FIX_Tag_44_Width_ 15
#define BMFEP_FIX_Tag_9_Width_ 6
#define BMFEP_FIX_Tag_112_Width_ 20  // Test Request

// While sending TEST REQ from our side no need to have 20 char field
#define OUR_BMFEP_FIX_Tag_112_Width_ 6  // Test Request sent by us

#define BMFEP_FIX_Tag_41_Width_ 38     // OrigClOrdid
#define OUR_BMFEP_FIX_Tag_41_Width_ 9  // OrigClOrdid

#define BMFEP_FIX_Tag_37_Width_ 32  // Exchange assigned OrderID (17 for CME)
#define BMFEP_FIX_Tag_36_Width_ 9

// FIX 4.4 , Tag=7, & Tag=16 have type seqno (value 99999 to -99999)
// But make to from 6 to 9 for safety
#define BMFEP_FIX_Tag_7_Width_ 9   // We assume the integer no sent from exchange is not more than 99999999
#define BMFEP_FIX_Tag_16_Width_ 9  // We assume the integer no sent from exchange is not more than 99999999

#define BMFEP_FIX_Tag_9_Width_ 6
#define BMFEP_FIX_Tag_10_Width_ 3  // Checksume
#define BMFEP_FIX_Time_Width_ 21
#define DELIMITER_SOH_WIDTH_ 1

#define TWO_DIGIT_AND_EQUAL_WIDTH 3
#define ONE_DIGIT_AND_EQUAL_WIDTH 2

#define THREE_DIGIT_AND_EQUAL_WIDTH 4
#define YYYYMMDD_AND_SLASH_WIDTH 9

const char bmf_time_digits[128] =
    "000102030405060708091011121314151617181920212223242526272829303132333435363738394041424344454647484950515253545556"
    "57585960";

// Types for FIX msgs.
typedef char *t_FIX_SecurityDesc;

typedef unsigned long long t_FIX_OrderID;

typedef unsigned long t_FIX_SeqNum;

typedef int t_FIX_Side;
typedef int t_FIX_OrderQty;
typedef double t_FIX_Price;
typedef char t_FIX_OrdStatus;
typedef char t_FIX_ExecType;

// A faster alternative for gmtime.
// This does not compute day, month, year, thus saving us time compared to gmtime.
inline void fastgmtime(time_t *UTC_time_, int *tm_hour, int *tm_min, int *tm_sec) {
  // *UTC_time_ -= 31; // Time sync between BMFEP server and our server.

  *tm_sec = *UTC_time_ % 60;
  *UTC_time_ /= 60;

  *tm_min = *UTC_time_ % 60;
  *UTC_time_ /= 60;

  *tm_hour = (*UTC_time_ % 24);
}

// Check whether having specialized functions for smaller data types helps.
inline void printToString(char *p_dest_, t_FIX_OrderID num_, int field_width_, bool stop_on_zero_ = true) {
  // Go to the end of the stream.
  p_dest_ += (field_width_ - 1);

  // Note that we stop when num_ reaches 0. May have repercussions. CRASH_ALERT : Use with caution.
  // Helps out with really long fields like OrderID or seqnum.
  // Stopping on zero is determined by stop_on_zero_
  for (; field_width_ && (num_ | !stop_on_zero_); --field_width_, num_ /= 10, --p_dest_) {
    *p_dest_ = ((num_ % 10) + '0');
  }
}

// Check whether having specialized functions for smaller data types helps.
inline void printToString(char *p_dest_, t_FIX_Price price_, int field_width_, int fraction_width_) {
  for (int i = 0; i < fraction_width_; ++i) {
    price_ *= 10.0;
  }

  unsigned long long num_ = (unsigned long long)(price_ + 0.1);

  // Go to the end of the stream.
  p_dest_ += (field_width_ - 1);

  field_width_ -= (fraction_width_ + 1);

  // Print out the fractional part.
  for (; fraction_width_; --fraction_width_, num_ /= 10, --p_dest_) {
    *p_dest_ = ((num_ % 10) + '0');
  }

  *p_dest_ = '.';
  --p_dest_;

  // Print out the non-fractional part.
  for (; field_width_; --field_width_, num_ /= 10, --p_dest_) {
    *p_dest_ = ((num_ % 10) + '0');
  }
}
}
}
}

#endif
