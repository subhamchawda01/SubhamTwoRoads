/**
    \file dvccode/CDef/error_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_ERROR_UTILS_H
#define BASE_CDEF_ERROR_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/debug_logger.hpp"

namespace HFSAT {
void ExitVerbose(ExitErrorCode_t _exit_error_code_);
void ExitVerbose(ExitErrorCode_t _exit_error_code_, DebugLogger& _dbglogger_);
void ExitVerbose(ExitErrorCode_t _exit_error_code_, const char* _text_to_write_);
void ExitVerbose(ExitErrorCode_t _exit_error_code_, DebugLogger& _dbglogger_, const char* _text_to_write_);
}
#endif  // BASE_CDEF_ERROR_UTILS_H
