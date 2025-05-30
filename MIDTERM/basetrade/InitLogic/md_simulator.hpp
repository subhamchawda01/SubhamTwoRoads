/**
    \file InitLogic/md_simulator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/email_utils.hpp"

#include "dvccode/Utils/in_memory_data.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "basetrade/MTools/data_processing.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "baseinfra/TradeUtils/market_update_manager.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"

#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"

#include "dvctrade/ModelMath/md_model_creator.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

// not useful but obligated to include libs
// #include "dvccode/TradingInfo/network_account_info_manager.hpp"
//#include "baseinfra/OrderRouting/prom_order_manager.hpp"
//#include "baseinfra/OrderRouting/shortcode_prom_order_manager_map.hpp"
