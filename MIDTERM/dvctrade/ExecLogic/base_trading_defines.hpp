/*
 * base_trading_defines.hpp
 *
 *  Created on: Dec 21, 2015
 *      Author: diwakar
 */

#ifndef BASE_TRADING_DEFINES_HPP_
#define BASE_TRADING_DEFINES_HPP_

#include "dvccode/Utils/slack_utils.hpp"

#define MAX_POS_MAP_SIZE 64
#define TRADE_EFFECT_TIME 2000
#define FAT_FINGER_FACTOR 5

#define MULT_ORDER_GET_FLAT_PROD_FILE "/spare/local/tradeinfo/OfflineInfo/prods_to_getflat_mult_ord.txt"
#define FOK_MODE_SHC_FILE "/spare/local/tradeinfo/getflatfok_shortcodes.txt"
// 30 secs before the close - aim is to relase the core at that point
#define RELEASE_CORE_BEFORE_GIVEN_INTERVAL_IN_MSEC 90000
// Output directory for exec pid log file
#define OUTPUT_LOGDIR "/spare/local/logs/tradelogs/PID_TEXEC_DIR/"

#endif /* BASE_TRADING_DEFINES_HPP_ */
