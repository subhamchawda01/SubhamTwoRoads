#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "tradeengine/Executioner/MarketOrderExecution.hpp"
#define ONESECMICRO 1000000

double frequency;
double agg_diff_;
void getFrequency() {
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  std::string hostName = std::string(hostname);
  //    cout<<"HostName:: "<<hostname<<endl;
  if (hostName == "sdv-ind-srv15") {
    frequency = 3312.058858592;
  } else if (hostName == "sdv-ind-srv16") {
    frequency = 3600.017;
  } else if (hostName == "sdv-ind-srv17" || hostName == "sdv-ind-srv18") {
    frequency = 2905;
  } else if (hostName == "sdv-ind-srv19") {
    frequency = 2808;
  } else if (hostName == "sdv-ind-srv20") {
    frequency = 2601;
  }
}

MarketOrderExecution::MarketOrderExecution(HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
                                           HFSAT::SecurityMarketView* _secondary_smv_,
                                           HFSAT::BasicOrderManager* _basic_om_, int _start_time_mfm_,
                                           int _size_to_exec_, HFSAT::TradeType_t _buysell_, double _price_,
                                           int _trigger_time_mfm_ = 0, int _midnight_mfm_ = 0,
                                           bool _polling_on_ = false)
    : shc_(_secondary_smv_->shortcode()),
      watch_(_watch_),
      dbglogger_(dbglogger_),
      secondary_smv_(_secondary_smv_),
      basic_om_(_basic_om_),
      start_time_mfm_(_start_time_mfm_),
      trigger_time_mfm_(_trigger_time_mfm_),
      midnight_mfm_(_midnight_mfm_),
      abs_pos_to_exec_(_size_to_exec_),
      buysell_(_buysell_),
      position_(0),
      order_(NULL),
      total_traded_value_(0),
      size_remanining_(_size_to_exec_),
      price_(_price_),
      int_price_(_price_ / _secondary_smv_->min_price_increment()),
      unique_exec_id_(-2),
      polling_on_(_polling_on_),
      mkt_exec_vec_(NULL) {
  // If pos is not a multiple of min order size then make it the nearest one
  if ((abs_pos_to_exec_ % secondary_smv_->min_order_size()) != 0) {
    DBGLOG_TIME_CLASS_FUNC << "Shortcode: " << shc_ << "POS_TO_EXECUTE: " << abs_pos_to_exec_
                           << " is not a multiple of min order size: " << secondary_smv_->min_order_size()
                           << DBGLOG_ENDL_FLUSH;
    abs_pos_to_exec_ = HFSAT::MathUtils::GetFlooredMultipleOf(abs_pos_to_exec_, secondary_smv_->min_order_size());
  }
}

void MarketOrderExecution::myusleep(double sleep_micros_) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double start_usec_ = tv.tv_usec;
  double start_sec_ = tv.tv_sec;

  while (1) {
    sleep(1 / (ONESECMICRO * frequency));
    gettimeofday(&tv, NULL);
    double diff_ = (double)(tv.tv_sec - start_sec_) * ONESECMICRO + (double)(tv.tv_usec - start_usec_);
    //     dbglogger_ << "diff:: "<<diff_<<endl;
    if (diff_ > sleep_micros_) {
      agg_diff_ += diff_;
      // dbglogger_ << "Sleep diff: " << diff_ << ", time: "<< tv.tv_sec << " " << tv.tv_usec << ", sleep_micros: " <<
      // sleep_micros_ << " finally\n";
      break;
    }
  }
}

