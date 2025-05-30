/**
 \file dvccode/CDef/online_debug_logger.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#include "dvccode/CDef/online_debug_logger.hpp"

namespace HFSAT {

OnlineDebugLogger::OnlineDebugLogger(size_t buf_capacity, int logging_id, size_t flush_trigger_size)
    : DebugLogger(buf_capacity, flush_trigger_size),
      client_logging_segment_initializer_ptr_(NULL),
      logging_id_(logging_id),
      log_buffer_(new HFSAT::CDef::LogBuffer()) {
  memset((void *)log_buffer_, 0, sizeof(HFSAT::CDef::LogBuffer));
  log_buffer_->content_type_ = HFSAT::CDef::UnstructuredText;
}

OnlineDebugLogger::~OnlineDebugLogger() { Close(); }

void OnlineDebugLogger::Close() {
  logfilename_.clear();
  loglevels_highervec_.clear();

  if (client_logging_segment_initializer_ptr_ != NULL) {
    DumpCurrentBuffer();
    client_logging_segment_initializer_ptr_->CleanUp();
  }

  if (NULL != in_memory_buffer_) {
    free(in_memory_buffer_);
    in_memory_buffer_ = NULL;
  }
}

void OnlineDebugLogger::DumpCurrentBuffer() {
  if (client_logging_segment_initializer_ptr_ != NULL) {
    int bytes_left = (int)front_marker_;
    int offset = 0;
    int bytes_to_write = 0;

    while (bytes_left > 0) {
      offset = front_marker_ - bytes_left;
      bytes_to_write = std::min(TEXT_BUFFER_SIZE, bytes_left);

      memcpy((void *)log_buffer_->buffer_data_.text_data_.buffer, (void *)(in_memory_buffer_ + offset), bytes_to_write);
      bytes_left -= bytes_to_write;
      log_buffer_->buffer_data_.text_data_.buffer[bytes_to_write] = '\0';
      client_logging_segment_initializer_ptr_->Log(log_buffer_);
    }
    front_marker_ = 0;
  }
}

void OnlineDebugLogger::OpenLogFile(const char *logfilename, std::ios_base::openmode open_mode) {
  if (client_logging_segment_initializer_ptr_ != NULL) {
    DumpCurrentBuffer();
    client_logging_segment_initializer_ptr_->CleanUp();
    delete client_logging_segment_initializer_ptr_;
  }

  logfilename_ = logfilename;

  TryOpeningFile();
}

void OnlineDebugLogger::TryOpeningFile() {
  // if directory does not exist create it
  FileUtils::MkdirEnclosing(logfilename_);
  size_t pos = logfilename_.rfind("/");
  std::string dir = logfilename_.substr(0, pos + 1);
  std::string filename = logfilename_.substr(pos + 1);
  client_logging_segment_initializer_ptr_ =
      new HFSAT::Utils::ClientLoggingSegmentInitializer(*this, logging_id_, dir.c_str(), filename.c_str());
}
}
