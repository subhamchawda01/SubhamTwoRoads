/**
 \file Tools/ose_itch_offline_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include "infracore/DataDecoders/OSE/ose_decoder.hpp"
#include "infracore/DataDecoders/OSE/ose_md_processor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: <exec> <input-file> [CONVERT]" << std::endl;
    exit(-1);
  }

  if (argc > 2) {
    if (!strcmp(argv[2], "CONVERT")) {
      HFSAT::OSEMdProcessor::SetInstance(HFSAT::kPriceFeedLogger, true);
    }
  } else {
    HFSAT::OSEMdProcessor::SetInstance(HFSAT::kLogger, true);
  }

  HFSAT::OSED::OSEDecoder* ose_decoder = &(HFSAT::OSED::OSEDecoder::GetUniqueInstance());

  HFSAT::BulkFileReader* reader = new HFSAT::BulkFileReader();
  reader->open(argv[1]);

  if (!reader->is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(-1);
  }

  std::cout << "SizeOfInt: " << sizeof(int) << "\n";
  int bytes_read = 0;

  while (true) {
    int msg_len;
    int read_size;
    timeval time;

    read_size = reader->read(&(msg_len), sizeof(int));
    if (read_size < (int)sizeof(int)) {
      std::cerr << "MsgLen read_size: " << read_size << " Break\n";
      break;
    }

    bytes_read += read_size;

    //    msg_len = __builtin_bswap16(msg_len);
    std::cout << "Msg Len: " << msg_len << "\n";

    read_size = reader->read(&time, sizeof(timeval));
    if (read_size < (int)sizeof(timeval)) {
      std::cerr << "Timeval read_size: " << read_size << " Break\n";
      break;
    }

    bytes_read += read_size;

    std::cout << "Time: " << time.tv_sec << "." << time.tv_usec << "\n";

    char buffer[2048];
    read_size = reader->read(buffer, msg_len);
    if (read_size < msg_len) {
      std::cerr << "Buffer read_size: " << read_size << " msg_len: " << msg_len << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "BytesRead: " << bytes_read << "\n";
    std::cout.flush();

    ose_decoder->Decode(buffer, msg_len, 1);  // here, 1 is just a dummy file descriptor
  }

  if (reader) {
    reader->close();
  }

  // Stop the mds loggers to flush the streams to the mds files and close them
  HFSAT::OSEMdProcessor::GetInstance()->StopLoggers();

  return 0;
}
