#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "dvccode/Utils/bulk_file_reader.hpp"

#define CME_Delimiter_SOH_ (char)0x01

#define Fast_Value_Tag_35_ 0x003D3533
#define Fast_Value_Tag_11_ 0x003D3131
#define Fast_Value_Tag_39_ 0x003D3933
#define Fast_Value_Tag_37_ 0x003D3733
#define Fast_Value_Tag_44_ 0x003D3434
#define Fast_Value_Tag_54_ 0x003D3435
#define Fast_Value_Tag_31_ 0x003D3133
#define Fast_Value_Tag_32_ 0x003D3233
#define Fast_Value_Tag_60_ 0x003D3036

#define Fast_Value_Tag_150_ 0X3D303531
#define Fast_Value_Tag_151_ 0x3D313531
#define Fast_Value_Tag_107_ 0x3D373031
#define Fast_Value_Tag_102_ 0x3D323031
#define Fast_Value_Tag_434_ 0x3D343334

typedef char *t_FIX_SecurityDesc;

typedef unsigned long long t_FIX_OrderID;

typedef unsigned long t_FIX_SeqNum;

typedef int t_FIX_Side;
typedef int t_FIX_OrderQty;

typedef double t_FIX_Price;

typedef char t_FIX_OrdStatus;

typedef char t_FIX_ExecType;
#define CME_FIX_Tag_37_Width_ 17  // Exchange assigned OrderID (17 for CME)

char ExecTag_37_[CME_FIX_Tag_37_Width_];  // Max size of the the Tag 37 length
int ExecTag_37_length_;
t_FIX_SeqNum ExecTag_11_;
t_FIX_OrdStatus ExecTag_39_;
t_FIX_Price ExecTag_44_;
t_FIX_OrderQty ExecTag_151_;
t_FIX_ExecType ExecTag_150_;
char ExecTag_107_[50];
t_FIX_Price ExecTag_31_;
t_FIX_OrderQty ExecTag_32_;
t_FIX_Side ExecTag_54_;
int ExecTag_102_;
char ExecTag_434_;
int Tag_Value_37_extracted_length_;