void MarketOrderExecution::OnTimePeriodUpdate() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long time_in_millis = 1000 * (tv.tv_sec - midnight_mfm_) + tv.tv_usec / 1000;
  // dbglogger_ << " loop " << time_in_millis << " " << trigger_time_mfm_ << DBGLOG_ENDL_FLUSH;
  if (time_in_millis > start_time_mfm_) {
    //	dbglogger_ << "current_time " << start_time_mfm_ << " " << tv.tv_sec << " " << tv.tv_usec  << " " <<
    // trigger_time_mfm_ << DBGLOG_ENDL_FLUSH;
    //    	dbglogger_ <<  time_in_millis << " " << midnight_mfm_ << DBGLOG_ENDL_FLUSH;
    getFrequency();
    if (polling_on_ && trigger_time_mfm_ && time_in_millis <= trigger_time_mfm_ + 1000) {
      if (time_in_millis >= trigger_time_mfm_ - 100 && time_in_millis <= trigger_time_mfm_ + 99) {
        agg_diff_ = 0;
        myusleep(750);
        dbglogger_ << "Sleep diff: " << agg_diff_ << ", sleep_micros: 750 finally\n";
        PlaceOrder();
      } else if (time_in_millis >= trigger_time_mfm_ - 1000 && time_in_millis <= trigger_time_mfm_ + 996) {
        int count = 0;
        agg_diff_ = 0;
        while (count < 10) {
          myusleep(400);
          count++;
        }
        dbglogger_ << "Sleep diff: " << agg_diff_ << ", sleep_micros: 4000 finally\n";
        PlaceOrder();
      } else if (time_in_millis + 3000 < trigger_time_mfm_) {
        dbglogger_ << 1000 * (trigger_time_mfm_ - time_in_millis - 3000) << " " << tv.tv_sec
                   << " All set going into sleep now. BYE!\n";
        agg_diff_ = 0;
        myusleep(1000 * (trigger_time_mfm_ - time_in_millis - 3000));
        dbglogger_ << "Sleep diff: " << agg_diff_
                   << ", sleep_micros: " << 1000 * (trigger_time_mfm_ - time_in_millis - 3000) << " finally\n";
        PlaceOrder();
      } else {
        int count = 0;
        agg_diff_ = 0;
        while (count < 10) {
          myusleep(1000);
          count++;
        }
        dbglogger_ << "Sleep diff: " << agg_diff_ << ", sleep_micros: 10000 finally\n";
        PlaceOrder();
      }
    } else {
    }
  } else {
    return;
  }
}

void MarketOrderExecution::PlaceOrder() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long time_in_millis = 1000 * (tv.tv_sec - midnight_mfm_) + tv.tv_usec / 1000;
  if (order_ && order_->order_status_ == HFSAT::kORRType_Conf) {
    dbglogger_ << watch_.tv() << " " << shc_ << " "
               << "Order confirmed" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
               << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
  }
  if (time_in_millis >= trigger_time_mfm_) {
    // dbglogger_ << "time status activate\n";
    activateExecVec();
  }
  if (order_) {
    if (order_->order_status_ == HFSAT::kORRType_Conf || order_->order_status_ == HFSAT::kORRType_Exec) {
      //     dbglogger_ << " order status activate\n";
      activateExecVec();
    }
    if (order_->order_status_ == HFSAT::kORRType_Exec || order_->order_status_ == HFSAT::kORRType_Rejc ||
        order_->order_status_ == HFSAT::kORRType_Cxld) {
      if (order_->order_status_ == HFSAT::kORRType_Cxld || order_->order_status_ == HFSAT::kORRType_Rejc) {
        dbglogger_ << watch_.tv() << " " << shc_ << " "
                   << "Order rejected" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                   << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;

      } else {
        dbglogger_ << watch_.tv() << " " << shc_ << " "
                   << "Order finished" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                   << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
      }
      order_ = NULL;
    }
  }

  if ((!order_) && (size_remanining_ > 0)) {
    dbglogger_ << watch_.tv() << " " << shc_ << " "
               << "Order sending"
               << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;

    if (buysell_ == HFSAT::TradeType_t::kTradeTypeSell) {
      order_ =
          basic_om_->SendTrade(price_, int_price_, size_remanining_, buysell_, 'B', HFSAT::kOrderDay, unique_exec_id_);
    } else {
      order_ =
          basic_om_->SendTrade(price_, int_price_, size_remanining_, buysell_, 'B', HFSAT::kOrderDay, unique_exec_id_);
    }

    if (order_) {
      dbglogger_ << watch_.tv() << " " << shc_ << " "
                 << "SEND MARKET ORDER " << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                 << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
    }
  }
}

void MarketOrderExecution::OnExec(const int _new_position_, const int _exec_quantity_,
                                  const HFSAT::TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                  const int _security_id_, int _caos_) {
  size_remanining_ -= _exec_quantity_;
  if (size_remanining_ == 0) {
    dbglogger_ << watch_.tv() << " Completed execution of " << shc_ << DBGLOG_ENDL_FLUSH;
  }

  total_traded_value_ += _exec_quantity_ * _price_;
  position_ = _new_position_;
}
