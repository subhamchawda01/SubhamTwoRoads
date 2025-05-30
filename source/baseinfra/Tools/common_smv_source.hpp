/**
   \file Tools/common_smv_source.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
 */
#pragma once

#define USE_UNCONVERTED_DATA_DATE 20240805
#define BSE_USE_UNCONVERTED_DATA_DATE 20241121

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/signals.hpp"

#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/OrderRouting/shortcode_prom_order_manager_map.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "baseinfra/BaseUtils/curve_utils.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"
#include "baseinfra/MarketAdapter/market_orders_view.hpp"

#include "baseinfra/SimMarketMaker/sim_config.hpp"
#include "baseinfra/LoggedSources/filesource_list.hpp"

#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

class CommonSMVSource {
  std::vector<std::string> source_shortcode_vec_;
  std::vector<std::string> ors_shortcode_vec_;
  std::vector<std::string> dep_shortcode_vec_;
  std::string dep_shortcode_;
  int tradingdate_;
  HFSAT::DebugLogger dbglogger_;
  HFSAT::Watch watch_;
  std::string simconfig_filename_;
  std::vector<const char *> exchange_symbol_vec_;
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_;
  std::vector<HFSAT::MarketOrdersView *> &sid_to_mov_ptr_map;
  std::vector<HFSAT::SecurityMarketView *> sid_to_sim_smv_ptr_map_;
  HFSAT::SimTimeSeriesInfo *sim_time_series_info_;  // to be deleted
  HFSAT::NetworkAccountInfoManager *network_account_info_manager_;
  HFSAT::TradingLocation_t dep_trading_location_;
  std::string network_account_info_filename_;
  std::string log_file_name_;
  std::string offline_mix_mms_filename_;
  std::string online_mix_price_consts_filename_;
  std::string online_beta_kalman_consts_filename_;
  std::vector<bool> sid_to_marketdata_needed_map_;  ///< security_id to bool map indicating whether market data is
  std::vector<HFSAT::ExchSource_t> sid_to_exch_source_map_;
  std::vector<bool> sid_to_ors_needed_map_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  HFSAT::PromOrderManagerPtrVec &sid_to_prom_order_manager_map_;
  std::vector<bool> sid_is_dependant_map_;
  ///< if using_non_self_market_view_ then ORS messages are listened to by SMM, PROMOM ...
  bool using_non_self_market_view_;
  bool use_fake_faster_data_;
  bool using_ose_ol_hist_data_;
  int datagen_start_utc_hhmm_;
  int datagen_end_utc_hhmm_;
  int global_datagen_start_utc_yymmdd_;
  int global_datagen_end_utc_yymmdd_;

  unsigned int num_sec_id_;

  bool USE_EOBI_PF_FR2;

  // book type
  bool isNTP;
  bool isNTPORD;
  bool isBMFEq;
  bool isPL;
  bool isOMD;
  bool isOMD_PF;
  bool isOMD_CPF;
  bool isFPGA;
  bool isCHIXORD;
  bool isCMEOBF;
  bool isRTSOF;

  // live or sim
  bool sim_smv_required_;
  bool ignore_user_msg_;
  bool livetrading_;
  HFSAT::HistoricalDispatcher historical_dispatcher_;
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  bool is_l1_mode_;
  // book building modification purpose
  bool is_TMX_OF_;
  bool using_OSE_OF_book_;
  bool using_MICEX_OF_book_;

  // more items that needs to be destroyed at the end
  // books
  // modify this
  std::unique_ptr<HFSAT::IndexedEobiMarketViewManager> indexed_eobi_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedEobiPriceLevelMarketViewManager> indexed_eobi_price_level_market_view_manager_ =
      nullptr;
  std::unique_ptr<HFSAT::IndexedFpgaMarketViewManager> indexed_fpga_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedLiffePriceLevelMarketViewManager> indexed_liffe_price_level_market_view_manager_ =
      nullptr;
  std::unique_ptr<HFSAT::IndexedRtsMarketViewManager> indexed_rts_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedRtsOFMarketViewManager> indexed_rts_of_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedNtpMarketViewManager> indexed_ntp_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedBMFFpgaMarketViewManager> indexed_bmf_fpga_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedNtpMarketViewManager> indexed_puma_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedCmeMarketViewManager> indexed_cme_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedMicexMarketViewManager> indexed_micex_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedMicexOFMarketViewManager> indexed_micex_of_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedOsePriceFeedMarketViewManager> indexed_ose_price_feed_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedOseOrderFeedMarketViewManager> indexed_ose_order_feed_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::OSEL1PriceMarketViewManager> ose_l1_price_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedCfeMarketViewManager> indexed_cfe_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::HKEXIndexedMarketViewManager> hkex_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedTmxMarketViewManager> indexed_tmx_market_view_manager_ = nullptr;
  // book building modification purpose
  std::unique_ptr<HFSAT::IndexedTMXOBFOFMarketViewManager> indexed_tmx_obf_of_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedIceMarketViewManager> indexed_ice_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedAsxMarketViewManager> indexed_asx_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedHKOMDPriceLevelMarketViewManager> hkomd_price_level_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::BMFOrderLevelMarketViewManager> bmf_order_level_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::HKOMDIndexedMarketViewManager> indexed_hkomd_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedNSEMarketViewManager2> indexed_nse_market_view_manager2_ = nullptr;
  std::unique_ptr<HFSAT::IndexedBSEMarketViewManager2> indexed_bse_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::L1PriceMarketViewManager> l1_price_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::GenericL1DataMarketViewManager> generic_l1_data_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedTmxMarketViewManager> indexed_krx_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedLiffePriceLevelMarketViewManager> indexed_liffe_sim_price_level_market_view_manager_ =
      nullptr;
  std::unique_ptr<HFSAT::IndexedRtsMarketViewManager> indexed_rts_sim_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::HKEXIndexedMarketViewManager> hkex_sim_market_view_manager_ = nullptr;
  std::unique_ptr<HFSAT::IndexedTmxMarketViewManager> indexed_tmx_sim_market_view_manager_ = nullptr;

  // book building modifications purpose
  std::unique_ptr<HFSAT::IndexedTMXOBFOFMarketViewManager> indexed_tmx_obf_of_sim_market_view_manager_ = nullptr;

  std::unique_ptr<HFSAT::IndexedIceMarketViewManager> indexed_ice_sim_market_view_manager_ = nullptr;

  std::unique_ptr<HFSAT::CMEMarketOrderManager> tmx_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::AsxMarketOrderManager> asx_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::CMEMarketOrderManager> cme_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::EOBIMarketOrderManager> eobi_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::IceMarketOrderManager> ice_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::NSEMarketOrderManager> nse_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::AsxMarketOrderManager> ose_market_order_manager_ = nullptr;
  std::unique_ptr<HFSAT::SGXMarketOrderManager> sgx_market_order_manager_ = nullptr;

  // One exchange can have multiple shortcodes added. So we need to have a map from shortcode -> filesource
  // There is one filesource for each shortcode [ MarketViewMangers are one for each exchange. ]
  // Modify this
  std::map<std::string, unique_ptr<HFSAT::CMELoggedMessageFileSource> > shortcode_cme_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::EUREXLoggedMessageFileSource> > shortcode_eurex_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::EOBIPriceFeedLoggedMessageFileSource> > shortcode_eobi_price_feed_source_map_;
  std::map<std::string, unique_ptr<HFSAT::FPGALoggedMessageFileSource> > shortcode_fpga_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::NTPLoggedMessageFileSource> > shortcode_ntp_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::BMFFPGALoggedMessageFileSource> > shortcode_bmf_fpga_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::PUMALoggedMessageFileSource> > shortcode_puma_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::LIFFELoggedMessageFileSource> > shortcode_liffe_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::ICELoggedMessageFileSource> > shortcode_ice_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::TMXLoggedMessageFileSource> > shortcode_tmx_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::TMXPFLoggedMessageFileSource> > shortcode_tmx_pf_data_filesource_map_;
  // book building modification purpose

  std::map<std::string, unique_ptr<HFSAT::RTSLoggedMessageFileSource> > shortcode_rts_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::MICEXLoggedMessageFileSource> > shortcode_micex_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::MICEXOFLoggedMessageFileSource> > shortcode_micex_of_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::HKEXLoggedMessageFileSource> > shortcode_hkex_data_filesource_map_;

  std::map<std::string, unique_ptr<HFSAT::HKOMDPFLoggedMessageFileSource> > shortcode_hkomd_pricefeed_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::OSEPriceFeedLoggedMessageFileSource> > shortcode_ose_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::OSEL1LoggedMessageFileSource> > shortcode_ose_l1_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::CFELoggedMessageFileSource> > shortcode_cfe_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::ASXPFLoggedMessageFileSource> > shortcode_asx_pf_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::NSELoggedMessageFileSource> > shortcode_nse_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::NSELoggedMessageFileSource2> > shortcode_nse_data_filesource_map2_;
  std::map<std::string, unique_ptr<HFSAT::BSELoggedMessageFileSource> > shortcode_bse_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::BSELoggedMessageFileSource2> > shortcode_bse_data_filesource_map2_;
  std::map<std::string, unique_ptr<HFSAT::SGXPFLoggedMessageFileSource> > shortcode_sgx_pf_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::BSEPFLoggedMessageFileSource> > shortcode_bse_pf_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::CHIXL1LoggedMessageFileSource> > shortcode_chix_l1_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::NSEL1LoggedMessageFileSource> > shortcode_nse_l1_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::OSEPFLoggedMessageFileSource> > shortcode_ose_pf_data_filesource_map_;

  std::map<std::string, unique_ptr<HFSAT::KRXLoggedMessageFileSource> > shortcode_krx_data_filesource_map_;
  std::unique_ptr<HFSAT::BMFLoggedMessageFileSource> bmf_logged_message_file_source_;
  std::map<std::string, HFSAT::ExternalDataListener *> shortcode_filesource_map_;

  // Order Feed data
  std::map<std::string, unique_ptr<HFSAT::CMEOBFLoggedMessageFileSource> > shortcode_cme_obf_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::EOBILoggedMessageFileSource> > shortcode_eobi_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::TMXOFLoggedMessageFileSource> > shortcode_tmx_of_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::RTSOrderFeedLoggedMessageFileSource> > shortcode_rts_of_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::HKOMDLoggedMessageFileSource> > shortcode_hkomd_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::OSEItchLoggedMessageFileSource> > shortcode_ose_itch_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::CMEOBFLoggedMessageFileSource> > shortcode_cme_of_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::SGXOrderLoggedMesageFileSource> > shortcode_sgx_of_data_filesource_map_;
  std::map<std::string, unique_ptr<HFSAT::ASXItchLoggedMessageFileSource> > shortcode_asx_itch_data_filesource_map_;

