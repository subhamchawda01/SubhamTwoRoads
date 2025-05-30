#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/Utils/Parser.hpp"

BaseExecutioner::BaseExecutioner(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                                 HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _basic_om,
                                 bool _livetrading_, bool _is_modify_before_confirmation_,
                                 bool _is_cancellable_before_confirmation_, TheoValues& theo_values)
    : theo_values_(theo_values),
      exec_param_file_(_exec_param_file),
      watch_(_watch_),
      dbglogger_(_dbglogger_),
      secondary_smv_(_secondary_smv),
      basic_om_(_basic_om),
      status_mask_(0xFF3F),  // Config status and control message status not set
      bid_status_(false),
      ask_status_(false),
      order_limit_(0),
      order_count_(0),
      bad_ratio_limit_(0),
      min_throttles_per_min_(0),
      max_throttles_per_min_(100),
      tick_size_(secondary_smv_->min_price_increment_),
      bid_size_(0),
      ask_size_(0),
      size_multiplier_(1),
      eff_squareoff_(false),
      passive_reduce_(false),
      aggressive_reduce_(false),
      enable_bid_on_reload_(false),
      enable_ask_on_reload_(false),
      use_reserve_msg_(false),
      unique_exec_id_(-1),
      livetrading_(_livetrading_),
      is_modify_before_confirmation_(_is_modify_before_confirmation_),
      is_cancellable_before_confirmation_(_is_cancellable_before_confirmation_),
      DEP_ONE_LOT_SIZE_(0),
      INDEP_ONE_LOT_SIZE_(0),
      using_grt_control_file_(false),
      grt_control_key_map_(NULL){

  grt_control_key_map_ = new std::map<std::string, std::string>();

  inverse_tick_size_ = 1 / tick_size_;

  sec_shortcode_ = secondary_smv_->shortcode();
  exec_key_val_map_ = new std::map<std::string, std::string>();
  Parser::ParseConfig(exec_param_file_, *exec_key_val_map_);
  order_log_buffer_.content_type_ = HFSAT::CDef::QueryOrder;

  //get one lot size value
  if (HFSAT::SecurityDefinitions::CheckIfContractSpecExists(secondary_smv_->shortcode(), watch_.YYYYMMDD())){
    DEP_ONE_LOT_SIZE_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(secondary_smv_->shortcode(), watch_.YYYYMMDD());
  }

  DBGLOG_CLASS_FUNC_LINE_INFO << "ONE_LOT_SIZE : " << secondary_smv_->shortcode() << " DATE : " << watch_.YYYYMMDD() << " " << DEP_ONE_LOT_SIZE_ << DBGLOG_ENDL_FLUSH ;
}

void BaseExecutioner::ReloadConfig() {
  Parser::ParseConfig(exec_param_file_, *exec_key_val_map_);
  int prev_order_limit_ = order_limit_;
  LoadParams();
  if ((prev_order_limit_ != order_limit_) && (order_count_ < order_limit_)) {
    if ((status_mask_ & SHOOT_STATUS_SET) == 0) {
      DBGLOG_TIME_CLASS_FUNC << "ORDER COUNT HIT LIMIT: " << order_limit_
                             << " not crossed ORDER COUNT: " << order_count_ << " ENABLING EXEC" << DBGLOG_ENDL_FLUSH;
    }
    TurnOn(SHOOT_STATUS_SET);
  }
}

void BaseExecutioner::EnableBid() { bid_status_ = true; }

void BaseExecutioner::EnableAsk() { ask_status_ = true; }

void BaseExecutioner::DisableBid() {
  bid_status_ = false;
  CancelBid();
}

void BaseExecutioner::DisableAsk() {
  ask_status_ = false;
  CancelAsk();
}

void BaseExecutioner::CancelBid() { basic_om_->CancelAllBidOrders(); }

void BaseExecutioner::CancelAsk() { basic_om_->CancelAllAskOrders(); }

void BaseExecutioner::TurnOff(uint16_t mask_to_unset_) {
  status_mask_ = status_mask_ & mask_to_unset_;
  CancelBid();
  CancelAsk();
}

void BaseExecutioner::TurnOn(uint16_t mask_to_set_) { status_mask_ = status_mask_ | mask_to_set_; }
