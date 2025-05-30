/**
   \file StratLogic/user_control_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef _USER_CONTROL_MANAGER_H
#define _USER_CONTROL_MANAGER_H

#include "tradeengine/Executioner/MarketOrderExecution.hpp"
#include "tradeengine/Executioner/TWAP.hpp"
#include "tradeengine/StratLogic/portfolio_risk_manager.hpp"
#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"

class UserControlManager : public HFSAT::ControlMessageListener {
 protected:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  std::vector<BaseTheoCalculator*> theo_vec_;
  std::vector<BaseTheoCalculator*> sqoff_theo_vec_;
  std::string position_file_;
  PortfolioRiskManager* risk_manager_;
  std::vector<HFSAT::BasicOrderManager*> basic_order_manager_vec_;
  std::vector<TWAP*> twap_vec_;
  std::vector<MarketOrderExecution*> mkt_order_exec_vec_;

 public:
  UserControlManager(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                     std::vector<BaseTheoCalculator*> _theo_vec_, std::vector<BaseTheoCalculator*> _sqoff_theo_vec_,
                     std::string _position_file_, PortfolioRiskManager* _risk_manager_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        theo_vec_(_theo_vec_),
        sqoff_theo_vec_(_sqoff_theo_vec_),
        position_file_(_position_file_),
        risk_manager_(_risk_manager_),
        basic_order_manager_vec_(),
        twap_vec_(),
        mkt_order_exec_vec_() {}

  UserControlManager(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                     std::vector<HFSAT::BasicOrderManager*> _basic_order_manager_vec_, std::vector<TWAP*> _twap_vec_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        theo_vec_(),
        sqoff_theo_vec_(),
        position_file_(),
        risk_manager_(),
        basic_order_manager_vec_(_basic_order_manager_vec_),
        twap_vec_(_twap_vec_),
        mkt_order_exec_vec_() {}

  UserControlManager(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                     std::vector<HFSAT::BasicOrderManager*> _basic_order_manager_vec_,
                     std::vector<MarketOrderExecution*> _mkt_order_exec_vec_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        theo_vec_(),
        sqoff_theo_vec_(),
        position_file_(),
        risk_manager_(),
        basic_order_manager_vec_(_basic_order_manager_vec_),
        twap_vec_(),
        mkt_order_exec_vec_(_mkt_order_exec_vec_) {}

  ~UserControlManager() {}

  void OnControlUpdate(const HFSAT::ControlMessage& _control_message_, const char* symbol_, const int trader_id) {
    switch (_control_message_.message_code_) {
      case HFSAT::kControlMessageCodeStartTrading: {
        DBGLOG_TIME_CLASS << "Start Trading Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->StartTrading();
          theo->NoSquareOff();
        }
      } break;
      case HFSAT::kControlMessageCodeAggGetflat: {
        DBGLOG_TIME_CLASS << "Aggressive Getflat Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->TurnOffTheo(CTRLMSG_STATUS_UNSET);
          theo->SquareOff(true);
        }
      } break;
      case HFSAT::kControlMessageCodeGetflat: {
        DBGLOG_TIME_CLASS << "Passive Getflat Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->TurnOffTheo(CTRLMSG_STATUS_UNSET);
          theo->SquareOff();
        }
      } break;
      case HFSAT::kControlMessageCodeDisableImprove: {
        DBGLOG_TIME_CLASS << "Turning off all theos called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->TurnOffTheo(CTRLMSG_STATUS_UNSET);
        }
        for (auto theo : sqoff_theo_vec_) {
          theo->TurnOffTheo(CTRLMSG_STATUS_UNSET);
        }
      } break;
      case HFSAT::kControlMessageCodeEnableImprove: {
        DBGLOG_TIME_CLASS << "Turning on all theos called deprecated" << DBGLOG_ENDL_FLUSH;
        //	     for (auto theo :theo_vec_) {
        //		theo->TurnOnTheo(CTRLMSG_STATUS_SET);
        //	     }
        //	     for (auto theo :sqoff_theo_vec_) {
        //		theo->TurnOnTheo(CTRLMSG_STATUS_SET);
        //	     }
      } break;
      case HFSAT::kControlMessageCodeShowIndicators: {
        DBGLOG_TIME_CLASS << "Reload configs called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->ReloadConfig();
        }
        for (auto theo : sqoff_theo_vec_) {
          theo->ReloadConfig();
        }
      } break;
      case HFSAT::kControlMessageDisableFreezeOnRejects: {
        DBGLOG_TIME_CLASS << "Disable Freeze On Rejects called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->UpdateAutoFreezeSystem(false);
        }
        for (auto theo : sqoff_theo_vec_) {
          theo->UpdateAutoFreezeSystem(false);
        }
      } break;
      case HFSAT::kControlMessageEnableFreezeOnRejects: {
        DBGLOG_TIME_CLASS << "Enable Freeze On Rejects called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : sqoff_theo_vec_) {
          theo->UpdateAutoFreezeSystem(true);
        }
        for (auto theo : theo_vec_) {
          theo->UpdateAutoFreezeSystem(true);
        }
      } break;
      case HFSAT::kControlMessageUnfreezeDataEntryReject: {
         DBGLOG_TIME_CLASS << "Enable KControlMessageUnfreezeDataEntryReject On Rejects called" << DBGLOG_ENDL_FLUSH;
	for (auto theo : theo_vec_) {
          theo->OrderDataEntryFreezeDisable();
	}
	for (auto theo : sqoff_theo_vec_) {
          theo->OrderDataEntryFreezeDisable();
        }
      } break;
      case HFSAT::kControlMessageCodeSetMaxPosition: {
        DBGLOG_TIME_CLASS << "Reloading positions file called" << DBGLOG_ENDL_FLUSH;
        std::map<std::string, std::string> pos_key_val_map_;
        Parser::ParseConfig(position_file_, pos_key_val_map_);
        for (auto theo : theo_vec_) {
          theo->LoadPositionCheck(pos_key_val_map_);
        }
        // No need to load for square off since it has same order manager
        // In case that logic changes, uncomment this.
        /*for (auto theo : sqoff_theo_vec_) {
                theo->LoadPositionCheck(false);
        }*/

        if (risk_manager_) {
          risk_manager_->ResetRiskChecks(pos_key_val_map_);
        }

      } break;
      case HFSAT::kControlMessageCodeDumpPositions: {
        DBGLOG_TIME_CLASS << "Dump Pnl stats called" << DBGLOG_ENDL_FLUSH;
        int sum_pnl_ = 0;
        double sum_net_exp_ = 0;
        double sum_gross_exp_ = 0;
        double sum_ttv_ = 0;
        int num_unhedged_theo_ = 0;
        int total_orders_ = 0;
        double sum_total_traded_value_nohedge_ = 0;
        double sum_total_traded_mkt_quantity_nohedge_ = 0;

        for (auto theo : theo_vec_) {
          theo->PNLStats(NULL, false);
          sum_pnl_ += theo->GetPNL();
          sum_net_exp_ += theo->GetExposure();
          sum_gross_exp_ += std::abs(theo->GetExposure());
          sum_ttv_ += theo->GetTTV();
          total_orders_ += theo->GetTotalOrderCount();
          if ((theo->IsNeedToHedge() == false) && (theo->GetTotalPosition() != 0)) {
            num_unhedged_theo_++;
          }
          if (!theo->IsParentTheoPresent()) {
            sum_total_traded_value_nohedge_ += theo->GetTTV();
            sum_total_traded_mkt_quantity_nohedge_ += theo->GetTotalTradedTTV();
          }
        }
        double mkt_volume_traded_ =
            (sum_total_traded_mkt_quantity_nohedge_ != 0)
                ? (sum_total_traded_value_nohedge_ / sum_total_traded_mkt_quantity_nohedge_) * 100
                : 0;
        dbglogger_ << "PORTFOLIO PNL: " << sum_pnl_ << " NETEXP: " << sum_net_exp_ << " GROSSEXP: " << sum_gross_exp_
                   << " TTV: " << sum_ttv_ << " NUMOPEN: " << num_unhedged_theo_ << " NUM_ORDERS: " << total_orders_
                   << " MKTPERCENT: " << mkt_volume_traded_ << "\n"
                   << DBGLOG_ENDL_FLUSH;
      } break;
      case HFSAT::kControlMessageCodeShowOrders: {
        DBGLOG_TIME_CLASS << "Show Unhedged Positions called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          if ((theo->IsNeedToHedge() == false) && (theo->GetTotalPosition() != 0)) {
            dbglogger_ << theo->ticker_name_ << ": " << theo->GetTotalPosition() << DBGLOG_ENDL_FLUSH;
          }
        }
      } break;
      case HFSAT::kControlMessageCodeSetUnitTradeSize: {
        DBGLOG_TIME_CLASS << "Setting flag called" << DBGLOG_ENDL_FLUSH;
        uint16_t flag = std::strtoul(_control_message_.strval_1_, NULL, 16);
        if (_control_message_.intval_1_ > 0) {
          dbglogger_ << "Setting flag as per " << _control_message_.strval_1_ << "for all theo" << DBGLOG_ENDL_FLUSH;
          for (auto theo : theo_vec_) {
            theo->TurnOnTheo(flag);
          }
          for (auto theo : sqoff_theo_vec_) {
            theo->TurnOnTheo(flag);
          }
          if ((flag & FREEZE_STATUS_SET) == FREEZE_STATUS_SET) {
            for (auto theo : theo_vec_) {
              theo->ResetFreezeCounters();
            }
            for (auto theo : sqoff_theo_vec_) {
              theo->ResetFreezeCounters();
            }
          }
          if ((flag & ORSRISK_STATUS_SET) == ORSRISK_STATUS_SET) {
            for (auto theo : theo_vec_) {
              theo->ResetOMRiskCounters();
            }
            for (auto theo : sqoff_theo_vec_) {
              theo->ResetOMRiskCounters();
            }
          }
        } else {
          dbglogger_ << "Unsetting flag as per " << _control_message_.strval_1_ << "for all theo" << DBGLOG_ENDL_FLUSH;
          for (auto theo : theo_vec_) {
            theo->TurnOffTheo(flag);
          }
          for (auto theo : sqoff_theo_vec_) {
            theo->TurnOffTheo(flag);
          }
        }

      } break;
      case HFSAT::kControlMessageCodeCancelAllFreezeTrading: {
        for (auto basic_om_ : basic_order_manager_vec_) {
          basic_om_->CancelAllOrders();
        }
      } break;
      case HFSAT::kControlMessageReloadEconomicEvents: {
        for (auto twap_ : twap_vec_) {
          twap_->PrintStatus();
        }
        for (auto mkt_order_exec_ : mkt_order_exec_vec_) {
          mkt_order_exec_->PrintStatus();
        }
      } break;

      case HFSAT::kControlMessageCodeDisableAggressive: {
        DBGLOG_TIME_CLASS << "Reduce Position Mode Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->SetPassiveReduce(true);
          theo->SetAggressiveReduce(true);
        }
      } break;
      case HFSAT::kControlMessageCodeForceAllIndicatorReady: {
        for (auto mkt_order_exec_ : mkt_order_exec_vec_) {
          mkt_order_exec_->OnTimePeriodUpdate();
        }
      } break;
      case HFSAT::kControlMessageCodeEnableAggressive: {
        DBGLOG_TIME_CLASS << "Efficient Squareoff Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->SetEfficientSquareOff(true);
        }
      } break;
      case HFSAT::kControlMessageEnableSelfOrderCheck: {
        DBGLOG_TIME_CLASS << "Log Full Order Status Called" << DBGLOG_ENDL_FLUSH;
        for (auto theo : theo_vec_) {
          theo->LogFullOrderStatus();
        }
      } break;
      case HFSAT::kControlMessageEnableEfficientSquareoff: {
        double eff_sq_multiplier_ = strtod(_control_message_.strval_1_, NULL);
        DBGLOG_TIME_CLASS << "Efficient Squareoff called with multiplier " << eff_sq_multiplier_ << DBGLOG_ENDL_FLUSH;
        // uint16_t flag = std::strtoul(_control_message_.doubleval_1_,NULL,16);
        if (eff_sq_multiplier_ > 0) {
          for (auto theo : theo_vec_) {
            theo->EnableEfficientSquareoff(eff_sq_multiplier_);
          }
        }

      } break;
      case HFSAT::kControlMessageCodeAddPosition:
      case HFSAT::kControlMessageCodeCleanSumSizeMaps:
      case HFSAT::kControlMessageDisableSelfOrderCheck:
      case HFSAT::kControlMessageCodeEnableAggCooloff:
      case HFSAT::kControlMessageCodeDisableAggCooloff:
      case HFSAT::kControlMessageCodeEnableNonStandardCheck:
      case HFSAT::kControlMessageCodeDisableNonStandardCheck:
      case HFSAT::kControlMessageCodeSetMaxIntSpreadToPlace:
      case HFSAT::kControlMessageCodeSetMaxIntLevelDiffToPlace:
      case HFSAT::kControlMessageCodeSetExplicitMaxLongPosition:
      case HFSAT::kControlMessageDisableMarketManager:
      case HFSAT::kControlMessageEnableMarketManager:
      case HFSAT::kControlMessageCodeEnableLogging:
      case HFSAT::kControlMessageCodeDisableLogging:
      case HFSAT::kControlMessageCodeEnableZeroLoggingMode:
      case HFSAT::kControlMessageCodeSetStartTime:
      case HFSAT::kControlMessageCodeSetEndTime:
      case HFSAT::kControlMessageCodeDisableZeroLoggingMode:
      case HFSAT::kControlMessageCodeSetMaxLoss:
      case HFSAT::kControlMessageSetDeltaThreshold:
      case HFSAT::kControlMessageCodeSetOpenTradeLoss:
        break;
      default:
        break;
    }
    dbglogger_.DumpCurrentBuffer();
  }
};

#endif  // _USER_CONTROL_MANAGER_H
