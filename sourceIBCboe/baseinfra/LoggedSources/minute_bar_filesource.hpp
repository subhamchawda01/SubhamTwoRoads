/**
    \file LoggedSources/minute_bar_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include <fstream>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/EventDispatcher/mbar_events.hpp"
#include "baseinfra/EventDispatcher/minute_bar_data_filenamer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"  // From hfsat
#include "baseinfra/EventDispatcher/minute_bar_events_listener.hpp"

#define MAX_LINE_SIZE 1024

namespace HFSAT {

/// @brief reads CME logged data stored in file for this { symbol, tradingdate }
/// Reads CME_MDS::CMECommonStruct
///    depending on CME_MDS::CMECommonStruct::msgType
///    and for msgType == CME_MDS::CME_DELTA
///        depending on CME_MDS::CMECommonStruct::data_::cme_dels_::action_
///        calls the listeners, if any, of the security, action_
class MidTermFileSource {
 public:
  DebugLogger& dbglogger_;
  bool are_we_filtering;
  const std::string underlying_;
  const char segment_;
  char line_buffer[MAX_LINE_SIZE];
  char list_of_expiries_to_consider[10];
  std::ifstream infile;
  hftrap::defines::MbarEvent next_event_;
  std::vector<hftrap::defines::MbarEvent> cur_event_vec;
  std::string expiry_list_;
  int next_event_timestamp_;
  int curr_event_timestamp_;

 public:
  /**
   * @param t_dbglogger_ for logging errors
   * @param t_sec_name_indexer_ to detect if the security is of interest and not to process if not. If string matching
   * is more efficient we could use t_exchange_symbol_ as well.
   * @param t_preevent_YYYYMMDD_ tradingdate to load the appropriate file
   * @param t_security_id_ also same as t_sec_name_indexer_ [ t_exchange_symbol_ ]
   * @param t_exchange_symbol_ needed to match
   *
   * For now assuming t_exchange_symbol_ matching is not required
   */
  MidTermFileSource(DebugLogger& t_dbglogger_, const std::string underlying_, const char segment_,
                    std::string expiry_list = "ALL");

  void SeekToFirstEventAfter(const time_t r_start_time_, bool& rw_hasevents_);

  void AddEventListeners(hftrap::eventdispatcher::MinuteBarEventsListener* minute_bar_events_listeners);

  bool SetNextEventVector();

  inline const time_t next_event_timestamp() const { return next_event_timestamp_; }
  inline const time_t curr_event_timestamp() const { return curr_event_timestamp_; }

  std::vector<hftrap::defines::MbarEvent> GetCurrentEventVector(const time_t time);

 private:
  void NotifyListenersOnAllEventsConsumed();
};
typedef std::vector<MidTermFileSource*> MidTermFileSourcePtrVec;
typedef std::vector<MidTermFileSource*>::iterator MidTermFileSourcePtrVecIter;
}
