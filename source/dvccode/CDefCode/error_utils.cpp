/**
    \file CDefCode/error_utils.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/error_utils.hpp"

namespace HFSAT {
#define ALSO_PRINT_TO_STDERR_AFTER_DBGLOGGER

void ExitVerbose(ExitErrorCode_t _exit_error_code_) {
  std::cerr << ErrorCodeToString(_exit_error_code_) << std::endl;
  exit((int)_exit_error_code_);
}

void ExitVerbose(ExitErrorCode_t _exit_error_code_, const char* _text_to_write_) {
  std::cerr << ErrorCodeToString(_exit_error_code_) << " " << _text_to_write_ << std::endl;
  exit((int)_exit_error_code_);
}

void ExitVerbose(ExitErrorCode_t _exit_error_code_, DebugLogger& t_dbglogger_) {
  t_dbglogger_ << ErrorCodeToString(_exit_error_code_) << '\n';
  t_dbglogger_.CheckToFlushBuffer();
#ifdef ALSO_PRINT_TO_STDERR_AFTER_DBGLOGGER
  std::cerr << ErrorCodeToString(_exit_error_code_) << std::endl;
#endif
  t_dbglogger_.Close();
  exit((int)_exit_error_code_);
}

void ExitVerbose(ExitErrorCode_t _exit_error_code_, DebugLogger& t_dbglogger_, const char* _text_to_write_) {
  t_dbglogger_ << ErrorCodeToString(_exit_error_code_) << " " << _text_to_write_ << '\n';
  t_dbglogger_.CheckToFlushBuffer();
#ifdef ALSO_PRINT_TO_STDERR_AFTER_DBGLOGGER
  std::cerr << ErrorCodeToString(_exit_error_code_) << " " << _text_to_write_ << std::endl;
#endif
  t_dbglogger_.Close();
  exit((int)_exit_error_code_);
}
}
