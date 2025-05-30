// =====================================================================================
//
//       Filename:  nse_config_based_checks.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/14/2015 09:18:22 AM
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

#include <iostream>
#include <algorithm>
#include <set>

#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define NSE_INDIVIDUAL_ORDER_BASED_CHECK_FILE "/spare/local/files/NSE/nse_individual_order_constraints.txt"
#define NSE_INDIVIDUAL_BANNED_PRODUCTS_FILE "/spare/local/files/NSE/nse_banned_products.txt"
#define NSE_MARGIN_FILE "/spare/local/files/NSE/nse_margin.txt"
#define NSE_SYMBOL_MAPPING_FILE "/spare/local/files/NSE/symbol_mapping.txt"
#define NSE_CONFIG_VALUES_FILE "/spare/local/files/NSE/nse_config.txt"
#define NSE_TRADE_PROTECTION_PRICERANGE_FILE "/spare/local/files/NSE/nse_trade_protection.txt"

#define MAX_ORDER_VALUE 100000000  // 10 cr
#define MAX_PRICE_VALUE 100000
#define MAX_POSITION_VALUE 10000

#define MAX_COMBINED_POSITION 1000

namespace HFSAT {
namespace NSE {

struct OrderValueChecks {
  int64_t order_qty_check;
  double order_value_check;
};

struct PriceBandChecks {
  double price_lower_limit;
  double price_higher_limit;
};

struct MarginValues {
  double exposure_margin;
  double span_margin;
};

class NSEConfigBasedChecks {
 private:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Lock limits_mutex;
  HFSAT::Utils::NSERefDataLoader &nse_ref_data_loader_;
  std::map<int32_t, NSE_UDP_MDS::NSERefData> &nse_ref_data;
  double cumulative_open_order_value_limit_;
  std::set<std::string> mwpl_check_;
  double overall_turnover_limit_trading_limit_check_;
  double overall_turnover_limit_trading_value_;
  double overall_branchwise_turnover_trading_limit_check_;
  double approved_exposure_margin_limit_;
  double approved_overall_margin_limit_;

  OrderValueChecks common_order_level_checks_;
  std::map<int32_t, PriceBandChecks> internal_symbol_to_individual_price_checks_map_;
  std::map<std::string, MarginValues> internal_symbol_to_marginvalues_map_;
  std::map<std::string, int32_t> internal_symbol_token_map_;
  std::map<int32_t, std::string> token_to_internal_symbol_map_;
  std::map<int32_t, int32_t> token_to_position_map_;
  std::set<int32_t> options_;
  std::map<int32_t, double> security_to_last_traded_price_;
  std::map<int32_t, int16_t> security_to_market_status_;
  std::map<int32_t, double> security_to_mid_price_;

  double LOWER_PRICE_LIMIT;
  double UPPER_PRICE_LIMIT;

  bool VerifySanityOfTheOrderValue(OrderValueChecks order_value_limits) {
    if (order_value_limits.order_qty_check <= 0) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID ORDER QTY LIMIT : " << order_value_limits.order_qty_check
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    if (order_value_limits.order_value_check <= 0 || order_value_limits.order_value_check > MAX_ORDER_VALUE) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID ORDER VALUE LIMIT : " << order_value_limits.order_value_check
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    return true;
  }

  bool VerifySanityOfThePositionLimit(int32_t token, int32_t position_limit) {
    if (position_limit <= 0 || position_limit > MAX_POSITION_VALUE) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID POSITION LIMIT : " << position_limit << " FOR : " << token
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    return true;
  }

  bool VerifySanityOfThePriceBand(int32_t token, PriceBandChecks price_band_checks) {
    if (price_band_checks.price_lower_limit <= 0 || price_band_checks.price_higher_limit <= 0 ||
        price_band_checks.price_lower_limit >= MAX_PRICE_VALUE ||
        price_band_checks.price_higher_limit >= MAX_PRICE_VALUE ||
        price_band_checks.price_lower_limit >= price_band_checks.price_higher_limit) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID PRICE RANGE : " << price_band_checks.price_lower_limit << " <-> "
                                   << price_band_checks.price_higher_limit << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    return true;
  }

