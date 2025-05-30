/**
 \file EobiD/eobi_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/af_msg_parser.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include <time.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: <exec> <input-file>" << std::endl;
    exit(-1);
  }

  AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance(NULL);

  HFSAT::BulkFileReader* reader = new HFSAT::BulkFileReader();
  reader->open(argv[1]);

  if (!reader->is_open()) {
    std::cerr << " Can't open file, exiting \n";
    exit(-1);
  }

  while (true) {
    uint32_t msg_len;
    timeval time_;
    int read_size;

    read_size = reader->read(&(msg_len), sizeof(uint32_t));
    if (read_size < (int)sizeof(uint32_t)) break;

    read_size = reader->read(&(time_), sizeof(time_));
    if (read_size < (int)sizeof(time_)) break;

    char buffer[65536];

    read_size = reader->read(buffer, msg_len);
    if (read_size < (int)msg_len) {
      std::cerr << " read_size of Message is less than the msg_length";
      break;
    }

    std::string parsed_ = af_msgparser_.msgParse(buffer);
    std::cout << "Msg: " << msg_len << "|" << time_.tv_sec << " " << parsed_ << std::endl;
  }

  if (reader) {
    reader->close();
  }

  return 0;
}
