// Cancel order Version 1.2.1 Doc
// Similar to CME and BMF BELL

#include "dvccode/CDef/order.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPFIXTypes.hpp"

#ifndef BMFEP_CANCEL_ORDER_HPP
#define BMFEP_CANCEL_ORDER_HPP

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {
class CancelOrder {
 public:
  CancelOrder(const char *sender_CompID_, const char *target_CompID_, const char *accountID_,
              const char *sender_location_, const char *entering_trader_, const char *entering_firm_,
              const char *securityID_, const char *symbol_, const bool _make_optimize_assumptions_, time_t time_)
      : Header_Tag_8_(NULL),
        Header_Tag_9_(NULL),
        Header_Tag_35_(NULL),
        Header_Tag_34_(NULL),
        Header_Tag_49_(NULL),
        Header_Tag_52_(NULL),
        Header_Tag_56_(NULL),

        OrderCancel_Tag_11_(NULL),
        OrderCancel_Tag_22_(NULL),
        OrderCancel_Tag_37_(NULL),
        OrderCancel_Tag_41_(NULL),
        OrderCancel_Tag_54_(NULL),
        OrderCancel_Tag_48_(NULL),
        OrderCancel_Tag_55_(NULL),
        OrderCancel_Tag_60_(NULL),
        OrderCancel_Tag_38_(NULL),

        // NoPartyId
        OrderCancel_Tag_453_(NULL),
        // NoPartyId Group 1
        OrderCancel_Tag_448_G1_(NULL),
        OrderCancel_Tag_447_G1_(NULL),
        OrderCancel_Tag_452_G1_(NULL),
        // NoPartyId, Group 2
        OrderCancel_Tag_448_G2_(NULL),
        OrderCancel_Tag_447_G2_(NULL),
        OrderCancel_Tag_452_G2_(NULL),
        // NoPartyId Group 3
        OrderCancel_Tag_448_G3_(NULL),
        OrderCancel_Tag_447_G3_(NULL),
        OrderCancel_Tag_452_G3_(NULL),

        Trailer_Tag_10_(NULL),
        const_sum_(0),

        make_optimize_assumptions_(_make_optimize_assumptions_),
        write_len_(0) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_CancelOrder_msg_size_ + BMFEP_Trailer_msg_size_);

    // This is a simple way of maintaining synchronization of our clock with the BMFEP clock.
    // Subtracting 25 seconds, seemed to get the job done for us.
    // If difference between our time and BMFEP time is more than 4 secs, it could trigger rejections.
    time_t UTCTime = time_;
    struct tm *UTCTimePtr = gmtime(&UTCTime);

    int zero_ = 0;

    // setFields is called only once for the engine.
    // sprintfs are acceptable here.
    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ =
        Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%*d%c", BMFEP_FIX_Tag_9_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=F%c", BMFEP_Delimiter_SOH_);
    Header_Tag_49_ =
        Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%*d%c", BMFEP_FIX_Tag_34_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_CompID_, BMFEP_Delimiter_SOH_);
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_11_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_CompID_, BMFEP_Delimiter_SOH_);

    OrderCancel_Tag_41_ = OrderCancel_Tag_11_ + sprintf(OrderCancel_Tag_11_, "11=%*d%c", OUR_BMFEP_FIX_Tag_11_Width_,
                                                        zero_, BMFEP_Delimiter_SOH_);

    OrderCancel_Tag_54_ = OrderCancel_Tag_41_ + sprintf(OrderCancel_Tag_41_, "41=%*d%c", OUR_BMFEP_FIX_Tag_41_Width_,
                                                        zero_, BMFEP_Delimiter_SOH_);

    OrderCancel_Tag_55_ = OrderCancel_Tag_54_ + sprintf(OrderCancel_Tag_54_, "54=0%c", BMFEP_Delimiter_SOH_);
    // OrderCancel_Tag_ = OrderCancel_Tag_48_ + sprintf ( OrderCancel_Tag_48_,
    // 							"48=%s%c",
    // 							securityID_,
    // 							BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_38_ = OrderCancel_Tag_55_ + sprintf(OrderCancel_Tag_55_, "55=%s%c", symbol_, BMFEP_Delimiter_SOH_);

    OrderCancel_Tag_60_ = OrderCancel_Tag_38_ + sprintf(OrderCancel_Tag_38_, "38=%*d%c", BMFEP_FIX_Tag_38_Width_, zero_,
                                                        BMFEP_Delimiter_SOH_);

    OrderCancel_Tag_453_ =
        OrderCancel_Tag_60_ + sprintf(OrderCancel_Tag_60_, "60=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                      UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_448_G1_ = OrderCancel_Tag_453_ + sprintf(OrderCancel_Tag_453_, "453=3%c", BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_447_G1_ =
        OrderCancel_Tag_448_G1_ + sprintf(OrderCancel_Tag_448_G1_, "448=%s%c", sender_location_, BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_452_G1_ =
        OrderCancel_Tag_447_G1_ + sprintf(OrderCancel_Tag_447_G1_, "447=D%c", BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_448_G2_ =
        OrderCancel_Tag_452_G1_ + sprintf(OrderCancel_Tag_452_G1_, "452=54%c", BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_447_G2_ =
        OrderCancel_Tag_448_G2_ + sprintf(OrderCancel_Tag_448_G2_, "448=%s%c", entering_trader_, BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_452_G2_ =
        OrderCancel_Tag_447_G2_ + sprintf(OrderCancel_Tag_447_G2_, "447=D%c", BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_448_G3_ =
        OrderCancel_Tag_452_G2_ + sprintf(OrderCancel_Tag_452_G2_, "452=36%c", BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_447_G3_ =
        OrderCancel_Tag_448_G3_ + sprintf(OrderCancel_Tag_448_G3_, "448=%s%c", entering_firm_, BMFEP_Delimiter_SOH_);
    OrderCancel_Tag_452_G3_ =
        OrderCancel_Tag_447_G3_ + sprintf(OrderCancel_Tag_447_G3_, "447=D%c", BMFEP_Delimiter_SOH_);
    Trailer_Tag_10_ = OrderCancel_Tag_452_G3_ + sprintf(OrderCancel_Tag_452_G3_, "452=7%c", BMFEP_Delimiter_SOH_);

    sprintf(Trailer_Tag_10_, "10=%*d%c", BMFEP_FIX_Tag_10_Width_, zero_, BMFEP_Delimiter_SOH_);

    // Set the message body length. This is also going to be constant.
    // TODO: 6 is still hardocded, make sure to change it if BMFEP_FIX_Tag_9_Width_ changes
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;

    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_CancelOrder_msg_size_ + BMFEP_Trailer_msg_size_; ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
    }

    const_sum_ = calcConstSum();
    // Trailer_Tag_10_ + 3 + 3 + 1 ==> points to Second BMFEP_Delimiter_SOH_ character in 10=000^A^A
    write_len_ = (unsigned int)(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                                DELIMITER_SOH_WIDTH_ - Header_Tag_8_);
  }
  ~CancelOrder() {}

  inline unsigned int getWriteLen() const { return write_len_; }

  inline unsigned int checksum() { return (const_sum_ + calcVarSum()) % 256; }

  // TODO_OPT left pad only the number of bits needed.
  // We probably do not need seqnums that high
  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, seq_num_, BMFEP_FIX_Tag_34_Width_,
                  make_optimize_assumptions_);
  }

  inline uint32_t SetDynamicCancelOrderFieldsUsingOrderStruct(HFSAT::ORS::Order const *rp_order_,
                                                              time_t last_send_time_,
                                                              BMFEPFIX::t_FIX_SeqNum const last_seq_num_) {
    // setClOrdID
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(OrderCancel_Tag_11_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->server_assigned_order_sequence_,
                  OUR_BMFEP_FIX_Tag_11_Width_, false);

    // setOrigClOrdID
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(OrderCancel_Tag_41_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->server_assigned_order_sequence_,
                  OUR_BMFEP_FIX_Tag_41_Width_, false);

    // setSide
    *(OrderCancel_Tag_54_ + TWO_DIGIT_AND_EQUAL_WIDTH) = (((rp_order_->buysell_ == kTradeTypeBuy) ? 1 : 2) + '0');

    // setCancelOrderQty
    // Order Qty should always be printed with false.
    printToString(OrderCancel_Tag_38_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->size_remaining_, BMFEP_FIX_Tag_38_Width_,
                  false);

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
    memcpy(OrderCancel_Tag_60_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH,
           Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH, 8);

    // setSeqNum
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, last_seq_num_, BMFEP_FIX_Tag_34_Width_,
                  make_optimize_assumptions_);

    // setCheckSum
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH, checksum(), BMFEP_FIX_Tag_10_Width_,
                  false);  // This should ALWAYS be false.

    return write_len_;
  }

  inline unsigned int calcVarSum() {
    unsigned int sum_ = 0;

    for (register unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_34_Width_; ++t_count_) {
      sum_ += Header_Tag_34_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // ClordID
    for (register unsigned int t_count_ = 0; t_count_ < OUR_BMFEP_FIX_Tag_11_Width_; ++t_count_) {
      sum_ += OrderCancel_Tag_11_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];  // 1
    }

    // side
    for (register unsigned int t_count_ = 0; t_count_ < 1; ++t_count_) {
      sum_ += OrderCancel_Tag_54_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }
    for (register unsigned int t_count_ = 0; t_count_ < OUR_BMFEP_FIX_Tag_41_Width_; ++t_count_) {
      sum_ += OrderCancel_Tag_41_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }
    for (register unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_38_Width_; ++t_count_) {
      sum_ += OrderCancel_Tag_38_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

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

  char *OrderCancel_Tag_11_;
  char *OrderCancel_Tag_22_;
  char *OrderCancel_Tag_37_;
  char *OrderCancel_Tag_41_;
  char *OrderCancel_Tag_54_;
  char *OrderCancel_Tag_48_;
  char *OrderCancel_Tag_55_;
  char *OrderCancel_Tag_60_;
  char *OrderCancel_Tag_38_;

  // NoPartyId
  char *OrderCancel_Tag_453_;
  // NoPartyId Group 1
  char *OrderCancel_Tag_448_G1_;
  char *OrderCancel_Tag_447_G1_;
  char *OrderCancel_Tag_452_G1_;
  // NoPartyId, Group 2
  char *OrderCancel_Tag_448_G2_;
  char *OrderCancel_Tag_447_G2_;
  char *OrderCancel_Tag_452_G2_;
  // NoPartyId Group 3
  char *OrderCancel_Tag_448_G3_;
  char *OrderCancel_Tag_447_G3_;
  char *OrderCancel_Tag_452_G3_;

  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_CancelOrder_msg_size_ + BMFEP_Trailer_msg_size_];

  int const_sum_;

  const bool make_optimize_assumptions_;

  unsigned int write_len_;
};
}
}
}

#endif
