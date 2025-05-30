// =====================================================================================
// 
//       Filename:  bardata_logged_message_filesource.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  11/23/2022 07:17:37 AM
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

#include <stdio.h>
#include <stdlib.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/bardata_logged_message_filenamer.hpp"

namespace HFSAT {

  class BarDataLoggedFileSource : public ExternalDataListener {

    protected:
      DebugLogger& dbglogger_;
      SecurityNameIndexer& sec_name_indexer_;
      uint32_t simrun_date_;
      const unsigned int security_id_;
      std::string bardata_symbol_;
      std::string bardata_mapped_symbol_in_file_;
      std::string shortcode_;
      ExternalTimeListener* p_time_keeper_;
      NSEBardataListener* p_nse_bardata_listener_;
      BSEBardataListener* p_bse_bardata_listener_;
      CBOEBardataListener* p_cboe_bardata_listener_;
      BulkFileReader bulk_file_reader_;
      char lastline_buffer_[1024];
      std::string line_read_string;
      bool is_options_;
      std::string options_expiry_;
      std::string exch_;


    public:
      BarDataLoggedFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                              const unsigned int _yyyymmdd_,
                              const unsigned int t_security_id_, const char* t_bardata_symbol_,
                              const char* t_bardata_mapped_symbol_in_file_, const char* t_shortcode_, 
                              bool t_is_options_,std::string t_options_expiry_, std::string t_exch_);

      inline int socket_file_descriptor() const { return 0; }

      void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);
      void ComputeEarliestDataTimestamp(bool& t_hasevents_);

      void ProcessAllEvents();
      void ProcessEventsTill(const ttime_t t_endtime_);
      inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

      inline void SetNSEBarDataListener(NSEBardataListener* p_new_listener_) {
        p_nse_bardata_listener_ = p_new_listener_;
      }

      inline void SetBSEBarDataListener(BSEBardataListener* p_new_listener_) {
        p_bse_bardata_listener_ = p_new_listener_;
      }

      inline void SetCBOEBarDataListener(CBOEBardataListener* p_new_listener_) {
        p_cboe_bardata_listener_ = p_new_listener_;
      }

      inline void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
      }

      //Global Scope
      static ttime_t last_processed_bardata_time_;
    private:
      void _ProcessThisMsg();

  };

}
