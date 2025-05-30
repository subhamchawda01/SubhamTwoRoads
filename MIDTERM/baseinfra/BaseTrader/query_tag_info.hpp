#ifndef QUERY_TAG_INFO_HPP
#define QUERY_TAG_INFO_HPP
#include <string.h>

class QueryTagInfo {
  int trading_date_;  // Initialize these static variables
  int query_id_;
  std::string query_tags_;
  QueryTagInfo() : trading_date_(0), query_id_(0), query_tags_(std::string("")) {}
  QueryTagInfo(QueryTagInfo& disabled_copy_constructor) = delete;

 public:
  static QueryTagInfo& GetUniqueInstance() {
    static QueryTagInfo unique_instance;
    return unique_instance;
  }
  void setTradingDate(int trade_date) { trading_date_ = trade_date; };
  void setQueryId(int query_id) { query_id_ = query_id; };
  void setQueryTags(std::string query_tags) { query_tags_ = query_tags; };

  int getTradingDate() { return trading_date_; };
  int getQueryId() { return query_id_; };
  std::string getQueryTags() { return query_tags_; };
};
#endif  // QUERY_TAG_INFO_HPP
