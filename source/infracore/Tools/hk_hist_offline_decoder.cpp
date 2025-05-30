/**
 \file HKHist/hk_hist_offline_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 162, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include "infracore/DataDecoders/HKHistorical/hk_hist_decoder.hpp"
#include "infracore/DataDecoders/HKHistorical/hk_hist_md_processor.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

void printHexString(const char* c, int len) {
  for (int i = 0; i < len; ++i) {
    uint8_t ch = c[i];
    printf("%d->%02x ", i, ch);
  }
  printf("\n");
  for (int i = 0; i < len; ++i) {
    char ch = c[i];
    printf("%c ", ch);
  }
  printf("\n");
}

void ProcessBulkFile(std::string filename, HFSAT::HKHist::HKHistDecoder* hk_hist_decoder) {
  HFSAT::BulkFileReader* reader = new HFSAT::BulkFileReader();
  reader->open(filename);
  std::cout << "Processing : " << filename << "\n";
  if (!reader->is_open()) {
    std::cerr << " Can't open file : " << filename << " exiting \n";
    return;
  }

  while (true) {
    uint16_t msg_len;
    int read_size;

    read_size = reader->read(&(msg_len), sizeof(uint16_t));
    if (read_size < (int)sizeof(uint16_t)) break;

    msg_len = __builtin_bswap16(msg_len);

    char buffer[1024];
    read_size = reader->read(buffer, msg_len);

    //    printHexString(buffer, msg_len);

    if (read_size < msg_len) break;

    hk_hist_decoder->Decode(buffer, msg_len, 1);  // here, 1 is just a dummy file descriptor
  }

  if (reader) {
    reader->close();
    delete reader;
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <input-dir> <date> [CONVERT]" << std::endl;
    exit(-1);
  }

  if (argc > 3) {
    if (!strcmp(argv[3], "CONVERT")) {
      HFSAT::HKHistMdProcessor::SetInstance(HFSAT::kPriceFeedLogger, true);
    }
  } else {
    HFSAT::HKHistMdProcessor::SetInstance(HFSAT::kLogger, true);
  }

  HFSAT::HKHist::HKHistDecoder* hk_hist_decoder = &(HFSAT::HKHist::HKHistDecoder::GetUniqueInstance());

  std::string dir = argv[1];
  std::string date = argv[2];

  std::string ref_file_name = dir + "/MC01_All_" + date;
  ProcessBulkFile(ref_file_name, hk_hist_decoder);

  hk_hist_decoder->FlushRefDataToFile(date);

  for (int i = 30; i <= 38; i++) {
    char file_num[3] = {0};
    sprintf(file_num, "%d", i);

    std::string stock_file_name = dir + "/MC" + std::string(file_num) + "_All_" + date;
    ProcessBulkFile(stock_file_name, hk_hist_decoder);
  }

  // Stop the mds loggers to flush the streams to the mds files and close them
  HFSAT::HKHistMdProcessor::GetInstance()->StopLoggers();

  return 0;
}