  void LoadTradeProtectionPriceRange() {
    std::ifstream trade_protection_fs;
    trade_protection_fs.open(NSE_TRADE_PROTECTION_PRICERANGE_FILE, std::ifstream::in);

    if (!trade_protection_fs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO LOAD NSE_TRADE_PROTECTION_PRICERANGE DATA : "
                                   << NSE_TRADE_PROTECTION_PRICERANGE_FILE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      LOWER_PRICE_LIMIT = 0.95;
      UPPER_PRICE_LIMIT = 1.05;
    }

    char buffer[1024];

    while (trade_protection_fs.good()) {
      trade_protection_fs.getline(buffer, 1024);

      HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
      const std::vector<const char *> &tokens = st.GetTokens();

      if (std::string(buffer).find("#") != std::string::npos) continue;
      if (tokens.size() != 2) continue;

      LOWER_PRICE_LIMIT = atof(tokens[0]) < 0.80 ? 0.80 : atof(tokens[0]);
      UPPER_PRICE_LIMIT = atof(tokens[0]) > 1.20 ? 1.20 : atof(tokens[1]);

      if (UPPER_PRICE_LIMIT <= LOWER_PRICE_LIMIT) {
        LOWER_PRICE_LIMIT = 0.80;
        UPPER_PRICE_LIMIT = 1.20;
      }
    }

    trade_protection_fs.close();
  }

