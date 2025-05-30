// =====================================================================================
//
//       Filename:  reverse_encode_construct_eti_offline.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/28/2012 06:10:18 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#ifndef ETI_STREAM_CONSTRUCTOR_H
#define ETI_STREAM_CONSTRUCTOR_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "dvccode/Utils/bulk_file_reader.hpp"

#define MAX_STREAM_SIZE 72000

class ETIStreamConstructor {
 private:
  char main_buffer[MAX_STREAM_SIZE];
  int last_index_;
  int index_;
  int refer_;
  std::vector<int> read_lengths_;
  std::vector<int> index_end_;

 public:
  void printHexString(char* c, int len) {
    for (int i = 0; i < len; ++i) {
      uint8_t ch = c[i];
      printf("%02x ", ch);
    }
    printf("\n");
  }

  ETIStreamConstructor(std::string eti_stream_filename_)
      : last_index_(0), index_(0), refer_(0), read_lengths_(), index_end_() {
    memset(main_buffer, 0, MAX_STREAM_SIZE);

    HFSAT::BulkFileReader bulk_file_reader_;

    bulk_file_reader_.open(eti_stream_filename_.c_str());

    if (!bulk_file_reader_.is_open()) {
      std::cerr << " Could not open eti_stream_file_ " << eti_stream_filename_ << "\n";
      exit(1);
    }

    char buffer[8192];
    memset(buffer, 0, 8192);
    int value_ = 0;

    while (true) {
      std::string text_ = "";
      memset(buffer, 0, 1024);
      value_ = 0;

      int count = bulk_file_reader_.read(buffer, 100);

      if (count < 1) break;

      for (int i = 0; i < count; i++) {
        if (buffer[i] >= 'A' && buffer[i] <= 'F')
          buffer[i] -= (int)(55);
        else
          buffer[i] -= (int)('0');

        if (text_.length() == 0) {
          value_ += ((int)buffer[i] * 16);
          text_ += buffer[i];

        } else if (text_.length() == 1) {
          value_ += (int)(buffer[i]);
          text_ = "";

          main_buffer[index_] = (char)(value_);

          index_++;

          if (index_ > MAX_STREAM_SIZE) {
            std::cerr << " File Stream to large to be operated with give buffer \n";
            break;
          }

          value_ = 0;
        }
      }

      index_end_.push_back(index_);

      if (last_index_ == 0) {
        last_index_ = index_;
        read_lengths_.push_back(last_index_);

      } else {
        read_lengths_.push_back(index_ - last_index_);
        last_index_ = index_;
      }
    }

    std::cout << " Data : \n";

    printHexString(main_buffer, index_);
  }

  int GetETIBinaryStream(char* eti_buffer_) {
    if (refer_ == 0)
      memcpy(eti_buffer_, main_buffer, read_lengths_[refer_]);

    else {
      std::cout << " Refer Index : " << refer_ << " End : " << index_end_[refer_ - 1]
                << " Read Length : " << read_lengths_[refer_] << "\n";
      memcpy(eti_buffer_, main_buffer + (index_end_[refer_ - 1]), read_lengths_[refer_]);
    }

    refer_++;

    return read_lengths_[refer_ - 1];
  }
};

#endif
