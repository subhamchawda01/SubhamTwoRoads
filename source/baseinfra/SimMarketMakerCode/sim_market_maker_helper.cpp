// =====================================================================================
//
//       Filename:  sim_market_maker_helper.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/01/2015 05:30:16 PM
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

#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"

namespace HFSAT {

BaseSimMarketMaker* SimMarketMakerHelper::GetSimMarketMaker(DebugLogger& dbglogger_, HFSAT::Watch& watch_,
                                                            HFSAT::SecurityMarketView* dep_smv,
                                                            HFSAT::SecurityMarketView* sim_smv, int market_model_index,
                                                            HFSAT::SimTimeSeriesInfo& sim_time_series_info,
                                                            std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map,
                                                            HFSAT::HistoricalDispatcher& historical_dispatcher,
                                                            CommonSMVSource* common_smv_source, bool unique_instance) {
  if (dep_smv == nullptr || sim_smv == nullptr || dep_smv->exch_source() != sim_smv->exch_source() ||
      dep_smv->security_id() != sim_smv->security_id()) {
    return nullptr;
  }

  // For exchanges which have separate sim/query smv instances
  if (dep_smv->exch_source() != HFSAT::kExchSourceLIFFE && dep_smv->exch_source() != HFSAT::kExchSourceICE &&
      dep_smv->exch_source() != HFSAT::kExchSourceTMX) {
    sim_smv = dep_smv;
  }

  std::string shortcode = dep_smv->shortcode();
  int security_id = dep_smv->security_id();
  int tradingdate = watch_.YYYYMMDD();
  HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::PriceLevelSimMarketMaker* plsmm = nullptr;
  HFSAT::OrderLevelSim* olsmm = nullptr;
  HFSAT::OrderLevelSimMarketMaker2* olsmm2 = nullptr;

  SMMType smm_type_ = kSMMPriceLevel;
  if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
      sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ &&
      GetSMMType(dep_smv, tradingdate) == kSMMOrderLevel) {
    smm_type_ = kSMMOrderLevel;

    if (tradingdate >= 20170414 && tradingdate <= 20170527 && dep_smv->exch_source() == HFSAT::kExchSourceJPY) {
      smm_type_ = kSMMPriceLevel;
    }
  }

  switch (dep_smv->exch_source()) {
    case HFSAT::kExchSourceCME: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance) {
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        } else {
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        }
        HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
        sid_to_mov_ptr_map[security_id] = mov;

