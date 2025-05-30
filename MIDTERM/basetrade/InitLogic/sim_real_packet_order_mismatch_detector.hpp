/**
    \file InitLogic/sim_real_packet_order_mismatch_detector.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_INITLOGIC_BASE_DATA_GEN_H
#define BASE_INITLOGIC_BASE_DATA_GEN_H

#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "baseinfra/BaseUtils/curve_utils.hpp"

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/sampling_shortcodes.hpp"  // specific to indicator_logger samplings

#include "baseinfra/LoggedSources/aflash_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/asx_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/bse_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/cme_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/control_message_filesource.hpp"
#include "baseinfra/LoggedSources/eobi_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/puma_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/real_packets_file_namer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/sgx_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filesource.hpp"

#include "baseinfra/MarketAdapter/bmf_order_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "baseinfra/MarketAdapter/hkex_indexed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/hybrid_security_market_view.hpp"
#include "baseinfra/MarketAdapter/indexed_cfe_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_cme_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ice_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_liffe_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_micex_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ntp_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ose_price_feed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_rts_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_tmx_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_l1_price_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_order_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_pricefeed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/OrderRouting/shortcode_prom_order_manager_map.hpp"

#include "dvccode/ORSMessages/alpes_retail_trading_exec_livesource.hpp"
#include "dvccode/ORSMessages/control_message_livesource.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"
#include "dvccode/ORSMessages/shortcode_ors_message_livesource_map.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "dvccode/Utils/retail_update_distributor.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/slack_utils.hpp"


#endif  // BASE_INITLOGIC_BASE_DATA_GEN_H
