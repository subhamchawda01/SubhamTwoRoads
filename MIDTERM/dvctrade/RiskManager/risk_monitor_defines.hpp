#pragma once
#include <string.h>

namespace HFSAT {
struct GetFlatStruct {
  bool getflat_;  // true=>getflat, false=>start_trading
  char tag_[32];
  std::string ToString() { return (getflat_ ? "FlatOnTag " : "StartTrading ") + std::string(tag_); }
};
enum RiskUpdateType { GET_FLAT, START_TRADING, UPDATE_CONFIG_FILE, UPDATE_VALUES };
// Each risk update is one object of this struct
struct RiskUpdateStruct {
  RiskUpdateType update_type_;
  char shortcode_[30];
  char tag_[128];
  double realized_pnl_;
  double unrealized_pnl_;
  int position_;
  int volume_;
  std::string ToString() {
    std::stringstream ss;
    ss << tag_ << "," << shortcode_ << "," << (realized_pnl_ + unrealized_pnl_) << "," << realized_pnl_ << ","
       << unrealized_pnl_ << "," << position_ << "," << volume_ << "\n";
    return ss.str();
  }
};
// One object for each rule
struct RuleDetails {
  double pnl_limit_;
  double drawdown_limit_;
  std::string tag_;

  RuleDetails(double _pnl_limit, double _drawdown_limit) : pnl_limit_(_pnl_limit), drawdown_limit_(_drawdown_limit) {}
  std::string ToString() {
    std::stringstream ss;
    ss << "RuleDetails: " << tag_ << ", " << pnl_limit_ << ", " << drawdown_limit_ << ":";
    return ss.str();
  }
};
// struct used to notify risk monitor slave about the saci-tag pairs, via TCP
struct QueryTag {
  int saci_;
  int query_id_;
  char tags_[128];
  int utc_start_time_;
  int utc_end_time_;
  QueryTag(int saci, int query_id, std::string tags, int utc_start_time, int utc_end_time) {
    saci_ = saci;
    query_id_ = query_id;
    strncpy(tags_, tags.c_str(), std::min(tags.length() + 1, sizeof(tags_)));
    utc_start_time_ = utc_start_time;
    utc_end_time_ = utc_end_time;
  }

  QueryTag() {
    saci_ = -1;
    query_id_ = -1;
    strncpy(tags_, "DUMMY", 5);
    utc_start_time_ = 0001;
    utc_end_time_ = 2359;
  }
  std::string ToString() {
    std::stringstream ss;
    ss << "QueryTag: " << query_id_ << " " << saci_ << " " << tags_ << " " << utc_start_time_ << " " << utc_end_time_
       << "\n";
    return ss.str();
  }
};

// struct used to notify risk monitor slave about the saci-tag pairs, via TCP
struct ExecutionMsg {
  int saci_;
  char shortcode_[30];
  TradeType_t side_;
  int order_size_executed_;
  double price_;
  int global_pos_;
  int saos_;
  char timestamp_[20];
  int sams_;

  ExecutionMsg(int saci, std::string shortcode, TradeType_t side, int order_size_executed, double price, int global_pos,
               int saos, std::string timestamp, int sams) {
    saci_ = saci;
    strncpy(shortcode_, shortcode.c_str(), std::min(shortcode.length() + 1, sizeof(shortcode_)));
    side_ = side;
    order_size_executed_ = order_size_executed;
    price_ = price;
    global_pos_ = global_pos;
    saos_ = saos;
    strncpy(timestamp_, timestamp.c_str(), std::min(timestamp.length() + 1, sizeof(timestamp_)));
    sams_ = sams;
  }

  std::string ToString() {
    std::stringstream ss;
    ss << "ExecutionMsg: " << saci_ << ", " << shortcode_ << ", " << (int)side_ << ", " << order_size_executed_ << ", "
       << price_ << ", " << global_pos_ << ", " << saos_ << ", " << timestamp_ << ", " << sams_ << "\n";
    return ss.str();
  }
};
}
