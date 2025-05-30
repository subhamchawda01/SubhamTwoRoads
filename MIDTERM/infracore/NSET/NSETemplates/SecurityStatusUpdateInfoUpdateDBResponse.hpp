// =====================================================================================
//
//       Filename:  SecurityStatusUpdateInfoUpdateDBResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 10:11:58 AM
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

#include "infracore/NSET/NSETemplates/DataDefines.hpp"

//===================================  NSE SECURITY UPDATE INFO DB UPDATE RESPONSE ===============================//

#define NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_LENGTH sizeof(int16_t)

#define NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET \
  (NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET +                          \
   NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_LENGTH)
#define NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_LENGTH 420

namespace HFSAT {
namespace NSE {

class NSESecurityStatusUpdate {
 private:
  ProcessedSecurityStatusUpdateInfo processed_security_status_update_info;

 public:
  NSESecurityStatusUpdate() {
    memset((void *)&processed_security_status_update_info, 0, sizeof(ProcessedSecurityStatusUpdateInfo));
  }

  inline ProcessedSecurityStatusUpdateInfo *GetProcessedSecurityStatus(char const *msg_ptr) {
    processed_security_status_update_info.number_of_records =
        ntoh16(*((int16_t *)(msg_ptr + NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET)));

    for (uint32_t status_counter = 0;
         status_counter < (uint32_t)processed_security_status_update_info.number_of_records; status_counter++) {
      processed_security_status_update_info.token_and_eligibility[status_counter].token = ntoh32(
          *((int32_t *)(msg_ptr +
                        NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET +
                        12 * status_counter)));

      processed_security_status_update_info.token_and_eligibility[status_counter].status[0] = ntoh16(
          *((int16_t *)(msg_ptr +
                        NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET + 4 +
                        12 * status_counter)));

      processed_security_status_update_info.token_and_eligibility[status_counter].status[1] = ntoh16(
          *((int16_t *)(msg_ptr +
                        NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET + 4 +
                        2 + 12 * status_counter)));

      processed_security_status_update_info.token_and_eligibility[status_counter].status[2] = ntoh16(
          *((int16_t *)(msg_ptr +
                        NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET + 4 +
                        4 + 12 * status_counter)));

      processed_security_status_update_info.token_and_eligibility[status_counter].status[3] = ntoh16(
          *((int16_t *)(msg_ptr +
                        NSE_SECUTIY_STATUS_UPDATE_UPDATEDB_RESPONSE_TOKEN_AND_ELIGIBILITY_REPITITIVE_STRUCT_OFFSET + 4 +
                        6 + 12 * status_counter)));
    }

    return &processed_security_status_update_info;
  }
};
}
}
