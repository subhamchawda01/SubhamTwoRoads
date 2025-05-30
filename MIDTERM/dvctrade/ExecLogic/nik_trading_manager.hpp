/**
   \file ExecLogic/nik_trading_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_EXECLOGIC_NIK_TRADING_MANAGER_H
#define BASE_EXECLOGIC_NIK_TRADING_MANAGER_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

typedef unsigned int uint32_t;

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class NikTradingManagerListener {
 public:
  virtual ~NikTradingManagerListener(){};
  virtual void UpdateCombinedPosition(int _risk_) = 0;
  virtual void CancelBestBid() = 0;
  virtual void CancelBestAsk() = 0;
  virtual void AggressiveBuy() = 0;
  virtual void AggressiveSell() = 0;
  virtual SmartOrderManager& order_manager() = 0;
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) = 0;
};

class NikTradingManager : public TradingManager,
                          public SecurityMarketViewChangeListener,
                          public MultBasePNLListener,
                          public ControlMessageListener {
 protected:
  SecurityMarketView* p_nk_market_view_;
  SecurityMarketView* p_nkm_market_view_;
  NikTradingManagerListener* nk_exec_;
  NikTradingManagerListener* nkm_exec_;

 public:
  int nk_pos_;
  int nkm_pos_;
  double nk_bestbid_price_;
  unsigned int nk_bestbid_size_;
  double nk_bestask_price_;
  unsigned int nk_bestask_size_;
  double nkm_bestbid_price_;
  unsigned int nkm_bestbid_size_;
  double nkm_bestask_price_;
  unsigned int nkm_bestask_size_;
  int last_pair_buy_msecs_;
  int last_pair_sell_msecs_;

  bool livetrading_;
  bool getting_flat_;

  int max_opentrade_loss_;
  int max_loss_;
  int break_msecs_on_max_opentrade_loss_;
  int last_max_opentrade_loss_hit_msecs_;
  bool is_max_opentrade_loss_set_;
  bool is_max_loss_set_;
  bool is_break_msecs_on_max_opentrade_loss_set_;

  int combined_position_;
  MultBasePNL* mult_base_pnl_;

  int total_pnl_;
  int realized_pnl_;
  int open_unrealized_pnl_;

  NikTradingManager(DebugLogger& t_dbglogger_, const Watch& t_watch_, MultBasePNL* t_mult_base_pnl_,
                    const bool t_livetrading_)
      : TradingManager(t_dbglogger_, t_watch_),
        p_nk_market_view_(NULL),
        p_nkm_market_view_(NULL),
        nk_exec_(NULL),
        nkm_exec_(NULL),
        nk_pos_(0),
        nkm_pos_(0),
        nk_bestbid_price_(0.0),
        nk_bestbid_size_(1),
        nk_bestask_price_(0),
        nk_bestask_size_(1),
        nkm_bestbid_price_(0.0),
        nkm_bestbid_size_(1),
        nkm_bestask_price_(0),
        nkm_bestask_size_(1),
        last_pair_buy_msecs_(0),
        last_pair_sell_msecs_(0),
        livetrading_(t_livetrading_),
        getting_flat_(false),
        max_opentrade_loss_(-100000),
        max_loss_(-100000),
        break_msecs_on_max_opentrade_loss_(900000),
        last_max_opentrade_loss_hit_msecs_(0),
        is_max_opentrade_loss_set_(false),
        is_max_loss_set_(false),
        is_break_msecs_on_max_opentrade_loss_set_(false),
        combined_position_(0),
        mult_base_pnl_(t_mult_base_pnl_),
        total_pnl_(0),
        realized_pnl_(0),
        open_unrealized_pnl_(0) {
    mult_base_pnl_->AddListener(this);
  }

  inline void AddListener(unsigned int _security_id_, SecurityMarketView* p_dep_market_view_,
                          NikTradingManagerListener* p_this_listener_) {
    p_dep_market_view_->subscribe_price_type(this, kPriceTypeMidprice);
    if (_security_id_ == 0 && p_dep_market_view_->shortcode().compare("NK_0") == 0) {
      p_nk_market_view_ = p_dep_market_view_;
      nk_exec_ = p_this_listener_;
    } else if (_security_id_ == 1 && p_dep_market_view_->shortcode().compare("NKM_0") == 0) {
      p_nkm_market_view_ = p_dep_market_view_;
      nkm_exec_ = p_this_listener_;
    } else {
      // ExitVerbose
    }
  }

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
    if (_security_id_ == 0) {
      nk_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      nk_bestask_price_ = cr_market_update_info_.bestask_price_;
      nk_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      nk_bestask_size_ = cr_market_update_info_.bestask_size_;
    } else if (_security_id_ == 1) {
      nkm_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      nkm_bestask_price_ = cr_market_update_info_.bestask_price_;
      nkm_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      nkm_bestask_size_ = cr_market_update_info_.bestask_size_;
    }
  }

  inline void GetFlat() { getting_flat_ = true; }

  inline void ResumeAfterGetFlat() { getting_flat_ = false; }

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id) {
    switch (_control_message_.message_code_) {
      case kControlMessageCodeGetflat: {
        if (!getting_flat_) {
          if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
            DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
            if (livetrading_) {
              DBGLOG_DUMP;
            }
          }
          GetFlat();

          nk_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
          nkm_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      } break;
      case kControlMessageCodeStartTrading: {
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
        ResumeAfterGetFlat();

        nk_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
        nkm_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
      } break;
      case kControlMessageCodeDumpPositions: {
        DBGLOG_TIME_CLASS << "DumpPositions" << DBGLOG_ENDL_FLUSH;
        if (livetrading_) {
          DBGLOG_DUMP;
        }
        DumpPositions();
      } break;

      case kControlMessageCodeSetMaxGlobalRisk:
      case kControlMessageCodeFreezeTrading:
      case kControlMessageCodeUnFreezeTrading:
      case kControlMessageCodeCancelAllFreezeTrading:
      case kControlMessageCodeSetTradeSizes:
      case kControlMessageCodeSetUnitTradeSize:
      case kControlMessageCodeSetMaxUnitRatio:
      case kControlMessageCodeSetMaxPosition:
      case kControlMessageCodeSetWorstCaseUnitRatio:
      case kControlMessageCodeAddPosition:
      case kControlMessageCodeSetWorstCasePosition:
      case kControlMessageCodeDisableImprove:
      case kControlMessageCodeEnableImprove:
      case kControlMessageCodeDisableAggressive:
      case kControlMessageCodeEnableAggressive:
      case kControlMessageCodeShowParams:
      case kControlMessageCodeCleanSumSizeMaps:
      case kControlMessageCodeSetEcoSeverity:
      case kControlMessageCodeForceIndicatorReady:
      case kControlMessageCodeForceAllIndicatorReady:
      case kControlMessageDisableSelfOrderCheck:
      case kControlMessageEnableSelfOrderCheck:
      case kControlMessageDumpNonSelfSMV:
      case kControlMessageCodeEnableAggCooloff:
      case kControlMessageCodeDisableAggCooloff:
      case kControlMessageCodeEnableNonStandardCheck:
      case kControlMessageCodeDisableNonStandardCheck:
      case kControlMessageCodeSetMaxIntSpreadToPlace:
      case kControlMessageCodeSetMaxIntLevelDiffToPlace:
      case kControlMessageCodeSetExplicitMaxLongPosition:
      case kControlMessageCodeSetExplicitWorstLongPosition: {
        if (strcmp(_control_message_.strval_1_, "NK_0") == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode NK_0" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
          nk_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
        }
        if (strcmp(_control_message_.strval_1_, "NKM_0") == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode NKM_0" << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
          nkm_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      } break;
      case kControlMessageCodeShowIndicators:
      case kControlMessageDisableMarketManager:
      case kControlMessageEnableMarketManager:
      case kControlMessageCodeEnableLogging:
      case kControlMessageCodeDisableLogging:
      case kControlMessageCodeShowOrders:
      case kControlMessageCodeEnableZeroLoggingMode:
      case kControlMessageCodeDisableZeroLoggingMode:
      case kControlMessageCodeSetStartTime:
      case kControlMessageCodeSetEndTime: {
        nk_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
        nkm_exec_->OnControlUpdate(_control_message_, symbol, trader_id);
      } break;

      case kControlMessageCodeSetMaxLoss: {
        if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
          max_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << max_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      case kControlMessageCodeSetOpenTradeLoss: {
        if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
          max_opentrade_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                            << " called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;

          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
        if (_control_message_.intval_1_ > 0) {
          break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
          break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                            << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                            << DBGLOG_ENDL_FLUSH;
          if (livetrading_) {
            DBGLOG_DUMP;
          }
        }
      } break;

      default:
        break;
    }
  }

  inline int GetNkPosition() const { return nk_pos_; }

  inline void BuyNKM() { nkm_exec_->AggressiveBuy(); }

  inline void BuyNK() { nk_exec_->AggressiveBuy(); }

  inline void SellNKM() { nkm_exec_->AggressiveSell(); }

  inline void SellNK() { nk_exec_->AggressiveSell(); }

  inline int GetNkmPosition() const { return nkm_pos_; }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& cr_market_update_info_) {
    if (_security_id_ == 0) {
      nk_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      nk_bestask_price_ = cr_market_update_info_.bestask_price_;
      nk_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      nk_bestask_size_ = cr_market_update_info_.bestask_size_;
    } else if (_security_id_ == 1) {
      nkm_bestbid_price_ = cr_market_update_info_.bestbid_price_;
      nkm_bestask_price_ = cr_market_update_info_.bestask_price_;
      nkm_bestbid_size_ = cr_market_update_info_.bestbid_size_;
      nkm_bestask_size_ = cr_market_update_info_.bestask_size_;
    }
  }

  inline void OnPositionUpdate(const unsigned int _security_id_, const int _new_position_) {
    if (_security_id_ == 0) {
      nk_pos_ = _new_position_;
    } else if (_security_id_ == 1) {
      nkm_pos_ = _new_position_;
    }

    int new_combined_position_ = nk_pos_ * 10 + nkm_pos_;

    // If the combined position sign flips, reset open_unrealized_pnl and change realized pnl here.
    if (combined_position_ * new_combined_position_ < 0) {
      open_unrealized_pnl_ = 0;
      realized_pnl_ = total_pnl_;
    }

    combined_position_ = new_combined_position_;

    //      mult_base_pnl_-> UpdateTotalRisk ( combined_position_ );

    nk_exec_->UpdateCombinedPosition(combined_position_ / 10);
    nkm_exec_->UpdateCombinedPosition(combined_position_);
  }

  inline void OnExec(const int _security_id_, const int t_new_position_, const int _exec_quantity_,
                     const TradeType_t _buysell_, const double _price_, const int r_int_price_) {
    if (_security_id_ == 0 || _security_id_ == 1) {
      if (_buysell_ == kTradeTypeBuy) {
        last_pair_buy_msecs_ = watch_.msecs_from_midnight();
      } else {
        last_pair_sell_msecs_ = watch_.msecs_from_midnight();
      }
    }

    if (_security_id_ == 0) {
      mult_base_pnl_->UpdateTotalRisk(t_new_position_ * 10 + nkm_pos_);
    } else if (_security_id_ == 1) {
      mult_base_pnl_->UpdateTotalRisk(nk_pos_ * 10 + t_new_position_);
    }
  }

  inline void UpdatePNL(int _total_pnl_) {
    total_pnl_ = _total_pnl_;
    open_unrealized_pnl_ = total_pnl_ - realized_pnl_;
  }

  inline bool IsHittingMaxLoss() {
    if (total_pnl_ < -max_loss_) {
      return true;
    } else {
      return false;
    }
  }

  inline bool IsHittingOpentradeLoss() {
    if (open_unrealized_pnl_ < -max_opentrade_loss_) {
      last_max_opentrade_loss_hit_msecs_ = watch_.msecs_from_midnight();
      return true;
    } else if (watch_.msecs_from_midnight() - last_max_opentrade_loss_hit_msecs_ < break_msecs_on_max_opentrade_loss_) {
      return true;
    } else {
      return false;
    }
  }

  inline void ReportResults(BulkFileWriter& trades_writer_) {
    SmartOrderManager& nk_order_manager_ = nk_exec_->order_manager();
    SmartOrderManager& nkm_order_manager_ = nkm_exec_->order_manager();
    int nk_traded_volume_ = nk_order_manager_.trade_volume();
    int nkm_traded_volume_ = nkm_order_manager_.trade_volume() / 10;

    //      std::cout << "NK: " << nk_traded_volume_ << " NKM: " << nkm_traded_volume_ << std::endl << std::flush;

    int t_total_pnl_ = nk_order_manager_.base_pnl().mult_total_pnl();

    int t_supporting_orders_filled_ = 0;
    int t_bestlevel_orders_filled_ = 0;
    int t_aggressive_orders_filled_ = 0;
    int t_improve_orders_filled_ = 0;

    if (nk_traded_volume_ + nkm_traded_volume_ > 0) {
      t_supporting_orders_filled_ = (nk_order_manager_.SupportingOrderFilledPercent() * nk_traded_volume_ +
                                     nkm_order_manager_.SupportingOrderFilledPercent() * nkm_traded_volume_) /
                                    (nk_traded_volume_ + nkm_traded_volume_);
      t_bestlevel_orders_filled_ = (nk_order_manager_.BestLevelOrderFilledPercent() * nk_traded_volume_ +
                                    nkm_order_manager_.BestLevelOrderFilledPercent() * nkm_traded_volume_) /
                                   (nk_traded_volume_ + nkm_traded_volume_);
      t_aggressive_orders_filled_ = (nk_order_manager_.AggressiveOrderFilledPercent() * nk_traded_volume_ +
                                     nkm_order_manager_.AggressiveOrderFilledPercent() * nkm_traded_volume_) /
                                    (nk_traded_volume_ + nkm_traded_volume_);
      t_improve_orders_filled_ = (nk_order_manager_.ImproveOrderFilledPercent() * nk_traded_volume_ +
                                  nkm_order_manager_.ImproveOrderFilledPercent() * nkm_traded_volume_) /
                                 (nk_traded_volume_ + nkm_traded_volume_);
    }

    //      std::cout << "NK " << nk_order_manager_.base_pnl ( ).total_pnl ( ) << " " << nk_order_manager_.trade_volume
    //      ( ) << std::endl;
    //      std::cout << "NKM " << nkm_order_manager_.base_pnl ( ).total_pnl ( ) << " " <<
    //      nkm_order_manager_.trade_volume ( ) << std::endl;

    printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (nk_traded_volume_ + nkm_traded_volume_),
           t_supporting_orders_filled_, t_bestlevel_orders_filled_, t_aggressive_orders_filled_,
           t_improve_orders_filled_);
  }

  inline void DumpPositions() {
    DBGLOG_TIME_CLASS_FUNC << "NK: " << nk_pos_ << "\n";
    DBGLOG_TIME_CLASS_FUNC << "NKM: " << nkm_pos_ << "\n";

    if (livetrading_) {
      DBGLOG_DUMP;
    }
  }
};
}
#endif  // BASE_EXECLOGIC_NIK_TRADING_MANAGER_H
