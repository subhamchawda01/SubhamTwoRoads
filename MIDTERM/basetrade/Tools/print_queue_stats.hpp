/**
   \file basetrade/Tools/print_queue_stats.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/LoggedSources/eobi_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"

#include "baseinfra/Tools/common_smv_source.hpp"

struct ORSMktStruct {
  HFSAT::GenericORSReplyStruct ors_struct;
  std::vector<HFSAT::ttime_t> send_time_vec;
  std::vector<HFSAT::ttime_t> data_time_vec;
  std::vector<HFSAT::ORRType_t> orr_type_vec;
  int queue_at_send = -1;
  int queue_at_conf = -1;
  int queue_at_cxl_seq = -1;
  int position_at_cxl_seq = -1;
  int queue_at_cxld = -1;
  int position_at_cxld = -1;
  int queue_at_exec = -1;
  int queue_at_modify = -1;
  int position_at_modify = -1;

  int orders_executed_before = -1;
  int size_executed_before = -1;
  bool is_live = false;

  std::string Header() {
    std::stringstream current_out;
    current_out << std::setw(5) << "SAOS"
                << " " << std::setw(6) << "#PX #B/S #SACI"
                << " " << std::setw(4) << "#S_REM"
                << " " << std::setw(4) << "#S_EXEC"
                << " " << std::setw(19) << "#ORDER_ID [ " << std::setw(6) << "#Q_SEND"
                << " " << std::setw(6) << "#Q_CONF"
                << " ( #Q_CXL_SEQ #P_CXL_SEQ ) ( #Q_CXLD #P_CXLD) ( #Q_MOD #P_MOD ) " << std::setw(6) << "#Q_EXEC ] "
                << std::setw(6) << "OE_BEF"
                << " " << std::setw(6) << "SE_BEF";
    return current_out.str();
  }

  std::string ToString() {
    std::stringstream current_out;

    /*current_out << std::setw(5) << ors_struct.server_assigned_order_sequence_ << ", " << std::setw(6)
                << ors_struct.price_ << ", " << ors_struct.buysell_ << ", " << ors_struct.server_assigned_client_id_
                << ", " << std::setw(4) << ors_struct.size_remaining_ << ", " << std::setw(4)
                << ors_struct.size_executed_ << ", " << std::setw(19) << ors_struct.exch_assigned_sequence_ << ", [, "
                << std::setw(6) << queue_at_send << ", " << std::setw(6) << queue_at_conf << ", (, " << queue_at_cxl_seq
                << ", " << position_at_cxl_seq << ", ), (, " << queue_at_cxld << ", " << position_at_cxld << ", ), (, "
                << queue_at_modify << ", " << position_at_modify << ", ), " << std::setw(6) << queue_at_exec << ", ], "
                << std::setw(6) << orders_executed_before << ", " << std::setw(6) << size_executed_before << ", ";
                */
    current_out << std::setw(5) << ors_struct.server_assigned_order_sequence_ << ", " << ors_struct.price_ << ", "
                << ors_struct.buysell_ << ", " << ors_struct.server_assigned_client_id_ << ", "
                << ors_struct.size_remaining_ << ", " << ors_struct.size_executed_ << ", "
                << ors_struct.exch_assigned_sequence_ << ", [, " << queue_at_send << ", " << queue_at_conf << ", (, "
                << queue_at_cxl_seq << ", " << position_at_cxl_seq << ", ), (, " << queue_at_cxld << ", "
                << position_at_cxld << ", ), (, " << queue_at_modify << ", " << position_at_modify << ", ), "
                << queue_at_exec << ", ], " << orders_executed_before << ", " << size_executed_before << ", ";

    for (auto i = 0u; i < send_time_vec.size(); i++) {
      current_out << send_time_vec[i].ToString() << ", " << HFSAT::ToString(orr_type_vec[i]) << ", ";
    }

    return current_out.str();
  }
};
