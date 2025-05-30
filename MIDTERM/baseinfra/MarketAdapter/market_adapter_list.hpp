/**
   \file MarketAdapter/market_adapter_list.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#pragma once

#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/hybrid_security_market_view.hpp"

#include "baseinfra/MarketAdapter/indexed_eurex_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_liffe_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ntp_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_rts_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_rts_of_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_cme_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_micex_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ose_price_feed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_cfe_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_tmx_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ice_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_asx_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_fpga_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/hkex_indexed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/bmf_order_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_l1_price_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_order_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/ose_pricefeed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/l1_price_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ose_order_feed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_bmf_fpga_market_view_manager.hpp"

// book building modification purpose
#include "baseinfra/MarketAdapter/indexed_tmx_obf_of_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_micex_of_market_view_manager.hpp"

#include "baseinfra/MarketAdapter/market_orders_view.hpp"
#include "baseinfra/MarketAdapter/asx_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/eobi_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/sgx_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/eurex_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/ose_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/ice_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/hk_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/ntp_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/cme_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/nse_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/spread_market_view.hpp"
#include "baseinfra/MarketAdapter/synthetic_market_view.hpp"
#include "baseinfra/MarketAdapter/generic_l1_data_market_view_manager.hpp"
