#pragma once
#include "basetrade/InitLogic/sim_strategy.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
// base_port_trading support
#include "dvctrade/InitCommon/port_exec_paramset.hpp"
#include "dvctrade/Signals/base_signal.hpp"
#include "dvctrade/Signals/signal_creator.hpp"

class SimStrategyHelper {
 public:
  SimStrategyHelper() {}

  // returns true if given shortcode corresponds to an HK equity
  bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

  // Checks if any of the instruments are HK stocks and if so adds HK stocks contract specs
  /*void CheckAndAddHKStocksDefinitions(std::vector<std::string>& shortcode_vec) {
    bool is_hk_equities_present = false;
    for (auto i = 0u; i < shortcode_vec.size(); i++) {
      if (IsHKEquity(shortcode_vec[i])) {
        is_hk_equities_present = true;
      }
    }
    if (is_hk_equities_present) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadHKStocksSecurityDefinitions();
    }
  }*/

  HFSAT::BaseTrader* GetBaseTrader(HFSAT::NetworkAccountInfoManager& network_account_manager,
                                   HFSAT::SecurityMarketView* dep_smv, HFSAT::BaseSimMarketMaker* smm) {
    assert(dep_smv != nullptr && smm != nullptr);

    // This is useless now
    // std::string trade_info = network_account_manager.GetDepTradeAccount(dep_smv->exch_source(),
    // dep_smv->shortcode());
    // TODO: remove arg from base trader itself

    std::string trade_info = "nullptr";
    return HFSAT::SimTraderHelper::GetSimTrader(trade_info, smm);
  }

  void SetShortcodeVectorsForStructuredStrategies(
      HFSAT::StrategyDesc& strategy_desc_, HFSAT::DebugLogger& dbglogger_, HFSAT::Watch& watch_,
      std::vector<std::string>& source_shortcode_vec_, std::vector<std::string>& ors_needed_by_indicators_vec_,
      std::vector<std::string>& dependant_shortcode_vec_, std::string& offline_mix_mms_wts_filename_,
      std::string& online_mix_price_consts_filename_, std::string& online_beta_kalman_consts_filename_,
      std::map<std::string, std::vector<std::string>>& modelfilename_ors_needed_by_indicators_vec_map_,
      std::map<std::string, std::vector<std::string>>& modelfilename_source_shortcode_vec_map_, int tradingdate_,
      std::map<int, vector<std::string>>& struct_strat_source_shortcode_vec_map_) {
    for (auto i = 0u; i < strategy_desc_.structured_strategy_vec_.size(); i++) {
      std::vector<string> shc_vec_;
      std::vector<string> dummy_vec_;
      HFSAT::VectorUtils::UniqueVectorAdd(dependant_shortcode_vec_,
                                          strategy_desc_.structured_strategy_vec_[i].shortcodes_);
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_,
                                          strategy_desc_.structured_strategy_vec_[i].shortcodes_);
      HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_,
                                          strategy_desc_.structured_strategy_vec_[i].shortcodes_);

      HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_, strategy_desc_.structured_strategy_vec_[i].shortcodes_);

      // Adding futures shortcode in case of base option trading
      if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(HFSAT::BaseOTrading::StrategyName()) == 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(dependant_shortcode_vec_,
                                            strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_,
                                            strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_,
                                            strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_, strategy_desc_.structured_strategy_vec_[i].trading_structure_);
      }

      std::map<std::string, std::vector<std::string>> shortcode_to_modelfile_vec_ =
          strategy_desc_.structured_strategy_vec_[i].shortcode_to_modelfile_vec_;

      for (unsigned int j = 0; j < strategy_desc_.structured_strategy_vec_[i].shortcodes_.size(); j++) {
        std::vector<std::string> modelfile_vec_ =
            shortcode_to_modelfile_vec_[strategy_desc_.structured_strategy_vec_[i].shortcodes_[j]];

        for (unsigned int k = 0; k < modelfile_vec_.size(); k++) {
          HFSAT::ModelCreator::CollectShortCodes(
              dbglogger_, watch_, modelfile_vec_[k], modelfilename_source_shortcode_vec_map_[modelfile_vec_[k]],
              modelfilename_ors_needed_by_indicators_vec_map_[modelfile_vec_[k]], false);

          std::string t_offline_mix_mms_wts_filename_ = HFSAT::ModelCreator::GetOfflineMixMMSWtsFileName(
              modelfile_vec_[k], strategy_desc_.structured_strategy_vec_[i].trading_start_utc_mfm_,
              strategy_desc_.structured_strategy_vec_[i].shortcodes_[0]);
          std::string t_online_mix_price_consts_filename_ = HFSAT::ModelCreator::GetOnlinePriceConstFilename(
              modelfile_vec_[k], strategy_desc_.structured_strategy_vec_[i].trading_start_utc_mfm_,
              strategy_desc_.structured_strategy_vec_[i].shortcodes_[0]);
          std::string t_online_beta_kalman_consts_filename_ = HFSAT::ModelCreator::GetOnlineBetaKalmanConstFilename(
              modelfile_vec_[k], strategy_desc_.structured_strategy_vec_[i].trading_start_utc_mfm_,
              strategy_desc_.structured_strategy_vec_[i].shortcodes_[0]);

          if (offline_mix_mms_wts_filename_ == "INVALIDFILE") {
            offline_mix_mms_wts_filename_ = t_offline_mix_mms_wts_filename_;
          } else if (offline_mix_mms_wts_filename_ != t_offline_mix_mms_wts_filename_) {
            HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
          }

          if (online_mix_price_consts_filename_ == "INVALIDFILE") {
            online_mix_price_consts_filename_ = t_online_mix_price_consts_filename_;
          } else if (online_mix_price_consts_filename_ != t_online_mix_price_consts_filename_) {
            HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
          }

          if (online_beta_kalman_consts_filename_ == "INVALIDFILE") {
            online_beta_kalman_consts_filename_ = t_online_beta_kalman_consts_filename_;
          } else if (online_beta_kalman_consts_filename_ != t_online_beta_kalman_consts_filename_) {
            HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
          }

          if (HFSAT::CurveUtils::IsSpreadStrategy(strategy_desc_.structured_strategy_vec_[i].strategy_name_)) {
            HFSAT::VectorUtils::UniqueVectorAdd(modelfilename_source_shortcode_vec_map_[modelfile_vec_[k]],
                                                dependant_shortcode_vec_);
            HFSAT::VectorUtils::UniqueVectorAdd(modelfilename_ors_needed_by_indicators_vec_map_[modelfile_vec_[k]],
                                                dependant_shortcode_vec_);
          }

          HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_,
                                              modelfilename_source_shortcode_vec_map_[modelfile_vec_[k]]);
          HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_,
                                              modelfilename_ors_needed_by_indicators_vec_map_[modelfile_vec_[k]]);
          HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_, modelfilename_source_shortcode_vec_map_[modelfile_vec_[k]]);
        }

        if ((strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(HFSAT::BaseOTrading::StrategyName()) !=
             0)) {
          HFSAT::BaseTrading::CollectORSShortCodes(dbglogger_,
                                                   strategy_desc_.structured_strategy_vec_[i].strategy_name_,
                                                   strategy_desc_.structured_strategy_vec_[i].shortcodes_[j],
                                                   source_shortcode_vec_, ors_needed_by_indicators_vec_);
          HFSAT::BaseTrading::CollectORSShortCodes(
              dbglogger_, strategy_desc_.structured_strategy_vec_[i].strategy_name_,
              strategy_desc_.structured_strategy_vec_[i].shortcodes_[j], shc_vec_, dummy_vec_);
        }
      }
      struct_strat_source_shortcode_vec_map_[i] = shc_vec_;
    }
  }

  void SetShortcodeVectorsForStrategies(
      HFSAT::StrategyDesc& strategy_desc_, HFSAT::DebugLogger& dbglogger_, HFSAT::Watch& watch_,
      std::vector<std::string>& source_shortcode_vec_, std::vector<std::string>& ors_needed_by_indicators_vec_,
      std::vector<std::string>& dependant_shortcode_vec_, std::string& offline_mix_mms_wts_filename_,
      std::string& online_mix_price_consts_filename_, std::string& online_beta_kalman_consts_filename_,
      std::map<std::string, std::vector<std::string>>& modelfilename_ors_needed_by_indicators_vec_map_,
      std::map<std::string, std::vector<std::string>>& modelfilename_source_shortcode_vec_map_, int tradingdate_,
      std::map<int, vector<std::string>>& strat_source_shortcode_vec_map_) {
    for (auto i = 0u; i < strategy_desc_.strategy_vec_.size(); i++) {
      std::vector<string> shc_vec_;
      std::vector<string> dummy_vec_;
      if (modelfilename_source_shortcode_vec_map_.find(strategy_desc_.strategy_vec_[i].modelfilename_) ==
          modelfilename_source_shortcode_vec_map_.end()) {
        HFSAT::VectorUtils::UniqueVectorAdd(dependant_shortcode_vec_, strategy_desc_.strategy_vec_[i].dep_shortcode_);
        HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, strategy_desc_.strategy_vec_[i].dep_shortcode_);

        HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_, strategy_desc_.strategy_vec_[i].dep_shortcode_);

        // Adding NKM whenever NK is there, since NikPricePair will be mixed with other strategies
        if (strategy_desc_.strategy_vec_[i].dep_shortcode_.compare("NK_0") == 0) {
          HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_0"));
          HFSAT::VectorUtils::UniqueVectorAdd(dependant_shortcode_vec_, std::string("NKM_0"));
          HFSAT::VectorUtils::UniqueVectorAdd(ors_needed_by_indicators_vec_, std::string("NKM_0"));

          HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_, std::string("NKM_0"));
        }

        HFSAT::ModelCreator::CollectShortCodes(
            dbglogger_, watch_, strategy_desc_.strategy_vec_[i].modelfilename_,
            modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
            modelfilename_ors_needed_by_indicators_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
            false);  // allow_non_dependant_ = false

        std::string cancellation_model_file_ =
            HFSAT::ExecLogicUtils::getCancellationModel(strategy_desc_.strategy_vec_[i].dep_shortcode_, tradingdate_,
                                                        strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
                                                        strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_);
        if (cancellation_model_file_ != "")
          HFSAT::ModelCreator::CollectShortCodes(
              dbglogger_, watch_, cancellation_model_file_,
              modelfilename_source_shortcode_vec_map_[cancellation_model_file_],
              modelfilename_ors_needed_by_indicators_vec_map_[cancellation_model_file_], false);

        std::string t_offline_mix_mms_wts_filename_ = HFSAT::ModelCreator::GetOfflineMixMMSWtsFileName(
            strategy_desc_.strategy_vec_[i].modelfilename_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.strategy_vec_[i].dep_shortcode_);
        std::string t_online_mix_price_consts_filename_ = HFSAT::ModelCreator::GetOnlinePriceConstFilename(
            strategy_desc_.strategy_vec_[i].modelfilename_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.strategy_vec_[i].dep_shortcode_);
        std::string t_online_beta_kalman_consts_filename_ = HFSAT::ModelCreator::GetOnlineBetaKalmanConstFilename(
            strategy_desc_.strategy_vec_[i].modelfilename_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.strategy_vec_[i].dep_shortcode_);

        if (offline_mix_mms_wts_filename_ == "INVALIDFILE") {
          offline_mix_mms_wts_filename_ = t_offline_mix_mms_wts_filename_;
        } else if (offline_mix_mms_wts_filename_ != t_offline_mix_mms_wts_filename_) {
          HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
        }

        if (online_mix_price_consts_filename_ == "INVALIDFILE") {
          online_mix_price_consts_filename_ = t_online_mix_price_consts_filename_;
        } else if (online_mix_price_consts_filename_ != t_online_mix_price_consts_filename_) {
          HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
        }
        if (online_beta_kalman_consts_filename_ == "INVALIDFILE") {
          online_beta_kalman_consts_filename_ = t_online_beta_kalman_consts_filename_;
        } else if (online_beta_kalman_consts_filename_ != t_online_beta_kalman_consts_filename_) {
          HFSAT::ExitVerbose(HFSAT::kDifferentOFFLINEMIXMMS_FILE);
        }
        if (strategy_desc_.strategy_vec_[i].dep_shortcode_.compare("NK_0") == 0 &&
            strategy_desc_.strategy_vec_[i].strategy_name_.compare("NikPricePairBasedAggressiveTrading") == 0) {
          HFSAT::VectorUtils::UniqueVectorAdd(
              modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
              std::string("NKM_0"));
          HFSAT::VectorUtils::UniqueVectorAdd(
              modelfilename_ors_needed_by_indicators_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
              std::string("NKM_0"));
        }

        HFSAT::VectorUtils::UniqueVectorAdd(
            source_shortcode_vec_,
            modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
        HFSAT::VectorUtils::UniqueVectorAdd(
            ors_needed_by_indicators_vec_,
            modelfilename_ors_needed_by_indicators_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);

        HFSAT::VectorUtils::UniqueVectorAdd(
            shc_vec_, modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
        if (cancellation_model_file_ != "") {
          HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_,
                                              modelfilename_source_shortcode_vec_map_[cancellation_model_file_]);
          HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_,
                                              modelfilename_source_shortcode_vec_map_[cancellation_model_file_]);
        }
      } else {
        HFSAT::VectorUtils::UniqueVectorAdd(
            shc_vec_, modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
        std::string cancellation_model_file_ =
            HFSAT::ExecLogicUtils::getCancellationModel(strategy_desc_.strategy_vec_[i].dep_shortcode_, tradingdate_,
                                                        strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
                                                        strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_);
        if (cancellation_model_file_ != "")
          HFSAT::VectorUtils::UniqueVectorAdd(shc_vec_,
                                              modelfilename_source_shortcode_vec_map_[cancellation_model_file_]);
      }
      // Moving it here in case deps are different or we want to add different shc depending on the different param
      HFSAT::BaseTrading::CollectShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].paramfilename_,
                                            source_shortcode_vec_);
      HFSAT::BaseTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                               strategy_desc_.strategy_vec_[i].dep_shortcode_, source_shortcode_vec_,
                                               ors_needed_by_indicators_vec_);  // strictly speaking the ors sources
                                                                                // here aren't needed by indicators.
                                                                                // they are needed by risk managing code
      HFSAT::ArbTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                              strategy_desc_.strategy_vec_[i].dep_shortcode_, source_shortcode_vec_,
                                              ors_needed_by_indicators_vec_, dependant_shortcode_vec_);

      HFSAT::ArbTradingTodTom::CollectORSShortCodes(
          dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_, strategy_desc_.strategy_vec_[i].dep_shortcode_,
          source_shortcode_vec_, ors_needed_by_indicators_vec_, dependant_shortcode_vec_);

      HFSAT::SgxNK1MMTrading::CollectORSShortCodes(
          dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_, strategy_desc_.strategy_vec_[i].dep_shortcode_,
          source_shortcode_vec_, ors_needed_by_indicators_vec_, dependant_shortcode_vec_);

      HFSAT::DirectionalSyntheticTrading::CollectORSShortCodes(
          dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_, strategy_desc_.strategy_vec_[i].dep_shortcode_,
          source_shortcode_vec_, ors_needed_by_indicators_vec_, dependant_shortcode_vec_);

      HFSAT::VectorUtils::UniqueVectorAdd(
          ors_needed_by_indicators_vec_,
          HFSAT::ExecLogicUtils::GetSecondaryShortcode(strategy_desc_.strategy_vec_[i].dep_shortcode_));

      HFSAT::BaseTrading::CollectShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].paramfilename_, shc_vec_);
      HFSAT::BaseTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                               strategy_desc_.strategy_vec_[i].dep_shortcode_, shc_vec_, dummy_vec_);
      HFSAT::ArbTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                              strategy_desc_.strategy_vec_[i].dep_shortcode_, shc_vec_, dummy_vec_,
                                              dummy_vec_);

      HFSAT::ArbTradingTodTom::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                                    strategy_desc_.strategy_vec_[i].dep_shortcode_, shc_vec_,
                                                    dummy_vec_, dummy_vec_);

      HFSAT::SgxNK1MMTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                                   strategy_desc_.strategy_vec_[i].dep_shortcode_, shc_vec_, dummy_vec_,
                                                   dummy_vec_);

      HFSAT::DirectionalSyntheticTrading::CollectORSShortCodes(
          dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_, strategy_desc_.strategy_vec_[i].dep_shortcode_,
          shc_vec_, dummy_vec_, dummy_vec_);

      strat_source_shortcode_vec_map_[i] = shc_vec_;
    }
  }

  void SetDepComponentsStructuredStrategies(
      HFSAT::DebugLogger& dbglogger_, HFSAT::Watch& watch_, HFSAT::StrategyDesc& strategy_desc_,
      HFSAT::NetworkAccountInfoManager& network_account_info_manager_,
      HFSAT::HistoricalDispatcher& historical_dispatcher_,
      std::map<std::string, HFSAT::BaseSimMarketMaker*>& shortcode_to_smm_map,
      std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map_, CommonSMVSource* common_smv_source,
      HFSAT::EconomicEventsManager& economic_events_manager_, bool& livetrading_, unsigned int& market_model_index_,
      HFSAT::SimTimeSeriesInfo& sim_time_series_info_, HFSAT::MulticastSenderSocket* p_strategy_param_sender_socket_,
      HFSAT::MarketUpdateManager& market_update_manager_, HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_,
      HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_,
      HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_,
      HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_,
      std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*>& retail_logged_message_filesource_map_,
      std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*>& retail_manual_logged_message_filesource_map_,
      HFSAT::TradingLocation_t& dep_trading_location_, bool& ignore_user_msg_, int& tradingdate_,
      std::map<std::string, std::vector<std::string>>& modelfilename_source_shortcode_vec_map_,
      std::vector<int>& real_saci_vec_, std::map<int, vector<std::string>>& struct_strat_source_shortcode_vec_map_,
      HFSAT::SecurityNameIndexer& sec_name_indexer_, HFSAT::BulkFileWriter& trades_writer_) {
    HFSAT::SpreadTradingManager* spread_trading_manager_ = nullptr;
    HFSAT::SpreadMarketView* spread_market_view_ = nullptr;

    for (auto i = 0u; i < strategy_desc_.structured_strategy_vec_.size(); i++) {
      HFSAT::StructuredTradingManager* structured_trading_manager_ = nullptr;
      HFSAT::CurveTradingManager* curve_trading_manager_ = nullptr;
      HFSAT::RetailFlyTradingManager* retfly_trading_manager_ = nullptr;

      HFSAT::MultBasePNL* p_structured_sim_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);

      std::string strategy_name_ = strategy_desc_.structured_strategy_vec_[i].strategy_name_;
      std::string strategy_string_ = strategy_desc_.structured_strategy_vec_[i].strategy_string_;

      unsigned int runtime_id_ = strategy_desc_.structured_strategy_vec_[i].runtime_id_;
      unsigned int trading_start_utc_mfm_ = strategy_desc_.structured_strategy_vec_[i].trading_start_utc_mfm_;
      unsigned int trading_end_utc_mfm_ = strategy_desc_.structured_strategy_vec_[i].trading_end_utc_mfm_;

      if (HFSAT::CurveUtils::IsSpreadStrategy(strategy_name_)) {
        spread_market_view_ = new HFSAT::SpreadMarketView(
            dbglogger_, watch_, strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::ShortcodeSpreadMarketViewMap::AddEntry(strategy_desc_.structured_strategy_vec_[i].trading_structure_,
                                                      spread_market_view_);
        spread_trading_manager_ = new HFSAT::SpreadTradingManager(dbglogger_, watch_, *spread_market_view_);
        // strategy_desc_.structured_strategy_vec_[i].trading_manager_ = spread_trading_manager_;
      } else if (strategy_name_.compare(HFSAT::RiskBasedStructuredTrading::StrategyName()) == 0) {
        std::string base_stir_ = strategy_desc_.structured_strategy_vec_[0].trading_structure_;
        base_stir_ = base_stir_.substr(0, base_stir_.size() - 2);
        curve_trading_manager_ = new HFSAT::LFITradingManager(
            dbglogger_, watch_, strategy_desc_.structured_strategy_vec_[i].shortcodes_,
            strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, p_structured_sim_base_pnl_, base_stir_);
        strategy_desc_.structured_strategy_vec_[i].curve_trading_manager_ = curve_trading_manager_;
      } else if (HFSAT::StructuredGeneralTrading::IsStructuredGeneralTrading(strategy_string_)) {
        std::string base_stir_ = strategy_desc_.structured_strategy_vec_[0].trading_structure_;
        base_stir_ = base_stir_.substr(0, base_stir_.size() - 2);
        std::string trading_manager_ = HFSAT::StructuredGeneralTrading::GetTradingManager(strategy_string_);
        if (trading_manager_.compare("LFITradingManager") == 0) {
          curve_trading_manager_ = new HFSAT::LFITradingManager(
              dbglogger_, watch_, strategy_desc_.structured_strategy_vec_[i].shortcodes_,
              strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, p_structured_sim_base_pnl_, base_stir_);
        } else if (trading_manager_.compare("DI1TradingManager") == 0) {
          curve_trading_manager_ = new HFSAT::DI1TradingManager(
              dbglogger_, watch_, strategy_desc_.structured_strategy_vec_[i].shortcodes_, livetrading_,
              strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, p_structured_sim_base_pnl_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, base_stir_);
        }
        strategy_desc_.structured_strategy_vec_[i].curve_trading_manager_ = curve_trading_manager_;
      } else if (strategy_name_.compare(HFSAT::MinRiskPriceBasedAggressiveTrading::StrategyName()) == 0) {
        // min_risk_trading_manager_ = new HFSAT::MinRiskTradingManager(dbglogger_,watch_, p_structured_sim_base_pnl_,
        // strategy_desc_.structured_strategy_vec_[i].common_paramfilename_ );
        std::string base_stir_ = strategy_desc_.structured_strategy_vec_[0].trading_structure_;
        base_stir_ = base_stir_.substr(0, base_stir_.size() - 2);
        strategy_desc_.structured_strategy_vec_[i].min_risk_trading_manager_ = new HFSAT::MinRiskTradingManager(
            dbglogger_, watch_, p_structured_sim_base_pnl_,
            strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, livetrading_, base_stir_);
      } else if (strategy_name_.compare(HFSAT::EquityTrading2::StrategyName()) == 0) {
      } else if (strategy_name_.compare(HFSAT::PairsTrading::StrategyName()) == 0) {
      } else if (strategy_name_.compare(HFSAT::BaseOTrading::StrategyName()) == 0) {
        const unsigned int security_id_ =
            sec_name_indexer_.GetIdFromString(strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::SecurityMarketView* underlying_market_view_ =
            shortcode_smv_map_.GetSecurityMarketView(strategy_desc_.structured_strategy_vec_[i].trading_structure_);
        HFSAT::SecurityMarketView* p_sim_dep_market_view_ = sid_to_sim_smv_ptr_map_[security_id_];

        HFSAT::BaseTrader* p_base_trader_ = nullptr;
        HFSAT::BaseSimMarketMaker* p_base_sim_market_maker_ = nullptr;  // created outside to add listeneres later

        if (shortcode_to_smm_map.find(strategy_desc_.structured_strategy_vec_[i].trading_structure_) ==
            shortcode_to_smm_map.end()) {
          shortcode_to_smm_map[strategy_desc_.structured_strategy_vec_[i].trading_structure_] =
              HFSAT::SimMarketMakerHelper::GetSimMarketMaker(
                  dbglogger_, watch_, underlying_market_view_, p_sim_dep_market_view_, market_model_index_,
                  sim_time_series_info_, sid_to_mov_ptr_map_, historical_dispatcher_, common_smv_source, true);
        }
        p_base_sim_market_maker_ = shortcode_to_smm_map[strategy_desc_.structured_strategy_vec_[i].trading_structure_];
        p_base_trader_ =
            GetBaseTrader(network_account_info_manager_, underlying_market_view_, p_base_sim_market_maker_);

        {  // add SimMarketMaker as a listener to ORS
          if (p_base_sim_market_maker_ != nullptr) {
            HFSAT::ORSMessageFileSource* p_ors_message_filesource_ =
                shortcode_ors_data_filesource_map_.GetORSMessageFileSource(
                    strategy_desc_.structured_strategy_vec_[i].trading_structure_);
            if (p_ors_message_filesource_ != nullptr) {
              p_ors_message_filesource_->AddOrderNotFoundListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderSequencedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderConfirmedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderConfCxlReplacedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderCanceledListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderExecutedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderRejectedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderCxlSeqdListener(p_base_sim_market_maker_);
            }
          }
        }

        HFSAT::SmartOrderManager* p_smart_order_manager_ =
            new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *(p_base_trader_),
                                         *(underlying_market_view_), runtime_id_, livetrading_, 1);

        p_smart_order_manager_->ComputeQueueSizes();

        HFSAT::SimBasePNL* p_sim_base_pnl_ =
            new HFSAT::SimBasePNL(dbglogger_, watch_, *(underlying_market_view_), runtime_id_, trades_writer_);

        int t_index_ = p_structured_sim_base_pnl_->AddSecurity(p_sim_base_pnl_) - 1;
        p_sim_base_pnl_->AddListener(t_index_, p_structured_sim_base_pnl_);

        p_smart_order_manager_->SetBasePNL(p_sim_base_pnl_);

        if ((p_base_sim_market_maker_ != nullptr) && (p_smart_order_manager_ != nullptr)) {
          p_base_sim_market_maker_->AddOrderNotFoundListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderSequencedListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderConfirmedListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderConfCxlReplaceRejectedListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderConfCxlReplacedListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderCanceledListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderExecutedListener(p_smart_order_manager_);
          p_base_sim_market_maker_->AddOrderRejectedListener(p_smart_order_manager_);
        }

        int saci = p_smart_order_manager_->server_assigned_client_id_;
        std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_ =
            common_smv_source->getShortcodeFilesourceMap();
        for (unsigned j = 0; j < struct_strat_source_shortcode_vec_map_[i].size(); j++) {
          auto filesource = shortcode_filesource_map_[struct_strat_source_shortcode_vec_map_[i][j]];

          filesource->AddExternalDataListenerListener(
              shortcode_to_smm_map[strategy_desc_.structured_strategy_vec_[i].trading_structure_]);
          int sec_id = sec_name_indexer_.GetIdFromString(struct_strat_source_shortcode_vec_map_[i][j]);
          shortcode_to_smm_map[strategy_desc_.structured_strategy_vec_[i].trading_structure_]->AddSecIdToSACI(saci,
                                                                                                              sec_id);
        }

        // For new trading logic - Exec Logic conditions can be put here by comparing to sub strategy name

        strategy_desc_.structured_strategy_vec_[i].options_trading_ = new HFSAT::BaseOTrading(
            dbglogger_, watch_, *(underlying_market_view_), *p_smart_order_manager_, p_structured_sim_base_pnl_,
            strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, livetrading_,
            p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
            runtime_id_, sec_name_indexer_);

        market_update_manager_.AddMarketDataInterruptedListener(
            strategy_desc_.structured_strategy_vec_[i].options_trading_);
        strategy_desc_.structured_strategy_vec_[i].options_trading_->SetStartTrading(true);

      } else if (strategy_name_.compare(HFSAT::RetailFlyTradingManager::StrategyName()) == 0) {
        std::vector<const HFSAT::SecurityMarketView*> p_smv_vec_;
        std::vector<std::string> paramfilename_vec_;
        std::vector<std::string>& shortcodes_ = strategy_desc_.structured_strategy_vec_[i].shortcodes_;

        for (unsigned int s_idx_ = 0; s_idx_ < shortcodes_.size(); s_idx_++) {
          p_smv_vec_.push_back(shortcode_smv_map_.GetSecurityMarketView(shortcodes_[s_idx_]));
          paramfilename_vec_.emplace_back(
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_paramfile_vec_[shortcodes_[s_idx_]][0]);
        }

        retfly_trading_manager_ = new HFSAT::RetailFlyTradingManager(
            dbglogger_, watch_, p_smv_vec_, p_structured_sim_base_pnl_,
            strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, livetrading_, economic_events_manager_,
            trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_, paramfilename_vec_,
            strategy_desc_.structured_strategy_vec_[i].trading_structure_);

        strategy_desc_.structured_strategy_vec_[i].common_trading_manager_ = retfly_trading_manager_;
        strategy_desc_.structured_strategy_vec_[i].trading_structure_ = retfly_trading_manager_->secname();

        // creating retail source for spd if not already created
        if (retail_logged_message_filesource_map_.find(retfly_trading_manager_->secname()) ==
            retail_logged_message_filesource_map_.end()) {
          retail_logged_message_filesource_map_[retfly_trading_manager_->secname()] =
              new HFSAT::RETAILLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, -1,
                                                       retfly_trading_manager_->secname().c_str(),
                                                       dep_trading_location_);
          retail_logged_message_filesource_map_[retfly_trading_manager_->secname()]->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(
              retail_logged_message_filesource_map_[retfly_trading_manager_->secname()]);
          retail_logged_message_filesource_map_[retfly_trading_manager_->secname()]->AddFPOrderExecutedListener(
              retfly_trading_manager_);
        }

        // creating manual retail source for spd if not already created (used to provide manual input)
        if (retail_manual_logged_message_filesource_map_.find(retfly_trading_manager_->secname()) ==
            retail_manual_logged_message_filesource_map_.end()) {
          retail_manual_logged_message_filesource_map_[retfly_trading_manager_->secname()] =
              new HFSAT::RETAILLoggedMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, -1,
                                                       retfly_trading_manager_->secname().c_str(),
                                                       dep_trading_location_, true);
          retail_manual_logged_message_filesource_map_[retfly_trading_manager_->secname()]->SetExternalTimeListener(
              &watch_);
          historical_dispatcher_.AddExternalDataListener(
              retail_manual_logged_message_filesource_map_[retfly_trading_manager_->secname()]);
          retail_manual_logged_message_filesource_map_[retfly_trading_manager_->secname()]->AddFPOrderExecutedListener(
              retfly_trading_manager_);
        }

        market_update_manager_.AddMarketDataInterruptedListener(retfly_trading_manager_);

      } else {
        structured_trading_manager_ = new HFSAT::StructuredTradingManager(
            dbglogger_, watch_, strategy_desc_.structured_strategy_vec_[i].trading_structure_,
            p_structured_sim_base_pnl_, livetrading_, strategy_desc_.structured_strategy_vec_[i].common_paramfilename_);
        strategy_desc_.structured_strategy_vec_[i].trading_manager_ = structured_trading_manager_;
      }

      for (unsigned int j = 0; j < strategy_desc_.structured_strategy_vec_[i].shortcodes_.size(); j++) {
        std::string shortcode_ = strategy_desc_.structured_strategy_vec_[i].shortcodes_[j];
        const unsigned int security_id_ = sec_name_indexer_.GetIdFromString(shortcode_);

        HFSAT::ExchSource_t exch_traded_on_ =
            HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
        HFSAT::SecurityMarketView* p_dep_market_view_ = shortcode_smv_map_.GetSecurityMarketView(shortcode_);
        HFSAT::SecurityMarketView* p_sim_dep_market_view_ = sid_to_sim_smv_ptr_map_[security_id_];

        strategy_desc_.structured_strategy_vec_[i].p_dep_market_view_vec_[j] = p_dep_market_view_;
        strategy_desc_.structured_strategy_vec_[i].exch_traded_on_ = exch_traded_on_;

        unsigned int num_strats_for_shortcode_ =
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_paramfile_vec_[shortcode_].size();

        for (unsigned int k = 0; k < num_strats_for_shortcode_; k++) {
          HFSAT::BaseTrader* p_base_trader_ = nullptr;
          HFSAT::BaseSimMarketMaker* p_base_sim_market_maker_ = nullptr;  // created outside to add listeneres later

          std::string modelfilename_ =
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_modelfile_vec_[shortcode_][k];
          std::string paramfilename_ =
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_paramfile_vec_[shortcode_][k];
          strategy_desc_.structured_strategy_vec_[i].shortcode_to_base_trader_vec_[shortcode_][k] = p_base_trader_;

          if (shortcode_to_smm_map.find(shortcode_) == shortcode_to_smm_map.end()) {
            shortcode_to_smm_map[shortcode_] = HFSAT::SimMarketMakerHelper::GetSimMarketMaker(
                dbglogger_, watch_, p_dep_market_view_, p_sim_dep_market_view_, market_model_index_,
                sim_time_series_info_, sid_to_mov_ptr_map_, historical_dispatcher_, common_smv_source, true);
          }
          p_base_sim_market_maker_ = shortcode_to_smm_map[shortcode_];
          p_base_trader_ = GetBaseTrader(network_account_info_manager_, p_dep_market_view_, p_base_sim_market_maker_);

          // add SimMarketMaker as a listener to ORS
          if (p_base_sim_market_maker_ != nullptr) {
            HFSAT::ORSMessageFileSource* p_ors_message_filesource_ =
                shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);
            if (p_ors_message_filesource_ != nullptr) {
              p_ors_message_filesource_->AddOrderNotFoundListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderSequencedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderConfirmedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderConfCxlReplacedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderCanceledListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderExecutedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderRejectedListener(p_base_sim_market_maker_);
              p_ors_message_filesource_->AddOrderCxlSeqdListener(p_base_sim_market_maker_);
              // To enable handling for FOKs in simreal
              if (real_saci_vec_.size() > 0) {
                p_ors_message_filesource_->AddOrderInternallyMatchedListener(p_base_sim_market_maker_);
              }
            }
          }

          HFSAT::SmartOrderManager* p_smart_order_manager_ =
              new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *(p_base_trader_),
                                           *(p_dep_market_view_), runtime_id_, livetrading_, 1);

          p_smart_order_manager_->ComputeQueueSizes();

          HFSAT::SimBasePNL* p_sim_base_pnl_ =
              new HFSAT::SimBasePNL(dbglogger_, watch_, *(p_dep_market_view_), runtime_id_, trades_writer_);

          int t_index_ = p_structured_sim_base_pnl_->AddSecurity(p_sim_base_pnl_) - 1;
          p_sim_base_pnl_->AddListener(t_index_, p_structured_sim_base_pnl_);

          p_smart_order_manager_->SetBasePNL(p_sim_base_pnl_);

          if ((p_base_sim_market_maker_ != nullptr) && (p_smart_order_manager_ != nullptr)) {
            p_base_sim_market_maker_->AddOrderNotFoundListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderSequencedListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderConfirmedListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderConfCxlReplaceRejectedListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderConfCxlReplacedListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderCanceledListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderExecutedListener(p_smart_order_manager_);
            p_base_sim_market_maker_->AddOrderRejectedListener(p_smart_order_manager_);
          }

          int saci = p_smart_order_manager_->server_assigned_client_id_;
          std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_ =
              common_smv_source->getShortcodeFilesourceMap();
          for (unsigned j = 0; j < struct_strat_source_shortcode_vec_map_[i].size(); j++) {
            auto filesource = shortcode_filesource_map_[struct_strat_source_shortcode_vec_map_[i][j]];

            filesource->AddExternalDataListenerListener(shortcode_to_smm_map[shortcode_]);
            int sec_id = sec_name_indexer_.GetIdFromString(struct_strat_source_shortcode_vec_map_[i][j]);
            shortcode_to_smm_map[shortcode_]->AddSecIdToSACI(saci, sec_id);
          }

          HFSAT::BaseModelMath* base_model_math_ = nullptr;
          if (strategy_name_.compare(HFSAT::RetailFlyTradingManager::StrategyName()) != 0) {
            std::vector<double> model_scaling_factor_vec_;
            HFSAT::ModelScaling::CheckIfStaticModelScalingPossible(modelfilename_, paramfilename_,
                                                                   model_scaling_factor_vec_);
            base_model_math_ = HFSAT::ModelCreator::CreateModelMathComponent(
                dbglogger_, watch_, modelfilename_, &model_scaling_factor_vec_, trading_start_utc_mfm_,
                trading_end_utc_mfm_, runtime_id_);
            base_model_math_->SubscribeMarketInterrupts(market_update_manager_);
          }

          if (strategy_name_.compare(HFSAT::StructuredPriceBasedAggressiveTrading::StrategyName()) == 0) {
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] =
                new HFSAT::StructuredPriceBasedAggressiveTrading(
                    dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, paramfilename_, livetrading_,
                    p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                    trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                    structured_trading_manager_);
          }

          else if (strategy_name_.compare(HFSAT::PriceBasedSpreadTrading::StrategyName()) == 0) {
            if (spread_market_view_->IsBellyShc(p_dep_market_view_->shortcode())) {
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] =
                  new HFSAT::PriceBasedSpreadTrading(
                      dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, paramfilename_, livetrading_,
                      p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                      trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                      *spread_trading_manager_);

              base_model_math_->SetTargetPriceReporter(spread_market_view_);
            } else {
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] =
                  new HFSAT::PriceBasedLegTrading(
                      dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, paramfilename_, livetrading_,
                      p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                      trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                      *spread_trading_manager_);
            }
          } else if (HFSAT::StructuredGeneralTrading::IsStructuredGeneralTrading(strategy_string_)) {
            // general version of structured trading where one can use all other exec-logic, keeping environment same

            auto exec_logic = new HFSAT::StructuredGeneralTrading(
                dbglogger_, watch_, *p_dep_market_view_, *p_smart_order_manager_, paramfilename_, livetrading_,
                p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
                runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                HFSAT::StructuredGeneralTrading::StrategyName(), curve_trading_manager_);
            exec_logic->SetExecLogic(strategy_desc_.structured_strategy_vec_[i].sub_strategy_name_);
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] = exec_logic;

          } else if (strategy_name_.compare(HFSAT::RiskBasedStructuredTrading::StrategyName()) == 0) {
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] =
                new HFSAT::RiskBasedStructuredTrading(
                    dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, paramfilename_, livetrading_,
                    p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                    trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                    *curve_trading_manager_);

          } else if (strategy_name_.compare(HFSAT::MinRiskPriceBasedAggressiveTrading::StrategyName()) == 0) {
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] =
                new HFSAT::MinRiskPriceBasedAggressiveTrading(
                    dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_,

                    paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
                    trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,

                    modelfilename_source_shortcode_vec_map_[modelfilename_],
                    *(strategy_desc_.structured_strategy_vec_[i].min_risk_trading_manager_));
          } else if (strategy_name_.compare(HFSAT::EquityTrading2::StrategyName()) == 0) {
            if (strategy_desc_.structured_strategy_vec_[i].equity_trading_ == nullptr) {
              // Passing dep_market view only to initialize base trading, no otehr use here
              strategy_desc_.structured_strategy_vec_[i].equity_trading_ = new HFSAT::EquityTrading2(
                  dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, p_structured_sim_base_pnl_,
                  strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, livetrading_,
                  p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                  trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                  sec_name_indexer_);
              market_update_manager_.AddMarketDataInterruptedListener(
                  strategy_desc_.structured_strategy_vec_[i].equity_trading_);
              int trading_start_mfm_ = -1;
              int trading_end_mfm_ = -1;
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.end()) {
                trading_start_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_[shortcode_][0];
              }
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.end()) {
                trading_end_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_[shortcode_][0];
              }
              strategy_desc_.structured_strategy_vec_[i].equity_trading_->AddProductToTrade(
                  p_dep_market_view_, p_smart_order_manager_, base_model_math_, paramfilename_, trading_start_mfm_,
                  trading_end_mfm_);

              strategy_desc_.structured_strategy_vec_[i].equity_trading_->SetStartTrading(true);
              if (strategy_desc_.structured_strategy_vec_[i].sub_strategy_name_.compare(
                      "DirectionalAggressiveTrading") == 0) {
                strategy_desc_.structured_strategy_vec_[i].equity_trading_->SetOrderPlacingLogic(
                    HFSAT::kDirectionalAggressiveTrading);
              } else if (strategy_desc_.structured_strategy_vec_[i].sub_strategy_name_.compare(
                             "PriceBasedAggressiveTrading") == 0) {
                strategy_desc_.structured_strategy_vec_[i].equity_trading_->SetOrderPlacingLogic(
                    HFSAT::kPriceBasedAggressiveTrading);
              }
            } else {
              int trading_start_mfm_ = -1;
              int trading_end_mfm_ = -1;
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.end()) {
                trading_start_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_[shortcode_][0];
              }
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.end()) {
                trading_end_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_[shortcode_][0];
              }
              strategy_desc_.structured_strategy_vec_[i].equity_trading_->AddProductToTrade(
                  p_dep_market_view_, p_smart_order_manager_, base_model_math_, paramfilename_, trading_start_mfm_,
                  trading_end_mfm_);
            }
          } else if (strategy_name_.compare(HFSAT::PairsTrading::StrategyName()) == 0) {
            if (strategy_desc_.structured_strategy_vec_[i].pairs_trading_ == nullptr) {
              // Passing dep_market view only to initialize base trading, no otehr use here
              strategy_desc_.structured_strategy_vec_[i].pairs_trading_ = new HFSAT::PairsTrading(
                  dbglogger_, watch_, *(p_dep_market_view_), *p_smart_order_manager_, p_structured_sim_base_pnl_,
                  strategy_desc_.structured_strategy_vec_[i].trading_structure_,
                  strategy_desc_.structured_strategy_vec_[i].common_paramfilename_, livetrading_,
                  p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
                  trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
                  sec_name_indexer_);
              market_update_manager_.AddMarketDataInterruptedListener(
                  strategy_desc_.structured_strategy_vec_[i].pairs_trading_);
              int trading_start_mfm_ = -1;
              int trading_end_mfm_ = -1;
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.end()) {
                trading_start_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_[shortcode_][0];
              }
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.end()) {
                trading_end_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_[shortcode_][0];
              }
              strategy_desc_.structured_strategy_vec_[i].pairs_trading_->AddProductToTrade(
                  p_dep_market_view_, p_smart_order_manager_, base_model_math_, paramfilename_, trading_start_mfm_,
                  trading_end_mfm_);

              strategy_desc_.structured_strategy_vec_[i].pairs_trading_->SetStartTrading(true);
              if (strategy_desc_.structured_strategy_vec_[i].sub_strategy_name_.compare(
                      "DirectionalAggressiveTrading") == 0) {
                strategy_desc_.structured_strategy_vec_[i].pairs_trading_->SetOrderPlacingLogic(
                    HFSAT::kDirectionalAggressiveTrading);
              } else if (strategy_desc_.structured_strategy_vec_[i].sub_strategy_name_.compare(
                             "PriceBasedAggressiveTrading") == 0) {
                strategy_desc_.structured_strategy_vec_[i].pairs_trading_->SetOrderPlacingLogic(
                    HFSAT::kPriceBasedAggressiveTrading);
              }
            } else {
              int trading_start_mfm_ = -1;
              int trading_end_mfm_ = -1;
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.end()) {
                trading_start_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_[shortcode_][0];
              }
              if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.find(shortcode_) !=
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.end()) {
                trading_end_mfm_ =
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_[shortcode_][0];
              }
              strategy_desc_.structured_strategy_vec_[i].pairs_trading_->AddProductToTrade(
                  p_dep_market_view_, p_smart_order_manager_, base_model_math_, paramfilename_, trading_start_mfm_,
                  trading_end_mfm_);
            }
          } else if ((strategy_name_.compare(HFSAT::BaseOTrading::StrategyName()) == 0)) {
            int trading_start_mfm_ = -1;
            int trading_end_mfm_ = -1;
            if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.find(shortcode_) !=
                strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_.end()) {
              trading_start_mfm_ =
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_start_mfm_vec_[shortcode_][0];
            }
            if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.find(shortcode_) !=
                strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_.end()) {
              trading_end_mfm_ =
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_trading_end_mfm_vec_[shortcode_][0];
            }
            strategy_desc_.structured_strategy_vec_[i].options_trading_->AddProductToTrade(
                p_dep_market_view_, p_smart_order_manager_, base_model_math_, paramfilename_, trading_start_mfm_,
                trading_end_mfm_);
          } else if (strategy_name_.compare(HFSAT::RetailFlyTradingManager::StrategyName()) == 0) {
            retfly_trading_manager_->SetOrderManager(p_smart_order_manager_, shortcode_);
          }

          if (strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k] != nullptr) {
            base_model_math_->AddListener(
                strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]);
            strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]->SetModelMathComponent(
                base_model_math_);
            market_update_manager_.AddMarketDataInterruptedListener(
                strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]);

            if (false && !ignore_user_msg_) {
              HFSAT::ControlMessageFileSource* control_messasge_filesource_ = nullptr;
              bool control_file_present = true;
              control_messasge_filesource_ =
                  new HFSAT::ControlMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, security_id_,
                                                      sec_name_indexer_.GetSecurityNameFromId(security_id_),
                                                      dep_trading_location_, runtime_id_, control_file_present);

              if (control_messasge_filesource_ != nullptr && control_file_present) {
                historical_dispatcher_.AddExternalDataListener(control_messasge_filesource_);
                control_messasge_filesource_->SetExternalTimeListener(&watch_);
                control_messasge_filesource_->AddControlMessageListener(
                    strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]);

                if (runtime_id_ == 0) {
                  strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]->SetStartTrading(
                      true);
                }
              } else {
                strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]->SetStartTrading(true);
                std::cerr << " Control message source is nullptr or CONTROL file not present\n";
              }
            } else {
              strategy_desc_.structured_strategy_vec_[i].shortcode_to_exec_vec_[shortcode_][k]->SetStartTrading(true);
            }
          }
        }
      }
    }
  }

  void SetDepComponentsStrategies(
      HFSAT::DebugLogger& dbglogger_, HFSAT::Watch& watch_, HFSAT::StrategyDesc& strategy_desc_,
      HFSAT::NetworkAccountInfoManager& network_account_info_manager_,
      HFSAT::HistoricalDispatcher& historical_dispatcher_,
      std::map<std::string, HFSAT::BaseSimMarketMaker*>& shortcode_to_smm_map,
      std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map_, CommonSMVSource* common_smv_source,
      HFSAT::EconomicEventsManager& economic_events_manager_, bool& livetrading_, unsigned int& market_model_index_,
      HFSAT::SimTimeSeriesInfo& sim_time_series_info_, HFSAT::MulticastSenderSocket* p_strategy_param_sender_socket_,
      HFSAT::MarketUpdateManager& market_update_manager_, HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_,
      HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_,
      HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_,
      HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_,
      std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*>& retail_logged_message_filesource_map_,
      std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*>& retail_manual_logged_message_filesource_map_,
      HFSAT::TradingLocation_t& dep_trading_location_, bool& ignore_user_msg_, int& tradingdate_,
      HFSAT::PromOrderManagerPtrVec& sid_to_prom_order_manager_map_, HFSAT::RiskManager* p_risk_manager_,
      std::map<std::string, std::vector<std::string>>& modelfilename_source_shortcode_vec_map_,
      const int first_dep_security_id_, const std::string& first_dep_shortcode_, std::vector<int>& real_saci_vec_,
      std::map<int, std::vector<std::string>>& strat_source_shortcode_vec_map_,
      HFSAT::SecurityNameIndexer& sec_name_indexer_, HFSAT::AFLASHLoggedMessageFileSource* aflash_data_filesource_,
      HFSAT::BulkFileWriter& trades_writer_, std::vector<HFSAT::SmartOrderManager*>& all_om_vec) {
    for (size_t i = 0; i < strategy_desc_.strategy_vec_.size(); i++) {
      std::vector<int> sec_id_vec;
      std::vector<std::string> shortcode_vec;
      std::vector<HFSAT::SecurityMarketView*> smv_vec;
      std::vector<HFSAT::BaseTrader*> base_trader_vec;
      std::vector<HFSAT::BaseSimMarketMaker*> smm_vec;
      std::vector<HFSAT::SmartOrderManager*> om_vec;
      std::vector<HFSAT::BasePNL*> base_pnl_vec;

      std::string modelfilename_ = strategy_desc_.strategy_vec_[i].modelfilename_;
      std::string paramfilename_ = strategy_desc_.strategy_vec_[i].paramfilename_;
      std::string strategy_name_ = strategy_desc_.strategy_vec_[i].strategy_name_;

      unsigned int runtime_id_ = strategy_desc_.strategy_vec_[i].runtime_id_;
      unsigned int trading_start_utc_mfm_ = strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_;
      unsigned int trading_end_utc_mfm_ = strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_;

      auto& dep_shortcode = strategy_desc_.strategy_vec_[i].dep_shortcode_;
      auto exch = HFSAT::SecurityDefinitions::GetContractExchSource(dep_shortcode, tradingdate_);
      strategy_desc_.strategy_vec_[i].p_dep_market_view_ = shortcode_smv_map_.GetSecurityMarketView(dep_shortcode);
      strategy_desc_.strategy_vec_[i].exch_traded_on_ = exch;

      // Collect dep shortcodes
      if (strategy_name_.compare(HFSAT::ArbTrading::StrategyName()) == 0) {
        HFSAT::ArbTrading::GetDepShortcodes(dep_shortcode, shortcode_vec);

      } else if (strategy_name_.compare(HFSAT::ArbTradingTodTom::StrategyName()) == 0) {
        HFSAT::ArbTradingTodTom::GetDepShortcodes(dep_shortcode, shortcode_vec);

      } else if (strategy_name_.compare(HFSAT::NikPricePairBasedAggressiveTrading::StrategyName()) == 0) {
        HFSAT::VectorUtils::UniqueVectorAdd(shortcode_vec, dep_shortcode);

        std::string mini_shortcode = "NKM_0";
        HFSAT::VectorUtils::UniqueVectorAdd(shortcode_vec, mini_shortcode);

      } else if (strategy_name_.compare(HFSAT::DirectionalSyntheticTrading::StrategyName()) == 0) {
        HFSAT::DirectionalSyntheticTrading::GetDepShortcodes(dep_shortcode, shortcode_vec);
      } else if (strategy_name_.compare(HFSAT::SgxNK1MMTrading::StrategyName()) == 0) {
        HFSAT::SgxNK1MMTrading::GetDepShortcodes(dep_shortcode, shortcode_vec);
      } else {
        HFSAT::VectorUtils::UniqueVectorAdd(shortcode_vec, dep_shortcode);
      }

      // Get sim market maker and base trader instances for each dep shortcode
      for (auto& shortcode : shortcode_vec) {
        auto security_id = sec_name_indexer_.GetIdFromString(shortcode);
        sec_id_vec.push_back(security_id);

        // Get sim market maker instance and push it to the vec
        auto dep_smv = shortcode_smv_map_.GetSecurityMarketView(shortcode);
        auto sim_dep_smv = sid_to_sim_smv_ptr_map_[security_id];
        smv_vec.push_back(dep_smv);

        if (shortcode_to_smm_map.find(shortcode) == shortcode_to_smm_map.end()) {
          shortcode_to_smm_map[shortcode] = HFSAT::SimMarketMakerHelper::GetSimMarketMaker(
              dbglogger_, watch_, dep_smv, sim_dep_smv, market_model_index_, sim_time_series_info_, sid_to_mov_ptr_map_,
              historical_dispatcher_, common_smv_source, true);
        }

        smm_vec.push_back(shortcode_to_smm_map[shortcode]);
        // Get base trader instance and push it to the vec
        auto base_trader = GetBaseTrader(network_account_info_manager_, dep_smv, shortcode_to_smm_map[shortcode]);
        base_trader_vec.push_back(base_trader);

        // Subscribe SMM to ors messages
        if (shortcode_to_smm_map[shortcode]) {
          auto ors_filesource = shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode);

          if (ors_filesource) {
            ors_filesource->AddOrderNotFoundListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderSequencedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderConfirmedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderConfCxlReplacedListener(shortcode_to_smm_map[shortcode]);
            // ors_filesource->AddOrderConfCxlReplaceRejectListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderCanceledListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderExecutedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderRejectedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderCxlSeqdListener(shortcode_to_smm_map[shortcode]);
            // To enable handling for FOKs in simreal
            if (real_saci_vec_.size() > 0) {
              ors_filesource->AddOrderInternallyMatchedListener(shortcode_to_smm_map[shortcode]);
            }
          }
        }

        // Create Order manager instances
        auto smart_om = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader, *dep_smv,
                                                     runtime_id_, livetrading_, 1);
        om_vec.push_back(smart_om);

        if (real_saci_vec_.size() == strategy_desc_.strategy_vec_.size() && real_saci_vec_[i] > 0) {
          shortcode_to_smm_map[shortcode]->AssignRealSACI(smart_om->server_assigned_client_id_, real_saci_vec_[i]);
        }

        int saci = smart_om->server_assigned_client_id_;
        std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_ =
            common_smv_source->getShortcodeFilesourceMap();
        for (unsigned j = 0; j < strat_source_shortcode_vec_map_[i].size(); j++) {
          auto filesource = shortcode_filesource_map_[strat_source_shortcode_vec_map_[i][j]];

          filesource->AddExternalDataListenerListener(shortcode_to_smm_map[shortcode]);
          int sec_id = sec_name_indexer_.GetIdFromString(strat_source_shortcode_vec_map_[i][j]);
          shortcode_to_smm_map[shortcode]->AddSecIdToSACI(saci, sec_id);
        }

        smart_om->ComputeQueueSizes();

        HFSAT::SimBasePNL* sim_base_pnl = nullptr;

        if (strategy_name_.compare(HFSAT::ArbTrading::StrategyName()) != 0 &&
            strategy_name_.compare(HFSAT::DirectionalSyntheticTrading::StrategyName()) != 0 &&
            strategy_name_.compare(HFSAT::ArbTradingTodTom::StrategyName()) != 0) {
          sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *dep_smv, runtime_id_, trades_writer_);
        }

        smart_om->SetBasePNL(sim_base_pnl);
        all_om_vec.push_back(smart_om);
        base_pnl_vec.push_back(sim_base_pnl);

        if (shortcode_to_smm_map[shortcode] && smart_om) {
          shortcode_to_smm_map[shortcode]->AddOrderNotFoundListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderSequencedListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderConfirmedListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderConfCxlReplaceRejectedListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderConfCxlReplacedListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderCanceledListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderExecutedListener(smart_om);
          shortcode_to_smm_map[shortcode]->AddOrderRejectedListener(smart_om);
        }
      }

      strategy_desc_.strategy_vec_[i].p_base_trader_ = base_trader_vec[0];

      // create ModelMath component if not created .
      // It is fine to have multiple strategy lines with same modelfilename_, and hence having the same
      // ModelmathComponent
      // (since modelmathcomponent is only created in ModelCreator if modelfilename is missing as key in map
      // It appears that in that case if the dependant is also the same ( which is likely to be the case ), for the
      // second
      // strategy instance
      //   the indicators will be a listener to SMV before this SimMarketMaker instance is even created and hence they
      //   will be a listener before SimMarketMaker
      //   But that is fine since ModelMath only calls it's listeners on an SMVOnReady ( ) call from SMV, which is
      //   called
      //   after all SMVChangeListeners are done.
      //   Hence strategy should get new Model output target price after SimMarketMaker and SmartOrderManager get
      //   SMVChange callbacks
      std::vector<double> model_scaling_factor_vec_;
      HFSAT::ModelScaling::CheckIfStaticModelScalingPossible(modelfilename_, paramfilename_, model_scaling_factor_vec_);
      HFSAT::BaseModelMath* base_model_math_ =
          HFSAT::ModelCreator::CreateModelMathComponent(dbglogger_, watch_, modelfilename_, &model_scaling_factor_vec_,
                                                        trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_);

      // subscribes all indicators to market data interrupts other indicator needs
      base_model_math_->SubscribeMarketInterrupts(market_update_manager_);

      std::string cancellation_model_file_ = HFSAT::ExecLogicUtils::getCancellationModel(
          (*smv_vec[0]).shortcode(), tradingdate_, trading_start_utc_mfm_, trading_end_utc_mfm_);
      HFSAT::BaseModelMath* base_cancel_model_ = nullptr;
      if (cancellation_model_file_ != "") {
        base_cancel_model_ = HFSAT::ModelCreator::CreateModelMathComponent(
            dbglogger_, watch_, cancellation_model_file_, &model_scaling_factor_vec_, trading_start_utc_mfm_,
            trading_end_utc_mfm_, runtime_id_);
      }

      HFSAT::PromPNLIndicator* p_prom_pnl_indicator_ = nullptr;
      if (sid_to_prom_order_manager_map_[sec_id_vec[0]] != nullptr) {
        p_prom_pnl_indicator_ = HFSAT::PromPNLIndicator::GetUniqueInstance(
            dbglogger_, watch_, *(sid_to_prom_order_manager_map_[sec_id_vec[0]]), *smv_vec[0]);
      }

      {
        // On Create Exec Strategy attaches itself as a listener to SecurityMarketView *
        if (HFSAT::ModelCreator::NeedsAflashFeed() ||
            strategy_name_.compare(HFSAT::EventBiasAggressiveTrading::StrategyName()) == 0) {
          aflash_data_filesource_ =
              new HFSAT::AFLASHLoggedMessageFileSource(dbglogger_, tradingdate_, dep_trading_location_);
          aflash_data_filesource_->SetExternalTimeListener(&watch_);
          historical_dispatcher_.AddExternalDataListener(aflash_data_filesource_);
        }

        if (strategy_name_.compare(HFSAT::DirectionalAggressiveTradingSingleOrder::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalAggressiveTradingSingleOrder(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::DirectionalAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::DirectionalAggressiveTradingModifyV2::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalAggressiveTradingModifyV2(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::EventBiasAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::EventBiasAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                       HFSAT::DesiredPositionTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DesiredPositionTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_,
              strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
              strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
              modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::EventDirectionalAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::EventDirectionalAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(HFSAT::RetailTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::RetailTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_,
              strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
              strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
              modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);

          /* currently not being used
          if ( sim_time_series_info_.sid_to_sim_config_[security_id_].retail_order_placing_prob_ < 1.0 )
          {
            ((HFSAT::RetailTrading*)(strategy_desc_.strategy_vec_[ i
          ].exec_))->SubscribeRetailOfferUpdate(p_base_sim_market_maker_);
            //to have same random order for a given day, to make comparisons more fair
            HFSAT::RandomNumberGenerator::SetSeed(tradingdate_);
          }
         */

          if (retail_logged_message_filesource_map_.find(shortcode_vec[0]) ==
              retail_logged_message_filesource_map_.end()) {
            retail_logged_message_filesource_map_[shortcode_vec[0]] = new HFSAT::RETAILLoggedMessageFileSource(
                dbglogger_, sec_name_indexer_, tradingdate_, sec_id_vec[0],
                sec_name_indexer_.GetSecurityNameFromId(sec_id_vec[0]), dep_trading_location_);
            retail_logged_message_filesource_map_[shortcode_vec[0]]->SetExternalTimeListener(&watch_);
            retail_logged_message_filesource_map_[shortcode_vec[0]]->AddFPOrderExecutedListener(
                (HFSAT::RetailTrading*)(strategy_desc_.strategy_vec_[i].exec_));
            historical_dispatcher_.AddExternalDataListener(retail_logged_message_filesource_map_[shortcode_vec[0]]);
          }
          // For providing manual inputs
          if (retail_manual_logged_message_filesource_map_.find(shortcode_vec[0]) ==
              retail_manual_logged_message_filesource_map_.end()) {
            retail_manual_logged_message_filesource_map_[shortcode_vec[0]] = new HFSAT::RETAILLoggedMessageFileSource(
                dbglogger_, sec_name_indexer_, tradingdate_, sec_id_vec[0],
                sec_name_indexer_.GetSecurityNameFromId(sec_id_vec[0]), dep_trading_location_, true);
            retail_manual_logged_message_filesource_map_[shortcode_vec[0]]->SetExternalTimeListener(&watch_);
            retail_manual_logged_message_filesource_map_[shortcode_vec[0]]->AddFPOrderExecutedListener(
                (HFSAT::RetailTrading*)(strategy_desc_.strategy_vec_[i].exec_));
            historical_dispatcher_.AddExternalDataListener(
                retail_manual_logged_message_filesource_map_[shortcode_vec[0]]);
          }
        } else if (strategy_name_.compare(HFSAT::DirectionalInterventionAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalInterventionAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                       HFSAT::DirectionalLogisticTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalLogisticTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::DirectionalInterventionLogisticTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalInterventionLogisticTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedAggressiveTrading2::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedAggressiveTrading2(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::EventPriceBasedAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::EventPriceBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedSecurityAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedSecurityAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);

          // Since we did not need it before this and we need it now check if it is nullptr
          if (p_risk_manager_ == nullptr) {
            p_risk_manager_ = HFSAT::RiskManager::GetUniqueInstance(dbglogger_, watch_, sec_name_indexer_,
                                                                    sid_to_prom_order_manager_map_,
                                                                    first_dep_shortcode_, first_dep_security_id_);
          }
          // add the strategy basetrading as a listener
          if (p_risk_manager_ != nullptr) {
            p_risk_manager_->AddRiskManagerListener(strategy_desc_.strategy_vec_[i].exec_);
          }
        } else if (strategy_name_.compare(HFSAT::PriceBasedScalper::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedScalper(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, market_update_manager_, trading_start_utc_mfm_,
              trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedInterventionAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedInterventionAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedVolTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedVolTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::VolMMTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::VolMMTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::RegimeBasedVolDatTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::RegimeBasedVolDatTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (HFSAT::RegimeTrading::IsRegimeStrategy(strategy_name_)) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::RegimeTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_], strategy_name_);
        } else if (strategy_name_.compare(HFSAT::PricePairBasedAggressiveTrading::StrategyName()) == 0) {
          std::string indep_shortcode_ = HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(smv_vec[0]->shortcode());
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PricePairBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)),
              *om_vec[0], paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,
              modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PricePairBasedAggressiveTradingV2::StrategyName()) == 0) {
          std::string indep_shortcode_ = HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(smv_vec[0]->shortcode());
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PricePairBasedAggressiveTradingV2(
              dbglogger_, watch_, *smv_vec[0], *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)),
              *om_vec[0], paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,
              modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::SGXNKPricePairAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::SGXNKPricePairAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_0"))),
              *om_vec[0], paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,
              modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::ImpliedPricePairAggressiveTrading::StrategyName()) == 0) {
          std::vector<HFSAT::SecurityMarketView*> implied_indep_market_view_vec_;
          std::vector<std::string> implied_indep_shc_vec_ =
              HFSAT::ImpliedPriceCalculator::GetIndepShortcodesForImpliedPrice(smv_vec[0]->shortcode());
          for (auto i = 0u; i < implied_indep_shc_vec_.size(); i++) {
            implied_indep_market_view_vec_.push_back(
                shortcode_smv_map_.GetSecurityMarketView(implied_indep_shc_vec_[i]));
            shortcode_smv_map_.GetSecurityMarketView(implied_indep_shc_vec_[i])->ComputeMktPrice();
          }
          smv_vec[0]->ComputeMktPrice();
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ImpliedPricePairAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], implied_indep_market_view_vec_, *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::ImpliedDirectionalAggressiveTrading::StrategyName()) == 0) {
          std::vector<HFSAT::SecurityMarketView*> implied_indep_market_view_vec_;
          std::vector<std::string> implied_indep_shc_vec_ =
              HFSAT::ImpliedPriceCalculator::GetIndepShortcodesForImpliedPrice(smv_vec[0]->shortcode());
          for (auto i = 0u; i < implied_indep_shc_vec_.size(); i++) {
            implied_indep_market_view_vec_.push_back(
                shortcode_smv_map_.GetSecurityMarketView(implied_indep_shc_vec_[i]));
            shortcode_smv_map_.GetSecurityMarketView(implied_indep_shc_vec_[i])->ComputeMktPrice();
          }
          smv_vec[0]->ComputeMktPrice();
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ImpliedDirectionalAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], implied_indep_market_view_vec_, *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::SGXNiftyPricePairAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::SGXNiftyPricePairAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0],
              *(shortcode_smv_map_.GetSecurityMarketView(std::string("NSE_NIFTY_FUT0"))), *om_vec[0], paramfilename_,
              livetrading_, p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_,
              trading_end_utc_mfm_, runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::ProjectedPricePairBasedAggressiveTrading::StrategyName()) == 0) {
          std::string indep_shortcode_ = HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(smv_vec[0]->shortcode());
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ProjectedPricePairBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)),
              *om_vec[0], paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,
              modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::SgxNK1MMTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::SgxNK1MMTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
              *smv_vec[1],  //(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_1"))),
              *om_vec[1], *smv_vec[2], *om_vec[2], *(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_1"))),
              *(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_0"))));
          //*(shortcode_smv_map_.GetSecurityMarketView(std::string("SGX_NK_0"))));
        } else if (strategy_name_.compare(HFSAT::SgxNKSpreadMMTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::SgxNKSpreadMMTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_],
              *(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_0"))),
              *(shortcode_smv_map_.GetSecurityMarketView(std::string("NKM_1"))));
        } else if (strategy_name_.compare(HFSAT::DirectionalPairAggressiveTrading::StrategyName()) == 0) {
          std::string indep_shortcode_ = HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(smv_vec[0]->shortcode());
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalPairAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)),
              *om_vec[0], paramfilename_, livetrading_, p_strategy_param_sender_socket_, economic_events_manager_,
              trading_start_utc_mfm_, trading_end_utc_mfm_, runtime_id_,
              modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::ReturnsBasedAggressiveTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ReturnsBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::NikPricePairBasedAggressiveTrading::StrategyName()) == 0) {
          auto p_mult_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);
          int pnl_index_ = p_mult_base_pnl_->AddSecurity(base_pnl_vec[0]) - 1;
          base_pnl_vec[0]->AddListener(pnl_index_, p_mult_base_pnl_);

          auto nik_trading_manager_ = new HFSAT::NikTradingManager(dbglogger_, watch_, p_mult_base_pnl_, false);
          strategy_desc_.strategy_vec_[i].trading_manager_ = nik_trading_manager_;

          auto nk_trading_exec_ = new HFSAT::NikPricePairBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_], nik_trading_manager_);

          nik_trading_manager_->AddListener(sec_id_vec[0], smv_vec[0], nk_trading_exec_);

          pnl_index_ = p_mult_base_pnl_->AddSecurity(base_pnl_vec[1]) - 1;
          base_pnl_vec[1]->AddListener(pnl_index_, p_mult_base_pnl_);

          om_vec[1]->SetBasePNL(base_pnl_vec[1]);

          strategy_desc_.strategy_vec_[i].exec_ = nk_trading_exec_;

          auto nkm_trading_exec_ = new HFSAT::NikPricePairBasedAggressiveTrading(
              dbglogger_, watch_, *smv_vec[1], *om_vec[1], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_], nik_trading_manager_);

          strategy_desc_.strategy_vec_[i].pair_exec_ = nkm_trading_exec_;

          nik_trading_manager_->AddListener(sec_id_vec[1], smv_vec[1], nkm_trading_exec_);

          base_model_math_->AddListener(strategy_desc_.strategy_vec_[i].exec_);
          strategy_desc_.strategy_vec_[i].exec_->SetModelMathComponent(base_model_math_);
          market_update_manager_.AddMarketDataInterruptedListener(strategy_desc_.strategy_vec_[i].exec_);

          base_model_math_->AddListener(nkm_trading_exec_);
          nkm_trading_exec_->SetModelMathComponent(base_model_math_);
          market_update_manager_.AddMarketDataInterruptedListener(nkm_trading_exec_);

        } else if (strategy_name_.compare(HFSAT::ArbTrading::StrategyName()) == 0) {
          // Create pnl writer for arb trading
          HFSAT::SimPnlWriter* pnl_writer_ =
              new HFSAT::SimPnlWriter(*smv_vec[0], *smv_vec[1], trades_writer_, runtime_id_);
          HFSAT::ttime_t start_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_start_utc_mfm_ / 1000),
              (trading_start_utc_mfm_ % 1000) * 1000);
          HFSAT::ttime_t end_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_end_utc_mfm_ / 1000),
              (trading_end_utc_mfm_ % 1000) * 1000);

          strategy_desc_.strategy_vec_[i].exec_ =
              new HFSAT::ArbTrading(dbglogger_, watch_, *smv_vec[0], *smv_vec[1], *om_vec[0], *om_vec[1],
                                    paramfilename_, livetrading_, start_time, end_time, runtime_id_, pnl_writer_);
        } else if (strategy_name_.compare(HFSAT::ArbTradingTodTom::StrategyName()) == 0) {
          // Create pnl writer for arb trading
          HFSAT::SimPnlWriter* pnl_writer_ =
              new HFSAT::SimPnlWriter(*smv_vec[0], *smv_vec[1], *smv_vec[2], trades_writer_, runtime_id_);
          HFSAT::ttime_t start_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_start_utc_mfm_ / 1000),
              (trading_start_utc_mfm_ % 1000) * 1000);
          HFSAT::ttime_t end_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_end_utc_mfm_ / 1000),
              (trading_end_utc_mfm_ % 1000) * 1000);

          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ArbTradingTodTom(
              dbglogger_, watch_, *smv_vec[0], *smv_vec[1], *smv_vec[2], *om_vec[0], *om_vec[1], *om_vec[2],
              paramfilename_, livetrading_, start_time, end_time, runtime_id_, pnl_writer_);
        } else if (strategy_name_.compare(HFSAT::DirectionalSyntheticTrading::StrategyName()) == 0) {
          auto dep_smv = shortcode_smv_map_.GetSecurityMarketView(dep_shortcode);
          HFSAT::SimPnlWriter* pnl_writer_ =
              new HFSAT::SimPnlWriter(*smv_vec[0], *smv_vec[1], trades_writer_, runtime_id_);
          HFSAT::ttime_t start_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_start_utc_mfm_ / 1000),
              (trading_start_utc_mfm_ % 1000) * 1000);
          HFSAT::ttime_t end_time(
              HFSAT::DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()) + (int)(trading_end_utc_mfm_ / 1000),
              (trading_end_utc_mfm_ % 1000) * 1000);
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalSyntheticTrading(
              dbglogger_, watch_, *dep_smv, smv_vec[0], smv_vec[1], om_vec[0], om_vec[1], paramfilename_, livetrading_,
              start_time, end_time, runtime_id_, pnl_writer_);
        } else if (strategy_name_.compare(HFSAT::FEU3MM::StrategyName()) == 0) {
          std::string indep_shortcode_ = HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(smv_vec[0]->shortcode());
          std::cerr << indep_shortcode_ << "\n";
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::FEU3MM(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0],
              *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)), paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::OSEPMM::StrategyName()) == 0) {
          std::string indep_shortcode_ = "TOPIX_0";
          // std::cerr << indep_shortcode_ << "\n";
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::OSEPMM(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0],
              *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)), paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::JP400PMM::StrategyName()) == 0) {
          std::string indep_shortcode_ = "TOPIX_0";
          // std::cerr << indep_shortcode_ << "\n";
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::JP400PMM(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0],
              *(shortcode_smv_map_.GetSecurityMarketView(indep_shortcode_)), paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::FillTimeLogger::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::FillTimeLogger(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_]);
        } else if (strategy_name_.compare(HFSAT::PriceBasedAggressiveProRataTrading::StrategyName()) == 0) {
          strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedAggressiveProRataTrading(
              dbglogger_, watch_, *smv_vec[0], *om_vec[0], paramfilename_, livetrading_,
              p_strategy_param_sender_socket_, economic_events_manager_, trading_start_utc_mfm_, trading_end_utc_mfm_,
              runtime_id_, modelfilename_source_shortcode_vec_map_[modelfilename_], true);
        }

        if (strategy_desc_.strategy_vec_[i].exec_ != nullptr) {
          p_prom_pnl_indicator_->AddGlobalPNLChangeListener(strategy_desc_.strategy_vec_[i].exec_);
        }

        // Add as ModelMath target price listener
        if (strategy_desc_.strategy_vec_[i].exec_ != nullptr) {
          if (strategy_name_.compare("NikPricePairBasedAggressiveTrading") != 0) {
            base_model_math_->AddListener(strategy_desc_.strategy_vec_[i].exec_);
            strategy_desc_.strategy_vec_[i].exec_->SetModelMathComponent(base_model_math_);

            if (strategy_desc_.strategy_vec_[i].exec_->UsingCancellationModel()) {
              if (base_cancel_model_ == nullptr) {
                std::cerr << "Cancellation model not present\n";
              } else {
                // subscribes all indicators to market data interrupts other indicator needs
                base_cancel_model_->SetCancellation();
                base_cancel_model_->SubscribeMarketInterrupts(market_update_manager_);
                base_cancel_model_->AddCancellationListener(strategy_desc_.strategy_vec_[i].exec_);
                strategy_desc_.strategy_vec_[i].exec_->SetCancellationModelComponent(base_cancel_model_);
              }
            }

            market_update_manager_.AddMarketDataInterruptedListener(strategy_desc_.strategy_vec_[i].exec_);

            // TODO eventually we need to replace this by making riskmanager a listener of positions and SMV and exec
            // a
            // listener or riskmanager
            if (sid_to_prom_order_manager_map_[sec_id_vec[0]] != nullptr) {
              sid_to_prom_order_manager_map_[sec_id_vec[0]]->AddGlobalPositionChangeListener(
                  strategy_desc_.strategy_vec_[i].exec_);
            }
          }
        }

      }  // Add this as GlobalPNLChangeListener to p_prom_pnl_indicator_

      if (!ignore_user_msg_) {
        HFSAT::ControlMessageFileSource* control_messasge_filesource_ = nullptr;
        bool control_file_present = true;
        control_messasge_filesource_ =
            new HFSAT::ControlMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, sec_id_vec[0],
                                                sec_name_indexer_.GetSecurityNameFromId(sec_id_vec[0]),
                                                dep_trading_location_, runtime_id_, control_file_present);

        if (control_messasge_filesource_ != nullptr && control_file_present) {
          historical_dispatcher_.AddExternalDataListener(control_messasge_filesource_);
          control_messasge_filesource_->SetExternalTimeListener(&watch_);
          control_messasge_filesource_->AddControlMessageListener(strategy_desc_.strategy_vec_[i].exec_);
          if (runtime_id_ == 0) {
            strategy_desc_.strategy_vec_[i].exec_->SetStartTrading(true);
            if (strategy_desc_.strategy_vec_[i].strategy_name_.compare("NikPricePairBasedAggressiveTrading") == 0) {
              strategy_desc_.strategy_vec_[i].pair_exec_->SetStartTrading(true);
            }
          }
        } else {
          strategy_desc_.strategy_vec_[i].exec_->SetStartTrading(true);
          if (strategy_desc_.strategy_vec_[i].strategy_name_.compare("NikPricePairBasedAggressiveTrading") == 0) {
            strategy_desc_.strategy_vec_[i].pair_exec_->SetStartTrading(true);
          }
          std::cerr << " Control message source is nullptr or CONTROL file not present\n";
        }
      } else {
        strategy_desc_.strategy_vec_[i].exec_->SetStartTrading(true);
        if (strategy_desc_.strategy_vec_[i].strategy_name_.compare("NikPricePairBasedAggressiveTrading") == 0) {
          strategy_desc_.strategy_vec_[i].pair_exec_->SetStartTrading(true);
        }
      }
    }
  }

  void SetDepComponentsPortfolioStrategies(
      HFSAT::DebugLogger& dbglogger_, HFSAT::Watch& watch_, HFSAT::StrategyDesc& strategy_desc_,
      HFSAT::NetworkAccountInfoManager& network_account_info_manager_,
      HFSAT::HistoricalDispatcher& historical_dispatcher_,
      std::map<std::string, HFSAT::BaseSimMarketMaker*>& shortcode_to_smm_map,
      std::vector<HFSAT::MarketOrdersView*>& sid_to_mov_ptr_map_, CommonSMVSource* common_smv_source,
      bool& livetrading_, unsigned int& market_model_index_, HFSAT::SimTimeSeriesInfo& sim_time_series_info_,
      HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_,
      HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_,
      HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_,
      HFSAT::TradingLocation_t& dep_trading_location_, bool& ignore_user_msg_, int& tradingdate_,
      HFSAT::PromOrderManagerPtrVec& sid_to_prom_order_manager_map_, std::vector<int>& real_saci_vec_,
      std::vector<std::string>& dependant_shortcode_vec_, HFSAT::SecurityNameIndexer& sec_name_indexer_,
      HFSAT::BulkFileWriter& trades_writer_, std::vector<std::string>& portfolio_source_shortcode_vec_,
      HFSAT::EconomicEventsManager& economic_events_manager_) {
    for (auto i = 0u; i < strategy_desc_.portfolio_strategy_vec_.size(); i++) {
      std::vector<HFSAT::SecurityMarketView*> smv_vec_;
      std::vector<HFSAT::SecurityMarketView*> all_smv_vec_;
      std::vector<HFSAT::BaseTrader*> base_trader_vec_;
      std::vector<HFSAT::BaseSimMarketMaker*> smm_vec_;
      std::vector<HFSAT::SmartOrderManager*> som_vec_;
      std::vector<HFSAT::BasePNL*> base_pnl_vec_;
      std::vector<int> sec_id_vec;
      unsigned int runtime_id_ = strategy_desc_.portfolio_strategy_vec_[i].runtime_id_;
      HFSAT::MultBasePNL* p_mult_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);

      for (std::string& shortcode : portfolio_source_shortcode_vec_) {
        HFSAT::SecurityMarketView* this_dep_smv = shortcode_smv_map_.GetSecurityMarketView(shortcode);
        all_smv_vec_.push_back(this_dep_smv);
      }

      for (std::string& shortcode : dependant_shortcode_vec_) {
        HFSAT::SecurityMarketView* this_dep_smv = shortcode_smv_map_.GetSecurityMarketView(shortcode);
        smv_vec_.push_back(this_dep_smv);

        int security_id = sec_name_indexer_.GetIdFromString(shortcode);
        sec_id_vec.push_back(security_id);
        HFSAT::SecurityMarketView* this_sim_dep_smv = sid_to_sim_smv_ptr_map_[security_id];
        if (shortcode_to_smm_map.find(shortcode) == shortcode_to_smm_map.end()) {
          shortcode_to_smm_map[shortcode] = HFSAT::SimMarketMakerHelper::GetSimMarketMaker(
              dbglogger_, watch_, this_dep_smv, this_sim_dep_smv, market_model_index_, sim_time_series_info_,
              sid_to_mov_ptr_map_, historical_dispatcher_, common_smv_source, true);
        }
        smm_vec_.push_back(shortcode_to_smm_map[shortcode]);

        HFSAT::BaseTrader* this_base_trader =
            GetBaseTrader(network_account_info_manager_, this_dep_smv, shortcode_to_smm_map[shortcode]);
        base_trader_vec_.push_back(this_base_trader);

        // Subscribe SMM to ors messages
        if (shortcode_to_smm_map[shortcode]) {
          HFSAT::ORSMessageFileSource* ors_filesource =
              shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode);

          if (ors_filesource) {
            ors_filesource->AddOrderNotFoundListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderSequencedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderConfirmedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderConfCxlReplacedListener(shortcode_to_smm_map[shortcode]);
            // ors_filesource->AddOrderConfCxlReplaceRejectListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderCanceledListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderExecutedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderRejectedListener(shortcode_to_smm_map[shortcode]);
            ors_filesource->AddOrderCxlSeqdListener(shortcode_to_smm_map[shortcode]);
            // To enable handling for FOKs in simreal
            if (real_saci_vec_.size() > 0) {
              ors_filesource->AddOrderInternallyMatchedListener(shortcode_to_smm_map[shortcode]);
            }
          }
        }

        HFSAT::SmartOrderManager* this_smart_om =
            new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *this_base_trader, *this_dep_smv,
                                         strategy_desc_.portfolio_strategy_vec_[i].runtime_id_, livetrading_, 1);
        som_vec_.push_back(this_smart_om);
        //        if (real_saci_vec_.size() == strategy_desc_.strategy_vec_.size() && real_saci_vec_[i] > 0) {
        //          shortcode_to_smm_map[shortcode]->AssignRealSACI(smart_om->server_assigned_client_id_,
        // real_saci_vec_[i]);
        //        }
        int saci = this_smart_om->server_assigned_client_id_;
        std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_ =
            common_smv_source->getShortcodeFilesourceMap();
        for (unsigned j = 0; j < portfolio_source_shortcode_vec_.size(); j++) {
          auto filesource = shortcode_filesource_map_[portfolio_source_shortcode_vec_[j]];

          filesource->AddExternalDataListenerListener(shortcode_to_smm_map[shortcode]);
          int sec_id = sec_name_indexer_.GetIdFromString(portfolio_source_shortcode_vec_[j]);
          shortcode_to_smm_map[shortcode]->AddSecIdToSACI(saci, sec_id);
        }
        this_smart_om->ComputeQueueSizes();

        HFSAT::SimBasePNL* this_sim_base_pnl = nullptr;
        if (this_dep_smv->exch_source() == HFSAT::kExchSourceNSE) {
          this_sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *this_dep_smv, security_id, trades_writer_);
        } else {
          this_sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *this_dep_smv, runtime_id_, trades_writer_);
        }
        this_smart_om->SetBasePNL(this_sim_base_pnl);
        base_pnl_vec_.push_back(this_sim_base_pnl);
        int pnl_index_ = p_mult_base_pnl_->AddSecurity(this_sim_base_pnl) - 1;
        this_sim_base_pnl->AddListener(pnl_index_, p_mult_base_pnl_);

        // this_sim_dep_smv->subscribe_L1_Only(this_sim_base_pnl);

        shortcode_to_smm_map[shortcode]->AddOrderNotFoundListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderSequencedListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderConfirmedListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderConfCxlReplacedListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderCanceledListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderExecutedListener(this_smart_om);
        shortcode_to_smm_map[shortcode]->AddOrderRejectedListener(this_smart_om);
      }
      if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "MeanRevertingTrading") {
        strategy_desc_.portfolio_strategy_vec_[i].exec_ = new HFSAT::MeanRevertingTrading(
            dbglogger_, watch_, smv_vec_, som_vec_, strategy_desc_.portfolio_strategy_vec_[i].global_paramfilename_,
            livetrading_, p_mult_base_pnl_, strategy_desc_.portfolio_strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.portfolio_strategy_vec_[i].trading_end_utc_mfm_);

        // market_update_manager_.AddMarketDataInterruptedListener(strategy_desc_.portfolio_strategy_vec_[i].exec_);
        sid_to_prom_order_manager_map_[sec_id_vec[0]]->AddGlobalPositionChangeListener(
            strategy_desc_.portfolio_strategy_vec_[i].exec_);
      }

      if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "IndexFuturesMeanRevertingTrading") {
        strategy_desc_.portfolio_strategy_vec_[i].exec_ = new HFSAT::IndexFuturesMeanRevertingTrading(
            dbglogger_, watch_, all_smv_vec_, smv_vec_, som_vec_,
            strategy_desc_.portfolio_strategy_vec_[i].global_paramfilename_, livetrading_, p_mult_base_pnl_,
            economic_events_manager_, strategy_desc_.portfolio_strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.portfolio_strategy_vec_[i].trading_end_utc_mfm_, runtime_id_);

        // market_update_manager_.AddMarketDataInterruptedListener(strategy_desc_.portfolio_strategy_vec_[i].exec_);
        sid_to_prom_order_manager_map_[sec_id_vec[0]]->AddGlobalPositionChangeListener(
            strategy_desc_.portfolio_strategy_vec_[i].exec_);
      }

      if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "BasePortTrading") {
        strategy_desc_.portfolio_strategy_vec_[i].port_interface_ =
            new HFSAT::BasePortTrading(dbglogger_, watch_, smv_vec_, som_vec_, livetrading_, p_mult_base_pnl_,
                                       strategy_desc_.portfolio_strategy_vec_[i].trading_start_utc_mfm_,
                                       strategy_desc_.portfolio_strategy_vec_[i].trading_end_utc_mfm_);
        // create param
        // create_signal
        // link to exec

        const std::string t_signal_listfile_ = strategy_desc_.portfolio_strategy_vec_[i].prod_filename_;
        std::vector<HFSAT::Signal_BaseSignal*> t_signal_vec_;
        HFSAT::SignalCreator::CreateSignalInstance(dbglogger_, watch_, tradingdate_, t_signal_listfile_, t_signal_vec_);
        for (unsigned int i = 0u; i < t_signal_vec_.size(); i++) {
          t_signal_vec_[i]->add_port_risk_listener(strategy_desc_.portfolio_strategy_vec_[i].port_interface_);
        }
      }

      if (false && !ignore_user_msg_) {
        HFSAT::ControlMessageFileSource* control_messasge_filesource_ = nullptr;
        bool control_file_present = true;
        control_messasge_filesource_ = new HFSAT::ControlMessageFileSource(
            dbglogger_, sec_name_indexer_, tradingdate_, 0, sec_name_indexer_.GetSecurityNameFromId(sec_id_vec[0]),
            dep_trading_location_, runtime_id_, control_file_present);

        if (control_messasge_filesource_ != nullptr && control_file_present) {
          historical_dispatcher_.AddExternalDataListener(control_messasge_filesource_);
          control_messasge_filesource_->SetExternalTimeListener(&watch_);
          control_messasge_filesource_->AddControlMessageListener(strategy_desc_.portfolio_strategy_vec_[i].exec_);
        } else {
          strategy_desc_.portfolio_strategy_vec_[i].exec_->SetStartTrading(true);
          std::cerr << " Control message source is nullptr or CONTROL file not present\n";
        }
      } else {
        strategy_desc_.portfolio_strategy_vec_[i].exec_->SetStartTrading(true);
      }
    }
  }
};
