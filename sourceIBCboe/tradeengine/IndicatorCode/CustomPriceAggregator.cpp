#include "tradeengine/Indicator/CustomPriceAggregator.hpp"
#include "tradeengine/Indicator/BookDelta.hpp"
#include "tradeengine/Indicator/MidSizeFilteredPrice.hpp"
#include "tradeengine/Indicator/SimpleTimeTrend.hpp"
#include "tradeengine/Indicator/SimpleTrend.hpp"
#include "tradeengine/Indicator/TradePrice.hpp"
#include "tradeengine/Indicator/TradeSpreadPrice.hpp"
#include "tradeengine/Indicator/VWAPPrice.hpp"
#include "tradeengine/Indicator/VWAPSizeFilteredPrice.hpp"
#include "tradeengine/Utils/Parser.hpp"

CustomPriceAggregator::CustomPriceAggregator(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_,
                                             HFSAT::DebugLogger& _dbglogger_,
                                             std::map<std::string, std::string>* key_val_map)
    : smv_(smv), watch_(_watch_), dbglogger_(_dbglogger_), key_val_map_(key_val_map) {
  CreatePriceVector();
}

CustomPriceAggregator::~CustomPriceAggregator() {}

void CustomPriceAggregator::CreatePriceVector() {
  uint32_t count = 0;
  std::string price_str = std::string("PRICE") + std::to_string(count);
  std::string price_type = price_str + std::string("_TYPE");
  std::map<std::string, std::string>::iterator iter = key_val_map_->find(price_type);
  double remaining_weight_ = 1;
  while (iter != key_val_map_->end()) {
    double weight = Parser::GetDouble(key_val_map_, price_str + std::string("_WEIGHT"), 0);
    remaining_weight_ -= weight;
    if (iter->second == "cTRADE_PRICE") {
      int64_t cutoff_time = Parser::GetInt(key_val_map_, price_str + std::string("_CUTOFF_TIME"), 0);
      int vwap_levels = Parser::GetInt(key_val_map_, price_str + std::string("_VWAP_LEVELS"), 1);
      BasePrice* base_price = new TradePrice(smv_, watch_, dbglogger_, weight, cutoff_time, vwap_levels);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cVWAP_PRICE") {
      int vwap_levels = Parser::GetInt(key_val_map_, price_str + std::string("_VWAP_LEVELS"), 1);
      BasePrice* base_price = new VWAPPrice(smv_, watch_, dbglogger_, weight, vwap_levels);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cTRADE_SPREAD_PRICE") {
      int vwap_levels = Parser::GetInt(key_val_map_, price_str + std::string("_VWAP_LEVELS"), 1);
      int window = Parser::GetInt(key_val_map_, price_str + std::string("_WINDOW"), 1);
      double skew_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_SKEW_FACTOR"), 1);
      BasePrice* base_price = new TradeSpreadPrice(smv_, watch_, dbglogger_, weight, window, vwap_levels, skew_factor);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cVWAP_SIZE_FILTERED_PRICE") {
      int size_filter = Parser::GetInt(key_val_map_, price_str + std::string("_SIZE_FILTER"), 0);
      BasePrice* base_price = new VWAPSizeFiltered(smv_, watch_, dbglogger_, weight, size_filter);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cBOOK_DELTA") {
      int num_levels = Parser::GetInt(key_val_map_, price_str + std::string("_NUM_LEVELS"), 1);
      double decay_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_DECAY_FACTOR"), 1);
      double skew_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_SKEW_FACTOR"), 1);
      double average_spread = Parser::GetDouble(key_val_map_, price_str + std::string("_AVERAGE_SPREAD"), 0);
      double level_decay_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_LEVEL_DECAY_FACTOR"), 1);
      BasePrice* base_price = new BookDelta(smv_, watch_, dbglogger_, weight, num_levels, decay_factor, skew_factor,
                                            average_spread, level_decay_factor);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cSIMPLE_TREND") {
      double skew_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_SKEW_FACTOR"), 1);
      int num_events = Parser::GetInt(key_val_map_, price_str + std::string("_NUM_EVENTS"), 1);
      BasePrice* base_price = new SimpleTrend(smv_, watch_, dbglogger_, weight, skew_factor, num_events);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cSIMPLE_TIME_TREND") {
      double skew_factor = Parser::GetDouble(key_val_map_, price_str + std::string("_SKEW_FACTOR"), 1);
      int64_t look_back_time = Parser::GetInt(key_val_map_, price_str + std::string("_LOOK_BACK_TIME"), 0);
      int64_t stable_px_look_back_time =
          Parser::GetInt(key_val_map_, price_str + std::string("_STABLE_PX_LOOK_BACK_TIME"), 0);
      int min_num_events = Parser::GetInt(key_val_map_, price_str + std::string("_MIN_NUM_EVENTS"), 1);
      BasePrice* base_price = new SimpleTimeTrend(smv_, watch_, dbglogger_, weight, skew_factor, look_back_time,
                                                  stable_px_look_back_time, min_num_events);
      price_vector_.push_back(base_price);
    } else if (iter->second == "cMID_PRICE") {
      int size_filter = Parser::GetInt(key_val_map_, price_str + std::string("_SIZE_FILTER"), 0);
      BasePrice* base_price = new MidSizeFiltered(smv_, watch_, dbglogger_, weight, size_filter);
      price_vector_.push_back(base_price);
    }
    count++;
    price_str = std::string("PRICE") + std::to_string(count);
    price_type = price_str + std::string("_TYPE");
    iter = key_val_map_->find(price_type);
  }
  if (remaining_weight_ != 0) {
    BasePrice* base_price = new VWAPPrice(smv_, watch_, dbglogger_, remaining_weight_, 1);
    price_vector_.push_back(base_price);
  }
}

void CustomPriceAggregator::GetPrice(double& bid_price, double& ask_price) {
  double accumulated_bid_price = 0;
  double accumulated_ask_price = 0;
  for (size_t i = 0; i < price_vector_.size(); i++) {
    double temp_bid_px = 0;
    double temp_ask_px = 0;
    price_vector_[i]->GetBasePrice(temp_bid_px, temp_ask_px);
    double weight = price_vector_[i]->GetWeight();
    accumulated_bid_price += (temp_bid_px * weight);
    accumulated_ask_price += (temp_ask_px * weight);
    // dbglogger_ << watch_.tv() << " CPA GetPrice iter:" << i << " curr_px " << temp_bid_px << " x " << temp_ask_px <<
    // " acc px " << accumulated_bid_price << " x " << accumulated_ask_price << DBGLOG_ENDL_FLUSH;
  }
  bid_price = accumulated_bid_price;
  ask_price = accumulated_ask_price;
}
