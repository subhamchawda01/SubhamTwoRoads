// Version 1.2.1
// Similar to CME & BMFBELL

#ifndef BASE_BMFEPB_BMFEPFIX_BMFEP_NEW_ORDER_HPP
#define BASE_BMFEPB_BMFEPFIX_BMFEP_NEW_ORDER_HPP

#include "dvccode/CDef/order.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPFIXTypes.hpp"

namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

class NewOrder {
 public:
  NewOrder(const char *sender_CompID_, const char *target_CompID_, const char *accountID_, const char *sender_location_,
           const char *entering_trader_, const char *entering_firm_, const char *securityID_, const char *symbol_,
           const bool _make_optimize_assumptions_, time_t time_, bool const is_exchange_stp_enabled_)
      : Header_Tag_8_(NULL),
        Header_Tag_9_(NULL),
        Header_Tag_35_(NULL),
        Header_Tag_34_(NULL),
        Header_Tag_43_(NULL),
        Header_Tag_49_(NULL),
        Header_Tag_52_(NULL),
        Header_Tag_56_(NULL),

        Order_Tag_1_(NULL),
        Order_Tag_11_(NULL),
        // Order_Tag_22_(NULL),
        Order_Tag_38_(NULL),
        Order_Tag_40_(NULL),
        Order_Tag_44_(NULL),
        // Order_Tag_48_(NULL),
        Order_Tag_54_(NULL),
        Order_Tag_55_(NULL),
        Order_Tag_60_(NULL),
        Order_Tag_447_G1_(NULL),
        Order_Tag_448_G1_(NULL),
        Order_Tag_452_G1_(NULL),

        Order_Tag_447_G2_(NULL),
        Order_Tag_448_G2_(NULL),
        Order_Tag_452_G2_(NULL),

        Order_Tag_447_G3_(NULL),
        Order_Tag_448_G3_(NULL),
        Order_Tag_452_G3_(NULL),

        Order_Tag_447_G4_(NULL),
        Order_Tag_448_G4_(NULL),
        Order_Tag_452_G4_(NULL),

        Order_Tag_453_(NULL),
        Trailer_Tag_10_(NULL),
        const_sum_(0),

