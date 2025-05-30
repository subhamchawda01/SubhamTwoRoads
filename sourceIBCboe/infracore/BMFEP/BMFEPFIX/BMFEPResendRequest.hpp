// Resend Request Version 1.2.1

// When last seen seq no from exchange has a gap
// we will send a resend request .
// E.g say if last seen seqno was 55 and next msg we received
// from exchange has a number >= 57, we want the msg #no 56
// from exchange

#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"

#ifndef BMFEP_RESEND_REQUEST_HPP
#define BMFEP_RESEND_REQUEST_HPP

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

class ResendRequest {
 public:
  ResendRequest(const bool _make_optimize_assumptions_)
      : Header_Tag_8_(NULL),
        Header_Tag_9_(NULL),
        Header_Tag_35_(NULL),
        Header_Tag_34_(NULL),
        Header_Tag_43_(NULL),
        Header_Tag_49_(NULL),
        Header_Tag_50_(NULL),
        Header_Tag_52_(NULL),
        Header_Tag_56_(NULL),
        Header_Tag_57_(NULL),

        Resend_Tag_7_(NULL),
        Resend_Tag_16_(NULL),

        Trailer_Tag_10_(NULL),
        const_sum_(0),

        make_optimize_assumptions_(_make_optimize_assumptions_) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_ResendRequest_msg_size_ + BMFEP_Trailer_msg_size_);
  }

  void setFields(const char *sender_compID_, const char *target_compID_) {
    time_t UTCTime = time(NULL);
    struct tm *UTCTimePtr = gmtime(&UTCTime);

    int zero_ = 0;

    // setFields is called only once for the engine.
    // sprintfs are acceptable here.
    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ = Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%06d%c", zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=2%c", BMFEP_Delimiter_SOH_);
    Header_Tag_43_ =
        Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%*d%c", BMFEP_FIX_Tag_34_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_49_ = Header_Tag_43_ + sprintf(Header_Tag_43_, "43=N%c", BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_compID_, BMFEP_Delimiter_SOH_);
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    Resend_Tag_7_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_compID_, BMFEP_Delimiter_SOH_);
    Resend_Tag_16_ =
        Resend_Tag_7_ + sprintf(Resend_Tag_7_, "7=%*d%c", BMFEP_FIX_Tag_7_Width_, zero_, BMFEP_Delimiter_SOH_);

    // Trailer_Tag_10_ = Resend_Tag_16_ + sprintf (Resend_Tag_16_, "16=%*d%c",
    // 					      BMFEP_FIX_Tag_16_Width_,
    // 					      zero_, BMFEP_Delimiter_SOH_);

    // For all our purposes we will send Tag_16 = 0 which means
    // send all the messages starting from Tag 7
    Trailer_Tag_10_ = Resend_Tag_16_ + sprintf(Resend_Tag_16_, "16=0%c", BMFEP_Delimiter_SOH_);

    sprintf(Trailer_Tag_10_, "10=000%c", BMFEP_Delimiter_SOH_);

    // Set the message body length.
    // This guy won't change for different test requests.
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;
    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_ResendRequest_msg_size_ + BMFEP_Trailer_msg_size_;
         ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
    }

    const_sum_ = calcConstSum();
  }
  ~ResendRequest() {}

  inline unsigned int checksum() { return (const_sum_ + calcVarSum()) % 256; }

  inline void setCheckSum() {
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + 3, checksum(), BMFEP_FIX_Tag_10_Width_, false);
    // This should ALWAYS be false.
  }

  // TODO_OPT left pad only the number of bits needed.
  // We probably do not need seqnums that high
  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + 3, seq_num_, BMFEP_FIX_Tag_34_Width_,
                  /* 9 is the total size of the field, left padded with 0s*/
                  make_optimize_assumptions_);
  }

  inline void setBeginSeqNo(t_FIX_SeqNum begin_seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Resend_Tag_7_ + ONE_DIGIT_AND_EQUAL_WIDTH, begin_seq_num_, BMFEP_FIX_Tag_7_Width_,
                  make_optimize_assumptions_);
  }
  inline void SetTag7_16(char *tag_7_buff) {
    // Dont change the Resend_Tag_7_ pointer
    //	  unsigned int tag_7_len = (unsigned int ) strlen ( tag_7_buff);
    // memcpy ( Resend_Tag_7_ + ONE_DIGIT_AND_EQUAL_WIDTH ,
    // 	   tag_7_buff,
    // 	   tag_7_len);

    // copy in reverse
    // for ( unsigned int ii = 0; ii < tag_7_len ; ii ++ )
    //   {
    //     *( Resend_Tag_7_ + ONE_DIGIT_AND_EQUAL_WIDTH + tag_7_len - ii -1 )
    // 	= tag_7_buff[ii];
    //   }
    // Resend_Tag_16_ = Resend_Tag_7_ + ONE_DIGIT_AND_EQUAL_WIDTH + tag_7_len + 1;
    // *(Resend_Tag_16_ - 1) = BMFEP_Delimiter_SOH_;
    // memcpy ( Resend_Tag_16_ , (void*) "16=0", 4 ); // Always send 16= 0
    // // 1 for Zero and 1 for SOH
    // Trailer_Tag_10_ = Resend_Tag_16_ + TWO_DIGIT_AND_EQUAL_WIDTH + 1 + 1 ;
    // *(Trailer_Tag_10_ - 1) = BMFEP_Delimiter_SOH_;
    // memcpy ( Trailer_Tag_10_, "10=", TWO_DIGIT_AND_EQUAL_WIDTH );
    // *( Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ ) = BMFEP_Delimiter_SOH_;
    // *( Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ + 1) = '\0';

    // // Length has changed
    // printToString ( Header_Tag_9_ + ONE_DIGIT_AND_EQUAL_WIDTH , (int ) (Trailer_Tag_10_ - Header_Tag_35_ ),
    // 		  BMFEP_FIX_Tag_9_Width_ );
  }

  inline void SetDupField() { Header_Tag_43_[3] = 'Y'; }

  inline void ClearDupField() { Header_Tag_43_[3] = 'N'; }

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
    // Begin Seq No
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_7_Width_; ++t_count_) {
      sum_ += Resend_Tag_7_[t_count_ + 2];
    }

    // UTC Time
    sum_ += (*(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1) + *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1) + *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1) +
             *(Header_Tag_52_ + 3 + 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1 + 1));

    // Poss Dup flag.
    sum_ += Header_Tag_43_[3];
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_9_Width_; ++t_count_) {
      sum_ += Header_Tag_9_[t_count_ + 2];
    }

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
  char *Header_Tag_56_;
  char *Header_Tag_57_;

  char *Resend_Tag_7_;
  char *Resend_Tag_16_;

  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_ResendRequest_msg_size_ + BMFEP_Trailer_msg_size_];

  int const_sum_;

  const bool make_optimize_assumptions_;
};
}
}
}

#endif