        HFSAT::CMEMarketOrderManager* cme_market_order_manager =
            new HFSAT::CMEMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);

        auto filesource = new HFSAT::CMEOBFLoggedMessageFileSource(
            dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocCHI, false);
        filesource->SetExternalTimeListener(&watch_);
        filesource->AddOrderLevelListener(cme_market_order_manager);
        historical_dispatcher.AddExternalDataListener(filesource, true);
        return olsmm;

      } else {
        if (unique_instance) {
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        } else {
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);
        }

        plsmm->SubscribeL2Events(*dep_smv);
        // Order Feed not available or set to false
        HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *sim_smv, market_model_index, sim_time_series_info);
        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }
    case HFSAT::kExchSourceTMX:
    case HFSAT::kExchSourceKRX:
    case HFSAT::kExchSourceLIFFE:
    case HFSAT::kExchSourceRTS:
    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceSGX: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);

        // In case of order level feed for sgx, we will need to create order book manager and subscribe to it.
        HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
        sid_to_mov_ptr_map[security_id] = mov;
        HFSAT::SGXMarketOrderManager* sgx_market_order_manager =
            new HFSAT::SGXMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);

        auto filesource = new HFSAT::SGXOrderLoggedMesageFileSource(
            dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocSPR, true);
        filesource->SetExternalTimeListener(&watch_);
        filesource->AddSGXListener(sgx_market_order_manager);
        historical_dispatcher.AddExternalDataListener(filesource, true);

        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }
    case HFSAT::kExchSourceBSE:
    case HFSAT::kExchSourceMICEX_EQ:
    case HFSAT::kExchSourceMICEX_CR:
    case HFSAT::kExchSourceCFE: {
      if (unique_instance)
        plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                   sim_time_series_info);
      else
        plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                             sim_time_series_info);
      plsmm->SubscribeL2Events(*dep_smv);
      return plsmm;
      break;
    }

    case HFSAT::kExchSourceNSE: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_) {
          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[security_id] = mov;

          bool is_hidden_order_available_ = HFSAT::NSESecurityDefinitions::IsHiddenOrderAvailable(dep_smv->shortcode());

          HFSAT::NSEMarketOrderManager* nse_market_order_manager = new HFSAT::NSEMarketOrderManager(
              dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map, is_hidden_order_available_);

          auto filesource = common_smv_source->nse_logged_message_filesource(dep_smv->shortcode());
          if (filesource == nullptr) {
            filesource = new HFSAT::NSELoggedMessageFileSource(dbglogger_, sec_name_indexer, tradingdate, security_id,
                                                               dep_smv->secname(), HFSAT::kTLocNSE);
            filesource->SetOrderGlobalListenerNSE(nse_market_order_manager);
          } else {
            filesource->SetOrderLevelListenerSim(nse_market_order_manager);
          }
          filesource->SetExternalTimeListener(&watch_);
          historical_dispatcher.AddExternalDataListener(filesource, true);
        }
        return olsmm;
      } else {
        if (unique_instance)

          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
        break;
      }
      break;
    }

    case HFSAT::kExchSourceASX: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20151201) {
          // In case of Itch feed for asx, we will need to create order book manager and subscribe to it.
          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[security_id] = mov;
          HFSAT::AsxMarketOrderManager* asx_market_order_manager =
              new HFSAT::AsxMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);

          if (tradingdate >= USING_ASX_NTP_ITCH_FROM) {
            auto filesource = new HFSAT::ASXItchLoggedMessageFileSource(
                dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocSYD);
            filesource->SetExternalTimeListener(&watch_);
            filesource->AddASXListener(asx_market_order_manager);
            historical_dispatcher.AddExternalDataListener(filesource, true);
          } else {
            auto filesource = new HFSAT::ASXLoggedMessageFileSource(dbglogger_, sec_name_indexer, tradingdate,
                                                                    security_id, dep_smv->secname(), HFSAT::kTLocSYD);
            filesource->SetExternalTimeListener(&watch_);
            filesource->AddASXListener(asx_market_order_manager);
            historical_dispatcher.AddExternalDataListener(filesource, true);
          }
        }
        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
        break;
      }
      break;
    }

    case HFSAT::kExchSourceEUREX: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20140425) {
          // Since the time EOBI is available, use order level sim
          HFSAT::EOBILoggedMessageFileSource* eobi_filesource =
              new HFSAT::EOBILoggedMessageFileSource(dbglogger_, sec_name_indexer, tradingdate, security_id,
                                                     dep_smv->secname(), HFSAT::kTLocFR2, false, false);
          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[security_id] = mov;
          HFSAT::EOBIMarketOrderManager* eobi_market_order_manager =
              new HFSAT::EOBIMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);
          eobi_filesource->SetOrderLevelListenerSim(eobi_market_order_manager);
          eobi_filesource->SetExternalTimeListener(&watch_);
          historical_dispatcher.AddExternalDataListener(eobi_filesource, true);
        }
        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }

    case HFSAT::kExchSourceBMF:
    case HFSAT::kExchSourceNTP:
    case HFSAT::kExchSourceBMFEQ: {
      if (smm_type_ == kSMMOrderLevel) {
        // TODO: set this to 20140424 after order sim consistently remains accurate
        if (unique_instance)
          olsmm2 = HFSAT::OrderLevelSimMarketMaker2::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                                       sim_time_series_info);
        else
          olsmm2 = HFSAT::OrderLevelSimMarketMaker2::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                                 sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20140507 &&
            (tradingdate != 20150615 && tradingdate != 20150911)) {
          // with assumption that the dates are correct, we are using order level sim
          // TODO: Someone please comment on the extra two dates why specifically these are excluded
          // TODO: set this to 20140424 after order sim consistently remains accurate

          HFSAT::NTPLoggedMessageFileSource* ntp_filesource =
              new HFSAT::NTPLoggedMessageFileSource(dbglogger_, sec_name_indexer, tradingdate, security_id,
                                                    dep_smv->secname(), HFSAT::kTLocBMF, true, false, false);
          ntp_filesource->SetOrderLevelListenerSim((HFSAT::OrderLevelSimMarketMaker2*)olsmm2);
          ntp_filesource->SetExternalTimeListener(&watch_);
          historical_dispatcher.AddExternalDataListener(ntp_filesource, true);
        }
        return olsmm2;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }

    case HFSAT::kExchSourceICE: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20140507) {
          // TODO: set this to 20140424 after order sim consistently remains accurate
          // we have OderLevel data available, we can use that.

          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[security_id] = mov;
          HFSAT::IceMarketOrderManager* ice_market_order_manager =
              new HFSAT::IceMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);

          HFSAT::ICELoggedMessageFileSource* ice_filesource = new HFSAT::ICELoggedMessageFileSource(
              dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocBSL, true);

          // Delay the orderfeed for few microseconds in case of LFI/LFL
          // WHY ?
          if (shortcode.find("LFI") != std::string::npos || shortcode.find("LFL") != std::string::npos) {
            ice_filesource->AddDelay(30);

            HFSAT::ICELoggedMessageFileSource* pf_filesource = new HFSAT::ICELoggedMessageFileSource(
                dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocBSL, false);
            pf_filesource->AddTradeListener((HFSAT::OrderLevelSim*)olsmm2);
            pf_filesource->SetExternalTimeListener(&watch_);
            historical_dispatcher.AddExternalDataListener(pf_filesource);
          }
          ice_filesource->SetExternalTimeListener(&watch_);
          ice_filesource->AddOrderLevelListener(ice_market_order_manager);

          historical_dispatcher.AddExternalDataListener(ice_filesource, true);
        }
        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }

    case HFSAT::kExchSourceHKOMD:
    case HFSAT::kExchSourceHKOMDCPF:
    case HFSAT::kExchSourceHKOMDPF:
    case HFSAT::kExchSourceHONGKONG: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20150101 &&
            tradingdate != 20150302 && tradingdate != 20150401) {
          // Order feed available here.
          // Whoever made the exact date exlude changes, please put the reason here.
          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[security_id] = mov;
          HFSAT::HKMarketOrderManager* hk_market_order_manager =
              new HFSAT::HKMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map);

          auto omd_filesource = new HFSAT::HKOMDLoggedMessageFileSource(dbglogger_, sec_name_indexer, tradingdate,
                                                                        security_id, dep_smv->secname(), HFSAT::kTLocHK,
                                                                        false, true, IsHKEquity(dep_smv->shortcode()));
          omd_filesource->SetExternalTimeListener(&watch_);
          omd_filesource->AddOrderLevelListener(hk_market_order_manager);
          historical_dispatcher.AddExternalDataListener(omd_filesource, true);
        }
        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
      }
      break;
    }

    case HFSAT::kExchSourceJPY: {
      if (smm_type_ == kSMMOrderLevel) {
        if (unique_instance)
          olsmm = HFSAT::OrderLevelSim::GetUniqueInstance(dbglogger_, watch_, *dep_smv, market_model_index,
                                                          sim_time_series_info);
        else
          olsmm =
              HFSAT::OrderLevelSim::GetInstance(dbglogger_, watch_, *dep_smv, market_model_index, sim_time_series_info);
        // Order level sim was not working fine before ITCH (20160719). Hence disabled (using false &&) TODO: fix this
        if (false && (int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
            sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ && tradingdate >= 20140520) {
          // Doesn't get called!!
          // TODO: set this to 20140418 after order sim consistently remains accurate

          auto ose_filesource = new HFSAT::OSELoggedMessageFileSource(
              dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocJPY, false);

          ose_filesource->SetOrderLevelListenerSim((HFSAT::OrderLevelSimMarketMaker2*)olsmm);
          ose_filesource->SetExternalTimeListener(&watch_);

          historical_dispatcher.AddExternalDataListener(ose_filesource, true);

          return olsmm;
        } else if ((int)sim_time_series_info.sid_to_sim_config_.size() > security_id &&
                   sim_time_series_info.sid_to_sim_config_[security_id].use_order_level_sim_ &&
                   tradingdate >= 20160719) {
          // Same as ASX
          HFSAT::MarketOrdersView* mov = HFSAT::MarketOrdersView::GetUniqueInstance(dbglogger_, watch_, security_id);
          sid_to_mov_ptr_map[dep_smv->security_id()] = mov;
          HFSAT::AsxMarketOrderManager* ose_market_order_manager =
              new HFSAT::AsxMarketOrderManager(dbglogger_, watch_, sec_name_indexer, sid_to_mov_ptr_map, true);
          auto filesource = new HFSAT::OSEItchLoggedMessageFileSource(
              dbglogger_, sec_name_indexer, tradingdate, security_id, dep_smv->secname(), HFSAT::kTLocJPY, false);
          filesource->SetExternalTimeListener(&watch_);
          filesource->AddOSEListener(ose_market_order_manager);
          historical_dispatcher.AddExternalDataListener(filesource, true);
        }
        return olsmm;
      } else {
        if (unique_instance)
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                                     sim_time_series_info);
        else
          plsmm = HFSAT::PriceLevelSimMarketMaker::GetInstance(dbglogger_, watch_, *sim_smv, market_model_index,
                                                               sim_time_series_info);

        plsmm->SubscribeL2Events(*dep_smv);
        return plsmm;
        break;
      }
      break;
    }

    default: { break; }
  }
  return nullptr;
}

