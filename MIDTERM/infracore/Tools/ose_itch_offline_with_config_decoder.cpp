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

#define MAX_UDP_BUFFER_SIZE 8192

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

  // Data in config mode is written in order:
  // 1. IP_ADDR [16]
  // 2. SOCKET_PORT [sizeof(int32_t)]
  // 3. READ_LENGTH [sizeof(int32_t)]
  // 4. CURRENT_TIME [sizeof(struct timeval)]
  // 5. ACTUAL_BUFFER [READ_LENGTH]

  while (true) {
    int read_size;

    // Reading IP
    char socket_ip_addr[16];
    memset((void*)socket_ip_addr, 0, 16);
    read_size = reader->read(socket_ip_addr, 16);
    if (read_size < 16) {
      std::cerr << "SOCK_IP_ADDR read_size: " << read_size << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "SOCK_IP_ADDR: " << socket_ip_addr << "\n";

    // Reading PORT
    int32_t socket_port;
    read_size = reader->read(&(socket_port), sizeof(int32_t));
    if (read_size < (int)sizeof(int32_t)) {
      std::cerr << "SOCK_PORT read_size: " << read_size << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "SOCK_PORT: " << socket_port << "\n";

    // Reading Buffer Length
    int32_t read_length;
    read_size = reader->read(&(read_length), sizeof(int32_t));
    if (read_size < (int)sizeof(int32_t)) {
      std::cerr << "READ_LENGTH read_size: " << read_size << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "READ_LENGTH: " << read_length << "\n";

    // Reading CURRENT_TIME
    timeval time;
    read_size = reader->read(&time, sizeof(timeval));
    if (read_size < (int)sizeof(timeval)) {
      std::cerr << "TIMEVAL read_size: " << read_size << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "TIMEVAL: " << time.tv_sec << "." << time.tv_usec << "\n";

    // Reading actual buffer (msg)
    char buffer[MAX_UDP_BUFFER_SIZE];
    read_size = reader->read(buffer, read_length);
    if (read_size < read_length) {
      std::cerr << "Buffer read_size: " << read_size << " read_length: " << read_length << " Break\n";
      break;
    }
    bytes_read += read_size;
    std::cout << "BytesRead: " << bytes_read << "\n";
    std::cout.flush();

    ose_decoder->Decode(buffer, read_length, 1);  // here, 1 is just a dummy file descriptor
  }

  if (reader) {
    reader->close();
  }

  // Stop the mds loggers to flush the streams to the mds files and close them
  HFSAT::OSEMdProcessor::GetInstance()->StopLoggers();

  return 0;
}