        make_optimize_assumptions_(_make_optimize_assumptions_),
        write_len_(0) {
    bzero(msg_char_buf_, BMFEP_Header_msg_size_ + BMFEP_NewOrder_msg_size_ + BMFEP_Trailer_msg_size_);

    // This is a simple way of maintaining synchronization of our clock with the BMFEP clock.
    // Subtracting 25 seconds, seemed to get the job done for us.
    // If difference between our time and BMFEP time is more than 4 secs, it could trigger rejections.
    time_t UTCTime = time_;
    struct tm *UTCTimePtr = gmtime(&UTCTime);

    const int zero_ = 0;

    // setFields is called only once for the engine.
    // sprintfs are acceptable here.
    // Trader_id = for not is compID that we are tading with

    Header_Tag_8_ = msg_char_buf_;
    Header_Tag_9_ = Header_Tag_8_ + sprintf(Header_Tag_8_, "8=FIX.4.4%c", BMFEP_Delimiter_SOH_);
    Header_Tag_35_ =
        Header_Tag_9_ + sprintf(Header_Tag_9_, "9=%*d%c", BMFEP_FIX_Tag_9_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_34_ = Header_Tag_35_ + sprintf(Header_Tag_35_, "35=D%c", BMFEP_Delimiter_SOH_);
    Header_Tag_49_ =
        Header_Tag_34_ + sprintf(Header_Tag_34_, "34=%*d%c", BMFEP_FIX_Tag_34_Width_, zero_, BMFEP_Delimiter_SOH_);
    Header_Tag_52_ = Header_Tag_49_ + sprintf(Header_Tag_49_, "49=%s%c", sender_CompID_, BMFEP_Delimiter_SOH_);
    ;
    Header_Tag_56_ =
        Header_Tag_52_ + sprintf(Header_Tag_52_, "52=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                 UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);
    // Account Id doesnot matter, send any value
    Order_Tag_1_ = Header_Tag_56_ + sprintf(Header_Tag_56_, "56=%s%c", target_CompID_, BMFEP_Delimiter_SOH_);
    Order_Tag_11_ = Order_Tag_1_ + sprintf(Order_Tag_1_, "1=%s%c", accountID_, BMFEP_Delimiter_SOH_);
    // Tag 11 is a string, MUST BE distinguished "000123" vs "123", CAREFUL while responding to
    // Exchange assigned OrderID for cancel etc.
    Order_Tag_38_ =
        Order_Tag_11_ + sprintf(Order_Tag_11_, "11=%*d%c", OUR_BMFEP_FIX_Tag_11_Width_, zero_, BMFEP_Delimiter_SOH_);
    // Order_Tag_38_ = Order_Tag_22_ + sprintf (Order_Tag_22_, "22=8%c", BMFEP_Delimiter_SOH_);
    Order_Tag_40_ =
        Order_Tag_38_ + sprintf(Order_Tag_38_, "38=%*d%c", BMFEP_FIX_Tag_38_Width_, zero_, BMFEP_Delimiter_SOH_);
    Order_Tag_44_ = Order_Tag_40_ + sprintf(Order_Tag_40_, "40=2%c", BMFEP_Delimiter_SOH_);
    Order_Tag_54_ =
        Order_Tag_44_ + sprintf(Order_Tag_44_, "44=%*d%c", BMFEP_FIX_Tag_44_Width_, zero_, BMFEP_Delimiter_SOH_);
    // Order_Tag_54_ = Order_Tag_48_ + sprintf (Order_Tag_48_, "48=%s%c", securityID_, BMFEP_Delimiter_SOH_);
    Order_Tag_55_ = Order_Tag_54_ + sprintf(Order_Tag_54_, "54=0%c", BMFEP_Delimiter_SOH_);
    Order_Tag_60_ = Order_Tag_55_ + sprintf(Order_Tag_55_, "55=%s%c", symbol_, BMFEP_Delimiter_SOH_);
    Order_Tag_453_ =
        Order_Tag_60_ + sprintf(Order_Tag_60_, "60=%04d%02d%02d-HH:MM:SS.000%c", UTCTimePtr->tm_year + 1900,
                                UTCTimePtr->tm_mon + 1, UTCTimePtr->tm_mday, BMFEP_Delimiter_SOH_);

    // Order_Tag_79_ = Order_Tag_78_ + sprintf (Order_Tag_78_, "78=1%c", BMFEP_Delimiter_SOH_ ) ;
    // Order_Tag_661_ = Order_Tag_79_ + sprintf ( Order_Tag_79_,"79=1%c", BMFEP_Delimiter_SOH_);
    // Order_Tag_453_ = Order_Tag_661_ + sprintf ( Order_Tag_661_,"661=99%c", BMFEP_Delimiter_SOH_);

    if (true == is_exchange_stp_enabled_) {
      Order_Tag_448_G1_ = Order_Tag_453_ + sprintf(Order_Tag_453_, "453=4%c", BMFEP_Delimiter_SOH_);
    } else {
      Order_Tag_448_G1_ = Order_Tag_453_ + sprintf(Order_Tag_453_, "453=3%c", BMFEP_Delimiter_SOH_);
    }

    Order_Tag_447_G1_ =
        Order_Tag_448_G1_ + sprintf(Order_Tag_448_G1_, "448=%s%c", sender_location_, BMFEP_Delimiter_SOH_);
    Order_Tag_452_G1_ = Order_Tag_447_G1_ + sprintf(Order_Tag_447_G1_, "447=D%c", BMFEP_Delimiter_SOH_);
    Order_Tag_448_G2_ = Order_Tag_452_G1_ + sprintf(Order_Tag_452_G1_, "452=54%c", BMFEP_Delimiter_SOH_);
    Order_Tag_447_G2_ =
        Order_Tag_448_G2_ + sprintf(Order_Tag_448_G2_, "448=%s%c", entering_trader_, BMFEP_Delimiter_SOH_);
    Order_Tag_452_G2_ = Order_Tag_447_G2_ + sprintf(Order_Tag_447_G2_, "447=D%c", BMFEP_Delimiter_SOH_);
    Order_Tag_448_G3_ = Order_Tag_452_G2_ + sprintf(Order_Tag_452_G2_, "452=36%c", BMFEP_Delimiter_SOH_);
    Order_Tag_447_G3_ = Order_Tag_448_G3_ + sprintf(Order_Tag_448_G3_, "448=%s%c", entering_firm_,
                                                    BMFEP_Delimiter_SOH_);  // 8 for Entry Point, 999 for BELL
    Order_Tag_452_G3_ = Order_Tag_447_G3_ + sprintf(Order_Tag_447_G3_, "447=D%c", BMFEP_Delimiter_SOH_);

    if (true == is_exchange_stp_enabled_) {
      Order_Tag_448_G4_ = Order_Tag_452_G3_ + sprintf(Order_Tag_452_G3_, "452=7%c", BMFEP_Delimiter_SOH_);
      Order_Tag_447_G4_ = Order_Tag_448_G4_ + sprintf(Order_Tag_448_G4_, "448=13248755000151%c", BMFEP_Delimiter_SOH_);
      Order_Tag_452_G4_ = Order_Tag_447_G4_ + sprintf(Order_Tag_447_G4_, "447=D%c", BMFEP_Delimiter_SOH_);

      Order_Tag_59_ = Order_Tag_452_G4_ + sprintf(Order_Tag_452_G4_, "452=5%c", BMFEP_Delimiter_SOH_);
    } else {
      Order_Tag_59_ = Order_Tag_452_G3_ + sprintf(Order_Tag_452_G3_, "452=7%c", BMFEP_Delimiter_SOH_);
    }

    Trailer_Tag_10_ = Order_Tag_59_ + sprintf(Order_Tag_59_, "59=0%c", BMFEP_Delimiter_SOH_);

    sprintf(Trailer_Tag_10_, "10=%*d%c", BMFEP_FIX_Tag_10_Width_, zero_, BMFEP_Delimiter_SOH_);
    sprintf(Header_Tag_9_, "9=%06d", (int)(Trailer_Tag_10_ - Header_Tag_35_));  // Still a dependencyon 6 digit
    msg_char_buf_[strlen(msg_char_buf_)] = BMFEP_Delimiter_SOH_;

    // Zero out the buffer, but not the traditional 0.
    for (unsigned int i = 0; i < BMFEP_Header_msg_size_ + BMFEP_NewOrder_msg_size_ + BMFEP_Trailer_msg_size_; ++i) {
      if (msg_char_buf_[i] == ' ') msg_char_buf_[i] = '0';
    }

    const_sum_ = calcConstSum();
    write_len_ = (unsigned int)(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH + BMFEP_FIX_Tag_10_Width_ +
                                DELIMITER_SOH_WIDTH_ - Header_Tag_8_);
  }
  ~NewOrder() {}

  inline unsigned int getWriteLen() const { return write_len_; }

  // TODO_OPT unsigned char instead of int
  inline unsigned int checksum() { return (const_sum_ + calcVarSum()) % 256; }

  inline void setSeqNum(t_FIX_SeqNum seq_num_) {
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, seq_num_,
                  BMFEP_FIX_Tag_34_Width_ /* 9 is the total size of the field, left padded with 0s*/,
                  make_optimize_assumptions_);
  }

  inline uint32_t SetDynamicSendOrderFieldsUsingOrderStruct(HFSAT::ORS::Order const *rp_order_, time_t last_send_time_,
                                                            BMFEPFIX::t_FIX_SeqNum const last_seq_num_) {
    // setClOrdID
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Order_Tag_11_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->server_assigned_order_sequence_,
                  OUR_BMFEP_FIX_Tag_11_Width_, false);

    // setOrderQty
    // Order Qty should always be printed with false.
    printToString(Order_Tag_38_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->size_remaining_, BMFEP_FIX_Tag_38_Width_,
                  false);

    // setPrice
    printToString(Order_Tag_44_ + TWO_DIGIT_AND_EQUAL_WIDTH, rp_order_->price_, BMFEP_FIX_Tag_44_Width_, 7);

    // setSide
    *(Order_Tag_54_ + TWO_DIGIT_AND_EQUAL_WIDTH) = (((rp_order_->buysell_ == kTradeTypeBuy) ? 1 : 2) + '0');

    // SetTimeInForce
    *(Order_Tag_59_ + TWO_DIGIT_AND_EQUAL_WIDTH) = ((true == rp_order_->is_ioc) ? '3' : '0');

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
    memcpy(Order_Tag_60_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH,
           Header_Tag_52_ + TWO_DIGIT_AND_EQUAL_WIDTH + YYYYMMDD_AND_SLASH_WIDTH, 8);

    // setSeqNum
    // Only if optimizations are on, we stop printing after no. drops to 0
    printToString(Header_Tag_34_ + TWO_DIGIT_AND_EQUAL_WIDTH, last_seq_num_,
                  BMFEP_FIX_Tag_34_Width_ /* 9 is the total size of the field, left padded with 0s*/,
                  make_optimize_assumptions_);

    // setCheckSum
    // When using printToString to print checksum, NEVER EVER stop on zero
    printToString(Trailer_Tag_10_ + TWO_DIGIT_AND_EQUAL_WIDTH, checksum(), BMFEP_FIX_Tag_10_Width_, false);
    // This should ALWAYS be false.

    return write_len_;
  }

