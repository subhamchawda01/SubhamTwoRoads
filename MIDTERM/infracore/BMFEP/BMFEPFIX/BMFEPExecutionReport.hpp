// @ Execution report parsing, taken from CME Engine
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"

#ifndef BMFEP_EXECUTION_REPORT_HPP
#define BMFEP_EXECUTION_REPORT_HPP

// Fast matching - Piyush.
// The masking is not needed, done to make the idea clear.
// This calculation has been done here to show how the values are obtained.
// const int Fast_Value_Tag_35_ = (*((uint32_t *)"35=") & 0x00FFFFFF);
// const int Fast_Value_Tag_11_ = (*((uint32_t *)"11=") & 0x00FFFFFF);
// const int Fast_Value_Tag_39_ = (*((uint32_t *)"39=") & 0x00FFFFFF);
// const int Fast_Value_Tag_37_ = (*((uint32_t *)"37=") & 0x00FFFFFF);
// const int Fast_Value_Tag_44_ = (*((uint32_t *)"44=") & 0x00FFFFFF);
// const int Fast_Value_Tag_54_ = (*((uint32_t *)"54=") & 0x00FFFFFF);
// const int Fast_Value_Tag_31_ = (*((uint32_t *)"31=") & 0x00FFFFFF);
// const int Fast_Value_Tag_32_ = (*((uint32_t *)"32=") & 0x00FFFFFF);
// const int Fast_Value_Tag_60_ = (*((uint32_t *)"60=") & 0x00FFFFFF);

// const int Fast_Value_Tag_151_ = (*((uint32_t *)"151=") & 0xFFFFFFFF);
// const int Fast_Value_Tag_55_ = (*((uint32_t *)"55=") & 0xFFFFFFFF);
// const int Fast_Value_Tag_102_ = (*((uint32_t *)"102=") & 0xFFFFFFFF);
// const int Fast_Value_Tag_434_ = (*((uint32_t *)"434=") & 0xFFFFFFFF);

// This has endianness issues.
// As of this writing all servers (fr2, chi, ny4, cfn) were little endian.
// Please be careful when using this.
#define Fast_Value_Tag_35_ 0x003D3533
#define Fast_Value_Tag_11_ 0x003D3131
#define Fast_Value_Tag_39_ 0x003D3933
#define Fast_Value_Tag_37_ 0x003D3733
#define Fast_Value_Tag_44_ 0x003D3434
#define Fast_Value_Tag_54_ 0x003D3435
#define Fast_Value_Tag_31_ 0x003D3133
#define Fast_Value_Tag_32_ 0x003D3233
#define Fast_Value_Tag_60_ 0x003D3036

#define Fast_Value_Tag_150_BMF 0X3D303531
#define Fast_Value_Tag_151_ 0x3D313531
#define Fast_Value_Tag_55_ 0x003D3535
#define Fast_Value_Tag_102_ 0x3D323031
#define Fast_Value_Tag_434_ 0x3D343334
#define Fast_Value_Tag_198_ 0x3D383931
namespace HFSAT {
namespace ORS {
namespace BMFEPFIX {

class ExecutionReport {
 public:
  ExecutionReport(const bool _make_optimize_assumptions_)
      : ExecTag_11_(0),
        ExecTag_39_(0),
        ExecTag_44_(0),
        ExecTag_151_(0),
        ExecTag_150_(0),
        ExecTag_31_(0),
        ExecTag_32_(0),
        ExecTag_54_(0),
        ExecTag_102_(0),
        ExecTag_434_(0),
        Tag_Value_37_extracted_length_(0),
        exch_seq_(0),

        make_optimize_assumptions_(_make_optimize_assumptions_) {
    memset(ExecTag_37_, 0, sizeof(ExecTag_37_));
    memset(ExecTag_55_, 0, sizeof(ExecTag_55_));
  }

  ~ExecutionReport() {}

  // Conditionally extract the fields which will be asked for for a given msg type.
  void generateReport(char *_msg_buf_) {
    char *MsgTag_35_ = NULL;
    const double decimal_factors_[] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001, 0.00000001};

    if (make_optimize_assumptions_) {
      _msg_buf_ += 15;  // See below.q
    }