  void LoadSymbolMapping() {
    std::ifstream symbol_mapping_fs;
    symbol_mapping_fs.open(NSE_SYMBOL_MAPPING_FILE, std::ifstream::in);

    if (!symbol_mapping_fs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO LOAD SYMBOL MAPPING DATA : " << NSE_SYMBOL_MAPPING_FILE
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    char buffer[1024];

    while (symbol_mapping_fs.good()) {
      symbol_mapping_fs.getline(buffer, 1024);

      HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
      const std::vector<const char *> &tokens = st.GetTokens();

      if (std::string(buffer).find("#") != std::string::npos) continue;
      if (tokens.size() < 4) continue;

      internal_symbol_token_map_[tokens[0]] = atoi(tokens[1]);
      token_to_internal_symbol_map_[atoi(tokens[1])] = tokens[0];

      if (std::string(tokens[2]) == std::string("OPT")) {
        options_.insert(atoi(tokens[1]));
      }

      if (true == VerifySanityOfThePositionLimit(atoi(tokens[1]), atoi(tokens[3]))) {
        // position
        token_to_position_map_[atoi(tokens[1])] = atoi(tokens[3]);
      }
    }

    symbol_mapping_fs.close();
  }

  bool ValidateMarginValues(double expo_margin, double span_margin) {
    if (expo_margin <= 0 || span_margin <= 0) return false;
    return true;
  }

  void LoadMarginValues() {
    std::ifstream margin_file_fs;
    margin_file_fs.open(NSE_MARGIN_FILE, std::ifstream::in);

    if (!margin_file_fs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO OPEN MARGIN FILE : " << NSE_MARGIN_FILE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    char buffer[4096];

    while (margin_file_fs.good()) {
      memset((void *)buffer, 0, 4096);
      margin_file_fs.getline(buffer, 4096);

      HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
      const std::vector<const char *> &tokens = st.GetTokens();

      if (std::string(buffer).find("#") != std::string::npos) continue;
      if (tokens.size() != 3) {
        continue;
      }

      MarginValues margin_values;
      margin_values.exposure_margin = atof(tokens[1]);
      margin_values.span_margin = atof(tokens[2]);

      if (true == ValidateMarginValues(margin_values.exposure_margin, margin_values.span_margin)) {
        if (internal_symbol_to_marginvalues_map_.find(tokens[0]) != internal_symbol_to_marginvalues_map_.end()) {
          DBGLOG_CLASS_FUNC_LINE_INFO << "MULTIPLE MARGIN ENTRIES FOR THE SAME INSTRUMENT : " << tokens[0]
                                      << " CURRENT ENTRY, EXPOSURE MARGIN : "
                                      << internal_symbol_to_marginvalues_map_[tokens[0]].exposure_margin
                                      << " SPAN MARGIN : "
                                      << internal_symbol_to_marginvalues_map_[tokens[0]].span_margin
                                      << " WILL BE REPLACED WITH NEW ENTRIES : " << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

        internal_symbol_to_marginvalues_map_[tokens[0]] = margin_values;
      }
    }

    margin_file_fs.close();
  }

  void LoadOrderConstraintsConfig() {
    std::ifstream order_constr_fs;
    order_constr_fs.open(NSE_INDIVIDUAL_ORDER_BASED_CHECK_FILE, std::ifstream::in);

    if (!order_constr_fs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO OPEN ORDER BASED CONSTRAINTS FILE : "
                                   << NSE_INDIVIDUAL_ORDER_BASED_CHECK_FILE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    char buffer[8096];
    memset((void *)buffer, 0, 8096);

    while (order_constr_fs.good()) {
      order_constr_fs.getline(buffer, 8096);

      HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
      const std::vector<const char *> &tokens = st.GetTokens();

      if (std::string(buffer).find("#") != std::string::npos) continue;
      if (tokens.size() != 2) {
        continue;
      }

      common_order_level_checks_.order_qty_check = atoi(tokens[0]);
      common_order_level_checks_.order_value_check = atof(tokens[1]);

      if (false == VerifySanityOfTheOrderValue(common_order_level_checks_)) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO ADD ORDER CONSTRAINTS, AS SANITY CHECK FAILED FOR GIVEN PARAMS "
                                     << DBGLOG_ENDL_NOFLUSH;
        exit(-1);
      }
    }

    order_constr_fs.close();
  }

  void LoadSecuritiesUnderBan() {
    mwpl_check_.clear();

    std::ifstream security_under_ban_stream;
    security_under_ban_stream.open(NSE_INDIVIDUAL_BANNED_PRODUCTS_FILE, std::ifstream::in);

    if (!security_under_ban_stream.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL
          << "BANNED PRODUCTS FILE DOESN'T EXIST, PROVIDE EMPTY FILE IF NO PRODUCTS ARE UNDER BAN"
          << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    char buffer[1024];

    while (security_under_ban_stream.good()) {
      memset((void *)buffer, 0, 1024);
      security_under_ban_stream.getline(buffer, 1024);
      if (strlen(buffer) <= 0) continue;
      if (std::string(buffer).find("#") != std::string::npos) continue;

      mwpl_check_.insert(buffer);
    }

    security_under_ban_stream.close();
  }

  void LoadConfigValues() {
    std::ifstream config_values_fs;
    config_values_fs.open(NSE_CONFIG_VALUES_FILE, std::ifstream::in);

    if (!config_values_fs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO OPEN ORDER BASED CONSTRAINTS FILE : " << NSE_CONFIG_VALUES_FILE
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    char buffer[8096];
    memset((void *)buffer, 0, 8096);

    while (config_values_fs.good()) {
      config_values_fs.getline(buffer, 8096);

      HFSAT::PerishableStringTokenizer st(buffer, sizeof(buffer));
      const std::vector<const char *> &tokens = st.GetTokens();

      if (std::string(buffer).find("#") != std::string::npos) continue;
      if (tokens.size() != 4) {
        continue;
      }

      cumulative_open_order_value_limit_ = std::max(0, std::min(MAX_ORDER_VALUE, atoi(tokens[0])));
      overall_turnover_limit_trading_limit_check_ = std::max(0, std::min(MAX_ORDER_VALUE, atoi(tokens[1])));
      approved_exposure_margin_limit_ = atof(tokens[2]);
      approved_overall_margin_limit_ = atof(tokens[3]);
    }

    config_values_fs.close();

    if (cumulative_open_order_value_limit_ <= 0 || cumulative_open_order_value_limit_ >= MAX_ORDER_VALUE ||
        overall_turnover_limit_trading_limit_check_ <= 0 ||
        overall_turnover_limit_trading_limit_check_ >= MAX_ORDER_VALUE || approved_exposure_margin_limit_ <= 0 ||
        approved_overall_margin_limit_ <= 0) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED TO LOAD/VALIDATE CONFIG VALUES, CUMULATIVE OPEN ORDER VALUE : "
                                   << cumulative_open_order_value_limit_
                                   << " TURNOVER VALUE : " << overall_turnover_limit_trading_limit_check_
                                   << " EXPO MARGIN : " << approved_exposure_margin_limit_
                                   << " OVERALL MARGIN : " << approved_overall_margin_limit_ << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "INITIALIZED TRADING LIMIT AS : " << overall_turnover_limit_trading_limit_check_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (overall_branchwise_turnover_trading_limit_check_ < overall_turnover_limit_trading_limit_check_) {
      DBGLOG_CLASS_FUNC_LINE_FATAL
          << "TRADING LIMIT ALLOWED CAN'T BE HIGHER THAN BRANCHWISE TRADING LIMIT, CURRENT TRADING LIMIT : "
          << overall_turnover_limit_trading_limit_check_
          << " BRANCHWISE TRADING LIMIT : " << overall_branchwise_turnover_trading_limit_check_ << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }
  }

  NSEConfigBasedChecks(HFSAT::DebugLogger &dbglogger, HFSAT::ORS::Settings const &settings)
      : dbglogger_(dbglogger),
        limits_mutex(),
        nse_ref_data_loader_(
            HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
        nse_ref_data(nse_ref_data_loader_.GetNSERefData()),
        cumulative_open_order_value_limit_(0),
        overall_turnover_limit_trading_limit_check_(0),
        overall_turnover_limit_trading_value_(0),
        overall_branchwise_turnover_trading_limit_check_(0.0),
        approved_exposure_margin_limit_(0.0),
        approved_overall_margin_limit_(0.0),
        LOWER_PRICE_LIMIT(0.0),
        UPPER_PRICE_LIMIT(0.0),
        bid_side_under_ban_(false),
        ask_side_under_ban_(false)

  {
    overall_branchwise_turnover_trading_limit_check_ = atof(settings.getValue("BranchWiseTradingLimit").c_str());
    if (overall_branchwise_turnover_trading_limit_check_ >= MAX_ORDER_VALUE) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "CAN'T SET BRNCHWISE LIMIT VALUE ABOVE -> " << MAX_ORDER_VALUE - 1
                                   << " TRADING SYSTEM WILL NOT RUN FURTHER AS REQUESTED BRANCHWISE LIMIT VALUE IS : "
                                   << overall_branchwise_turnover_trading_limit_check_ << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      exit(-1);
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "BRANCH ID : " << settings.getValue("BranchID")
                                << " LIMITS SET AS : " << overall_branchwise_turnover_trading_limit_check_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    LoadConfigValues();
    LoadTradeProtectionPriceRange();
    LoadOrderConstraintsConfig();
    LoadSecuritiesUnderBan();
    LoadMarginValues();
    LoadSymbolMapping();

    DBGLOG_CLASS_FUNC_LINE_INFO << "CUMULATIVE OPEN ORDER VALUE LIMIT SET TO : " << cumulative_open_order_value_limit_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;
  }

  NSEConfigBasedChecks(NSEConfigBasedChecks const &disabled_copy_constructor);

 public:
  bool bid_side_under_ban_;
  bool ask_side_under_ban_;

  static NSEConfigBasedChecks &GetUniqueInstance(HFSAT::DebugLogger &dbglogger, HFSAT::ORS::Settings const &settings) {
    static NSEConfigBasedChecks unique_instance(dbglogger, settings);
    return unique_instance;
  }

  void UpdataMidPriceAndStatus(int32_t token, double mid_price, int16_t status) {
    limits_mutex.LockMutex();

    security_to_mid_price_[token] = mid_price;
    security_to_market_status_[token] = status;

    limits_mutex.UnlockMutex();
  }

  void UpdateTradePriceAndStatus(int32_t token, double trade_price, int16_t status) {
    limits_mutex.LockMutex();

    security_to_last_traded_price_[token] = trade_price;
    security_to_market_status_[token] = status;

    limits_mutex.UnlockMutex();
  }

  NSE_UDP_MDS::NSERefData GetNseRefDataForGivenToken(int32_t token, bool &data_valid) {
    NSE_UDP_MDS::NSERefData dummy;
    memset((void *)&dummy, 0, sizeof(NSE_UDP_MDS::NSERefData));

    if (nse_ref_data.find(token) == nse_ref_data.end()) {
      data_valid = false;
      return dummy;
    }

    data_valid = true;
    return nse_ref_data[token];
  }

  int32_t GetTokenFromInternalSymbol(char const *internal_symbol) {
    int32_t token = -1;

    limits_mutex.LockMutex();

    if (internal_symbol_token_map_.find(internal_symbol) == internal_symbol_token_map_.end()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "NO TOKEN FOUND FOR SYMBOL : " << internal_symbol << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

    } else {
      token = internal_symbol_token_map_[internal_symbol];
    }

    limits_mutex.UnlockMutex();

    return token;
  }

  bool isProductAnOption(int32_t token) {
    bool is_option = false;

    limits_mutex.LockMutex();

    if (options_.find(token) == options_.end()) {
      is_option = false;

    } else {
      is_option = true;
    }

    limits_mutex.UnlockMutex();

    return is_option;
  }

  char const *GetInternalSymbolFromToken(int32_t token) {
    std::string internal_symbol = "INVALID";

    limits_mutex.LockMutex();

    if (token_to_internal_symbol_map_.find(token) == token_to_internal_symbol_map_.end()) {
      //          DBGLOG_CLASS_FUNC_LINE_ERROR << "NO SYMBOL FOUND FOR TOKEN : "<< token << DBGLOG_ENDL_NOFLUSH ;
    } else {
      internal_symbol = token_to_internal_symbol_map_[token];
    }

    limits_mutex.UnlockMutex();

    return internal_symbol.c_str();
  }

  bool isSecurityAllowedToTrade(const char *internal_symbol) {
    bool allowed_to_trade = false;

    limits_mutex.LockMutex();

    if (mwpl_check_.find(internal_symbol) == mwpl_check_.end()) allowed_to_trade = true;

    limits_mutex.UnlockMutex();

    return allowed_to_trade;
  }

  void UpdateSecurityForMWPLAndStatus(const char *internal_symbol, int32_t token, int16_t status) {
    limits_mutex.LockMutex();
    security_to_market_status_[token] = status;
    mwpl_check_.insert(internal_symbol);
    limits_mutex.UnlockMutex();
  }

  bool isOrderLevelChecksCleared(const char *internal_symbol, int64_t order_qty, double order_price) {
    bool order_value_checks_cleared = true;

    limits_mutex.LockMutex();

    if (order_qty <= 0) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL ORDER LEVEL CHECKS FAILED FOR : " << internal_symbol
                                   << " INVALID ORDER QTY : " << order_qty << DBGLOG_ENDL_NOFLUSH;
      order_value_checks_cleared = false;
    }

    if (order_price <= 0) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL ORDER LEVEL CHECKS FAILED FOR : " << internal_symbol
                                   << " INVALID ORDER PRICE : " << order_price << DBGLOG_ENDL_NOFLUSH;
      order_value_checks_cleared = false;
    }

    if (order_qty > common_order_level_checks_.order_qty_check) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL ORDER LEVEL CHECKS FAILED FOR : " << internal_symbol
                                   << " REQUESTED QTY : " << order_qty
                                   << " EXCEEDS GIVEN LIMIT : " << common_order_level_checks_.order_qty_check
                                   << DBGLOG_ENDL_NOFLUSH;
      order_value_checks_cleared = false;
    }

    if ((order_qty * order_price) > common_order_level_checks_.order_value_check) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL ORDER LEVEL CHECKS FAILED FOR : " << internal_symbol
                                   << " ORDER VALUE : " << (order_qty * order_price) << " WITH QTY : " << order_qty
                                   << " AND PRICE : " << order_price
                                   << " EXCEEDS ORDER VALUE LIMIT : " << common_order_level_checks_.order_value_check
                                   << DBGLOG_ENDL_NOFLUSH;
      order_value_checks_cleared = false;
    }

    limits_mutex.UnlockMutex();
    return order_value_checks_cleared;
  }

  bool isPositionLevelCheckCleared(int32_t token, int32_t current_position, int32_t requested_qty) {
    limits_mutex.LockMutex();

    if (token_to_position_map_.find(token) == token_to_position_map_.end()) {
      limits_mutex.UnlockMutex();
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL POSITION LEVEL CHECKS FAILED FOR : " << token
                                   << " SECURITY DOESN'T HAVE ANY CHECKS DEFINED" << DBGLOG_ENDL_NOFLUSH;
      return false;
    }

    if ((int32_t)(abs(current_position) + requested_qty) > token_to_position_map_[token]) {
      limits_mutex.UnlockMutex();
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL POSITION LEVEL CHECKS FAILED FOR : " << token
                                   << " CURRENT POSITION : " << current_position << " REQUESTED QTY : " << requested_qty
                                   << " EXCEEDS LIMIT : " << token_to_position_map_[token] << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if ((int32_t)(abs(current_position) + requested_qty) > MAX_COMBINED_POSITION) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "FAILED COMBINED POSITION LIMIT CHECK FOR GIVEN REQUEST : " << requested_qty
                                     << " EXCEEDS LIMIT : " << MAX_COMBINED_POSITION << DBGLOG_ENDL_FLUSH;
      }

      return false;
    }

    limits_mutex.UnlockMutex();

    return true;
  }

  bool isPriceChecksCleared(int32_t token, double order_price) {
    limits_mutex.LockMutex();

    if (internal_symbol_to_individual_price_checks_map_.find(token) ==
        internal_symbol_to_individual_price_checks_map_.end()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "PRICE CHECKS NOT ADDED FOR TOKEN : " << token
                                   << " AT ORDER PRICE : " << order_price << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      limits_mutex.UnlockMutex();
      return false;
    }

    if (order_price < internal_symbol_to_individual_price_checks_map_[token].price_lower_limit ||
        order_price > internal_symbol_to_individual_price_checks_map_[token].price_higher_limit) {
      limits_mutex.UnlockMutex();
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INDIVIDUAL ORDER LEVEL CHECKS FAILED FOR : " << token
                                   << " ORDER PRICE : " << order_price << " FALLS BEYOND PRICE BAND : "
                                   << internal_symbol_to_individual_price_checks_map_[token].price_lower_limit
                                   << " <-> "
                                   << internal_symbol_to_individual_price_checks_map_[token].price_higher_limit
                                   << DBGLOG_ENDL_NOFLUSH;
      return false;
    }

    limits_mutex.UnlockMutex();
    return true;
  }

  void UpdateSymbolMapping(char const *internal_symbol, int32_t token, char const *options_flag, int32_t position) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "UPDATE SYMBOL : " << internal_symbol << " TOKEN : " << token
                                << " OPTIONS_FLAG : " << options_flag << " POSITION : " << position
                                << DBGLOG_ENDL_NOFLUSH;

    limits_mutex.LockMutex();
    internal_symbol_token_map_[internal_symbol] = token;
    token_to_internal_symbol_map_[token] = internal_symbol;

    if (nse_ref_data.find(token) == nse_ref_data.end()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "CAN'T FIND REFERENCE DATA FOR THE GIVEN TOKEN : " << token
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

    } else {
      PriceBandChecks price_band_checks;
      price_band_checks.price_lower_limit = nse_ref_data[token].lower_price_range;
      price_band_checks.price_higher_limit = nse_ref_data[token].upper_price_range;
      internal_symbol_to_individual_price_checks_map_[token] = price_band_checks;

      if (std::string(options_flag) == std::string("OPT")) {
        options_.insert(token);
      }

      if (true == VerifySanityOfThePositionLimit(token, position)) {
        token_to_position_map_[token] = position;
      }
    }

    limits_mutex.UnlockMutex();
  }

