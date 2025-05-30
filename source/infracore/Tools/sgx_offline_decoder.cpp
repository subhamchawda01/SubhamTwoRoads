/**
 \file Tools/sgx_offline_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 354, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

//#include "infracore/DataDecoders/SGX/sgx_md_processor.hpp"
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "infracore/DataDecoders/SGX/sgx_decoder.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <input-file> <output-file>" << std::endl;
    exit(-1);
  }

  HFSAT::SGX::SGXDecoder decoder = HFSAT::SGX::SGXDecoder::GetUniqueInstance();
  SGX_MDS::SGXOrder sgx_data;
  HFSAT::BulkFileWriter bulk_file_writer_;
  bulk_file_writer_.Open(argv[2]);

  std::ifstream infile(argv[1], std::ifstream::in);
  std::vector<std::string> data;

  if (infile.is_open()) {
    std::string line;
    while (infile.good()) {
      getline(infile, line);
      if (line.empty()) {
        break;
      }

      std::istringstream ss(line);
      data.clear();

      std::string field;
      while (getline(ss, field, ',')) {
        data.push_back(field);
      }

      decoder.Decode(data);
      sgx_data = decoder.GetSGXData();
      bulk_file_writer_.Write(&sgx_data, sizeof(SGX_MDS::SGXOrder));
      bulk_file_writer_.CheckToFlushBuffer();
    }
  }

  return 0;
}
