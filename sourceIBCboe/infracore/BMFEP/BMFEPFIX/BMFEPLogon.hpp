
// Following Tags are needed as of Version 1.2.1
// 98, 108, 95, 96
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"

#ifndef BMFEP_LOGON_HPP
#define BMFEP_LOGON_HPP

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

class Logon {
 public:
  Logon(const bool _make_optimize_assumptions_)
      : Header_Tag_8_(NULL),
        Header_Tag_9_(NULL),
        Header_Tag_35_(NULL),
        Header_Tag_34_(NULL),
        Header_Tag_43_(NULL),
        Header_Tag_49_(NULL),
        Header_Tag_50_(NULL),
        Header_Tag_52_(NULL),
        Header_Tag_369_(NULL),
        Header_Tag_56_(NULL),
        Header_Tag_57_(NULL),

        Logon_Tag_95_(NULL),
        Logon_Tag_96_(NULL),
        Logon_Tag_98_(NULL),
        Logon_Tag_108_(NULL),
        Logon_Tag_141_(NULL),
        Logon_Tag_35002_(NULL),  // this is for COD
        Logon_Tag_58_(NULL),     // this is for COD
        Trailer_Tag_10_(NULL),

        make_optimize_assumptions_(_make_optimize_assumptions_) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_Logon_msg_size_ + BMFEP_Trailer_msg_size_);
  }

  void setFields(const char *sender_compID_, const char *target_compID_,
                 // const char *target_subID_, const char *sender_subID_,
                 const char *password_, const char *appl_name_, const char *appl_version_
                 //, const char *sender_locationID_)
                 ) {
    int zero_ = 0;

    // This is a simple way of maintaining synchronization of our clock with the BMFEP clock.
    // Subtracting 25 seconds, seemed to get the job done for us.
    // If difference between our time and BMFEP time is more than 4 secs, it could trigger rejections.
    time_t UTCTime = time(NULL) - 25;
    struct tm *UTCTimePtr = gmtime(&UTCTime);

    // setFields is called only once for the engine.
    // sprintfs are acceptable here.
    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ = Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%06d%c", zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=A%c", BMFEP_Delimiter_SOH_);
    Header_Tag_43_ = Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%09d%c", zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_49_ = Header_Tag_43_ + sprintf(Header_Tag_43_, "43=N%c", BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_compID_, BMFEP_Delimiter_SOH_);
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);

    Logon_Tag_95_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_compID_, BMFEP_Delimiter_SOH_);
    Logon_Tag_96_ = Logon_Tag_95_ + sprintf(Logon_Tag_95_, "95=%d%c", (int)strlen(password_), BMFEP_Delimiter_SOH_);
    Logon_Tag_98_ = Logon_Tag_96_ + sprintf(Logon_Tag_96_, "96=%s%c", password_, BMFEP_Delimiter_SOH_);
    Logon_Tag_108_ = Logon_Tag_98_ + sprintf(Logon_Tag_98_, "98=0%c", BMFEP_Delimiter_SOH_);
    Logon_Tag_141_ = Logon_Tag_108_ + sprintf(Logon_Tag_108_, "108=30%c", BMFEP_Delimiter_SOH_);

    Logon_Tag_35002_ = Logon_Tag_141_ + sprintf(Logon_Tag_141_, "141=N%c", BMFEP_Delimiter_SOH_);
    Logon_Tag_58_ = Logon_Tag_35002_ + sprintf(Logon_Tag_35002_, "35002=3%c", BMFEP_Delimiter_SOH_);
    Trailer_Tag_10_ = Logon_Tag_58_ + sprintf(Logon_Tag_58_, "58=%s%c%s%c", appl_name_, (char)2, appl_version_,
                                              BMFEP_Delimiter_SOH_);  // Cancel on disconnect or logout - 3
    sprintf(Trailer_Tag_10_, "10=000%c", BMFEP_Delimiter_SOH_);

    // Set the message body length. This is also going to be constant.
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;

    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_Logon_msg_size_ + BMFEP_Trailer_msg_size_; ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
      if (msg_char_buf_[i] == (char)2) msg_char_buf_[i] = ' ';
    }
  }
  ~Logon() {}

  int checksum() {
    int sum_ = 0;
    for (char *p_msg_ = msg_char_buf_; p_msg_ != Trailer_Tag_10_; ++p_msg_) {
      // The Delimiter must be included in checksum computation.
      sum_ += *p_msg_;
    }

    return sum_ % 256;
  }

  inline void setCheckSum() {
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + 3, checksum(), BMFEP_FIX_Tag_10_Width_, false);  // This should ALWAYS be false.
  }

  // TODO_OPT left pad only the number of bits needed.
  // We probably do not need seqnums that high
  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + 3, seq_num_,
                  BMFEP_FIX_Tag_34_Width_ /* 9 is the total size of the field, left padded with 0s*/,
                  make_optimize_assumptions_);
  }

  // TODO_OPT : Find a way to optimize this.
  inline void setUTCTime(time_t UTCTime) {
    int tm_hour, tm_min, tm_sec;
    fastgmtime(&UTCTime, &tm_hour, &tm_min, &tm_sec);

    // The constants being added together may look inefficient,
    // but I believe they are folded into 1 constant at compile time.
    // They are written in this form, to help me understand a bit of what's happening.
    // Assuming that the fields YYYY MM DD will not change intra day - print fewer chars.

    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1) = (tm_hour / 10) + '0';
    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 1) = (tm_hour % 10) + '0';

    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1) = (tm_min / 10) + '0';
    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 1) = (tm_min % 10) + '0';

    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1) = (tm_sec / 10) + '0';
    *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1 + 1) = (tm_sec % 10) + '0';
  }

 public:
  // Constants will be pre-calculated at compile time.
  // For greater optimization, change these to char *. // TODO_OPT
  char *Header_Tag_8_;
  char *Header_Tag_9_;
  char *Header_Tag_35_;
  char *Header_Tag_34_;
  char *Header_Tag_43_;
  char *Header_Tag_49_;
  char *Header_Tag_50_;
  char *Header_Tag_52_;
  char *Header_Tag_369_;
  char *Header_Tag_56_;
  char *Header_Tag_57_;

  char *Logon_Tag_95_;
  char *Logon_Tag_96_;
  char *Logon_Tag_98_;
  char *Logon_Tag_108_;
  char *Logon_Tag_141_;
  char *Logon_Tag_35002_;
  char *Logon_Tag_58_;
  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_Logon_msg_size_ + BMFEP_Trailer_msg_size_];
  const bool make_optimize_assumptions_;
};
}
}
}

#endif