  void ReloadAllConfigBasedChecksFiles() {
    limits_mutex.LockMutex();
    LoadTradeProtectionPriceRange();
    LoadConfigValues();
    LoadMarginValues();
    LoadOrderConstraintsConfig();
    LoadSecuritiesUnderBan();
    LoadSymbolMapping();
    limits_mutex.UnlockMutex();
  }

  void OnOrderExecutionUpdateTurnOverValue(double trade_price, int32_t order_qty) {
    limits_mutex.LockMutex();
    overall_turnover_limit_trading_value_ += (trade_price * order_qty);
    limits_mutex.UnlockMutex();
  }

  bool isTradingLimitAvailable(double order_value) {
    limits_mutex.LockMutex();

    if ((overall_turnover_limit_trading_value_ + order_value) > overall_turnover_limit_trading_limit_check_) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "REQUESTED ORDER VALUE EXCEEDS TRADING LIMIT, CAN'T ALLOW ORDER WITH VALUE : "
                                   << order_value
                                   << " CURRENT TURNOVER VALUE : " << overall_turnover_limit_trading_value_
                                   << " ALLOWED TRADING LIMIT : " << overall_turnover_limit_trading_limit_check_
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if ((overall_turnover_limit_trading_value_ + order_value) > overall_branchwise_turnover_trading_limit_check_) {
        DBGLOG_CLASS_FUNC_LINE_ERROR
            << "REQUESTED ORDER VALUE ALSO EXCEEDS BRANCHWISE LIMIT, ORDER WILL BE REJECTED AS REQUESTED ORDER VALUE : "
            << order_value << " WITH CURRENT TURNOVER VALUE : " << overall_turnover_limit_trading_value_
            << " EXCEEDS BRANCH LIMIT : " << overall_branchwise_turnover_trading_limit_check_ << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      }

      limits_mutex.UnlockMutex();
      return false;
    }

