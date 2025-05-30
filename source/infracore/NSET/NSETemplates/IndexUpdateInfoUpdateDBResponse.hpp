// =====================================================================================
//
//       Filename:  IndexUpdateInfoUpdateDBResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 10:38:12 AM
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

//==================== NSE INDEX UPDATE INFO UPDATE DB RESPONSE ====================================================//

#define NSE_INDEXUPDATE_UPDATEDB_RESPONSE_NOOFRECORDS_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_INDEXUPDATE_UPDATEDB_RESPONSE_NOOFRECORDS_LENGTH sizeof(int16_t)

#define NSE_INDEXUPDATE_UPDATEDB_RESPONSE_INDEX_DETAILS_REPETITIVE_STRUCT_OFFSET \
  (NSE_INDEXUPDATE_UPDATEDB_RESPONSE_NOOFRECORDS_OFFSET + NSE_INDEXUPDATE_UPDATEDB_RESPONSE_NOOFRECORDS_LENGTH)
#define NSE_INDEXUPDATE_UPDATEDB_RESPONSE_INDEX_DETAILS_REPETITIVE_STRUCT_LENGTH 408

namespace HFSAT {
namespace NSE {

class NSEIndexUpdateInfo {
 private:
  ProcessedDownloadIndex processed_download_index;

 public:
  NSEIndexUpdateInfo() { memset((void *)&processed_download_index, 0, sizeof(ProcessedDownloadIndex)); }

  inline ProcessedDownloadIndex *ProcessIndexInfo(char const *msg_ptr) {
    processed_download_index.number_of_records =
        ntoh16(*((int16_t *)(msg_ptr + NSE_INDEXUPDATE_UPDATEDB_RESPONSE_NOOFRECORDS_OFFSET)));

    for (uint32_t record_counter = 0; record_counter < (uint32_t)processed_download_index.number_of_records;
         record_counter++) {
      memcpy((void *)processed_download_index.indexes[record_counter].index_name,
             (void *)(msg_ptr + NSE_INDEXUPDATE_UPDATEDB_RESPONSE_INDEX_DETAILS_REPETITIVE_STRUCT_OFFSET +
                      24 * record_counter),
             15);
      processed_download_index.indexes[record_counter].token =
          ntoh32(*((int32_t *)(msg_ptr + NSE_INDEXUPDATE_UPDATEDB_RESPONSE_INDEX_DETAILS_REPETITIVE_STRUCT_OFFSET + 16 +
                               24 * record_counter)));
      processed_download_index.indexes[record_counter].last_update_date_time =
          ntoh32(*((int32_t *)(msg_ptr + NSE_INDEXUPDATE_UPDATEDB_RESPONSE_INDEX_DETAILS_REPETITIVE_STRUCT_OFFSET + 20 +
                               24 * record_counter)));
    }

    return &processed_download_index;
  }
};
}
}