  inline unsigned int calcVarSum() {
    unsigned int sum_ = 0;

    for (register unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_34_Width_; ++t_count_) {
      sum_ += Header_Tag_34_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // ClordID
    for (register unsigned int t_count_ = 0; t_count_ < OUR_BMFEP_FIX_Tag_11_Width_; ++t_count_) {
      sum_ += (Order_Tag_11_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH]);  // 1 for 11 and another for 9717
    }

    // qty
    for (register unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_38_Width_; ++t_count_) {
      sum_ += Order_Tag_38_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // price
    for (register unsigned int t_count_ = 0; t_count_ < BMFEP_FIX_Tag_44_Width_; ++t_count_) {
      sum_ += Order_Tag_44_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // side
    for (register unsigned int t_count_ = 0; t_count_ < 1; ++t_count_) {
      sum_ += Order_Tag_54_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
    }

    // TimeInForce
    for (register unsigned int t_count_ = 0; t_count_ < 1; ++t_count_) {
      sum_ += Order_Tag_59_[t_count_ + TWO_DIGIT_AND_EQUAL_WIDTH];
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
  inline unsigned int calcConstSum() {
    unsigned int sum_ = 0;

    for (char *p_msg_ = msg_char_buf_; p_msg_ != Trailer_Tag_10_; ++p_msg_) {
      // The Delimiter must be included in checksum computation.
      sum_ += *p_msg_;
    }

    return (sum_ - calcVarSum());
  }

 public:
  char *Header_Tag_8_;
  char *Header_Tag_9_;
  char *Header_Tag_35_;
  char *Header_Tag_34_;
  char *Header_Tag_43_;
  char *Header_Tag_49_;
  char *Header_Tag_52_;
  char *Header_Tag_56_;

  char *Order_Tag_1_;
  char *Order_Tag_11_;
  // char *Order_Tag_22_;
  char *Order_Tag_38_;
  char *Order_Tag_40_;
  char *Order_Tag_44_;
  // char *Order_Tag_48_;
  char *Order_Tag_54_;
  char *Order_Tag_55_;
  char *Order_Tag_59_;
  char *Order_Tag_60_;

  // Repeating Group of {PartyID, PartySourceID, PartyRole}

  char *Order_Tag_447_G1_;
  char *Order_Tag_448_G1_;
  char *Order_Tag_452_G1_;

  char *Order_Tag_447_G2_;
  char *Order_Tag_448_G2_;
  char *Order_Tag_452_G2_;

  char *Order_Tag_447_G3_;
  char *Order_Tag_448_G3_;
  char *Order_Tag_452_G3_;

  char *Order_Tag_447_G4_;
  char *Order_Tag_448_G4_;
  char *Order_Tag_452_G4_;

  char *Order_Tag_453_;
  //	char *Order_Tag_661_;
  char *Trailer_Tag_10_;

  char msg_char_buf_[BMFEP_Header_msg_size_ + BMFEP_NewOrder_msg_size_ + BMFEP_Trailer_msg_size_];

  int const_sum_;
  const bool make_optimize_assumptions_;
  unsigned int write_len_;
};
}
}
}

#endif  // BASE_BMFEPB_BMFEPFIX_BMFEP_NEW_ORDER_HPP
