// Version 1.2.1
// Cancel Replace same as CME and BMFBELL

#include "dvccode/CDef/order.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPFIXTypes.hpp"

#ifndef BMFEP_CANCEL_REPLACE_HPP
#define BMFEP_CANCEL_REPLACE_HPP

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

class CancelReplace {
 public:
  CancelReplace(const char *sender_CompID_, const char *target_CompID_, const char *accountID_,
                const char *sender_location_, const char *entering_trader_, const char *entering_firm_,
                const char *securityID_, const char *symbol_, const bool _make_optimize_assumptions_, time_t time_)
      : Header_Tag_8_(NULL),
        Header_Tag_9_(NULL),
        Header_Tag_35_(NULL),
        Header_Tag_34_(NULL),
        Header_Tag_49_(NULL),
        Header_Tag_52_(NULL),
        Header_Tag_56_(NULL),
        CancelReplace_Tag_41_(NULL),
        //	 CancelReplace_Tag_48_(NULL),
        CancelReplace_Tag_78_(NULL),
        CancelReplace_Tag_79_(NULL),
        CancelReplace_Tag_11_(NULL),
        CancelReplace_Tag_37_(NULL),
        CancelReplace_Tag_38_(NULL),
        CancelReplace_Tag_40_(NULL),
        CancelReplace_Tag_44_(NULL),
        CancelReplace_Tag_54_(NULL),
        CancelReplace_Tag_55_(NULL),
        //	 CancelReplace_Tag_22_(NULL),
        CancelReplace_Tag_60_(NULL),
        CancelReplace_Tag_453_(NULL),
        CancelReplace_Tag_661_(NULL),
        CancelReplace_Tag_448_G1_(NULL),
        CancelReplace_Tag_447_G1_(NULL),
        CancelReplace_Tag_452_G1_(NULL),
        CancelReplace_Tag_448_G2_(NULL),
        CancelReplace_Tag_447_G2_(NULL),
        CancelReplace_Tag_452_G2_(NULL),
        CancelReplace_Tag_448_G3_(NULL),
        CancelReplace_Tag_447_G3_(NULL),
        CancelReplace_Tag_452_G3_(NULL),
        Trailer_Tag_10_(NULL),
        const_sum_(0),
        make_optimize_assumptions_(_make_optimize_assumptions_),
        write_len_(0),
        trader_id_(NULL) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_CancelReplace_msg_size_ + BMFEP_Trailer_msg_size_);

    time_t UTCTime = time_;
    struct tm *UTCTimePtr = gmtime(&UTCTime);
    const int zero_ = 0;

    // setFields is called only once for the engine.
    // sprintfs are acceptable here.
    trader_id_ = sender_CompID_;
    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ =
        Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%*d%c", BMFEP_FIX_Tag_9_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=G%c", BMFEP_Delimiter_SOH_);
    Header_Tag_49_ =
        Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%*d%c", BMFEP_FIX_Tag_34_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_CompID_, BMFEP_Delimiter_SOH_);
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_41_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_CompID_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_11_ = CancelReplace_Tag_41_ + sprintf(CancelReplace_Tag_41_, "41=%*d%c",
                                                            OUR_BMFEP_FIX_Tag_41_Width_, zero_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_55_ = CancelReplace_Tag_11_ + sprintf(CancelReplace_Tag_11_, "11=%*d%c",
                                                            OUR_BMFEP_FIX_Tag_11_Width_, zero_, BMFEP_Delimiter_SOH_);
    // CancelReplace_Tag_55_ = CancelReplace_Tag_37_ + sprintf (CancelReplace_Tag_37_,
    // 							   "37=%*d%c",
    // 							   BMFEP_FIX_Tag_37_Width_,
    // 							   zero_, BMFEP_Delimiter_SOH_);
    // Fill Instrument Identificaqtion block {55, 48, 22} only for BELL not for EP
    CancelReplace_Tag_54_ =
        CancelReplace_Tag_55_ + sprintf(CancelReplace_Tag_55_, "55=%s%c", symbol_, BMFEP_Delimiter_SOH_);
    // CancelReplace_Tag_22_ = CancelReplace_Tag_48_  + sprintf ( CancelReplace_Tag_48_ ,
    // 							     "48=%s%c",
    // 							     securityID_, BMFEP_Delimiter_SOH_);
    // CancelReplace_Tag_54_ = CancelReplace_Tag_22_ + sprintf ( CancelReplace_Tag_22_,
    // 							    "22=8%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_38_ = CancelReplace_Tag_54_ + sprintf(CancelReplace_Tag_54_, "54=0%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_40_ = CancelReplace_Tag_38_ + sprintf(CancelReplace_Tag_38_, "38=%*d%c", BMFEP_FIX_Tag_38_Width_,
                                                            zero_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_44_ = CancelReplace_Tag_40_ + sprintf(CancelReplace_Tag_40_, "40=2%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_78_ = CancelReplace_Tag_44_ + sprintf(CancelReplace_Tag_44_, "44=%*d%c", BMFEP_FIX_Tag_44_Width_,
                                                            zero_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_79_ = CancelReplace_Tag_78_ + sprintf(CancelReplace_Tag_78_, "78=1%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_661_ = CancelReplace_Tag_79_ + sprintf(CancelReplace_Tag_79_, "79=1%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_453_ = CancelReplace_Tag_661_ + sprintf(CancelReplace_Tag_661_, "661=99%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_448_G1_ =
        CancelReplace_Tag_453_ + sprintf(CancelReplace_Tag_453_, "453=3%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_447_G1_ = CancelReplace_Tag_448_G1_ +
                                sprintf(CancelReplace_Tag_448_G1_, "448=%s%c", sender_location_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_452_G1_ =
        CancelReplace_Tag_447_G1_ + sprintf(CancelReplace_Tag_447_G1_, "447=D%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_448_G2_ =
        CancelReplace_Tag_452_G1_ + sprintf(CancelReplace_Tag_452_G1_, "452=54%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_447_G2_ = CancelReplace_Tag_448_G2_ +
                                sprintf(CancelReplace_Tag_448_G2_, "448=%s%c", entering_trader_, BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_452_G2_ =
        CancelReplace_Tag_447_G2_ + sprintf(CancelReplace_Tag_447_G2_, "447=D%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_448_G3_ =
        CancelReplace_Tag_452_G2_ + sprintf(CancelReplace_Tag_452_G2_, "452=36%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_447_G3_ =
        CancelReplace_Tag_448_G3_ +
        sprintf(CancelReplace_Tag_448_G3_, "448=%s%c", entering_firm_, BMFEP_Delimiter_SOH_);  /// 999 for BELL
    CancelReplace_Tag_452_G3_ =
        CancelReplace_Tag_447_G3_ + sprintf(CancelReplace_Tag_447_G3_, "447=D%c", BMFEP_Delimiter_SOH_);
    CancelReplace_Tag_60_ =
        CancelReplace_Tag_452_G3_ + sprintf(CancelReplace_Tag_452_G3_, "452=7%c", BMFEP_Delimiter_SOH_);

    Trailer_Tag_10_ = CancelReplace_Tag_60_ + sprintf(CancelReplace_Tag_60_, "60=%04d%02d%02d-HH:MM:SS.000%c",
                                                      UTCTimePtr->tm_year + 1900, UTCTimePtr->tm_mon + 1,
                                                      UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    sprintf(Trailer_Tag_10_, "10=%*d%c", BMFEP_FIX_Tag_10_Width_, zero_, BMFEP_Delimiter_SOH_);

    // Set the message body length. This is also going to be constant.
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;

    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_CancelReplace_msg_size_ + BMFEP_Trailer_msg_size_;
         ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
    }

    const_sum_ = calcConstSum();

    // + 3 to skip over "10=", +3 to skip over "XXX" <- checksum and +1 to skip over ^A at the end.
    write_len_ = (unsigned int)(Trailer_Tag_10_ + 3 + 3 + 1 - Header_Tag_8_);
  }

  ~CancelReplace() {}

  inline unsigned int getWriteLen() const { return write_len_; }

  inline unsigned int checksum() { return (const_sum_ + calcVarSum()) % 256; }

  // TODO_OPT left pad only the number of bits needed.
  // We probably do not need seqnums that high
  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, seq_num_,
                  BMFEP_FIX_Tag_34_Width_ /* 9 is the total size of the field, left padded with 0s*/,
                  make_optimize_assumptions_);
  }

  inline uint32_t SetDynamicCancelReplaceOrderFieldsUsingOrderStruct(HFSAT::ORS::Order const *rp_order_,
                                                                     time_t last_send_time_,
                                                                     BMFEPFIX::t_FIX_SeqNum const last_seq_num_) {
    // setClOrdID
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(CancelReplace_Tag_11_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->server_assigned_order_sequence_,
                  OUR_BMFEP_FIX_Tag_11_Width_, make_optimize_assumptions_);

    // setOrigClOrdID
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(CancelReplace_Tag_41_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->server_assigned_order_sequence_,
                  OUR_BMFEP_FIX_Tag_41_Width_, make_optimize_assumptions_);

    // setSide
    *(CancelReplace_Tag_54_ + TWO_DIGIT_AND_EQUAL_WIDTH) = (((rp_order_->buysell_ == kTradeTypeBuy) ? 1 : 2) + '0');

    // setUTCTime
    time_t UTCTime = last_send_time_;

    int tm_hour, tm_min, tm_sec;
    fastgmtime(&UTCTime, &tm_hour, &tm_min, &tm_sec);

    // The constants being added together may look inefficient,
    // but I believe they are folded into 1 constant at compile time.
    // They are written in this form, to help me understand a bit of what's happening.
    // Assuming that the fields YYYY MM DD will not change intra day - print fewer chars.

    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH) = bmf_time_digits[tm_hour << 1];
    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 1) = bmf_time_digits[(tm_hour << 1) + 1];

    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1) = bmf_time_digits[tm_min << 1];
    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 1) =
        bmf_time_digits[(tm_min << 1) + 1];
    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 2 + 1) =
        bmf_time_digits[tm_sec << 1];
    *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 2 + 1 + 1) =
        bmf_time_digits[(tm_sec << 1) + 1];

    // setUTCTransactTime
    // This is strictly speaking, not correct, but works and saves another call to time ().
    memcpy(CancelReplace_Tag_60_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH,
           Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH, 8);

    // setOrderQty
    // CancelReplace Qty should always be printed with false.
    printToString(CancelReplace_Tag_38_ + TWO_DIGIT_AND_EQUAL_WIDTH,
                  rp_order_->size_remaining_ + rp_order_->size_executed_, BMFEP_FIX_Tag_38_Width_, false);

    // setPrice
    printToString(CancelReplace_Tag_44_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->price_, BMFEP_FIX_Tag_44_Width_, 7);

    // setSeqNum
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, last_seq_num_,
                  BMFEP_FIX_Tag_34_Width_ /* 9 is the total size of the field, left padded with 0s*/,
                  make_optimize_assumptions_);

    // setCheckSum
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH, checksum(), BMFEP_FIX_Tag_10_Width_,
                  false);  // This should ALWAYS be false.

    return write_len_;
  }

  inline unsigned int calcVarSum() {
    unsigned int sum_ = 0;
    // Seno
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_34_Width_; ++t_count_) {
      sum_ += Header_Tag_34_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // ClordID
    for (unsigned int t_count_ = 0; t_count_ < OUR_BMFEP_FIX_Tag_11_Width_; ++t_count_) {
      sum_ += (CancelReplace_Tag_11_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH]);  //  for 11
    }

    // OriginalCLorID
    for (unsigned int t_count_ = 0; t_count_ < OUR_BMFEP_FIX_Tag_41_Width_; ++t_count_) {
      sum_ += (CancelReplace_Tag_41_[t_count_ + 3]);  // 41
    }

    // qty
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_38_Width_; ++t_count_) {
      sum_ += CancelReplace_Tag_38_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // price
    for (unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_44_Width_; ++t_count_) {
      sum_ += CancelReplace_Tag_44_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // side
    for (unsigned int t_count_ = 0; t_count_ < 1; ++t_count_) {
      sum_ += CancelReplace_Tag_54_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // UTC Time and UTCTransact time
    // 1 for 52 and another for 60
    sum_ += (2 * (*(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH) +
                  *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 1) +
                  *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1) +
                  *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 1) +
                  *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 2 + 1) +
                  *(Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH + 2 + 1 + 2 + 1 + 1)));
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
  // Constants will be pre-calculated at compile time.
  // For greater optimization, change these to char *. // TODO_OPT
  char *Header_Tag_8_;
  char *Header_Tag_9_;
  char *Header_Tag_35_;
  char *Header_Tag_34_;
  char *Header_Tag_49_;
  char *Header_Tag_52_;
  char *Header_Tag_56_;

  char *CancelReplace_Tag_41_;
  //	char *CancelReplace_Tag_48_;
  char *CancelReplace_Tag_78_;
  char *CancelReplace_Tag_79_;
  char *CancelReplace_Tag_11_;
  char *CancelReplace_Tag_37_;
  char *CancelReplace_Tag_38_;
  char *CancelReplace_Tag_40_;
  char *CancelReplace_Tag_44_;
  char *CancelReplace_Tag_54_;
  char *CancelReplace_Tag_55_;
  //	char *CancelReplace_Tag_22_;
  char *CancelReplace_Tag_60_;
  char *CancelReplace_Tag_453_;
  char *CancelReplace_Tag_661_;
  char *CancelReplace_Tag_448_G1_;
  char *CancelReplace_Tag_447_G1_;
  char *CancelReplace_Tag_452_G1_;
  char *CancelReplace_Tag_448_G2_;
  char *CancelReplace_Tag_447_G2_;
  char *CancelReplace_Tag_452_G2_;
  char *CancelReplace_Tag_448_G3_;
  char *CancelReplace_Tag_447_G3_;
  char *CancelReplace_Tag_452_G3_;

  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_CancelReplace_msg_size_ + BMFEP_Trailer_msg_size_];

  unsigned int const_sum_;

  const bool make_optimize_assumptions_;

  unsigned int write_len_;
  const char *trader_id_;
};
}
}
}

#endif
