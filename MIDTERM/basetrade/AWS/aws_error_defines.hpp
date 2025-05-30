// =====================================================================================
//
//       Filename:  aws_error_defines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/02/2014 09:06:22 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

namespace HFSAT {
namespace AWS {

enum ERROR_TYPE { FILE_INFO = 0, FILE_ISSUE, SCHED_ISSUE, CONFIG_ISSUE, JOB_ISSUE, SCHED_INFO };

static void ReportAWSError(ERROR_TYPE _error_type_, std::string _mail_body_, std::string _email_to_ = "") {}
}
}
