// =====================================================================================
//
//       Filename:  InstrumentUpdateInfoUpdateDBResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 10:34:22 AM
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

//===================  NSE INSTRUMENT UPDATE DB RESPONSE ========================================//

#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTID_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTID_LENGTH sizeof(int16_t)

#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_OFFSET \
  (NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTID_OFFSET +      \
   NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTID_LENGTH)
#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_LENGTH 6

#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_OFFSET \
  (NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_OFFSET +    \
   NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_LENGTH)
#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_LENGTH 25

#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTUPDATETIME_OFFSET \
  (NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_OFFSET +          \
   NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_LENGTH)
#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTUPDATETIME_LENGTH sizeof(int32_t)

#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_DELETEFLAG_OFFSET        \
  (NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTUPDATETIME_OFFSET + \
   NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTUPDATETIME_LENGTH)
#define NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_DELETEFLAG_LENGTH 1

namespace HFSAT {
namespace NSE {

class NSEInstrumentUpdateInfo {
 private:
  ProcessedInstrumentUpdateInfo processed_instrument_update_info;

 public:
  NSEInstrumentUpdateInfo() {
    memset((void *)&processed_instrument_update_info, 0, sizeof(ProcessedInstrumentUpdateInfo));
  }

  inline ProcessedInstrumentUpdateInfo *ProcessInstrumentUpdate(char const *msg_ptr) {
    processed_instrument_update_info.instrument_id =
        ntoh32(*((int32_t *)(msg_ptr + NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTID_OFFSET)));
    processed_instrument_update_info.instrument_update_date_time =
        ntoh32(*((int32_t *)(msg_ptr + NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTUPDATETIME_OFFSET)));
    memcpy((void *)processed_instrument_update_info.instrument_name,
           (void *)(msg_ptr + NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_OFFSET),
           NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTNAME_LENGTH);
    memcpy((void *)processed_instrument_update_info.instrument_description,
           (void *)(msg_ptr + NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_OFFSET),
           NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_INSTRUMENTDESC_LENGTH);
    processed_instrument_update_info.delete_flag =
        *((char *)(msg_ptr + NSE_INSTRUMENTUPDATE_UPDATEDB_RESPONSE_DELETEFLAG_OFFSET));

    return &processed_instrument_update_info;
  }
};
}
}
