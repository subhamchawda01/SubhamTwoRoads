// =====================================================================================
//
//       Filename:  ParticipantUpdateInfoUpdateDBResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 10:21:42 AM
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

//=========== NSE PARTICIPANT UPDATE INFO UPDATE DB RESPONSE ============================//

#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_LENGTH 12

#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_OFFSET \
  (NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_OFFSET +      \
   NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_LENGTH)
#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_LENGTH 25

#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTSTATUS_OFFSET \
  (NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_OFFSET +      \
   NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_LENGTH)
#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTSTATUS_LENGTH 1

#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTUPDATEDATETIME_OFFSET \
  (NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTSTATUS_OFFSET +            \
   NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTSTATUS_LENGTH)
#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTUPDATEDATETIME_LENGTH sizeof(int32_t)

#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_DELETEFLAG_OFFSET             \
  (NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTUPDATEDATETIME_OFFSET + \
   NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTUPDATEDATETIME_LENGTH)
#define NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_DELETEFLAG_LENGTH 1

namespace HFSAT {
namespace NSE {

class NSEParticipantUpdate {
 private:
  ProcessedParticipantUpdateInfo participant_update_info;

 public:
  NSEParticipantUpdate() { memset((void *)&participant_update_info, 0, sizeof(ProcessedParticipantUpdateInfo)); }

  inline ProcessedParticipantUpdateInfo *GetParticipanUpdateInfo(char const *msg_ptr) {
    memcpy((void *)participant_update_info.participant_id,
           (void *)(msg_ptr + NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_OFFSET),
           NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTID_LENGTH);

    memcpy((void *)participant_update_info.participant_name,
           (void *)(msg_ptr + NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_OFFSET),
           NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTNAME_LENGTH);

    participant_update_info.participant_status =
        *((char *)(msg_ptr + NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTSTATUS_OFFSET));

    participant_update_info.participant_update_date_time = ntoh32(
        *((int32_t *)(msg_ptr + NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_PARTICIPANTUPDATEDATETIME_OFFSET)));

    participant_update_info.delete_flag =
        *((char *)(msg_ptr + NSE_PARTICIPANT_UPDATE_INFO_UPDATEDB_RESPONSE_DELETEFLAG_OFFSET));

    return &participant_update_info;
  }
};
}
}
