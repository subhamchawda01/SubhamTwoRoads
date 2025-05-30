/**
    \file time_shift_mdsfile.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#define MILLION 1000000
#define THOUSAND 1000

// assumption that eventual time will be greater than 0 always.
// i.e. param milli_secs will never be larger than tv
void addMilliSecs(timeval& tv, long milli_secs) {
  long t = tv.tv_sec * MILLION + tv.tv_usec + milli_secs * THOUSAND;
  tv.tv_sec = t / MILLION;
  tv.tv_usec = t % MILLION;
}

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cout << "usage:" << argv[0] << " <input-file> <output-file> <exchange> <time-shift-to-be-added(in msecs)>\n";
    exit(0);
  }

  // ensure input-file exists
  // ensure output file does not exist
  // ensure exchange is valid
  // ensure time-shift is an integer

  if (!HFSAT::FileUtils::ExistsAndReadable(argv[1])) {
    std::cout << "input file " << argv[1] << " does not exist. aborting ... ";
    exit(0);
  }

  // done so that we don't accidently erase overwrite files.
  if (HFSAT::FileUtils::ExistsAndReadable(argv[2])) {
    std::cout << "output file " << argv[2] << " exists. aborting...";
    exit(0);
  }
  std::string exch_ = argv[3];
  long time_shift = atol(argv[4]);

  HFSAT::BulkFileReader reader_;
  reader_.open(argv[1]);
  HFSAT::BulkFileWriter writer_;
  writer_.Open(argv[2]);

  if (exch_ == "CME") {
    CME_MDS::CMECommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else if (exch_ == "EUREX") {
    EUREX_MDS::EUREXCommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else if (exch_ == "BMF") {
    BMF_MDS::BMFCommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else if (exch_ == "TMX") {
    TMX_MDS::TMXCommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else if (exch_ == "CHIX") {
    HFSAT::BATSCHI_MDS::BATSCHICommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else if (exch_ == "CHIX_L1") {
    HFSAT::BATSCHI_PL_MDS::BatsChiPLCommonStruct next_event_;
    while (reader_.read(&next_event_, sizeof(next_event_)) >= sizeof(next_event_)) {
      addMilliSecs(next_event_.time_, time_shift);
      writer_.Write(&next_event_, sizeof(next_event_));
      writer_.CheckToFlushBuffer();
    }
  } else {
    std::cout << "invalid exchange " << exch_ << "\n";
  }

  writer_.Close();
  reader_.close();
  return 0;
}
