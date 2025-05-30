/**
 \file dvccode/CDef/online_debug_logger.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

namespace HFSAT {

class OnlineDebugLogger : public DebugLogger {
 protected:
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr_;
  int logging_id_;
  HFSAT::CDef::LogBuffer* log_buffer_;

 public:
  OnlineDebugLogger(size_t buf_capacity, int logging_id, size_t flush_trigger_size = 0);

  ~OnlineDebugLogger();

  void Close();

  void DumpCurrentBuffer();

  void OpenLogFile(const char* logfilename, std::ios_base::openmode open_mode);

  void TryOpeningFile();
};
}
