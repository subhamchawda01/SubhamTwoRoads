/**
    \file dvccode/Utils/binary_struct_reader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#pragma once

#include <string.h>
#include <string>
#include <iostream>
#include <fstream>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

namespace HFSAT {

template <class T>
class BinaryStructReader {
  HFSAT::BulkFileReader reader;

 public:
  BinaryStructReader(std::string filename) {
    reader.open(filename.c_str());
    if (!reader.is_open()) {
      std::cerr << "file could not be opened " << filename << "\n";
      exit(-1);
    }
  }

  virtual ~BinaryStructReader() { close(); }

  bool readNext(T& data) {
    int readLen = reader.read((void*)&data, sizeof(T));
    return (readLen == sizeof(T));
  }

  void close() {
    if (reader.is_open()) reader.close();
  }
};
}
