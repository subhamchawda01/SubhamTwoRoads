// =====================================================================================
//
//       Filename:  IndexMapDetailsUpdateDBResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 10:41:32 AM
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

//========  NSE INDEX MAP DETAILS UPDATE INFO UPDATE DB RESPONSE =============================================//

#define NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_LENGTH sizeof(int16_t)

#define NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_OFFSET \
  (NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET +                  \
   NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_LENGTH)
#define NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_LENGTH \
  420  // ALthough doc says 410 (41*10), It's 42*10
namespace HFSAT {
namespace NSE {

class NSEIndexMapDetails {
 private:
  ProcessedDownloadIndexMap processed_download_index_map;

 public:
  NSEIndexMapDetails() { memset((void *)&processed_download_index_map, 0, sizeof(ProcessedDownloadIndexMap)); }

  inline ProcessedDownloadIndexMap *ProcessIndexMapDetails(char const *msg_ptr) {
    processed_download_index_map.number_of_records =
        ntoh16(*((int16_t *)(msg_ptr + NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_NUMBER_OF_RECORDS_OFFSET)));

    for (uint32_t counter = 0; counter < (uint32_t)processed_download_index_map.number_of_records; counter++) {
      memcpy((void *)processed_download_index_map.indexes[counter].bcast_name,
             (void *)(msg_ptr + NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_OFFSET +
                      42 * counter),
             26);
      memcpy((void *)processed_download_index_map.indexes[counter].changed_name,
             (void *)(msg_ptr + NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_OFFSET + 26 +
                      42 * counter),
             10);
      processed_download_index_map.indexes[counter].delete_flag =
          *((char *)(msg_ptr + NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_OFFSET + 36 +
                     42 * counter));
      processed_download_index_map.indexes[counter].last_update_date_time =
          ntoh32(*((int32_t *)(msg_ptr + NSE_INDEXMAP_DETAILS_UPDATEDB_RESPONSE_BCASTINDEXMAP_REPITITIVE_STRUCT_OFFSET +
                               38 + 42 * counter)));
    }

    return &processed_download_index_map;
  }
};
}
}
