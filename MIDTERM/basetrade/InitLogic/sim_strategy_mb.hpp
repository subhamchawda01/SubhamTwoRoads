#pragma once

/**
    \file InitLogic/sim_strategy_mb.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/slack_utils.hpp"

#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/eti_algo_tagging.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"

#include "baseinfra/MinuteBar/minute_bar_sim_trader.hpp"
#include "baseinfra/MinuteBar/minute_bar_strategy_desc.hpp"
#include "basetrade/MinuteBar/minute_bar_pnl.hpp"

#include "baseinfra/MinuteBar/minute_bar_order_manager.hpp"
#include "basetrade/MinuteBar/minute_bar_smv_source.hpp"

#include "baseinfra/MinuteBar/base_minute_bar_exec_logic.hpp"
