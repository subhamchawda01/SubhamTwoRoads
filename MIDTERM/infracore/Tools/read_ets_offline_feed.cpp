// =====================================================================================
//
//       Filename:  read_ets_offline_feed.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/28/2012 05:24:30 AM
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

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/reverse_encode_construct_eti_offline.hpp"

void printHexString(char* c, int len) {
  for (int i = 0; i < len; ++i) {
    uint8_t ch = c[i];
    printf("%02x ", ch);
  }
  printf("\n");
}

int main() {
  std::string this_ets_file_ = "/home/ravi/ETS_FEED.dat";

  ETIStreamConstructor eti_stream_construct_(this_ets_file_);

  char eti_buffer_[1024];
  int read_length_ = 0;

  for (int i = 0; i < 4; i++) {
    memset(eti_buffer_, 0, 1024);

    read_length_ = eti_stream_construct_.GetETIBinaryStream(eti_buffer_);

    std::cout << "Read Length : " << read_length_ << "\n";

    printHexString(eti_buffer_, read_length_);
  }

  //
  //  HFSAT::BulkFileReader bulk_file_reader_ ;
  //
  //  bulk_file_reader_.open ( this_ets_file_.c_str () ) ;
  //
  //  if ( ! bulk_file_reader_.is_open () ) {
  //
  //    std::cerr << " Could not open file : " << this_ets_file_ << "\n" ;
  //    exit ( 1 ) ;
  //
  //  }
  //
  //  char buffer [ 1024 ] ;
  //  memset ( buffer, 0, 1024 ) ;
  //
  //  char eti_stream_ [ 1024 ] ;
  //  memset ( eti_stream_, 0, 1024 ) ;
  //
  //  int index_ = 0 ;
  //
  //  int count = bulk_file_reader_.read( buffer, 1024 ) ;
  //
  //  std::string text_ = "" ;
  //
  //  int value_ = 0 ;
  //
  //  for ( int i=0; i<count; i++ ) {
  //
  //      if ( buffer [ i ] >= 'A' && buffer [ i ] <= 'F' ) {
  //
  //        buffer [ i ] -= ( int ) 55 ;
  //
  //      }else {
  //
  //        buffer [ i ] -= ( int ) '0' ;
  //
  //      }
  //
  //    if ( text_.length() == 0 ) {
  //
  //      value_ += ( ( int ) buffer [ i ] * 16 ) ;
  //      text_ += buffer [ i ] ;
  //
  //    }else if ( text_.length () == 1 ) {
  //
  //      value_ += ( int ) buffer [ i ] ;
  //      text_ += buffer [ i ] ;
  //
  //      eti_stream_ [ index_ ] = ( char ) ( value_ ) ;
  //      index_ ++ ;
  //
  //      std::cout << " Value : " << value_ << "\n" ;
  //
  //      text_ = "" ;
  //      value_ = 0 ;
  //
  //    }
  //
  //  }
  //
  //  for ( int i = 0 ; i<index_ ; i++ ) {
  //
  //    std::cout << eti_stream_ [ i ] ;
  //
  //  }
  //  std::cout << "\n" ;

  return 0;
}
