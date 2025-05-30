// Heartbeat file
// Only Tag we need to set/unset is Tag 112
// Version  1.2.1
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"

#ifndef BMFEP_HEART_BEAT_HPP
#define BMFEP_HEART_BEAT_HPP

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {
class Heartbeat {
 public:
  Heartbeat(const bool _make_optimize_assumptions_)
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
        Header_Tag_122_(NULL),
        Header_Tag_142_(NULL),
        Heartbeat_Tag_112_(NULL),
        Trailer_Tag_10_(NULL),
        const_sum_(0),
        make_optimize_assumptions_(_make_optimize_assumptions_) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_Heartbeat_msg_size_ + BMFEP_Trailer_msg_size_);
  }

  void setFields(const char *sender_compID_, const char *target_compID_) {
    // This is a simple way of maintaining synchronization of our clock with the BMFEP clock.
    // Subtracting 25 seconds, seemed to get the job done for us.
    // If difference between our time and BMFEP time is more than 4 secs, it could trigger rejections.
    time_t UTCTime = time(NULL);
    struct tm *UTCTimePtr = gmtime(&UTCTime);

    int zero_ = 0;

    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ = Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%06d%c", zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=0%c", BMFEP_Delimiter_SOH_);
    Header_Tag_43_ =
        Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%*d%c", BMFEP_FIX_Tag_34_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_49_ = Header_Tag_43_ + sprintf(Header_Tag_43_, "43=N%c", BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_compID_, BMFEP_Delimiter_SOH_);
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    Heartbeat_Tag_112_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_compID_, BMFEP_Delimiter_SOH_);
    Trailer_Tag_10_ = Heartbeat_Tag_112_;

    sprintf(Trailer_Tag_10_, "10=%*d%c", BMFEP_FIX_Tag_10_Width_, zero_, BMFEP_Delimiter_SOH_);

    // Set the message body length.
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;

    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_Heartbeat_msg_size_ + BMFEP_Trailer_msg_size_; ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
    }

    const_sum_ = calcConstSum();
  }
  inline void SendHeartbeat(t_FIX_SeqNum seq_num_, t_FIX_SeqNum last_proc_seq_num_, time_t UTCTime) {}

  ~Heartbeat() {}

  inline void SetTag112(char *tag_112_value_) {
    int tag_112_length_ = (int)strlen(tag_112_value_);

    memcpy(Heartbeat_Tag_112_, (void *)"112=", 4);
    memcpy(Heartbeat_Tag_112_ + THREE_DIGIT_AND_EQUAL_WIDTH, tag_112_value_, tag_112_length_);
    // Possibly more efficient than strcpy and helps us later.

    Trailer_Tag_10_ = Heartbeat_Tag_112_ + THREE_DIGIT_AND_EQUAL_WIDTH + tag_112_length_ + 1;
    *(Trailer_Tag_10_ - 1) = BMFEP_Delimiter_SOH_;

    memcpy(Trailer_Tag_10_, (void *)"10=", 3);
    Trailer_Tag_10_[6] = BMFEP_Delimiter_SOH_;
    Trailer_Tag_10_[7] = '\0';

    // Set body length field.
    printToString(Header_Tag_9_ + ONE_DIGIT_AND_EQUAL_WIDTH /* +2 to skip over '9=' */,
                  (int)(Trailer_Tag_10_ - Header_Tag_35_),
                  BMFEP_FIX_Tag_9_Width_ /* 6 is the total size of the field, left padded with 0s*/, false);
    // NEVER stop on zero for tag 9
  }

  inline void ClearTag112() {
    Trailer_Tag_10_ = Heartbeat_Tag_112_;

    memcpy(Trailer_Tag_10_, (void *)"10=", 3);
    Trailer_Tag_10_[6] = BMFEP_Delimiter_SOH_;
    Trailer_Tag_10_[7] = '\0';

    // Set body length field.
    printToString(Header_Tag_9_ + ONE_DIGIT_AND_EQUAL_WIDTH /* +2 to skip over '9=' */,
                  (int)(Trailer_Tag_10_ - Header_Tag_35_),
                  BMFEP_FIX_Tag_9_Width_ /* 6 is the total size of the field, left padded with 0s*/, false);
    // NEVER stop on zero for tag 9
  }

  inline unsigned int checksum() { return (const_sum_ + calcVarSum()) % 256; }

  inline void setCheckSum() {
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH, checksum(), BMFEP_FIX_Tag_10_Width_, false);
    // This should ALWAYS be false.
  }

  // TODO_OPT left pad only the number of bits needed.
  // We probably do not need seqnums that high
  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, seq_num_,
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

  inline unsigned int calcVarSum() {
    unsigned int sum_ = 0;

    // seq num
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_34_Width_; ++t_count_) {
      sum_ += Header_Tag_34_[t_count_ + 3];
    }

    // Add the Tag 112 ID field IF it exists.
    for (char *p_char_ = Heartbeat_Tag_112_; p_char_ != Trailer_Tag_10_; ++p_char_) {
      sum_ += *p_char_;
    }

    // The length of the msg is now variable because of the possibility of changing 112 tag above.
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_9_Width_; ++t_count_) {
      sum_ += Header_Tag_9_[t_count_ + 2];
    }

    // UTC Time
    sum_ += (*(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1) + *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1) + *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1 + 1));

    return sum_;
  }

  // Since only called once, made a little easier to read.
  unsigned int calcConstSum() {
    unsigned int sum_ = 0;

    for (char *p_msg_ = msg_char_buf_; p_msg_ != Trailer_Tag_10_; ++p_msg_) {
      // The Delimiter must be included in checksum computation.
      sum_ += *p_msg_;
    }

    return (sum_ - calcVarSum());
  }

 public:
  // A pointer to each tag.
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
  char *Header_Tag_122_;
  char *Header_Tag_142_;

  char *Heartbeat_Tag_112_;

  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_Heartbeat_msg_size_ + BMFEP_Trailer_msg_size_];

  int const_sum_;
  const bool make_optimize_assumptions_;
};
}
}
}

#endif
