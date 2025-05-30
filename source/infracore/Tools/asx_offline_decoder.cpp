/**
 \file EobiD/eobi_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include "infracore/DataDecoders/ASX/asx_decoder.hpp"
//#include "infracore/DataDecoders/ASX/asx_md_processor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

int main(int argc, char** argv) {
  /*if (argc < 2) {
    std::cerr << "Usage: <exec> <input-file> [CONVERT]" << std::endl;
    exit(-1);
  }

  if (argc > 2) {
    if (!strcmp(argv[2], "CONVERT")) {
      HFSAT::AsxMdProcessor::SetInstance(HFSAT::kPriceFeedLogger);
    }
  } else {
    HFSAT::AsxMdProcessor::SetInstance(HFSAT::kLogger);
  }

  HFSAT::ASXD::ASXDecoder* asx_decoder = &(HFSAT::ASXD::ASXDecoder::GetUniqueInstance());

  HFSAT::BulkFileReader* reader = new HFSAT::BulkFileReader();
  reader->open(argv[1]);

  if (!reader->is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(-1);
  }

  while (true) {
    uint16_t msg_len;
    int read_size;

    read_size = reader->read(&(msg_len), sizeof(uint16_t));
    if (read_size < (int)sizeof(uint16_t)) break;

    msg_len = __builtin_bswap16(msg_len);

    char buffer[1024];
    read_size = reader->read(buffer, msg_len);
    if (read_size < msg_len) break;

    asx_decoder->Decode(buffer, msg_len, 1);  // here, 1 is just a dummy file descriptor
  }

  if (reader) {
    reader->close();
  }

  // Stop the mds loggers to flush the streams to the mds files and close them
  HFSAT::AsxMdProcessor::GetInstance()->StopLoggers();
  */
  return 0;
}
