// Definition of message lengths

#ifndef BMFEP_MESSAGE_DEFS_HPP
#define BMFEP_MESSAGE_DEFS_HPP

#include <iostream>
#include <fstream>
#include <cstdio>

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

// Taken from CME & BMFBELL: Assuming that BMFEP sizes will never be more than CME sizes
// which is in general true
// SOH
#define BMFEP_Delimiter_SOH_ (char)0x01

// Max field sizes. Need to account for all fields. Also need to account for the Delimiter between fields.
#define BMFEP_Header_msg_size_ (7 + 6 + 2 + 9 + 1 + 7 + 20 + 21 + 9 + 7 + 1 + 21 + 32 + /* delimiters */ 13)  // 156

#define BMFEP_Trailer_msg_size_ (3 + /* delimiters */ 1)

#define BMFEP_Logon_msg_size_ (2 + 20 + 1 + 3 + 1 + 50 + 1 + /* delimiters */ 9)
#define BMFEP_Logout_msg_size_ (9 + /* delimiters */ 1)

#define BMFEP_Heartbeat_msg_size_ (20 + /* delimiters */ 1)
#define BMFEP_TestRequest_msg_size_ (20 + /* delimiters */ 1)

#define BMFEP_SeqReset_msg_size_ (9 + /* delimiters */ 1)

// const int NewOrder_msg_size_ = (12 + 20 + 1 + 9 + 1 + 20 + 1 + 6 + 1 + 21 + 20 + 3 + 1 + 1 + 20 + /* delimiters */
// 15);
#define BMFEP_NewOrder_msg_size_ 500

#define BMFEP_CancelOrder_msg_size_ (12 + 20 + 17 + 20 + 1 + 6 + 21 + 20 + 3 + 20 + 6 + /* delimiters */ 10)
#define BMFEP_CancelReplace_msg_size_ \
  (12 + 20 + 1 + 17 + 9 + 1 + 20 + 20 + 1 + 6 + 18 + 21 + 20 + 3 + 1 + 1 + 20 + 1 + 6 + /* delimiters */ 18)

#define BMFEP_ResendRequest_msg_size_ (9 + 9 + /* delimiters */ 2)
}
}
}
#endif