    exch_seq_ = 0;

    for (; *_msg_buf_;) {
      // Check tag no.
      if ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF) == Fast_Value_Tag_35_) {
        MsgTag_35_ = _msg_buf_ + 3;
        _msg_buf_ += 5;
        break;
      }

      // Move to the next field.
      for (; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_)
        ;
      ++_msg_buf_;
    }

    if (*MsgTag_35_ == '8') {
      // Execution report.

      // This is a relatively DANGEROUS assumption that the distance in the message between
      // the tag "35=8" field to the first interesting field "11=" is atleast 50 places.
      // Here we jump ahead those many places.
      // the tags in the execution report it sends out.
      //
      // 8=FIX.4.4|9=478|35=8|34=282|49=BRK06|52=2015521-01:12:21.598|56=CODVC01|
      // 1=1|6=0|11=00000000000000000002|14=0|17=9_9|22=8|37=9|38=1|39=8|40=2|44=1300|48=BMFEPBR4800511|54=1|
      // 55=WINQ11|58=[0042001][Not authorized to create Single Order. Order ID: [9] | Security ID: [BMFEPBR480051
      // 1] |Trading Date: [20/07/2011 00:00:00].]|59=0|60=2015521-01:12:21.614|75=2015520|103=11|150=8|151=1|
      // 198=000002|207=XBMFEP|453=3|448=CVT|447=D|452=54|448=CODVC01|447=D|452=36|448=999|447=D|452=7|10=159|
      if (make_optimize_assumptions_) {
        _msg_buf_ += 50;  // For PUMA anymore than 50 will not work.
      }

      // Skip to beginning of next field.
      for (; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_)
        ;

      ++_msg_buf_;
      bool has_ExecTag_11_processed_already_ = false;

      for (; *_msg_buf_;) {
        // Check tag no.
        switch ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF)) {
          case Fast_Value_Tag_11_: {
            for (ExecTag_11_ = 0, _msg_buf_ += 3; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
              ExecTag_11_ = ExecTag_11_ * 10 + *_msg_buf_ - '0';
            }

            ++_msg_buf_;
          }
            has_ExecTag_11_processed_already_ = true;
            break;

          case Fast_Value_Tag_39_: {
            ExecTag_39_ = _msg_buf_[3];

            _msg_buf_ += 4;
            ++_msg_buf_;

            if ((ExecTag_39_ == '8' || ExecTag_39_ == 'C') && has_ExecTag_11_processed_already_) {
              // increment msg_buf_ to the end and we dont want to process anything else
              // only need Tag 11, saos for reject and  cancel confirmation report

              // Premature termination  of the whole for loop
              //*_msg_buf_ = '\0';
              return;
            }
          } break;

          // Tag 37 should be extracted  carefully, the length of it can change from the
          // report to report and we must send the same value to the exchange for cancelling.
          // Currently however in BMFEP though we dont send the Tag 37 value to the exchange
          // in any kind of message

          case Fast_Value_Tag_37_: {
            int index_;
            for (index_ = 0, _msg_buf_ += 3; _msg_buf_[index_] != BMFEP_Delimiter_SOH_; ++index_) {
              ExecTag_37_[index_] = _msg_buf_[index_];
            }
            ExecTag_37_[index_] = '\0';
            Tag_Value_37_extracted_length_ = strlen(ExecTag_37_);
            _msg_buf_ += index_;
            ++_msg_buf_;
          } break;

          // TO DO..Also Parse Field  41

          case Fast_Value_Tag_44_: {
            for (ExecTag_44_ = 0, _msg_buf_ += 3; *_msg_buf_ != '.' && *_msg_buf_ != BMFEP_Delimiter_SOH_;
                 ++_msg_buf_) {
              ExecTag_44_ = ExecTag_44_ * 10 + *_msg_buf_ - '0';
            }

            int factor_ = 0;
            if (*_msg_buf_ == '.') {
              for (++_msg_buf_; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_, ++factor_) {
                ExecTag_44_ = ExecTag_44_ * 10 + *_msg_buf_ - '0';
              }
            }

            ExecTag_44_ *= decimal_factors_[factor_];

            ++_msg_buf_;
          } break;

          case Fast_Value_Tag_54_: {
            ExecTag_54_ = _msg_buf_[3] - '0';

            _msg_buf_ += 4;
            ++_msg_buf_;
          } break;

          case Fast_Value_Tag_31_: {
            for (ExecTag_31_ = 0, _msg_buf_ += 3; *_msg_buf_ != '.' && *_msg_buf_ != BMFEP_Delimiter_SOH_;
                 ++_msg_buf_) {
              ExecTag_31_ = ExecTag_31_ * 10 + *_msg_buf_ - '0';
            }

            int factor_ = 0;
            if (*_msg_buf_ == '.') {
              for (++_msg_buf_; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_, ++factor_) {
                ExecTag_31_ = ExecTag_31_ * 10 + *_msg_buf_ - '0';
              }
            }

            ExecTag_31_ *= decimal_factors_[factor_];

            ++_msg_buf_;
          } break;

          case Fast_Value_Tag_32_: {
            for (ExecTag_32_ = 0, _msg_buf_ += 3; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
              ExecTag_32_ = ExecTag_32_ * 10 + *_msg_buf_ - '0';
            }

            ++_msg_buf_;
          } break;

          case Fast_Value_Tag_60_: {
            _msg_buf_ += 23;  // See above.
          } break;

          case Fast_Value_Tag_55_: {
            int index_;
            for (index_ = 0, _msg_buf_ += 3; _msg_buf_[index_] != BMFEP_Delimiter_SOH_; ++index_) {
              ExecTag_55_[index_] = _msg_buf_[index_];
            }
            ExecTag_55_[index_] = '\0';

            _msg_buf_ += index_;
            ++_msg_buf_;
          } break;

          default: {
            switch ((*((uint32_t *)_msg_buf_) & 0xFFFFFFFF)) {
              case Fast_Value_Tag_150_BMF:  // ExecType for BMFEP
              {
                ExecTag_150_ = _msg_buf_[4];

                _msg_buf_ += 5;  // POints to SOH
                ++_msg_buf_;     // Should point to next tag after 150

              } break;

              case Fast_Value_Tag_151_: {
                for (ExecTag_151_ = 0, _msg_buf_ += 4; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
                  ExecTag_151_ = ExecTag_151_ * 10 + *_msg_buf_ - '0';
                }
                ++_msg_buf_;
              } break;

              case Fast_Value_Tag_198_: {
                _msg_buf_ += 4;
                for (; *_msg_buf_ && *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
                  exch_seq_ = exch_seq_ * 10 + *_msg_buf_ - '0';
                }

                ++_msg_buf_;
              } break;

              default: {
                // Skip to next field.
                for (; *_msg_buf_ && *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_)
                  ;

                ++_msg_buf_;
              } break;
            }
          } break;
        }
      }
    }

    if (*MsgTag_35_ == '9') {
      // Cancel reject report.
      //	    8=FIX.4.4^A9=301^A35=9^A34=55^A49=BRK06^A52=2015521-14:17:24.548^A56=CODVC01^A10016=-1_UNKNOWN_1_1^A
      // 11=00000000000000000002^A22=8^A37=00000000000000000^A39=8^A41=00000000000000000000^A48=Unknown^A55=WINQ11^A
      // 58=[0000015][Unknown Symbol.]^A102=99^A207=XBMFEP^A434=1^A453=3^A448=CVT^A447=D^A452=54^A448=CODVC01^A447=D^A
      // 452=36^A448=999^A447=D^A452=7^A10=084^A

      if (make_optimize_assumptions_) {
        _msg_buf_ += 50;
      }

      // Skip to beginning of next field.
      for (; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_)
        ;

      for (; *_msg_buf_;) {
        // Check tag no.
        switch ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF)) {
          case Fast_Value_Tag_11_: {
            for (ExecTag_11_ = 0, _msg_buf_ += 3; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
              ExecTag_11_ = ExecTag_11_ * 10 + *_msg_buf_ - '0';
            }

            ++_msg_buf_;
          } break;

          case Fast_Value_Tag_37_: {
            int index_;
            for (index_ = 0, _msg_buf_ += 3; _msg_buf_[index_] != BMFEP_Delimiter_SOH_; ++index_) {
              ExecTag_37_[index_] = _msg_buf_[index_];
            }
            ExecTag_37_[index_] = '\0';
            Tag_Value_37_extracted_length_ = strlen(ExecTag_37_);
            _msg_buf_ += index_;
            ++_msg_buf_;

          } break;

          case Fast_Value_Tag_60_: {
            _msg_buf_ += 23;  // See above.
          } break;

          default: {
            switch ((*((uint32_t *)_msg_buf_) & 0xFFFFFFFF)) {
              case Fast_Value_Tag_102_: {
                for (ExecTag_102_ = 0, _msg_buf_ += 4; *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_) {
                  ExecTag_102_ = ExecTag_102_ * 10 + *_msg_buf_ - '0';
                }
                ++_msg_buf_;
              } break;

              case Fast_Value_Tag_434_: {
                ExecTag_434_ = _msg_buf_[4];

                _msg_buf_ += 5;
                ++_msg_buf_;
              } break;

              default: {
                // Skip to next field.
                for (; *_msg_buf_ && *_msg_buf_ != BMFEP_Delimiter_SOH_; ++_msg_buf_)
                  ;

                ++_msg_buf_;
              } break;
            }
          } break;
        }
      }
    }
  }

  const char *getExecTag37() { return ExecTag_37_; }

  uint64_t GetExchOrderId() { return exch_seq_; }

  const int getExecTag37Len() { return Tag_Value_37_extracted_length_; }

  t_FIX_OrderID getExecTag11() { return ExecTag_11_; }

  t_FIX_OrdStatus getExecTag39() { return ExecTag_39_; }

  t_FIX_Price getExecTag44() { return ExecTag_44_; }

  t_FIX_OrderQty getExecTag151() { return ExecTag_151_; }

  const char *getExecTag55() { return ExecTag_55_; }

  t_FIX_Price getExecTag31() { return ExecTag_31_; }

  t_FIX_OrderQty getExecTag32() { return ExecTag_32_; }

  t_FIX_Side getExecTag54() { return ExecTag_54_; }

  int getExecTag102() { return ExecTag_102_; }

  char getExecTag434() { return ExecTag_434_; }
  char getExecTag150() { return ExecTag_150_; }

 private:
  // The fields which will be asked for by BMFEPEngine.
  char ExecTag_37_[BMFEP_FIX_Tag_37_Width_];  // Max size of the the Tag 37 length
  t_FIX_SeqNum ExecTag_11_;
  t_FIX_OrdStatus ExecTag_39_;
  t_FIX_Price ExecTag_44_;
  t_FIX_OrderQty ExecTag_151_;
  t_FIX_ExecType ExecTag_150_;
  char ExecTag_55_[50];
  t_FIX_Price ExecTag_31_;
  t_FIX_OrderQty ExecTag_32_;
  t_FIX_Side ExecTag_54_;
  int ExecTag_102_;
  char ExecTag_434_;
  int Tag_Value_37_extracted_length_;
  uint64_t exch_seq_;

  const bool make_optimize_assumptions_;

  void printFields() {
    std::cout << ExecTag_37_ << "[]"
              << "[]" << ExecTag_11_ << "[]" << ExecTag_39_ << "[]" << ExecTag_44_ << "[]" << ExecTag_151_ << "[]"
              << ExecTag_55_ << "[]" << ExecTag_31_ << "[]" << ExecTag_32_ << "[]" << ExecTag_54_ << "[]"
              << ExecTag_102_ << "[]" << ExecTag_434_ << std::endl;
  }
};
}
}
}
#undef Fast_Value_Tag_35_
#undef Fast_Value_Tag_11_
#undef Fast_Value_Tag_39_
#undef Fast_Value_Tag_37_
#undef Fast_Value_Tag_44_
#undef Fast_Value_Tag_54_
#undef Fast_Value_Tag_31_
#undef Fast_Value_Tag_32_
#undef Fast_Value_Tag_60_

#undef Fast_Value_Tag_150_BMF
#undef Fast_Value_Tag_151_
#undef Fast_Value_Tag_55_
#undef Fast_Value_Tag_102_
#undef Fast_Value_Tag_434_
#undef Fast_Value_Tag_198_

#endif