    limits_mutex.UnlockMutex();
    return true;
  }

  bool isCumulativeOpenOrderValueCheckClean(double current_cumulative_open_orders_value) {
    if (current_cumulative_open_orders_value >= cumulative_open_order_value_limit_) return false;
    return true;
  }

  void DumpAllNSEChecks() {
    limits_mutex.LockMutex();

    std::map<int32_t, PriceBandChecks> internal_symbol_to_individual_price_checks_map_temp_;
    for (auto &itr : internal_symbol_to_individual_price_checks_map_) {
      internal_symbol_to_individual_price_checks_map_temp_[itr.first] = itr.second;
    }

    std::map<std::string, MarginValues> internal_symbol_to_marginvalues_map_temp_;
    for (auto &itr : internal_symbol_to_marginvalues_map_) {
      internal_symbol_to_marginvalues_map_temp_[itr.first] = itr.second;
    }

    std::set<std::string> mwpl_check_temp_;
    for (auto &itr : mwpl_check_) {
      mwpl_check_temp_.insert(itr);
    }

    std::map<int32_t, int32_t> token_to_position_map_temp_;
    for (auto &itr : token_to_position_map_) {
      token_to_position_map_temp_[itr.first] = itr.second;
    }

    std::map<int32_t, double> security_to_last_traded_price_temp_;
    std::map<int32_t, int16_t> security_to_market_status_temp_;
    std::map<int32_t, double> security_to_mid_price_temp_;

    for (auto &itr : security_to_mid_price_) {
      security_to_mid_price_temp_[itr.first] = itr.second;
    }

    for (auto &itr : security_to_market_status_) {
      security_to_market_status_temp_[itr.first] = itr.second;
    }

    for (auto &itr : security_to_last_traded_price_) {
      security_to_last_traded_price_temp_[itr.first] = itr.second;
    }

    limits_mutex.UnlockMutex();

    dbglogger_ << "==================================== NSE CHECKS ================================\n\n";

    dbglogger_ << "=> PRICE_RANGE_CHECKS : \n";

    for (auto &itr : internal_symbol_to_individual_price_checks_map_temp_) {
      NSE_UDP_MDS::NSERefData nrd = nse_ref_data[itr.first];
      dbglogger_ << "TOKEN : " << itr.first << " EXCHANGE SYMBOL : " << nrd.exchange_symbol
                 << " PRICE BAND : " << (itr.second).price_lower_limit << " <=> " << (itr.second).price_higher_limit
                 << "\n";
    }

    DBGLOG_DUMP;

    dbglogger_ << "=> ORDER_QTY_AND_VALUE_CHECKS : ORDER QTY CHECK : " << common_order_level_checks_.order_qty_check
               << " ORDER VALUE : " << common_order_level_checks_.order_value_check << "\n";
    DBGLOG_DUMP;

    dbglogger_ << "=> CUMULATIVE_OPEN_ORDER_VALUE_CHECK -> " << cumulative_open_order_value_limit_ << "\n";
    DBGLOG_DUMP;

    dbglogger_ << "=> TOTAL_AVAILABLE_EXPOSURE_MARGIN -> " << approved_exposure_margin_limit_
               << " TOTAL_AVAILABLE_NET_MARGIN -> " << approved_overall_margin_limit_ << "\n";
    dbglogger_ << "=> MARGIN_CONFIGURATION : \n";

    for (auto &itr : internal_symbol_to_marginvalues_map_temp_) {
      NSE_UDP_MDS::NSERefData nrd = nse_ref_data[GetTokenFromInternalSymbol((itr.first).c_str())];
      dbglogger_ << "INTERNAL SYMBOL : " << itr.first << " TOKEN : " << GetTokenFromInternalSymbol((itr.first).c_str())
                 << " EXCHSYMBOL : " << nrd.exchange_symbol << " EXPOSURE MARGIN : " << (itr.second).exposure_margin
                 << " SPAN MARGIN : " << (itr.second).span_margin << "\n";
    }

    DBGLOG_DUMP;

    dbglogger_ << "=> MWPL BANNED SECURITY LIST : \n";

    for (auto &itr : mwpl_check_temp_) {
      dbglogger_ << "UNDERLYING BANNED FROM TRADING : " << itr << "\n";
    }

    DBGLOG_DUMP;

    dbglogger_ << "=> POSITION LIMIT CHECK : \n";

    for (auto &itr : token_to_position_map_temp_) {
      NSE_UDP_MDS::NSERefData nrd = nse_ref_data[itr.first];
      dbglogger_ << "TOKEN : " << itr.first << " EXCHSYMBOL : " << nrd.exchange_symbol
                 << " INTERNAL SYMBOL : " << GetInternalSymbolFromToken(itr.first)
                 << " POSITION ALLOWED : " << itr.second << "\n";
    }

    DBGLOG_DUMP;

    dbglogger_ << "=> TRADING LIMIT CHECK_NET_TURNOVER_LIMIT -> " << overall_turnover_limit_trading_limit_check_
               << " CURRENT_NET_TURNOVER_VALUE -> " << overall_turnover_limit_trading_value_ << "\n";
    DBGLOG_DUMP;

    dbglogger_ << "=> PRICE PROTECTION LIMIT VALUES : \n";

    for (auto &itr : security_to_mid_price_temp_) {
      if (security_to_mid_price_temp_.find(itr.first) != security_to_mid_price_temp_.end()) {
        NSE_UDP_MDS::NSERefData nrd = nse_ref_data[itr.first];

        if (std::string("INVALID") == std::string(GetInternalSymbolFromToken(itr.first))) continue;

        dbglogger_ << " TOKEN : " << itr.first << " EXCHSYMBOL : " << nrd.exchange_symbol
                   << " INTERNAL SYMBOL : " << GetInternalSymbolFromToken(itr.first)
                   << " PRICE_PROTECTION_LOWER_RANGE : " << security_to_mid_price_temp_[itr.first] * LOWER_PRICE_LIMIT
                   << " PRICE_PROTECTION_UPPER_RANGE : " << security_to_mid_price_temp_[itr.first] * UPPER_PRICE_LIMIT
                   << " MARKET STATUS : " << security_to_market_status_temp_[itr.first];
      }

      if (security_to_last_traded_price_temp_.find(itr.first) != security_to_last_traded_price_temp_.end()) {
        if (std::string("INVALID") == std::string(GetInternalSymbolFromToken(itr.first))) continue;

        dbglogger_ << " LAST_TRADE_PRICE : " << security_to_last_traded_price_temp_[itr.first];
      }
    }

    dbglogger_ << "\n";
    DBGLOG_DUMP;
  }