 public:
  CommonSMVSource(std::vector<std::string> source_shortcode_vec, int tradingdate_, bool live_trading_ = false)
      : source_shortcode_vec_(source_shortcode_vec),
        ors_shortcode_vec_(),
        dep_shortcode_vec_(),
        dep_shortcode_("NULL"),
        tradingdate_(tradingdate_),
        dbglogger_(1024000, 1),
        watch_(dbglogger_, tradingdate_),
        simconfig_filename_(),
        sid_to_smv_ptr_map_(HFSAT::sid_to_security_market_view_map()),
        sid_to_mov_ptr_map(HFSAT::sid_to_market_orders_view_map()),
        sid_to_sim_smv_ptr_map_(),
        sim_time_series_info_(),
        network_account_info_manager_(),
        dep_trading_location_(HFSAT::kTLocCHI),
        network_account_info_filename_(HFSAT::FileUtils::AppendHome(
            std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt")),
        log_file_name_("/spare/local/logs/alllogs/temp_log.log"),
        offline_mix_mms_filename_(DEFAULT_OFFLINEMIXMMS_FILE),
        online_mix_price_consts_filename_(DEFAULT_ONLINE_MIX_PRICE_FILE),
        online_beta_kalman_consts_filename_(DEFAULT_ONLINE_BETA_KALMAN_FILE),
        sid_to_marketdata_needed_map_(),
        sid_to_exch_source_map_(),
        sid_to_ors_needed_map_(),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        sid_to_prom_order_manager_map_(HFSAT::sid_to_prom_order_manager_map()),
        sid_is_dependant_map_(),
        using_non_self_market_view_(false),
        use_fake_faster_data_(false),
        using_ose_ol_hist_data_(false),
        datagen_start_utc_hhmm_(-1),
        datagen_end_utc_hhmm_(-1),
        global_datagen_start_utc_yymmdd_(-1),
        global_datagen_end_utc_yymmdd_(-1),
        num_sec_id_(1),
        USE_EOBI_PF_FR2(false),
        isNTP(true),
        isNTPORD(false),
        isBMFEq(false),
        isPL(false),
        isOMD(false),
        isOMD_PF(true),
        isOMD_CPF(false),
        isFPGA(false),
        isCHIXORD(false),
        isCMEOBF(false),
        isRTSOF(false),
        sim_smv_required_(false),
        ignore_user_msg_(true),
        livetrading_(live_trading_),
        is_l1_mode_(false),
        is_TMX_OF_(false),
        using_MICEX_OF_book_(false) {
    if (tradingdate_ >= DATE_OF_NEW_VERSION_OF_TMX_ORDER_BOOK) {
      is_TMX_OF_ = true;
    }

    if (tradingdate_ >= USING_OSE_OF_BOOK_FROM) {
      using_OSE_OF_book_ = true;
    }

    if (tradingdate_ >= DATE_OF_NEW_VERSION_OF_RTS_ORDER_BOOK) {
      isRTSOF = true;
    }
    if (tradingdate_ >= DATE_OF_NEW_VERSION_OF_MICEX_ORDER_BOOK) {
      using_MICEX_OF_book_ = true;
    }
  }

  void SetNSEL1Mode(bool is_l1_mode) { is_l1_mode_ = is_l1_mode; }

  void SetBookType(bool isNTP_, bool isNTPORD_, bool isBMFEq_, bool isPL_, bool isQuincy_, bool isOMD_, bool isOMD_PF_,
                   bool isOMD_CPF_, bool isFPGA_, bool _isCHIXORD, bool _isCMEOBF = false) {
    isNTP = isNTP_;
    isNTPORD = isNTPORD_;
    isBMFEq = isBMFEq_;
    isPL = isPL_;
    isOMD = isOMD_;
    isOMD_PF = isOMD_PF_;
    isOMD_CPF = isOMD_CPF_;
    isFPGA = isFPGA_;
    isCHIXORD = _isCHIXORD;
    isCMEOBF = _isCMEOBF;
  }

  // book building modification purpose
  void SetTMXBookType(bool t_is_TMX_OF_) {
    is_TMX_OF_ = t_is_TMX_OF_;
    return;
  }

  void SetOSEBookType(bool using_OSE_OF_book) {
    using_OSE_OF_book_ = using_OSE_OF_book;
    return;
  }

  void SetMICEXBookType(bool using_MICEX_OF_book) {
    using_MICEX_OF_book_ = using_MICEX_OF_book;
    return;
  }

  void SetNTPBookType(bool isNTP_, bool isNTPORD_, bool isBMFEq_) {
    isNTP = isNTP_;
    isNTPORD = isNTPORD_;
    isBMFEq = isBMFEq_;
  }

  void SetRTSOFBookType() { isRTSOF = true; }

  void SetNetworkAccountInfoFilename(std::string filename_) { network_account_info_filename_ = filename_; }

  void SetDbgloggerFileName(std::string filename_) { log_file_name_ = filename_; }

  void SetSourcesNeedingOrs(std::vector<std::string> ors_shortcode_vec) { ors_shortcode_vec_ = ors_shortcode_vec; }

  void SetSourceShortcodes(std::vector<std::string> &source_shortcode_vec) {
    source_shortcode_vec_ = source_shortcode_vec;
  }

  void SetDepShortcodeVector(std::vector<std::string> &dep_shortcode_vec) { dep_shortcode_vec_ = dep_shortcode_vec; }

  void SetFakeFasterData(bool fake_faster_data) { use_fake_faster_data_ = fake_faster_data; }

  void SetOfflineMixMMSFilename(std::string offline_mix_mms_filename) {
    offline_mix_mms_filename_ = offline_mix_mms_filename;
  }

  void SetOnlineMixPriceFilename(std::string t_online_mix_price_consts_filename_) {
    online_mix_price_consts_filename_ = t_online_mix_price_consts_filename_;
  }

  void SetOnlineBetaKalmanFileName(std::string t_online_beta_kalman_consts_filename_) {
    online_beta_kalman_consts_filename_ = t_online_beta_kalman_consts_filename_;
  }

  void SetDepShortcode(std::string dep_shortcode) { dep_shortcode_ = dep_shortcode; }

  void SetStartEndTime(int datagen_start_utc_hhmm, int datagen_end_utc_hhmm) {
    datagen_start_utc_hhmm_ = datagen_start_utc_hhmm;
    datagen_end_utc_hhmm_ = datagen_end_utc_hhmm;
  }

  void SetStartEndUTCDate(int datagen_start_utc_yymmdd, int datagen_end_utc_yymmdd) {
    global_datagen_start_utc_yymmdd_ = datagen_start_utc_yymmdd;
    global_datagen_end_utc_yymmdd_ = datagen_end_utc_yymmdd;
  }

  void SetIgnoreUserMsg(bool ignore_user_msg) { ignore_user_msg_ = ignore_user_msg; }

  void SetSimSmvRequired(bool sim_smv_required) { sim_smv_required_ = sim_smv_required; }

  void SetStrategyDesc(std::string _simconfig_filename_) { simconfig_filename_ = _simconfig_filename_; }

  // Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
  void CheckAndAddNSEDefinitions(std::vector<std::string> &t_shortcode_vec_) {
    bool is_nse_present_ = false;
    for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
      //std::cout << "for " << t_shortcode_vec_[i] << std::endl; 
      if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
        is_nse_present_ = true;
      }
    }
    //std::cout << "is_nse_present_: " << is_nse_present_ << std::endl;
    if (is_nse_present_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
    }
  }
  // Checks if any of the instruments are BSE symbols and if so adds BSE contract specs
  void CheckAndAddBSEDefinitions(std::vector<std::string> &t_shortcode_vec_) {
    bool is_bse_present_ = false;
    for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
      if (strncmp(t_shortcode_vec_[i].c_str(), "BSE_", 4) == 0) {
        is_bse_present_ = true;
      }
    }
    if (is_bse_present_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadBSESecurityDefinitions();
    }
  }

  // returns true if given shortcode corresponds to an HK equity
  bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

  // Checks if any of the instruments are HK stocks and if so adds HK stocks contract specs
  void CheckAndAddHKStocksDefinitions(std::vector<std::string> &shortcode_vec) {
    bool is_hk_equities_present = false;

    for (auto i = 0u; i < shortcode_vec.size(); i++) {
      if (IsHKEquity(shortcode_vec[i])) {
        is_hk_equities_present = true;
      }
    }
    if (is_hk_equities_present) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadHKStocksSecurityDefinitions();
    }
  }

  void GetSimConfig(uint32_t secid, vector<bool> &sid_is_dependant_map, std::string shortcode) {
    if (sim_smv_required_ && !simconfig_filename_.empty()) {
      if (sid_is_dependant_map[secid] && sim_time_series_info_->sid_to_sim_config_.size() <= secid) {
        sim_time_series_info_->sid_to_sim_config_.push_back(
            HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode, simconfig_filename_));
      }
    } else {
      if (sid_is_dependant_map[secid] && sim_time_series_info_->sid_to_sim_config_.size() <= secid) {
        sim_time_series_info_->sid_to_sim_config_.push_back(
            HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode));
      }
    }
  }

  /**
   * Initialize the market order view managers
   */
  void InitializeMarketOrderMgrs() {
    sid_to_mov_ptr_map.resize(sec_name_indexer_.NumSecurityId(), nullptr);

    for (auto secid = 0u; secid < sec_name_indexer_.NumSecurityId(); secid++) {
      if (!sid_to_mov_ptr_map[secid]) {
        auto mov = new HFSAT::MarketOrdersView(dbglogger_, watch_, secid);
        sid_to_mov_ptr_map[secid] = mov;
      }

      switch (sid_to_exch_source_map_[secid]) {
        case HFSAT::kExchSourceCME: {
          if (!cme_market_order_manager_) {
            cme_market_order_manager_ = std::unique_ptr<HFSAT::CMEMarketOrderManager>(
                new HFSAT::CMEMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map));
          }
        } break;
        case HFSAT::kExchSourceBSE:
        case HFSAT::kExchSourceEUREX:
        case HFSAT::kExchSourceEOBI: {
          if (!eobi_market_order_manager_) {
            eobi_market_order_manager_ = std::unique_ptr<HFSAT::EOBIMarketOrderManager>(
                new HFSAT::EOBIMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map));
          }
        } break;
        case HFSAT::kExchSourceICE:
        case HFSAT::kExchSourceICEFOD: {
          if (!ice_market_order_manager_) {
            ice_market_order_manager_ = std::unique_ptr<HFSAT::IceMarketOrderManager>(
                new HFSAT::IceMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map));
          }
        } break;
        case HFSAT::kExchSourceJPY: {
          if (!ose_market_order_manager_) {
            ose_market_order_manager_ = std::unique_ptr<HFSAT::AsxMarketOrderManager>(
                new HFSAT::AsxMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map, true));
          }
        } break;
        case HFSAT::kExchSourceASX: {
          if (!asx_market_order_manager_) {
            asx_market_order_manager_ = std::unique_ptr<HFSAT::AsxMarketOrderManager>(
                new HFSAT::AsxMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map, false));
          }
        } break;
        case HFSAT::kExchSourceNSE:
        case HFSAT::kExchSourceNSE_CD:
        case HFSAT::kExchSourceNSE_FO:
        case HFSAT::kExchSourceNSE_EQ: {
          if (!nse_market_order_manager_) {
            nse_market_order_manager_ = std::unique_ptr<HFSAT::NSEMarketOrderManager>(
                new HFSAT::NSEMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map, false));
          }
        } break;
        case HFSAT::kExchSourceSGX: {
          if (!sgx_market_order_manager_) {
            sgx_market_order_manager_ = std::unique_ptr<HFSAT::SGXMarketOrderManager>(
                new HFSAT::SGXMarketOrderManager(dbglogger_, watch_, sec_name_indexer_, sid_to_mov_ptr_map));
          }
        } break;
        default:
          break;
      }
    }
  }

  /**
   * Initialize price level book managers for the securities
   */
  void InitializeMarketViewMgrs() {
    for (unsigned int secid = 0; secid < sec_name_indexer_.NumSecurityId(); secid++) {
      switch (sid_to_exch_source_map_[secid]) {
        case HFSAT::kExchSourceCME: {
          if (!indexed_cme_market_view_manager_) {
            indexed_cme_market_view_manager_ = std::unique_ptr<HFSAT::IndexedCmeMarketViewManager>(
                new HFSAT::IndexedCmeMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          indexed_fpga_market_view_manager_ = std::unique_ptr<HFSAT::IndexedFpgaMarketViewManager>(
              new HFSAT::IndexedFpgaMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));

        } break;
        case HFSAT::kExchSourceEUREX: {
          if (!indexed_cme_market_view_manager_) {
            indexed_cme_market_view_manager_ = std::unique_ptr<HFSAT::IndexedCmeMarketViewManager>(
                new HFSAT::IndexedCmeMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceEOBI: {
          if (!indexed_eobi_market_view_manager_) {
            indexed_eobi_market_view_manager_ = std::unique_ptr<HFSAT::IndexedEobiMarketViewManager>(
                new HFSAT::IndexedEobiMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_eobi_price_level_market_view_manager_) {
            indexed_eobi_price_level_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedEobiPriceLevelMarketViewManager>(
                    new HFSAT::IndexedEobiPriceLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceNSE: {
          if (!indexed_nse_market_view_manager2_ && !is_l1_mode_) {
            indexed_nse_market_view_manager2_ =
                std::unique_ptr<HFSAT::IndexedNSEMarketViewManager2>(new HFSAT::IndexedNSEMarketViewManager2(
                    dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, false));
          }
          if (!generic_l1_data_market_view_manager_) {
            generic_l1_data_market_view_manager_ = std::unique_ptr<HFSAT::GenericL1DataMarketViewManager>(
                new HFSAT::GenericL1DataMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!l1_price_market_view_manager_ && is_l1_mode_) {
            l1_price_market_view_manager_ = std::unique_ptr<HFSAT::L1PriceMarketViewManager>(
                new HFSAT::L1PriceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceNTP:
        case HFSAT::kExchSourceBMF: {
          if (!indexed_ntp_market_view_manager_) {
            indexed_ntp_market_view_manager_ = std::unique_ptr<HFSAT::IndexedNtpMarketViewManager>(
                new HFSAT::IndexedNtpMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!bmf_order_level_market_view_manager_) {
            bmf_order_level_market_view_manager_ = std::unique_ptr<HFSAT::BMFOrderLevelMarketViewManager>(
                new HFSAT::BMFOrderLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }

          if (!indexed_bmf_fpga_market_view_manager_) {
            indexed_bmf_fpga_market_view_manager_ = std::unique_ptr<HFSAT::IndexedBMFFpgaMarketViewManager>(
                new HFSAT::IndexedBMFFpgaMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceBMFEQ: {
          if (!indexed_puma_market_view_manager_) {
            indexed_puma_market_view_manager_ = std::unique_ptr<HFSAT::IndexedNtpMarketViewManager>(
                new HFSAT::IndexedNtpMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!bmf_order_level_market_view_manager_) {
            bmf_order_level_market_view_manager_ = std::unique_ptr<HFSAT::BMFOrderLevelMarketViewManager>(
                new HFSAT::BMFOrderLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }

        } break;
        case HFSAT::kExchSourceLIFFE: {
          if (!indexed_liffe_sim_price_level_market_view_manager_ && sim_smv_required_) {
            indexed_liffe_sim_price_level_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedLiffePriceLevelMarketViewManager>(
                    new HFSAT::IndexedLiffePriceLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_sim_smv_ptr_map_));
          }
          if (!indexed_liffe_price_level_market_view_manager_) {
            indexed_liffe_price_level_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedLiffePriceLevelMarketViewManager>(
                    new HFSAT::IndexedLiffePriceLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceICE: {
          if (!indexed_ice_market_view_manager_) {
            indexed_ice_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_ice_sim_market_view_manager_ && sim_smv_required_) {
            indexed_ice_sim_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }

        } break;
        case HFSAT::kExchSourceTMX: {
          if (!indexed_tmx_market_view_manager_) {
            indexed_tmx_market_view_manager_ = std::unique_ptr<HFSAT::IndexedTmxMarketViewManager>(
                new HFSAT::IndexedTmxMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_ice_market_view_manager_) {
            indexed_ice_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_ice_sim_market_view_manager_ && sim_smv_required_) {
            indexed_ice_sim_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }
          if (!indexed_tmx_sim_market_view_manager_ && sim_smv_required_) {
            indexed_tmx_sim_market_view_manager_ = std::unique_ptr<HFSAT::IndexedTmxMarketViewManager>(
                new HFSAT::IndexedTmxMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }
          // book building modification purpose
          if (!indexed_tmx_obf_of_market_view_manager_) {
            indexed_tmx_obf_of_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedTMXOBFOFMarketViewManager>(new HFSAT::IndexedTMXOBFOFMarketViewManager(
                    dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          // book building modification purpose
          if (!indexed_tmx_obf_of_sim_market_view_manager_ && sim_smv_required_) {
            indexed_tmx_obf_of_sim_market_view_manager_ =
                std::unique_ptr<HFSAT::IndexedTMXOBFOFMarketViewManager>(new HFSAT::IndexedTMXOBFOFMarketViewManager(
                    dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceKRX: {
          if (!indexed_krx_market_view_manager_) {
            indexed_krx_market_view_manager_ = std::unique_ptr<HFSAT::IndexedTmxMarketViewManager>(
                new HFSAT::IndexedTmxMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceRTS: {
          if (!indexed_rts_market_view_manager_) {
            indexed_rts_market_view_manager_ = std::unique_ptr<HFSAT::IndexedRtsMarketViewManager>(
                new HFSAT::IndexedRtsMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_rts_of_market_view_manager_) {
            indexed_rts_of_market_view_manager_ = std::unique_ptr<HFSAT::IndexedRtsOFMarketViewManager>(
                new HFSAT::IndexedRtsOFMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_rts_sim_market_view_manager_ && sim_smv_required_) {
            indexed_rts_sim_market_view_manager_ = std::unique_ptr<HFSAT::IndexedRtsMarketViewManager>(
                new HFSAT::IndexedRtsMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceMICEX:
        case HFSAT::kExchSourceMICEX_CR:
        case HFSAT::kExchSourceMICEX_EQ: {
          if (!indexed_micex_market_view_manager_) {
            indexed_micex_market_view_manager_ = std::unique_ptr<HFSAT::IndexedMicexMarketViewManager>(
                new HFSAT::IndexedMicexMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_micex_of_market_view_manager_) {
            indexed_micex_of_market_view_manager_ = std::unique_ptr<HFSAT::IndexedMicexOFMarketViewManager>(
                new HFSAT::IndexedMicexOFMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceJPY: {
          if (!indexed_ice_market_view_manager_) {
            indexed_ice_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!indexed_ose_price_feed_market_view_manager_) {
            indexed_ose_price_feed_market_view_manager_ = std::unique_ptr<HFSAT::IndexedOsePriceFeedMarketViewManager>(
                new HFSAT::IndexedOsePriceFeedMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                sid_to_smv_ptr_map_));
          }
          if (!indexed_ose_order_feed_market_view_manager_) {
            indexed_ose_order_feed_market_view_manager_ = std::unique_ptr<HFSAT::IndexedOseOrderFeedMarketViewManager>(
                new HFSAT::IndexedOseOrderFeedMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                sid_to_smv_ptr_map_));
          }
          if (!ose_l1_price_market_view_manager_) {
            ose_l1_price_market_view_manager_ = std::unique_ptr<HFSAT::OSEL1PriceMarketViewManager>(
                new HFSAT::OSEL1PriceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceHONGKONG: {
          if (!hkex_market_view_manager_) {
            hkex_market_view_manager_ = std::unique_ptr<HFSAT::HKEXIndexedMarketViewManager>(
                new HFSAT::HKEXIndexedMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!hkex_sim_market_view_manager_ && hkex_sim_market_view_manager_) {
            hkex_sim_market_view_manager_ =
                std::unique_ptr<HFSAT::HKEXIndexedMarketViewManager>(new HFSAT::HKEXIndexedMarketViewManager(
                    dbglogger_, watch_, sec_name_indexer_, sid_to_sim_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceCFE: {
          if (!indexed_cfe_market_view_manager_) {
            indexed_cfe_market_view_manager_ = std::unique_ptr<HFSAT::IndexedCfeMarketViewManager>(
                new HFSAT::IndexedCfeMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceASX: {
          if (!indexed_asx_market_view_manager_) {
            indexed_asx_market_view_manager_ = std::unique_ptr<HFSAT::IndexedAsxMarketViewManager>(
                new HFSAT::IndexedAsxMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceSGX: {
          if (!indexed_ice_market_view_manager_) {
            indexed_ice_market_view_manager_ = std::unique_ptr<HFSAT::IndexedIceMarketViewManager>(
                new HFSAT::IndexedIceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceBSE: {
          if (!indexed_bse_market_view_manager_ && !is_l1_mode_) {
            indexed_bse_market_view_manager_ = std::unique_ptr<HFSAT::IndexedBSEMarketViewManager2>(
                new HFSAT::IndexedBSEMarketViewManager2(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, false));
          }
          if (!generic_l1_data_market_view_manager_) {
            generic_l1_data_market_view_manager_ = std::unique_ptr<HFSAT::GenericL1DataMarketViewManager>(
                new HFSAT::GenericL1DataMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
          if (!l1_price_market_view_manager_ && is_l1_mode_) {
            l1_price_market_view_manager_ = std::unique_ptr<HFSAT::L1PriceMarketViewManager>(
                new HFSAT::L1PriceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceHKOMD: {
          if (!indexed_hkomd_market_view_manager_) {
            indexed_hkomd_market_view_manager_ = std::unique_ptr<HFSAT::HKOMDIndexedMarketViewManager>(
                new HFSAT::HKOMDIndexedMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceHKOMDCPF:
        case HFSAT::kExchSourceHKOMDPF: {
          if (!hkomd_price_level_market_view_manager_) {
            hkomd_price_level_market_view_manager_ = std::unique_ptr<HFSAT::IndexedHKOMDPriceLevelMarketViewManager>(
                new HFSAT::IndexedHKOMDPriceLevelMarketViewManager(dbglogger_, watch_, sec_name_indexer_,
                                                                   sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceBATSCHI: {
          if (!l1_price_market_view_manager_) {
            l1_price_market_view_manager_ = std::unique_ptr<HFSAT::L1PriceMarketViewManager>(
                new HFSAT::L1PriceMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_));
          }
        } break;
        case HFSAT::kExchSourceMEFF:
        case HFSAT::kExchSourceIDEM:
        case HFSAT::kExchSourceREUTERS:
        default: { } break; }
    }
  }

  void InitializeVariables(bool initilize_order_feed_data_ = false) {
    // setup DebugLogger
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << log_file_name_;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
    dbglogger_.AddLogLevel(BOOK_ERROR);
    dbglogger_.AddLogLevel(BOOK_INFO);

    network_account_info_manager_ = new HFSAT::NetworkAccountInfoManager();

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);


    //std::cout << "Before: CheckAndAddNSEDefinitions" << std::endl;
    CheckAndAddNSEDefinitions(source_shortcode_vec_);
    CheckAndAddBSEDefinitions(source_shortcode_vec_);

    for (auto i = 0u; i < source_shortcode_vec_.size();
         i++) {  // go through all source_shortcodes and check for hybrid securities
      if (source_shortcode_vec_[i].compare("HYB_ESPY") == 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("ES_0"));
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("SPY"));
      } else if (source_shortcode_vec_[i].compare("HYB_NQQQ") == 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NQ_0"));
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("QQQ"));
      } else if (source_shortcode_vec_[i].compare("HYB_YDIA") == 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("YM_0"));
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("DIA"));
      } else if (strncmp(source_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
        /*if (HFSAT::NSESecurityDefinitions::IsOption(source_shortcode_vec_[i])) {
          //          option_view_map_.AddEntry(source_shortcode_vec_[i],NULL);
          HFSAT::VectorUtils::UniqueVectorAdd(
              source_shortcode_vec_,
              HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(source_shortcode_vec_[i]));
        }*/
      }
    }

    CheckAndAddHKStocksDefinitions(source_shortcode_vec_);
    if (dep_shortcode_ == "NULL") {
      dep_shortcode_ = source_shortcode_vec_[0];
    }

    dep_trading_location_ = HFSAT::kTLocCHI;
    if (dep_shortcode_.compare("NONAME") != 0) {
      dep_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
          HFSAT::SecurityDefinitions::GetContractExchSource(dep_shortcode_, tradingdate_));
    }

    // Get exchange symbols corresponding to the shortcodes of interest
    // Add exchange symbols to SecurityNameIndexer
    for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
      if (!sec_name_indexer_.HasString(source_shortcode_vec_[i])) {  // need to add this source to sec_name_indexer_
                                                                     // since it was not added already
        // A unique instance of ExchangeSymbolManager gets the current symbol that the exchange knows this shortcode
        // as
        // and also allocates permanent storage to this instrument, that allows read access from outside.
        const char *exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(source_shortcode_vec_[i]);
        sec_name_indexer_.AddString(exchange_symbol_, source_shortcode_vec_[i]);
        sid_to_marketdata_needed_map_.push_back(true);  // we need market data for every symbol in
          // source_shortcode_vec_ since this was based on the modelfile
        exchange_symbol_vec_.push_back(exchange_symbol_);
      }
    }
    // Add all Ors shortcodes to sec name indexer
    for (auto i = 0u; i < ors_shortcode_vec_.size(); i++) {
      if (!sec_name_indexer_.HasString(ors_shortcode_vec_[i])) {  // need to add this source to
                                                                  // sec_name_indexer_ since it was not added
                                                                  // already
        const char *exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(ors_shortcode_vec_[i]);
        sec_name_indexer_.AddString(exchange_symbol_, ors_shortcode_vec_[i]);
        exchange_symbol_vec_.push_back(exchange_symbol_);
        sid_to_marketdata_needed_map_.push_back(false);
      }
    }
    // Set EOBI,HKOMD bools to select the correct feed
    bool USE_EOBI = true;
    //    char *use_eobi_ = getenv("USE_EOBI");
    //    if (use_eobi_ != NULL && strlen(use_eobi_) > 0) {
    //      if (atoi(use_eobi_) <= 0) {
    //        USE_EOBI = false;
    //      }
    //    }
    bool USE_OMDCPF = true;
    char *use_omd_pf_ = getenv("USE_HKOMDCPF");
    if (use_omd_pf_ != NULL && strlen(use_omd_pf_) > 0) {
      if (atoi(use_omd_pf_) <= 0) {
        USE_OMDCPF = false;
      }
    }
    // Set sid_to_exch_source maps
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);

      sid_is_dependant_map_.push_back(
          HFSAT::VectorUtils::LinearSearchValue(dep_shortcode_vec_, sec_name_indexer_.GetShortcodeFromId(i)));

      HFSAT::ExchSource_t t_exch_source_ =
          HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, tradingdate_);
      if (HFSAT::UseEOBIData(dep_trading_location_, tradingdate_, _this_shortcode_)) {
        t_exch_source_ = HFSAT::kExchSourceEOBI;
      } else if (HFSAT::UseHKOMDData(dep_trading_location_, tradingdate_, t_exch_source_)) {
        if (USE_OMDCPF) {
          t_exch_source_ = HFSAT::kExchSourceHKOMDCPF;
        } else {
          t_exch_source_ = HFSAT::kExchSourceHKOMD;
        }
      }
      sid_to_exch_source_map_.push_back(t_exch_source_);
    }

    // Setting EOBI bool for correct feed selection
    if ((true == USE_EOBI) && (HFSAT::kTLocFR2 == dep_trading_location_) && (tradingdate_ >= 20140225)) {
      USE_EOBI_PF_FR2 = true;
    }

    // THIS IS DONE TO ENFORCE EOBI ORDER FEED FOR FR2 LOCATION TO USE FAST_ORDERS
    // NOT SURE IF THERE ARE ANY OTHER EXECS WHICH CAN BREAK DUE TO THIS //ravi
    //    USE_EOBI_PF_FR2 = false;

    // All the maps for smv and prom order managers

    ///< Unique Instance of map from shortcode to p_smv_
    HFSAT::ShortcodeSecurityMarketViewMap &shortcode_smv_map_ =
        HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();
    ///< Unique Instance of map from shortcode to p_pom_
    HFSAT::ShortcodePromOrderManagerMap &shortcode_pom_map_ = HFSAT::ShortcodePromOrderManagerMap::GetUniqueInstance();

    // Making all the smvs and adding to sid_to_smv_map
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      const char *_this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
      if (sid_to_exch_source_map_[i] != HFSAT::kExchSourceHYB) {
        bool set_temporary_bool_checking_if_this_is_an_indexed_book_ = HFSAT::CommonSimIndexedBookBool(
            sid_to_exch_source_map_[i], "NOMATCH", dep_trading_location_,
            tradingdate_);  // moved to a common function due to conflicts with smv analyser
        HFSAT::SecurityMarketView *p_smv_ = new HFSAT::SecurityMarketView(
            dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_, i,
            sid_to_exch_source_map_[i], set_temporary_bool_checking_if_this_is_an_indexed_book_,
            offline_mix_mms_filename_, online_mix_price_consts_filename_, online_beta_kalman_consts_filename_);
        p_smv_->SetL1OnlyFlag(is_l1_mode_);
        sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
        shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map

        if (sim_smv_required_) {
          HFSAT::SecurityMarketView *p_smv_sim_ = new HFSAT::SecurityMarketView(
              dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_, i,
              sid_to_exch_source_map_[i], set_temporary_bool_checking_if_this_is_an_indexed_book_,
              offline_mix_mms_filename_, online_mix_price_consts_filename_, online_beta_kalman_consts_filename_);
          p_smv_sim_->SetL1OnlyFlag(is_l1_mode_);
          sid_to_sim_smv_ptr_map_.push_back(p_smv_sim_);
        }

        if (p_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
          p_smv_->InitializeSMVForIndexedBook();
        }

        // set bool to false since we do not want to process market data for this
        if (!sid_to_marketdata_needed_map_[i]) {
          p_smv_->SetProcessMarketDataBool(false);
        }

      } else {
        sid_to_smv_ptr_map_.push_back(NULL);  // sid points to NULL since we will add HSMV * in the next loop
      }
    }
//Ignoring the FUT smv for option
    /*for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      if (HFSAT::NSESecurityDefinitions::IsOption(_this_shortcode_)) {
        HFSAT::SecurityMarketView *p_smv1_ = NULL;
        HFSAT::SecurityMarketView *p_smv2_ = NULL;

        p_smv1_ = shortcode_smv_map_.GetSecurityMarketView(_this_shortcode_);
        p_smv2_ = shortcode_smv_map_.GetSecurityMarketView(
            HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(_this_shortcode_));

        p_smv1_->SetFutureSMV(p_smv2_);
      }
    }*/
    ///< used for all sources in case using_non_self_market_view_=true.
    HFSAT::PromOrderManagerPtrVec &sid_to_prom_order_manager_map_ = HFSAT::sid_to_prom_order_manager_map();
    sid_to_prom_order_manager_map_.resize(sec_name_indexer_.NumSecurityId(), NULL);
    /// Otherwise used for ors_needed_by_indicators_vec_
    ///< true for all sources in case using_non_self_market_view_=true.
    /// Otherwise true for sids corresponding to
    /// ors_needed_by_indicators_vec_
    /** Setup PromOrderManager if either
     * (i) doing so for all sources to adjust market view based on this ( using_non_self_market_view_ )
     * (ii) is needed by some indicator
     */
    sid_to_ors_needed_map_.assign(sec_name_indexer_.NumSecurityId(), false);
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      const char *_this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
      sid_to_ors_needed_map_[i] = ((using_non_self_market_view_) ||
                                   (HFSAT::VectorUtils::LinearSearchValue(ors_shortcode_vec_, _this_shortcode_)));

      if (sid_to_ors_needed_map_[i]) {
        HFSAT::PromOrderManager *p_pom_ = HFSAT::PromOrderManager::GetUniqueInstance(
            dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, i, _this_exchange_symbol_);
        sid_to_prom_order_manager_map_[i] = p_pom_;
        shortcode_pom_map_.AddEntry(
            _this_shortcode_,
            p_pom_);  // add to shortcode_ to POM* map ... for indicators to be able to load from text
      }
    }

    num_sec_id_ = sec_name_indexer_.NumSecurityId();

    // Remove self orders if required
    bool use_non_self_book_in_sim_ = (!ignore_user_msg_);
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); ++i) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      if (HFSAT::SecurityDefinitions::GetRemoveSelfOrdersFromBook(_this_shortcode_, tradingdate_) &&
          (sid_to_smv_ptr_map_[i] != NULL) && (sid_to_prom_order_manager_map_[i] != NULL) &&
          use_non_self_book_in_sim_) {
        sid_to_smv_ptr_map_[i]->SetSelfOrdersFromBook(true);

        if (HFSAT::SecurityDefinitions::GetConfToMarketUpdateMsecs(_this_shortcode_, tradingdate_)) {
          sid_to_smv_ptr_map_[i]->SetSmartSelfOrdersFromBook(true);
        }
      }
    }


    // All the Market Managers are initialized here. Market Managers are one for each exchange.
    InitializeMarketViewMgrs();
    if (initilize_order_feed_data_) {
      InitializeMarketOrderMgrs();
    }
    sim_time_series_info_ = new HFSAT::SimTimeSeriesInfo(sec_name_indexer_.NumSecurityId());
  }

  /**
   *
   * @param initialize_market_order_manager
   */
  void Initialize(bool initialize_market_order_manager = false) {
    InitializeVariables(initialize_market_order_manager);
    HFSAT::ShortcodeORSMessageFilesourceMap &shortcode_ors_data_filesource_map_ =
        HFSAT::ShortcodeORSMessageFilesourceMap::GetUniqueInstance();

    // Go over all the security ids.
    // Create it's filesource and link it to the shortcode's exchange's MarketViewManager
    // Link the Filesource to the HistoricalDispatcher
    for (unsigned int secid = 0; secid < sec_name_indexer_.NumSecurityId(); secid++) {
      //      HFSAT::SecurityMarketView *p_smv_ = sid_to_smv_ptr_map_[secid];
      if (sim_time_series_info_->sid_to_sim_config_.size() < secid) {
        sim_time_series_info_->sid_to_sim_config_.resize(secid);
      }

      const char *exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(secid);
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(secid);

      // Set The SMV for Indexed Book, You dont need to set it with each exch, - ravi
      if (sim_smv_required_ &&
          sid_to_smv_ptr_map_[secid]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
        sid_to_smv_ptr_map_[secid]->InitializeSMVForIndexedBook();
        sid_to_sim_smv_ptr_map_[secid]->InitializeSMVForIndexedBook();
      }

      if (sid_to_ors_needed_map_[secid]) {
        if (shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_) == NULL) {
          HFSAT::ORSMessageFileSource *p_ors_message_filesource_ =
              new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                              sec_name_indexer_.GetSecurityNameFromId(secid), dep_trading_location_);
          p_ors_message_filesource_->SetExternalTimeListener(&watch_);
          shortcode_ors_data_filesource_map_.AddEntry(shortcode_, p_ors_message_filesource_);
          historical_dispatcher_.AddExternalDataListener(p_ors_message_filesource_);
        }
      }

      GetSimConfig(secid, sid_is_dependant_map_, shortcode_);
      // modify this
      switch (sid_to_exch_source_map_[secid]) {
        case HFSAT::kExchSourceCME: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager) {
              shortcode_cme_obf_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::CMEOBFLoggedMessageFileSource>(
                      new HFSAT::CMEOBFLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                               exchange_symbol_, dep_trading_location_, false));
              shortcode_cme_obf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_cme_obf_data_filesource_map_[shortcode_]->AddOrderLevelListener(
                  cme_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_cme_obf_data_filesource_map_[shortcode_].get());
            }

            if (isCMEOBF) {
            } else {
              if (tradingdate_ >= 20170706 && sid_to_exch_source_map_[secid] == HFSAT::kExchSourceCME &&
                  dep_trading_location_ == HFSAT::kTLocCHI) {
                if (shortcode_fpga_data_filesource_map_.find(shortcode_) == shortcode_fpga_data_filesource_map_.end()) {
                  shortcode_fpga_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::FPGALoggedMessageFileSource>(new HFSAT::FPGALoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                          use_fake_faster_data_));
                  shortcode_fpga_data_filesource_map_[shortcode_]->SetHalfBookGlobalListener(
                      (HFSAT::FPGAHalfBookGlobalListener *)indexed_fpga_market_view_manager_.get());
                  shortcode_fpga_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_filesource_map_[shortcode_] = shortcode_fpga_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_fpga_data_filesource_map_[shortcode_].get());
                }
              } else {
                if (shortcode_cme_data_filesource_map_.find(shortcode_) == shortcode_cme_data_filesource_map_.end()) {
                  shortcode_cme_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::CMELoggedMessageFileSource>(new HFSAT::CMELoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                          use_fake_faster_data_));
                  shortcode_cme_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                      indexed_cme_market_view_manager_.get());
                  shortcode_cme_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_filesource_map_[shortcode_] = shortcode_cme_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_cme_data_filesource_map_[shortcode_].get());
                }
              }
            }
          }

        } break;
        case HFSAT::kExchSourceEUREX: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_eurex_data_filesource_map_.find(shortcode_) == shortcode_eurex_data_filesource_map_.end()) {
              shortcode_eurex_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::EUREXLoggedMessageFileSource>(new HFSAT::EUREXLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));

              shortcode_eurex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  indexed_cme_market_view_manager_.get());
              shortcode_eurex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_eurex_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_eurex_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceEOBI: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager) {
              shortcode_eobi_data_filesource_map_[shortcode_] = std::unique_ptr<HFSAT::EOBILoggedMessageFileSource>(
                  new HFSAT::EOBILoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                         exchange_symbol_, dep_trading_location_, false));
              shortcode_eobi_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_eobi_data_filesource_map_[shortcode_]->SetOrderLevelListenerSim(
                  eobi_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_eobi_data_filesource_map_[shortcode_].get());
            }

            if (dep_shortcode_.compare("NONAME") != 0 && (dep_trading_location_ == HFSAT::kTLocFR2 || 
		dep_trading_location_ == HFSAT::kTLocBSE) &&
                (false == USE_EOBI_PF_FR2)) {
              if (shortcode_eobi_data_filesource_map_.find(shortcode_) == shortcode_eobi_data_filesource_map_.end()) {
                shortcode_eobi_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::EOBILoggedMessageFileSource>(new HFSAT::EOBILoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        false, use_fake_faster_data_));
                shortcode_eobi_data_filesource_map_[shortcode_]->SetOrderGlobalListenerEOBI(
                    indexed_eobi_market_view_manager_.get());
                shortcode_eobi_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_eobi_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_eobi_data_filesource_map_[shortcode_].get());
              }
            } else {
              if (shortcode_eobi_price_feed_source_map_.find(shortcode_) ==
                  shortcode_eobi_price_feed_source_map_.end()) {
                shortcode_eobi_price_feed_source_map_[shortcode_] =
                    std::unique_ptr<HFSAT::EOBIPriceFeedLoggedMessageFileSource>(
                        new HFSAT::EOBIPriceFeedLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                        secid, exchange_symbol_, dep_trading_location_,
                                                                        use_fake_faster_data_));
                shortcode_eobi_price_feed_source_map_[shortcode_]->SetPriceLevelGlobalListener(
                    indexed_eobi_price_level_market_view_manager_.get());
                shortcode_eobi_price_feed_source_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_eobi_price_feed_source_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_eobi_price_feed_source_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceNSE: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager && !is_l1_mode_) {
              //Using Unconverted data for running sim from 20240408
              if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
                std::cout << "NSE logged : " << dep_trading_location_ << std::endl ;
                shortcode_nse_data_filesource_map2_[shortcode_] = std::unique_ptr<HFSAT::NSELoggedMessageFileSource2>(
                    new HFSAT::NSELoggedMessageFileSource2(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                          exchange_symbol_, dep_trading_location_, false));
                shortcode_nse_data_filesource_map2_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_nse_data_filesource_map2_[shortcode_]->SetOrderGlobalListenerNSE(
                    nse_market_order_manager_.get());
                historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map2_[shortcode_].get());
	      } else {
                std::cout << "NSE logged : " << dep_trading_location_ << std::endl ;
                shortcode_nse_data_filesource_map_[shortcode_] = std::unique_ptr<HFSAT::NSELoggedMessageFileSource>(
                    new HFSAT::NSELoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                          exchange_symbol_, dep_trading_location_, false));
                shortcode_nse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_nse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerNSE(
                    nse_market_order_manager_.get());
                historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map_[shortcode_].get());
	      }
            }

            if (is_l1_mode_) {
              shortcode_nse_l1_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::NSEL1LoggedMessageFileSource>(new HFSAT::NSEL1LoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
              shortcode_nse_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_nse_l1_data_filesource_map_[shortcode_]->SetL1DataListener(
                  generic_l1_data_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_nse_l1_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_nse_l1_data_filesource_map_[shortcode_].get());

            } else {
	      //Using Unconverted data for running sim from 20240408
              if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
                if (shortcode_nse_data_filesource_map2_.find(shortcode_) == shortcode_nse_data_filesource_map2_.end()) {
                  std::cout << "NSE logged : " << dep_trading_location_ << std::endl ;
                  shortcode_nse_data_filesource_map2_[shortcode_] =
                      std::unique_ptr<HFSAT::NSELoggedMessageFileSource2>(new HFSAT::NSELoggedMessageFileSource2(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_nse_data_filesource_map2_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_nse_data_filesource_map2_[shortcode_]->SetOrderGlobalListenerNSE(
                      indexed_nse_market_view_manager2_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_nse_data_filesource_map2_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map2_[shortcode_].get());
  	        }
	      } else {
                if (shortcode_nse_data_filesource_map_.find(shortcode_) == shortcode_nse_data_filesource_map_.end()) {
                  std::cout << "NSE logged : " << dep_trading_location_ << std::endl ;
                  shortcode_nse_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::NSELoggedMessageFileSource>(new HFSAT::NSELoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
                  shortcode_nse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_nse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerNSE(
                      indexed_nse_market_view_manager2_.get());
                  shortcode_filesource_map_[shortcode_] = shortcode_nse_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_nse_data_filesource_map_[shortcode_].get());
  	        }
	      }
            }
          }
        } break;
        case HFSAT::kExchSourceBSE: {
          if (sid_to_marketdata_needed_map_[secid] && !is_l1_mode_) {

            if (shortcode_bse_data_filesource_map_.find(shortcode_) == shortcode_bse_data_filesource_map_.end()) {

              if( tradingdate_ >= BSE_USE_UNCONVERTED_DATA_DATE){
                std::cout << "BSE logged : " << dep_trading_location_ << std::endl ;
                shortcode_bse_data_filesource_map2_[shortcode_] = std::unique_ptr<HFSAT::BSELoggedMessageFileSource2>(
                    new HFSAT::BSELoggedMessageFileSource2(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                          exchange_symbol_, dep_trading_location_, false));
                shortcode_bse_data_filesource_map2_[shortcode_]->SetExternalTimeListener(&watch_);
                 shortcode_bse_data_filesource_map2_[shortcode_]->SetOrderGlobalListenerBSE(
                     indexed_bse_market_view_manager_.get());
                historical_dispatcher_.AddExternalDataListener(shortcode_bse_data_filesource_map2_[shortcode_].get());
	            } 
              
              else{
              shortcode_bse_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::BSELoggedMessageFileSource>(new HFSAT::BSELoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
              shortcode_bse_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_bse_data_filesource_map_[shortcode_]->SetOrderGlobalListenerBSE(
                  indexed_bse_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_bse_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_bse_data_filesource_map_[shortcode_].get());
            }
          }

          } else if (is_l1_mode_) {
              shortcode_nse_l1_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::NSEL1LoggedMessageFileSource>(new HFSAT::NSEL1LoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
              shortcode_nse_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_nse_l1_data_filesource_map_[shortcode_]->SetL1DataListener(
                  generic_l1_data_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_nse_l1_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_nse_l1_data_filesource_map_[shortcode_].get());

          } 
        } break;
        case HFSAT::kExchSourceNTP:
        case HFSAT::kExchSourceBMF: {
          if (!isNTP && !isNTPORD && !isBMFEq) {
            bmf_logged_message_file_source_ =
                std::unique_ptr<HFSAT::BMFLoggedMessageFileSource>(new HFSAT::BMFLoggedMessageFileSource(
                    dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_));
            bmf_logged_message_file_source_->SetOrderLevelGlobalListener(bmf_order_level_market_view_manager_.get());
            bmf_logged_message_file_source_->SetExternalTimeListener(&watch_);
            shortcode_filesource_map_[shortcode_] = bmf_logged_message_file_source_.get();
            historical_dispatcher_.AddExternalDataListener(bmf_logged_message_file_source_.get());
          } else {
            if (isNTP) {
              if (sid_to_marketdata_needed_map_[secid]) {
                if (tradingdate_ >= USING_BMF_FPGA_FROM) {
                  if (shortcode_bmf_fpga_data_filesource_map_.find(shortcode_) ==
                      shortcode_bmf_fpga_data_filesource_map_.end()) {
                    shortcode_bmf_fpga_data_filesource_map_[shortcode_] =
                        std::unique_ptr<HFSAT::BMFFPGALoggedMessageFileSource>(
                            new HFSAT::BMFFPGALoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                      secid, exchange_symbol_, dep_trading_location_,
                                                                      use_fake_faster_data_));
                    shortcode_bmf_fpga_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                        indexed_bmf_fpga_market_view_manager_.get());
                    shortcode_bmf_fpga_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                    shortcode_filesource_map_[shortcode_] = shortcode_bmf_fpga_data_filesource_map_[shortcode_].get();
                    historical_dispatcher_.AddExternalDataListener(
                        shortcode_bmf_fpga_data_filesource_map_[shortcode_].get());
                  }
                } else {
                  if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
                    shortcode_ntp_data_filesource_map_[shortcode_] =
                        std::unique_ptr<HFSAT::NTPLoggedMessageFileSource>(new HFSAT::NTPLoggedMessageFileSource(
                            dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                            false, false, use_fake_faster_data_));
                    shortcode_ntp_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(
                        indexed_ntp_market_view_manager_.get());
                    shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                    shortcode_filesource_map_[shortcode_] = shortcode_ntp_data_filesource_map_[shortcode_].get();
                    historical_dispatcher_.AddExternalDataListener(
                        shortcode_ntp_data_filesource_map_[shortcode_].get());
                  }
                }
              }
            } else if (isNTPORD || isBMFEq) {
              shortcode_ntp_data_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                  bmf_order_level_market_view_manager_.get());
            }
          }
        } break;
        case HFSAT::kExchSourceBMFEQ: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (tradingdate_ >= 20140306) {
              if (shortcode_puma_data_filesource_map_.find(shortcode_) == shortcode_puma_data_filesource_map_.end()) {
                shortcode_puma_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::PUMALoggedMessageFileSource>(new HFSAT::PUMALoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                shortcode_puma_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(
                    indexed_puma_market_view_manager_.get());
                shortcode_puma_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_puma_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_puma_data_filesource_map_[shortcode_].get());
              }
            } else {
              if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
                if (sid_to_smv_ptr_map_[secid]
                        ->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
                  indexed_puma_market_view_manager_.get()->DropIndexedBookForSource(sid_to_exch_source_map_[secid],
                                                                                    secid);
                  sid_to_smv_ptr_map_[secid]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ =
                      false;
                }

                shortcode_ntp_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::NTPLoggedMessageFileSource>(new HFSAT::NTPLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        false, true, use_fake_faster_data_));
                shortcode_ntp_data_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                    bmf_order_level_market_view_manager_.get());
                shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_ntp_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceLIFFE: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_liffe_data_filesource_map_.find(shortcode_) == shortcode_liffe_data_filesource_map_.end()) {
              shortcode_liffe_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::LIFFELoggedMessageFileSource>(new HFSAT::LIFFELoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              if (sim_smv_required_) {
                shortcode_liffe_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                    indexed_liffe_sim_price_level_market_view_manager_.get());
              }
              shortcode_liffe_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                  indexed_liffe_price_level_market_view_manager_.get());
              shortcode_liffe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_liffe_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_liffe_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceICE: {
          if (sid_to_marketdata_needed_map_[secid] &&
              shortcode_ice_data_filesource_map_.find(shortcode_) == shortcode_ice_data_filesource_map_.end()) {
            shortcode_ice_data_filesource_map_[shortcode_] =
                std::unique_ptr<HFSAT::ICELoggedMessageFileSource>(new HFSAT::ICELoggedMessageFileSource(
                    dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_, false,
                    use_fake_faster_data_));
            if (sim_smv_required_) {
              shortcode_ice_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                  indexed_ice_sim_market_view_manager_.get());
            }
            shortcode_ice_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                indexed_ice_market_view_manager_.get());
            shortcode_ice_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
            shortcode_filesource_map_[shortcode_] = shortcode_ice_data_filesource_map_[shortcode_].get();
            historical_dispatcher_.AddExternalDataListener(shortcode_ice_data_filesource_map_[shortcode_].get());

            if (initialize_market_order_manager) {
              shortcode_ice_data_filesource_map_[shortcode_] = std::unique_ptr<HFSAT::ICELoggedMessageFileSource>(
                  new HFSAT::ICELoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                        exchange_symbol_, dep_trading_location_, false));
              shortcode_ice_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_ice_data_filesource_map_[shortcode_]->AddOrderLevelListener(ice_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_ice_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceTMX: {
          if (sid_to_marketdata_needed_map_[secid]) {
            /*
            if (initialize_market_order_manager) {
              shortcode_tmx_of_data_filesource_map_[shortcode_] = std::unique_ptr<HFSAT::TMXOFLoggedMessageFileSource>(
                  new HFSAT::TMXOFLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                          exchange_symbol_, dep_trading_location_, false));
              shortcode_tmx_of_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_tmx_of_data_filesource_map_[shortcode_]->AddOrderLevelListener(tmx_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_tmx_of_data_filesource_map_[shortcode_].get());
            }
             */

            if (tradingdate_ >= USING_TMX_OBF_FROM) {
              if (is_TMX_OF_) {
                if (shortcode_tmx_of_data_filesource_map_.find(shortcode_) ==
                    shortcode_tmx_of_data_filesource_map_.end()) {
                  shortcode_tmx_of_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::TMXOFLoggedMessageFileSource>(new HFSAT::TMXOFLoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                          use_fake_faster_data_));
                  if (sim_smv_required_) {
                    shortcode_tmx_of_data_filesource_map_[shortcode_]->AddOrderFeedGlobalListener(
                        indexed_tmx_obf_of_sim_market_view_manager_.get());
                  }
                  shortcode_tmx_of_data_filesource_map_[shortcode_]->AddOrderFeedGlobalListener(
                      indexed_tmx_obf_of_market_view_manager_.get());
                  shortcode_tmx_of_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

                  shortcode_filesource_map_[shortcode_] = shortcode_tmx_of_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(
                      shortcode_tmx_of_data_filesource_map_[shortcode_].get());
                }

              } else {
                if (shortcode_tmx_pf_data_filesource_map_.find(shortcode_) ==
                    shortcode_tmx_pf_data_filesource_map_.end()) {
                  shortcode_tmx_pf_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::TMXPFLoggedMessageFileSource>(new HFSAT::TMXPFLoggedMessageFileSource(
                          dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                          use_fake_faster_data_));
                  if (sim_smv_required_) {
                    shortcode_tmx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                        indexed_ice_sim_market_view_manager_.get());
                  }
                  shortcode_tmx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                      indexed_ice_market_view_manager_.get());
                  shortcode_tmx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_filesource_map_[shortcode_] = shortcode_tmx_pf_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(
                      shortcode_tmx_pf_data_filesource_map_[shortcode_].get());
                }
              }

            } else {
              if (shortcode_tmx_data_filesource_map_.find(shortcode_) == shortcode_tmx_data_filesource_map_.end()) {
                shortcode_tmx_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::TMXLoggedMessageFileSource>(new HFSAT::TMXLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                if (sim_smv_required_) {
                  shortcode_tmx_data_filesource_map_[shortcode_]->AddFullBookGlobalListener(
                      indexed_tmx_sim_market_view_manager_.get());
                }
                shortcode_tmx_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                    indexed_tmx_market_view_manager_.get());
                shortcode_tmx_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_tmx_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_tmx_data_filesource_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceKRX: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_krx_data_filesource_map_.find(shortcode_) == shortcode_krx_data_filesource_map_.end()) {
              shortcode_krx_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::KRXLoggedMessageFileSource>(new HFSAT::KRXLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              shortcode_krx_data_filesource_map_[shortcode_]->AddFullBookGlobalListener(
                  indexed_krx_market_view_manager_.get());  // TODO: Check its market view manager
              shortcode_krx_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                  indexed_krx_market_view_manager_.get());
              shortcode_krx_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_krx_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_krx_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceRTS: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (isRTSOF) {
              if (shortcode_rts_of_data_filesource_map_.find(shortcode_) ==
                  shortcode_rts_of_data_filesource_map_.end()) {
                shortcode_rts_of_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::RTSOrderFeedLoggedMessageFileSource>(
                        new HFSAT::RTSOrderFeedLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                       secid, exchange_symbol_, dep_trading_location_,
                                                                       use_fake_faster_data_));

                shortcode_rts_of_data_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                    indexed_rts_of_market_view_manager_.get());
                shortcode_rts_of_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_rts_of_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_rts_of_data_filesource_map_[shortcode_].get());
              }
            } else {
              if (shortcode_rts_data_filesource_map_.find(shortcode_) == shortcode_rts_data_filesource_map_.end()) {
                shortcode_rts_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::RTSLoggedMessageFileSource>(new HFSAT::RTSLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                if (sim_smv_required_) {
                  shortcode_rts_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                      indexed_rts_sim_market_view_manager_.get());
                }
                shortcode_rts_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                    indexed_rts_market_view_manager_.get());
                shortcode_rts_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_rts_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_rts_data_filesource_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceMICEX:
        case HFSAT::kExchSourceMICEX_CR:
        case HFSAT::kExchSourceMICEX_EQ: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (using_MICEX_OF_book_) {
              if (shortcode_micex_of_data_filesource_map_.find(shortcode_) ==
                  shortcode_micex_of_data_filesource_map_.end()) {
                shortcode_micex_of_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::MICEXOFLoggedMessageFileSource>(new HFSAT::MICEXOFLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                shortcode_micex_of_data_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                    indexed_micex_of_market_view_manager_.get());
                shortcode_micex_of_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_micex_of_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(
                    shortcode_micex_of_data_filesource_map_[shortcode_].get());
              }
            } else {
              if (shortcode_micex_data_filesource_map_.find(shortcode_) == shortcode_micex_data_filesource_map_.end()) {
                shortcode_micex_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::MICEXLoggedMessageFileSource>(new HFSAT::MICEXLoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                shortcode_micex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                    indexed_micex_market_view_manager_.get());
                shortcode_micex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_micex_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_micex_data_filesource_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceJPY: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager) {
              shortcode_ose_itch_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::OSEItchLoggedMessageFileSource>(
                      new HFSAT::OSEItchLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                                exchange_symbol_, dep_trading_location_, false));
              shortcode_ose_itch_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_ose_itch_data_filesource_map_[shortcode_]->AddOSEListener(ose_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_ose_itch_data_filesource_map_[shortcode_].get());
            }

            // ose dependant shortcode OR CME dependant , use order-level data for ALL ose sources.
            if (dep_trading_location_ == HFSAT::kTLocJPY || dep_trading_location_ == HFSAT::kTLocCHI ||
                dep_trading_location_ == HFSAT::kTLocFR2 || dep_trading_location_ == HFSAT::kTLocSYD ||
                dep_trading_location_ == HFSAT::kTLocNSE || dep_trading_location_ == HFSAT::kTLocSPR ||
                tradingdate_ >= USING_OSE_ITCH_FROM) {
              if (tradingdate_ >= USING_OSE_ITCH_FROM) {
                if (using_OSE_OF_book_ &&
                    dep_trading_location_ == HFSAT::kTLocJPY) {  // using order_feed_book for OSE servers
                  if (shortcode_ose_itch_data_filesource_map_.find(shortcode_) ==
                      shortcode_ose_itch_data_filesource_map_.end()) {
                    shortcode_ose_itch_data_filesource_map_[shortcode_] =
                        std::unique_ptr<HFSAT::OSEItchLoggedMessageFileSource>(
                            new HFSAT::OSEItchLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                      secid, exchange_symbol_, dep_trading_location_,
                                                                      use_fake_faster_data_));
                    shortcode_ose_itch_data_filesource_map_[shortcode_]->AddOSEListener(
                        indexed_ose_order_feed_market_view_manager_.get());
                    shortcode_ose_itch_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                    shortcode_filesource_map_[shortcode_] = shortcode_ose_itch_data_filesource_map_[shortcode_].get();
                    historical_dispatcher_.AddExternalDataListener(
                        shortcode_ose_itch_data_filesource_map_[shortcode_].get());

                    using_ose_ol_hist_data_ = false;
                  }
                } else {  // using price_feed_book for non-OSE servers
                  if (shortcode_ose_pf_data_filesource_map_.find(shortcode_) ==
                      shortcode_ose_pf_data_filesource_map_.end()) {
                    shortcode_ose_pf_data_filesource_map_[shortcode_] =
                        std::unique_ptr<HFSAT::OSEPFLoggedMessageFileSource>(new HFSAT::OSEPFLoggedMessageFileSource(
                            dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                            use_fake_faster_data_));
                    shortcode_ose_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                        indexed_ice_market_view_manager_.get());
                    shortcode_ose_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                    shortcode_filesource_map_[shortcode_] = shortcode_ose_pf_data_filesource_map_[shortcode_].get();
                    historical_dispatcher_.AddExternalDataListener(
                        shortcode_ose_pf_data_filesource_map_[shortcode_].get());

                    using_ose_ol_hist_data_ = false;
                  }
                }
              } else {
                if (shortcode_ose_data_filesource_map_.find(shortcode_) == shortcode_ose_data_filesource_map_.end()) {
                  shortcode_ose_data_filesource_map_[shortcode_] =
                      std::unique_ptr<HFSAT::OSEPriceFeedLoggedMessageFileSource>(
                          new HFSAT::OSEPriceFeedLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                         secid, exchange_symbol_, dep_trading_location_,
                                                                         use_fake_faster_data_));
                  shortcode_ose_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                      indexed_ose_price_feed_market_view_manager_.get());

                  shortcode_ose_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                  shortcode_filesource_map_[shortcode_] = shortcode_ose_data_filesource_map_[shortcode_].get();
                  historical_dispatcher_.AddExternalDataListener(shortcode_ose_data_filesource_map_[shortcode_].get());

                  using_ose_ol_hist_data_ = false;
                }
              }
            } else {  // dep shortcode is NOT ose , use L1 price-level sources for all.
              if (shortcode_ose_l1_data_filesource_map_.find(shortcode_) ==
                  shortcode_ose_l1_data_filesource_map_.end()) {
                // For OSE_L1 unconditionally mark as not using indexed book
                sid_to_smv_ptr_map_[secid]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ =
                    false;

                shortcode_ose_l1_data_filesource_map_[shortcode_] =
                    std::unique_ptr<HFSAT::OSEL1LoggedMessageFileSource>(new HFSAT::OSEL1LoggedMessageFileSource(
                        dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                        use_fake_faster_data_));
                shortcode_ose_l1_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                    ose_l1_price_market_view_manager_.get());
                shortcode_ose_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
                shortcode_filesource_map_[shortcode_] = shortcode_ose_l1_data_filesource_map_[shortcode_].get();
                historical_dispatcher_.AddExternalDataListener(shortcode_ose_l1_data_filesource_map_[shortcode_].get());
              }
            }
          }
        } break;
        case HFSAT::kExchSourceHONGKONG: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_hkex_data_filesource_map_.find(shortcode_) == shortcode_hkex_data_filesource_map_.end()) {
              shortcode_hkex_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::HKEXLoggedMessageFileSource>(new HFSAT::HKEXLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              if (sim_smv_required_) {
                shortcode_hkex_data_filesource_map_[shortcode_]->SetHKHalfBookGlobalListener(
                    hkex_sim_market_view_manager_.get());
              }
              shortcode_hkex_data_filesource_map_[shortcode_]->SetHKHalfBookGlobalListener(
                  hkex_market_view_manager_.get());
              shortcode_hkex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_hkex_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_hkex_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceCFE: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_cfe_data_filesource_map_.find(shortcode_) == shortcode_cfe_data_filesource_map_.end()) {
              shortcode_cfe_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::CFELoggedMessageFileSource>(new HFSAT::CFELoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              shortcode_cfe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_cfe_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  indexed_cfe_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_cfe_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_cfe_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceASX: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager) {
              shortcode_asx_itch_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::ASXItchLoggedMessageFileSource>(
                      new HFSAT::ASXItchLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                                exchange_symbol_, dep_trading_location_, false));
              shortcode_asx_itch_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_asx_itch_data_filesource_map_[shortcode_]->AddASXListener(asx_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_asx_itch_data_filesource_map_[shortcode_].get());
            }

            if (shortcode_asx_pf_data_filesource_map_.find(shortcode_) == shortcode_asx_pf_data_filesource_map_.end()) {
              shortcode_asx_pf_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::ASXPFLoggedMessageFileSource>(new HFSAT::ASXPFLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              shortcode_asx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_asx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                  indexed_ice_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_asx_pf_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_asx_pf_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceSGX: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (initialize_market_order_manager) {
              shortcode_sgx_of_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::SGXOrderLoggedMesageFileSource>(
                      new HFSAT::SGXOrderLoggedMesageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                                exchange_symbol_, dep_trading_location_, false));
              shortcode_sgx_of_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_sgx_of_data_filesource_map_[shortcode_]->AddSGXListener(sgx_market_order_manager_.get());
              historical_dispatcher_.AddExternalDataListener(shortcode_sgx_of_data_filesource_map_[shortcode_].get());
            }

            if (shortcode_sgx_pf_data_filesource_map_.find(shortcode_) == shortcode_sgx_pf_data_filesource_map_.end()) {
              shortcode_sgx_pf_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::SGXPFLoggedMessageFileSource>(new HFSAT::SGXPFLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              shortcode_sgx_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_sgx_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                  indexed_ice_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_sgx_pf_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_sgx_pf_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
/*
        case HFSAT::kExchSourceBSE: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_bse_pf_data_filesource_map_.find(shortcode_) == shortcode_bse_pf_data_filesource_map_.end()) {
              shortcode_bse_pf_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::BSEPFLoggedMessageFileSource>(new HFSAT::BSEPFLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      use_fake_faster_data_));
              shortcode_bse_pf_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_bse_pf_data_filesource_map_[shortcode_]->AddPriceLevelGlobalListener(
                  indexed_ice_market_view_manager_.get());
              shortcode_filesource_map_[shortcode_] = shortcode_bse_pf_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_bse_pf_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
*/
        case HFSAT::kExchSourceHKOMD: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_hkomd_filesource_map_.find(shortcode_) == shortcode_hkomd_filesource_map_.end()) {
              shortcode_hkomd_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::HKOMDLoggedMessageFileSource>(new HFSAT::HKOMDLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      false, use_fake_faster_data_, IsHKEquity(shortcode_)));
              shortcode_hkomd_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                  indexed_hkomd_market_view_manager_.get());
              shortcode_hkomd_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_hkomd_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_hkomd_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceHKOMDCPF:
        case HFSAT::kExchSourceHKOMDPF: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_hkomd_pricefeed_filesource_map_.find(shortcode_) ==
                shortcode_hkomd_pricefeed_filesource_map_.end()) {
              shortcode_hkomd_pricefeed_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::HKOMDPFLoggedMessageFileSource>(new HFSAT::HKOMDPFLoggedMessageFileSource(
                      dbglogger_, sec_name_indexer_, tradingdate_, secid, exchange_symbol_, dep_trading_location_,
                      false, use_fake_faster_data_, IsHKEquity(shortcode_)));
              shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  hkomd_price_level_market_view_manager_.get());
              shortcode_hkomd_pricefeed_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_hkomd_pricefeed_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(
                  shortcode_hkomd_pricefeed_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceBATSCHI: {
          if (sid_to_marketdata_needed_map_[secid]) {
            if (shortcode_chix_l1_data_filesource_map_.find(shortcode_) ==
                shortcode_chix_l1_data_filesource_map_.end()) {
              shortcode_chix_l1_data_filesource_map_[shortcode_] =
                  std::unique_ptr<HFSAT::CHIXL1LoggedMessageFileSource>(
                      new HFSAT::CHIXL1LoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, secid,
                                                               exchange_symbol_, dep_trading_location_, false));
              shortcode_chix_l1_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  l1_price_market_view_manager_.get());
              shortcode_chix_l1_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              shortcode_filesource_map_[shortcode_] = shortcode_chix_l1_data_filesource_map_[shortcode_].get();
              historical_dispatcher_.AddExternalDataListener(shortcode_chix_l1_data_filesource_map_[shortcode_].get());
            }
          }
        } break;
        case HFSAT::kExchSourceMEFF:
        case HFSAT::kExchSourceIDEM:
        case HFSAT::kExchSourceREUTERS:
        default: { } break; }
    }  // for loop ends

    // Setting up PromOderManager -> ORS File Subscription
    for (unsigned int sid_ = 0; sid_ < sec_name_indexer_.NumSecurityId(); sid_++) {
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(sid_);

      // specially for the sources on which ORS data is needed for indicators and
      // hence promordermanager needs to listen to ors file / livesource
      if (sid_to_ors_needed_map_[sid_]) {
        // if PromOrderManager setup for this security ... as should be ... then attach it as listener to ORS source
        if (sid_to_prom_order_manager_map_[sid_] != NULL) {
          HFSAT::ORSMessageFileSource *p_ors_message_filesource_ =
              shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);
          if (p_ors_message_filesource_ != NULL) {
            // shortcode_ors_data_filesource_map_ [ shortcode_ ]->AddOrderNotFoundListener (
            // sid_to_prom_order_manager_map_ [ sid_ ] ); // PromOrderManager does not need to listen to reply of
            // NotFound
            // from client replay requests ?
            p_ors_message_filesource_->AddOrderSequencedListener(sid_to_prom_order_manager_map_[sid_]);
            p_ors_message_filesource_->AddOrderConfirmedListener(sid_to_prom_order_manager_map_[sid_]);
            p_ors_message_filesource_->AddOrderConfCxlReplacedListener(sid_to_prom_order_manager_map_[sid_]);
            p_ors_message_filesource_->AddOrderCanceledListener(sid_to_prom_order_manager_map_[sid_]);
            p_ors_message_filesource_->AddOrderExecutedListener(sid_to_prom_order_manager_map_[sid_]);
            // shortcode_ors_data_filesource_map_ [ shortcode_ ]->AddOrderRejectedListener (
            // sid_to_prom_order_manager_map_ [ sid_ ] ); // PromOrderManager does not need to listen to reply of
            // Reject
            // to client sendtrade messages which fail
          }
        }
      }
    }
  }  // Init ends

  void SetToActualSeek(bool _actual_skip_){

    if( tradingdate_ >= USE_UNCONVERTED_DATA_DATE){
      for(auto & itr : shortcode_nse_data_filesource_map2_){
        (itr.second)->SetActualDataSkip(_actual_skip_);
      }
      return;
    }

    for(auto & itr : shortcode_nse_data_filesource_map_){
      (itr.second)->SetActualDataSkip(_actual_skip_);
    }
  }


  void Seek(HFSAT::ttime_t seek_time = HFSAT::ttime_t(0, 0)) {
    // If seek time is given as proper input use that, else use the date, hhmm given as input
    if (seek_time.tv_sec > 0) {
      historical_dispatcher_.SeekHistFileSourcesTo(seek_time);
      if (HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time) != tradingdate_) {
        watch_.ResetWatch(HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time));
      }
      return;
    }
#define MINUTES_TO_PREP 30
    // To only process data starting MINUTES_TO_PREP minutes before datagen_start_utc_hhmm_
    int event_process_start_utc_hhmm_ = datagen_start_utc_hhmm_;
    {
      event_process_start_utc_hhmm_ =
          ((event_process_start_utc_hhmm_ / 100) * 60) + (event_process_start_utc_hhmm_ % 100);
      event_process_start_utc_hhmm_ = std::max(0, event_process_start_utc_hhmm_ - MINUTES_TO_PREP);
      event_process_start_utc_hhmm_ =
          (event_process_start_utc_hhmm_ % 60) + ((event_process_start_utc_hhmm_ / 60) * 100);
    }
#undef MINUTES_TO_PREP

    if (using_ose_ol_hist_data_) {
      // when using ose-ol data , start from start of day ,
      // to increase odds of having most correct book at trading-start-time.
      // also avoid rollover from midnight by starting after 0000 utc.
      historical_dispatcher_.SeekHistFileSourcesTo(
          HFSAT::ttime_t(watch_.last_midnight_sec() + 10, 0));  // 10 seconds after midnight
    } else {
      // In all the file sources, read events till
      // we reach the first event after the specified ttime_t
      historical_dispatcher_.SeekHistFileSourcesTo(HFSAT::ttime_t(
          HFSAT::DateTime::GetTimeUTC(global_datagen_start_utc_yymmdd_, event_process_start_utc_hhmm_), 0));
    }

    if (global_datagen_start_utc_yymmdd_ != tradingdate_) {
      watch_.ResetWatch(global_datagen_start_utc_yymmdd_);
    }
  }

  void Run(HFSAT::ttime_t end_time = HFSAT::ttime_t(0, 0)) {
    //std::cout << "RUN" << std::endl;
    // // start event loop
    try {
      if (livetrading_) {
        simple_live_dispatcher_.RunLive();
      } else {
	//std::cout << "ELSE" << std::endl;
        if (end_time.tv_sec > 0) {
	  //std::cout << "IF " << end_time.tv_sec << std::endl;
          historical_dispatcher_.RunHist(end_time);
        } else {
          if (global_datagen_end_utc_yymmdd_ == -1 || datagen_end_utc_hhmm_ == -1) {
	    //std::cout << "historical_dispatcher_.RunHist()" << std::endl;
            historical_dispatcher_.RunHist();
          } else {
	    //std::cout << "historical_dispatcher_.RunHist(HFSAT)" << std::endl;
            historical_dispatcher_.RunHist(
                HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(global_datagen_end_utc_yymmdd_, datagen_end_utc_hhmm_), 0));
          }
        }
      }
    } catch (int e) {
      std::cerr << "Unable to run historical dispatcher\n";
    }

    historical_dispatcher_.DeleteSources();
  }

  HFSAT::SecurityMarketView *getSMV() { return sid_to_smv_ptr_map_[0]; }

  HFSAT::SecurityMarketViewPtrVec &getSMVMap() { return sid_to_smv_ptr_map_; }

  std::vector<HFSAT::MarketOrdersView *> &getMOVMap() { return sid_to_mov_ptr_map; }

  HFSAT::SecurityMarketViewPtrVec &getSimSMVMap() { return sid_to_sim_smv_ptr_map_; }

  HFSAT::SimTimeSeriesInfo &getSimTimeSeriesInfo() { return *sim_time_series_info_; }

  unsigned int getNumSecId() { return num_sec_id_; }

  HFSAT::NetworkAccountInfoManager &getNetworkAccountInfoManager() { return *network_account_info_manager_; }

  HFSAT::SpreadMarketView *getSpreadMV(std::string structure_shc_) {
    HFSAT::SpreadMarketView *p_spread_market_view_ = new HFSAT::SpreadMarketView(dbglogger_, watch_, structure_shc_);
    return p_spread_market_view_;
  }

  HFSAT::TradingLocation_t getDepTradingLocation() { return dep_trading_location_; }

  HFSAT::HistoricalDispatcher &getHistoricalDispatcher() { return historical_dispatcher_; }

  std::vector<bool> &getSidORSNeededMap() { return sid_to_ors_needed_map_; }

  HFSAT::Watch &getWatch() { return watch_; }

  HFSAT::DebugLogger &getLogger() { return dbglogger_; }

  const char *getExchangleSymbol() { return exchange_symbol_vec_[0]; }

  std::map<std::string, HFSAT::ExternalDataListener *> &getShortcodeFilesourceMap() {
    return shortcode_filesource_map_;
  }

  void cleanup() {
    historical_dispatcher_.DeleteSources();
    for (auto i = 0u; i < sid_to_smv_ptr_map_.size(); i++) {
      if (sid_to_smv_ptr_map_[i] != NULL) {
        // std::cerr << "deleting " << sid_to_smv_ptr_map_[i]->shortcode() << "\n";
        delete (sid_to_smv_ptr_map_[i]);
      }
    }
    // this points to static reference !
    sid_to_smv_ptr_map_.clear();

    for (auto i = 0u; i < sid_to_sim_smv_ptr_map_.size(); i++) {
      if (sid_to_sim_smv_ptr_map_[i] != NULL) {
        // std::cerr << "deleting " << sid_to_sim_smv_ptr_map_[i]->shortcode() << "\n";
        delete (sid_to_sim_smv_ptr_map_[i]);
      }
    }
    sid_to_sim_smv_ptr_map_.clear();


    /*HFSAT::SyntheticSecurityManager
    HFSAT::SecurityDefinitions
    HFSAT::ExchangeSymbolManager
    HFSAT::SecurityNameIndexer
    HFSAT::ShortcodeSecurityMarketViewMap
    HFSAT::ShortcodePromOrderManagerMap
    HFSAT::PromOrderManager
    HFSAT::ShortcodeORSMessageFilesourceMap*/
    HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
    sec_name_indexer_.Clear();

    for (auto i = 0u; i < sid_to_mov_ptr_map.size(); i++) {
      if (sid_to_mov_ptr_map[i] != nullptr) {
        delete sid_to_mov_ptr_map[i];
      }
    }

    if (!network_account_info_manager_) {
      delete network_account_info_manager_;
    }
    if (!sim_time_series_info_) {
      delete sim_time_series_info_;
    }
  }

  HFSAT::IndexedTMXOBFOFMarketViewManager *indexed_tmx_obf_of_market_view_manager() {
    return indexed_tmx_obf_of_market_view_manager_.get();
  }

  HFSAT::IndexedEobiMarketViewManager *indexed_eobi_market_view_manager() {
    return indexed_eobi_market_view_manager_.get();
  }
  HFSAT::IndexedEobiPriceLevelMarketViewManager *indexed_eobi_price_level_market_view_manager() {
    return indexed_eobi_price_level_market_view_manager_.get();
  }

  HFSAT::IndexedFpgaMarketViewManager *indexed_fpga_market_view_manager() {
    return indexed_fpga_market_view_manager_.get();
  }

  HFSAT::IndexedLiffePriceLevelMarketViewManager *indexed_liffe_price_level_market_view_manager() {
    return indexed_liffe_price_level_market_view_manager_.get();
  }

  HFSAT::IndexedRtsMarketViewManager *indexed_rts_market_view_manager() {
    return indexed_rts_market_view_manager_.get();
  }

  HFSAT::IndexedRtsOFMarketViewManager *indexed_rts_of_market_view_manager() {
    return indexed_rts_of_market_view_manager_.get();
  }

  HFSAT::IndexedNtpMarketViewManager *indexed_ntp_market_view_manager() {
    return indexed_ntp_market_view_manager_.get();
  }

  HFSAT::IndexedNtpMarketViewManager *indexed_puma_market_view_manager() {
    return indexed_puma_market_view_manager_.get();
  }

  HFSAT::IndexedCmeMarketViewManager *indexed_cme_market_view_manager() {
    return indexed_cme_market_view_manager_.get();
  }

  HFSAT::IndexedMicexMarketViewManager *indexed_micex_market_view_manager() {
    return indexed_micex_market_view_manager_.get();
  }

  HFSAT::IndexedMicexOFMarketViewManager *indexed_micex_of_market_view_manager() {
    return indexed_micex_of_market_view_manager_.get();
  }

  HFSAT::IndexedOsePriceFeedMarketViewManager *indexed_ose_price_feed_market_view_manager() {
    return indexed_ose_price_feed_market_view_manager_.get();
  }

  HFSAT::IndexedOseOrderFeedMarketViewManager *indexed_ose_order_feed_market_view_manager() {
    return indexed_ose_order_feed_market_view_manager_.get();
  }

  HFSAT::OSEL1PriceMarketViewManager *ose_l1_price_market_view_manager() {
    return ose_l1_price_market_view_manager_.get();
  }

  HFSAT::IndexedCfeMarketViewManager *indexed_cfe_market_view_manager() {
    return indexed_cfe_market_view_manager_.get();
  }

  HFSAT::HKEXIndexedMarketViewManager *hkex_market_view_manager() { return hkex_market_view_manager_.get(); }

  HFSAT::IndexedTmxMarketViewManager *indexed_tmx_market_view_manager() {
    return indexed_tmx_market_view_manager_.get();
  }

  HFSAT::IndexedIceMarketViewManager *indexed_ice_market_view_manager() {
    return indexed_ice_market_view_manager_.get();
  }

  HFSAT::IndexedAsxMarketViewManager *indexed_asx_market_view_manager() {
    return indexed_asx_market_view_manager_.get();
  }

  HFSAT::IndexedHKOMDPriceLevelMarketViewManager *hkomd_price_level_market_view_manager() {
    return hkomd_price_level_market_view_manager_.get();
  }

  HFSAT::BMFOrderLevelMarketViewManager *bmf_order_level_market_view_manager() {
    return bmf_order_level_market_view_manager_.get();
  }

  HFSAT::HKOMDIndexedMarketViewManager *indexed_hkomd_market_view_manager() {
    return indexed_hkomd_market_view_manager_.get();
  }

  HFSAT::IndexedNSEMarketViewManager2 *indexed_nse_market_view_manager2() {
    return indexed_nse_market_view_manager2_.get();
  }

  HFSAT::L1PriceMarketViewManager *indexed_l1_price_market_view_manager() {
    return l1_price_market_view_manager_.get();
  }

  HFSAT::GenericL1DataMarketViewManager *generic_l1_data_market_view_manager() {
    return generic_l1_data_market_view_manager_.get();
  }

  HFSAT::IndexedTmxMarketViewManager *indexed_krx_market_view_manager() {
    return indexed_krx_market_view_manager_.get();
  }

  HFSAT::IndexedLiffePriceLevelMarketViewManager *indexed_liffe_sim_price_level_market_view_manager() {
    return indexed_liffe_sim_price_level_market_view_manager_.get();
  }

  HFSAT::IndexedRtsMarketViewManager *indexed_rts_sim_market_view_manager() {
    return indexed_rts_sim_market_view_manager_.get();
  }

  HFSAT::HKEXIndexedMarketViewManager *hkex_sim_market_view_manager() { return hkex_sim_market_view_manager_.get(); }

  HFSAT::IndexedTmxMarketViewManager *indexed_tmx_sim_market_view_manager() {
    return indexed_tmx_sim_market_view_manager_.get();
  }
  HFSAT::IndexedIceMarketViewManager *indexed_ice_sim_market_view_manager() {
    return indexed_ice_sim_market_view_manager_.get();
  }
  HFSAT::NSELoggedMessageFileSource *nse_logged_message_filesource(std::string _shortcode_) {
    if (shortcode_nse_data_filesource_map_.find(_shortcode_) == shortcode_nse_data_filesource_map_.end()) {
      return nullptr;
    } else {
      return shortcode_nse_data_filesource_map_[_shortcode_].get();
    }
  }

  HFSAT::NSELoggedMessageFileSource2 *nse_logged_message_filesource2(std::string _shortcode_) {
    if (shortcode_nse_data_filesource_map2_.find(_shortcode_) == shortcode_nse_data_filesource_map2_.end()) {
      return nullptr;
    } else {
      return shortcode_nse_data_filesource_map2_[_shortcode_].get();
    }
  }

  HFSAT::BSELoggedMessageFileSource *bse_logged_message_filesource(std::string _shortcode_) {
    if (shortcode_bse_data_filesource_map_.find(_shortcode_) == shortcode_bse_data_filesource_map_.end()) {
      return nullptr;
    } else {
      return shortcode_bse_data_filesource_map_[_shortcode_].get();
    }
  }

  ~CommonSMVSource() { cleanup(); };
};