SMMType SimMarketMakerHelper::GetSMMType(const SecurityMarketView* _dep_smv_, int _YYYYMMDD_) {
  switch (_dep_smv_->exch_source()) {
    case HFSAT::kExchSourceCME: {
      if (_YYYYMMDD_ >= USING_CME_OBF) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceASX: {
      if (_YYYYMMDD_ >= 20151201) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceEUREX: {
      if (_YYYYMMDD_ >= 20140425) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceNSE: {
      if (_YYYYMMDD_ >= 20150911) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceBMF:
    case HFSAT::kExchSourceNTP:
    case HFSAT::kExchSourceBMFEQ: {
      if (_YYYYMMDD_ >= 20140507 && _YYYYMMDD_ != 20150615) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceICE: {
      if (_YYYYMMDD_ >= 20140507 &&
          !(_dep_smv_->shortcode() == "LFZ_0" && _YYYYMMDD_ == 20150703))  // LFZ data issue for OLSSM on 20150703
      {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceHKOMD:
    case HFSAT::kExchSourceHKOMDCPF:
    case HFSAT::kExchSourceHKOMDPF:
    case HFSAT::kExchSourceHONGKONG: {
      if (_YYYYMMDD_ >= 20150101 && _YYYYMMDD_ != 20150302 && _YYYYMMDD_ != 20150401) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceJPY: {
      if (_YYYYMMDD_ >= 20160716) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    case HFSAT::kExchSourceSGX: {
      if (_YYYYMMDD_ >= 20170101) {
        return kSMMOrderLevel;
      } else {
        return kSMMPriceLevel;
      }
      break;
    }
    default:
      return kSMMPriceLevel;
  }
}
}