  bool isTradePriceProtectionCheckCleared(int32_t token, double order_price) {
    bool trade_protection_price_check = true;
    std::string internal_symbol = GetInternalSymbolFromToken(token);

    if (std::string("INVALID") == internal_symbol) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "NO INTERNAL SYMBOL TO TOKEN MAPPING FOUND : " << token << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      return false;
    }

    limits_mutex.LockMutex();

    if (security_to_mid_price_.find(token) == security_to_mid_price_.end()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "TOKEN : " << token << " INTERNAL SYMBOL : " << internal_symbol
                                   << " DOESN't HAVE MKT DATA" << DBGLOG_ENDL_NOFLUSH;
      trade_protection_price_check = false;

    } else {
      if (order_price < LOWER_PRICE_LIMIT * security_to_mid_price_[token] ||
          order_price > UPPER_PRICE_LIMIT * security_to_mid_price_[token]) {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "TOKEN : " << token << " INTERNAL SYMBOL : " << internal_symbol
                                     << " ORDER PRICE : " << order_price
                                     << " EITHER BELOW LOWER TRADE PROTECTION PRICE : "
                                     << LOWER_PRICE_LIMIT * security_to_mid_price_[token]
                                     << " OR ABOVE HIGHER RANGE : " << UPPER_PRICE_LIMIT * security_to_mid_price_[token]
                                     << DBGLOG_ENDL_NOFLUSH;
        trade_protection_price_check = false;
      }
    }