char *generateReport(char *_msg_buf_) {
  bool make_optimize_assumptions_ = true;
  char *MsgTag_35_ = NULL;
  const double decimal_factors_[] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001, 0.00000001};

  if (make_optimize_assumptions_) {
    _msg_buf_ += 15;  // See below.
  }

  for (; *_msg_buf_;) {
    // Check tag no.
    if ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF) == Fast_Value_Tag_35_) {
      MsgTag_35_ = _msg_buf_ + 3;
      _msg_buf_ += 5;
      break;
    }

    // Move to the next field.
    for (; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_)
      ;
    ++_msg_buf_;
  }

  if (*MsgTag_35_ == '8') {
    // Execution report.

    // This is a relatively DANGEROUS assumption that the distance in the message between
    // the tag "35=8" field to the first interesting field "11=" is atleast 70 places.
    // Here we jump ahead those many places.

    // ASSUMPTION no longer holds true if CME changes the "ORDER" in which it places
    // the tags in the execution report it sends out.

    //	    8=FIX.4.2|9=307| ** Skip over this guy initially atleast = 15 characters **
    //      35=8|
    //      34=4751|369=3967|52=20110707-14:11:32.906|49=CME|50=G|56=8Q3998N|57=DVC|143=US,NY|1=57E57416|6=0| ** Trying
    //      to skip over this chunk of fields atleast = 70 characters **
    //      11=00000000000000000002|
    //      14=0|17=64219:1157826|20=0|37=6437514722|38=5|39=0|40=2|41=0|44=131000|48=49278|54=1|55=ES|59=0|
    //      60=20110707-14:11:32.905| ** Can skip 21 characters when encountering tag "60=" **
    //      107=ESU1|150=0|151=5|167=FUT|432=20110707|1028=N|9717=00002|10=188|
    if (make_optimize_assumptions_) {
      _msg_buf_ += 70;
    }

    // Skip to beginning of next field.
    for (; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_)
      ;

    ++_msg_buf_;

    for (; *_msg_buf_;) {
      // Check tag no.
      switch ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF)) {
        case Fast_Value_Tag_11_: {
          for (ExecTag_11_ = 0, _msg_buf_ += 3; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
            ExecTag_11_ = ExecTag_11_ * 10 + *_msg_buf_ - '0';
          }

          ++_msg_buf_;
          std::cout << "TAG 11: " << ExecTag_11_ << std::endl;
        } break;

        case Fast_Value_Tag_39_: {
          ExecTag_39_ = _msg_buf_[3];

          _msg_buf_ += 4;
          ++_msg_buf_;
          std::cout << "TAG 39: " << ExecTag_39_ << std::endl;

        } break;

        case Fast_Value_Tag_37_: {
          int index_;
          for (index_ = 0, _msg_buf_ += 3; _msg_buf_[index_] != CME_Delimiter_SOH_; ++index_) {
            ExecTag_37_[index_] = _msg_buf_[index_];
          }
          ExecTag_37_[index_] = '\0';
          ExecTag_37_length_ = index_;

          _msg_buf_ += index_;
          ++_msg_buf_;
          std::cout << "TAG 37: " << ExecTag_37_ << std::endl;

        } break;

        case Fast_Value_Tag_44_: {
          for (ExecTag_44_ = 0, _msg_buf_ += 3; *_msg_buf_ != '.' && *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
            ExecTag_44_ = ExecTag_44_ * 10 + *_msg_buf_ - '0';
          }

          int factor_ = 0;
          if (*_msg_buf_ == '.') {
            for (++_msg_buf_; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_, ++factor_) {
              ExecTag_44_ = ExecTag_44_ * 10 + *_msg_buf_ - '0';
            }
          }

          ExecTag_44_ *= decimal_factors_[factor_];

          ++_msg_buf_;
          std::cout << "TAG 44: " << ExecTag_44_ << std::endl;

        } break;

        case Fast_Value_Tag_54_: {
          ExecTag_54_ = _msg_buf_[3] - '0';

          _msg_buf_ += 4;
          ++_msg_buf_;
          std::cout << "TAG 54: " << ExecTag_54_ << std::endl;

        } break;

        case Fast_Value_Tag_31_: {
          for (ExecTag_31_ = 0, _msg_buf_ += 3; *_msg_buf_ != '.' && *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
            ExecTag_31_ = ExecTag_31_ * 10 + *_msg_buf_ - '0';
          }

          int factor_ = 0;
          if (*_msg_buf_ == '.') {
            for (++_msg_buf_; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_, ++factor_) {
              ExecTag_31_ = ExecTag_31_ * 10 + *_msg_buf_ - '0';
            }
          }

          ExecTag_31_ *= decimal_factors_[factor_];

          ++_msg_buf_;
          std::cout << "TAG 31: " << ExecTag_31_ << std::endl;

        } break;

        case Fast_Value_Tag_32_: {
          for (ExecTag_32_ = 0, _msg_buf_ += 3; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
            ExecTag_32_ = ExecTag_32_ * 10 + *_msg_buf_ - '0';
          }

          ++_msg_buf_;
        } break;

        case Fast_Value_Tag_60_: {
          _msg_buf_ += 23;  // See above.
        } break;

        default: {
          switch ((*((uint32_t *)_msg_buf_) & 0xFFFFFFFF)) {
            case Fast_Value_Tag_151_: {
              for (ExecTag_151_ = 0, _msg_buf_ += 4; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
                ExecTag_151_ = ExecTag_151_ * 10 + *_msg_buf_ - '0';
              }
              ++_msg_buf_;
              std::cout << "TAG 151: " << ExecTag_151_ << std::endl;

            } break;

            case Fast_Value_Tag_107_: {
              int index_;
              for (index_ = 0, _msg_buf_ += 4; _msg_buf_[index_] != CME_Delimiter_SOH_; ++index_) {
                ExecTag_107_[index_] = _msg_buf_[index_];
              }
              ExecTag_107_[index_] = '\0';

              _msg_buf_ += index_;
              ++_msg_buf_;
              std::cout << "TAG 107: " << ExecTag_107_ << std::endl;

            } break;

            default: {
              // Skip to next field.
              for (; *_msg_buf_ && *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_)
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

    // 8=FIX.4.2|9=256| ** Skipped already **
    // 35=9|
    // 34=716|369=705|52=20110705-14:36:14.682|49=CME|50=G|56=8Q3998N|57=DVC|143=US,NY| ** Trying to skip over this
    // chunk of fields atleast = 50 characters **
    // 11=00000000000000000033|17=64219:491027|37=6437281807|39=U|41=00000000000000000000|58=This order is not in the
    // book '0'|60=20110705-14:36:14.614|102=2045|434=1|9717=00033|10=018
    if (make_optimize_assumptions_) {
      _msg_buf_ += 50;
    }

    // Skip to beginning of next field.
    for (; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_)
      ;

    for (; *_msg_buf_;) {
      // Check tag no.
      switch ((*((uint32_t *)_msg_buf_) & 0x00FFFFFF)) {
        case Fast_Value_Tag_11_: {
          for (ExecTag_11_ = 0, _msg_buf_ += 3; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
            ExecTag_11_ = ExecTag_11_ * 10 + *_msg_buf_ - '0';
          }

          ++_msg_buf_;
        } break;

        case Fast_Value_Tag_37_: {
          int index_;
          for (index_ = 0, _msg_buf_ += 3; _msg_buf_[index_] != CME_Delimiter_SOH_; ++index_) {
            ExecTag_37_[index_] = _msg_buf_[index_];
          }
          ExecTag_37_[index_] = '\0';

          _msg_buf_ += index_;
          ++_msg_buf_;

        } break;

        case Fast_Value_Tag_60_: {
          _msg_buf_ += 23;  // See above.
        } break;

        default: {
          switch ((*((uint32_t *)_msg_buf_) & 0xFFFFFFFF)) {
            case Fast_Value_Tag_102_: {
              for (ExecTag_102_ = 0, _msg_buf_ += 4; *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_) {
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
              for (; *_msg_buf_ && *_msg_buf_ != CME_Delimiter_SOH_; ++_msg_buf_)
                ;

              ++_msg_buf_;
            } break;
          }
        } break;
      }
    }
  }
  return ExecTag_37_;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << " Give the filename to READ man...." << std::endl;
    exit(0);
  }
  HFSAT::BulkFileReader bulk_file_reader_;

  std::string cme_fix_log(argv[1]);
  bulk_file_reader_.open(cme_fix_log);

  char buff[5120] = {'\0'};  // read 256 bytes

  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t len = bulk_file_reader_.read(buff, sizeof(buff));

      if (len >= sizeof(buff)) {
        std::cout << buff << std::endl;
        std::cout << "----" << len << std::endl;
      } else {
        std::cout << "FINISH----" << len << std::endl;
        std::cout << buff << std::endl;

        break;
      }
    }
  }
  bulk_file_reader_.close();

  // std::ifstream file (cme_fix_log.c_str() , std::ios::in|std::ios::binary|std::ios::ate);

  // int size;
  // char* mem;
  // char tt;
  // if (file.is_open())
  //   {
  //     while(!file.eof())
  // 	{
  // 	  // size = (int)file.tellg();
  // 	  // std::cout << size << std::endl;
  // 	  // mem = new char [size];
  // 	  // file.read (mem, size );
  // 	  // for ( int ii =0; ii <size;ii++)
  // 	  //   std::cout << mem[ii];
  // 	  // file.close();
  // 	  // delete[]mem;
  // 	  file.get(tt);
  // 	  std::cout <<tt;
  // 	}
  //     file.close();
  //   }
  return 0;
}