    limits_mutex.UnlockMutex();

    return trade_protection_price_check;
  }

  double GetLastMidPrice(int32_t token) {
    double last_price = 0.0;

    limits_mutex.LockMutex();

    if (security_to_mid_price_.find(token) != security_to_mid_price_.end()) {
      last_price = security_to_mid_price_[token];
    }

    limits_mutex.UnlockMutex();

    return last_price;
  }

  bool isExposureMarginCheckCleared(double expo_margin) {
    if (approved_exposure_margin_limit_ > expo_margin) return true;
    return false;
  }

  bool isNetMarginCheckCleared(double net_margin) {
    if (approved_overall_margin_limit_ > net_margin) return true;
    return false;
  }

  MarginValues GetMarginFromInternalSymbol(char const *symbol, bool &is_valid) {
    is_valid = true;
    MarginValues margin_values;

    limits_mutex.LockMutex();

    if (internal_symbol_to_marginvalues_map_.find(symbol) == internal_symbol_to_marginvalues_map_.end()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "MARGIN VALUES DON't EXIST FOR : " << symbol << "\n";
      is_valid = false;

    } else {
      is_valid = true;
      margin_values = internal_symbol_to_marginvalues_map_[symbol];
    }

    limits_mutex.UnlockMutex();

    return margin_values;
  }

  //      bool isBookTypeAllowed ( int16_t const & book_type_entered ) {
  //
  //        return ( book_type_ == book_type_entered ) ;
  //
  //
  //      }

  //      bool isOrderQtyAllowed ( int32_t const & token, uint32_t const & order_qty ) {
  //
  //        if ( order_qty <= 0 ) return false ;
  //
  //        return ! ( order_qty % ( nse_ref_data [ token ].lot_size ) ) ;
  //
  //      }
  //
  //      bool isPriceAllowed ( int32_t const & token, double const & order_price ) {
  //
  //        if ( order_price <= 0.0 ) return false ;
  //
  //        return ! ( order_price % ( nse_ref_data [ token ].min_price_increment ) ) ;
  //
  //      }
};
}
}
