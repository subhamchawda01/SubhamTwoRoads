/**
   \file SimMarketMaker/security_delay_stats.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SimMarketMaker/security_delay_stats.hpp"
#include "dvccode/Utils/common_files_path.hpp"

// defining non bounding delays as we want to present only 'fact' with the API provided under this class.
#define MIN_DELAY_USECS 0
#define MAX_DELAY_USECS INT_MAX
#define ORS_MKT_DIFF 0
#define SEND_DELAY_DIFF 0
#define CXL_DELAY_DIFF 0

namespace HFSAT {
std::map<std::pair<unsigned, unsigned>, SecurityDelayStats*> SecurityDelayStats::secid_tradingdate_to_delay_stats_map_;
std::vector<std::vector<std::string>> SecurityDelayStats::shortcodes_in_same_gateway_;

/**
 *
 * @param watch
 * @param security_id
 * @param trading_location
 */
SecurityDelayStats::SecurityDelayStats(Watch& watch, SecurityMarketView* smv, TradingLocation_t trading_location,
                                       int _normal_delay_usecs) {
  dbglogger_ = new DebugLogger(10240, 1);
  ors_message_stats_ =
      new ORSMessageStats(*dbglogger_, watch, smv, trading_location, MIN_DELAY_USECS, _normal_delay_usecs,
                          MAX_DELAY_USECS, ORS_MKT_DIFF, SEND_DELAY_DIFF, CXL_DELAY_DIFF);
  GenerateSortedDelays();
  LoadGatewayShcMapping(watch.YYYYMMDD());
}

SecurityDelayStats::~SecurityDelayStats() {
  if (dbglogger_ != nullptr) {
    delete dbglogger_;
    dbglogger_ = nullptr;
  }

  if (ors_message_stats_ != nullptr) {
    delete ors_message_stats_;
    ors_message_stats_ = nullptr;
  }
}

/**
 *
 * @param security_id
 * @param watch
 * @param trading_location
 * @return
 */
SecurityDelayStats& SecurityDelayStats::GetUniqueInstance(SecurityMarketView* smv, Watch& watch,
                                                          TradingLocation_t trading_location, int _normal_delay_usecs) {
  std::pair<unsigned, unsigned> secid_tradingdate_key = std::make_pair(smv->security_id(), watch.YYYYMMDD());

  if (secid_tradingdate_to_delay_stats_map_.find(secid_tradingdate_key) ==
      secid_tradingdate_to_delay_stats_map_.end()) {
    secid_tradingdate_to_delay_stats_map_[secid_tradingdate_key] =
        new SecurityDelayStats(watch, smv, trading_location, _normal_delay_usecs);
  }

  return *(secid_tradingdate_to_delay_stats_map_[secid_tradingdate_key]);
}

void SecurityDelayStats::PrintAllDelays() {
  if (ors_message_stats_ != nullptr) {
    ors_message_stats_->PrintAllDelays();
  }
}

void SecurityDelayStats::ResetUniqueInstance(const unsigned security_id, Watch& watch,
                                             TradingLocation_t trading_location, int _normal_delay_usecs) {
  std::pair<unsigned, unsigned> secid_tradingdate_key = std::make_pair(security_id, watch.YYYYMMDD());
  if (secid_tradingdate_to_delay_stats_map_.find(secid_tradingdate_key) !=
      secid_tradingdate_to_delay_stats_map_.end()) {
    if (secid_tradingdate_to_delay_stats_map_[secid_tradingdate_key] != nullptr) {
      delete secid_tradingdate_to_delay_stats_map_[secid_tradingdate_key];
    }
    secid_tradingdate_to_delay_stats_map_.erase(secid_tradingdate_key);
  }

  return;
}

void SecurityDelayStats::ResetAllUniqueInstance() {
  std::map<std::pair<unsigned, unsigned>, SecurityDelayStats*>::iterator it =
      secid_tradingdate_to_delay_stats_map_.begin();

  for (; it != secid_tradingdate_to_delay_stats_map_.end(); it++) {
    if (it->second != nullptr) {
      delete it->second;
    }
    secid_tradingdate_to_delay_stats_map_.erase(it);
  }

  return;
}

/**
 * Provides order send confirmation delay at a given time.
 * Return false if there is no data or confirmation time < sequenced time.
 * Returns true if it successfully writes send_conf_delay
 *
 * @param current_time
 * @param send_conf_delay
 * @return
 */

bool SecurityDelayStats::GetSendConfDelay(ttime_t current_time, DelayOutput& send_conf_delay,
                                          double seq2conf_multiplier_, int seq2conf_addend_) {
  if (ors_message_stats_->GetSeqTimeToOrsOrderMap().empty()) {
    return false;
  }

  OrsOrderInfo* ors_info = ors_message_stats_->GetNearestConfOrderInfo(current_time);

  ttime_t seqd_time = ors_info->seq_time_;
  ttime_t conf_time = ors_info->conf_time_;

  if (conf_time <= seqd_time) {
    return false;
  }

  if (seqd_time - current_time < ttime_t(0, 100)) {
    const double multiplier_ = seq2conf_multiplier_;
    send_conf_delay.delay = (conf_time - current_time) * multiplier_;
    send_conf_delay.delay.addusecs(seq2conf_addend_);
    send_conf_delay.seqd_time = seqd_time;
  } else {
    const double multiplier_ = seq2conf_multiplier_;
    send_conf_delay.delay = (conf_time - seqd_time) * multiplier_;
    send_conf_delay.delay.addusecs(seq2conf_addend_);
    send_conf_delay.seqd_time = seqd_time;
  }
  return true;
}

/**
 * Provides order send market delay at a given time.
 * Return false if there is no data or market time < sequenced time.
 * Returns true if it successfully writes send_mkt_delay
 *
 * @param current_time
 * @param send_mkt_delay
 * @return
 */

bool SecurityDelayStats::GetSendMktDelay(ttime_t current_time, DelayOutput& send_mkt_delay, double seq2conf_multiplier_,
                                         int seq2conf_addend_) {
  if (ors_message_stats_->GetSeqTimeToOrsOrderMap().empty()) {
    return false;
  }

  OrsOrderInfo* ors_info = ors_message_stats_->GetNearestConfOrderInfo(current_time);

  ttime_t seqd_time = ors_info->seq_time_;
  ttime_t mkt_time = ors_info->mkt_time_;

  if (mkt_time <= seqd_time) {
    return false;
  }
  if (seqd_time - current_time < ttime_t(0, 100)) {
    const double multiplier_ = seq2conf_multiplier_;
    send_mkt_delay.delay = (mkt_time - current_time) * multiplier_;
    send_mkt_delay.delay.addusecs(seq2conf_addend_);
    send_mkt_delay.seqd_time = seqd_time;
  } else {
    const double multiplier_ = seq2conf_multiplier_;
    send_mkt_delay.delay = (mkt_time - seqd_time) * multiplier_;
    send_mkt_delay.delay.addusecs(seq2conf_addend_);
    send_mkt_delay.seqd_time = seqd_time;
  }
  return true;
}

/**
 * Provides order cancel confirmation delay at a given time.
 * Return false if there is no data or confirmation time < sequenced time.
 * Returns true if it successfully writes cxl_conf_delay
 *
 * @param current_time
 * @param cxl_conf_delay
 * @return
 */
bool SecurityDelayStats::GetCancelConfDelay(ttime_t current_time, DelayOutput& cxl_conf_delay,
                                            double seq2conf_multiplier_, int seq2conf_addend_) {
  if (ors_message_stats_->GetCxlSeqTimeToOrsOrderMap().empty()) {
    return false;
  }

  OrsOrderInfo* ors_info = ors_message_stats_->GetNearestCxlOrderInfo(current_time);

  ttime_t seqd_time = ors_info->cxlseq_time_;
  ttime_t cxld_time = ors_info->cxl_time_;

  if (cxld_time <= seqd_time) {
    return false;
  }
  if (seqd_time - current_time < ttime_t(0, 100)) {
    const double multiplier_ = seq2conf_multiplier_;
    cxl_conf_delay.delay = (cxld_time - current_time) * multiplier_;
    cxl_conf_delay.delay.addusecs(seq2conf_addend_);
    cxl_conf_delay.seqd_time = seqd_time;
  } else {
    const double multiplier_ = seq2conf_multiplier_;
    cxl_conf_delay.delay = (cxld_time - seqd_time) * multiplier_;
    cxl_conf_delay.delay.addusecs(seq2conf_addend_);
    cxl_conf_delay.seqd_time = seqd_time;
  }
  return true;
}

/**
 *
 * Provides order cancel market delay at a given time.
 * Return false if there is no data or market time < sequenced time.
 * Returns true if it successfully writes cxl_mkt_delay
 *
 * @param current_time
 * @param cxl_mkt_delay
 * @return
 */

bool SecurityDelayStats::GetCancelMktDelay(ttime_t current_time, DelayOutput& cxl_mkt_delay,
                                           double seq2conf_multiplier_, int seq2conf_addend_) {
  if (ors_message_stats_->GetCxlSeqTimeToOrsOrderMap().empty()) {
    return false;
  }

  OrsOrderInfo* ors_info = ors_message_stats_->GetNearestCxlOrderInfo(current_time);

  ttime_t seqd_time = ors_info->cxlseq_time_;
  ttime_t cxlmkt_time = ors_info->cxlmkt_time_;

  if (cxlmkt_time <= seqd_time) {
    return false;
  }
  if (seqd_time - current_time < ttime_t(0, 100)) {
    const double multiplier_ = seq2conf_multiplier_;
    cxl_mkt_delay.delay = (cxlmkt_time - current_time) * multiplier_;
    cxl_mkt_delay.delay.addusecs(seq2conf_addend_);
    cxl_mkt_delay.seqd_time = seqd_time;
  } else {
    const double multiplier_ = seq2conf_multiplier_;
    cxl_mkt_delay.delay = (cxlmkt_time - seqd_time) * multiplier_;
    cxl_mkt_delay.delay.addusecs(seq2conf_addend_);
    cxl_mkt_delay.seqd_time = seqd_time;
  }

  return true;
}

/**
 *
 * @param x
 * @param send_conf_delay
 * @return
 */
bool SecurityDelayStats::GetXthPercentileSendConfDelay(int x, ttime_t& send_conf_delay) {
  if (x < 0 || x > 100 || send_delays_sorted_.empty()) {
    return false;
  }

  int idx = (double)(send_delays_sorted_.size() - 1) * ((double)x / 100);
  send_conf_delay = send_delays_sorted_[idx];

  return true;
}

/**
 *
 * @param x
 * @param send_mkt_delay
 * @return
 */
bool SecurityDelayStats::GetXthPercentileSendMktDelay(int x, ttime_t& send_mkt_delay) {
  if (x < 0 || x > 100 || send_mkt_delays_sorted_.empty()) {
    return false;
  }

  int idx = (double)(send_mkt_delays_sorted_.size() - 1) * ((double)x / 100);
  send_mkt_delay = send_mkt_delays_sorted_[idx];

  return true;
}

/**
 *
 * @param x
 * @param cxl_conf_delay
 * @return
 */
bool SecurityDelayStats::GetXthPercentileCancelConfDelay(int x, ttime_t& cxl_conf_delay) {
  if (x < 0 || x > 100 || cxl_delays_sorted_.empty()) {
    return false;
  }

  int idx = (double)(cxl_delays_sorted_.size() - 1) * ((double)x / 100);
  cxl_conf_delay = cxl_delays_sorted_[idx];

  return true;
}

/**
 *
 * @param x
 * @param cxl_mkt_delay
 * @return
 */
bool SecurityDelayStats::GetXthPercentileCancelMktDelay(int x, ttime_t& cxl_mkt_delay) {
  if (x < 0 || x > 100 || cxl_mkt_delays_sorted_.empty()) {
    return false;
  }

  int idx = (double)(cxl_mkt_delays_sorted_.size() - 1) * ((double)x / 100);
  cxl_mkt_delay = cxl_mkt_delays_sorted_[idx];

  return true;
}

std::map<ttime_t, OrsOrderInfo>& SecurityDelayStats::GetSeqTimeToOrsOrderMap() {
  if (ors_message_stats_ != nullptr) {
    for (auto& pair : ors_message_stats_->GetSeqTimeToOrsOrderMap()) {
      if (pair.second != nullptr) {
        seq_time_to_ors_order_map_[pair.first] = *(pair.second);
      }
    }
  }

  return seq_time_to_ors_order_map_;
}

std::map<ttime_t, OrsOrderInfo>& SecurityDelayStats::GetCxlSeqTimeToOrsOrderMap() {
  if (ors_message_stats_ != nullptr) {
    for (auto& pair : ors_message_stats_->GetCxlSeqTimeToOrsOrderMap()) {
      if (pair.second != nullptr) {
        // Avoiding negative CxlSeqToCong values because of internal exec
        if ((*(pair.second)).cxlseq_time_ < (*(pair.second)).cxl_time_) {
          cxlseq_time_to_ors_order_map_[pair.first] = *(pair.second);
        }
      }
    }
  }

  return cxlseq_time_to_ors_order_map_;
}

std::vector<std::string> SecurityDelayStats::GetShortCodesInSameGateway(const std::string shortcode) {
  std::vector<std::string> shortcode_set;

  for (unsigned i = 0; i < shortcodes_in_same_gateway_.size(); i++) {
    if (shortcodes_in_same_gateway_[i][0].compare(shortcode) == 0) {
      shortcode_set = shortcodes_in_same_gateway_[i];
      shortcode_set.erase(std::remove(shortcode_set.begin(), shortcode_set.end(), shortcode), shortcode_set.end());
      return shortcode_set;
    }
  }
  return shortcode_set;
}

void SecurityDelayStats::GenerateSortedDelays() {
  for (auto& pair : ors_message_stats_->GetSeqTimeToOrsOrderMap()) {
    if (pair.second->conf_time_ > pair.second->seq_time_) {
      ttime_t send_diff = pair.second->conf_time_ - pair.second->seq_time_;
      send_delays_sorted_.push_back(send_diff);
    }

    if (pair.second->mkt_time_ > pair.second->seq_time_) {
      ttime_t send_mkt_diff = pair.second->mkt_time_ - pair.second->seq_time_;
      send_mkt_delays_sorted_.push_back(send_mkt_diff);
    }
  }

  for (auto& pair : ors_message_stats_->GetCxlSeqTimeToOrsOrderMap()) {
    if (pair.second->cxl_time_ > pair.second->cxlseq_time_) {
      ttime_t cxl_diff = pair.second->cxl_time_ - pair.second->cxlseq_time_;
      cxl_delays_sorted_.push_back(cxl_diff);
    }

    if (pair.second->cxlmkt_time_ > pair.second->cxlseq_time_) {
      ttime_t cxl_mkt_diff = pair.second->cxlmkt_time_ - pair.second->cxlseq_time_;
      cxl_mkt_delays_sorted_.push_back(cxl_mkt_diff);
    }
  }

  std::sort(send_delays_sorted_.begin(), send_delays_sorted_.end());
  std::sort(send_mkt_delays_sorted_.begin(), send_mkt_delays_sorted_.end());
  std::sort(cxl_delays_sorted_.begin(), cxl_delays_sorted_.end());
  std::sort(cxl_mkt_delays_sorted_.begin(), cxl_mkt_delays_sorted_.end());

  return;
}

void SecurityDelayStats::LoadGatewayShcMapping(int tradingdate_YYYYMMDD) {
  const std::string gateway_shc_map_filepath(FILEPATH::kGatewayShcMapFile);

  if (FileUtils::ExistsAndReadable(gateway_shc_map_filepath)) {
    std::ifstream infile(gateway_shc_map_filepath, std::ifstream::in);
    if (infile.is_open()) {
      char line_buffer_[1024];

      std::map<std::pair<std::string, std::string>, GatewayShcMapping*> gateway_shortcodes_map;

      while (infile.good()) {
        memset(line_buffer_, 0, 1024);
        infile.getline(line_buffer_, 1024);

        if (line_buffer_[0] == '#') {
          continue;
        }

        PerishableStringTokenizer string_tokenizer(line_buffer_, 1024, true);
        auto&& tokens = string_tokenizer.GetTokens();
        if (tokens.size() >= 4 && IsNumber(tokens[2])) {
          // Valid_from_date token
          int yyyymmdd = std::atoi(tokens[2]);

          // valid from date is less than or equal to trading date YYYYMMDD
          if (yyyymmdd <= tradingdate_YYYYMMDD) {
            std::string exch(tokens[0]);
            std::string gateway(tokens[1]);
            auto exch_gateway_key = std::make_pair(exch, gateway);

            // Set of shortcodes
            std::vector<std::string> shortcode_vector;
            PerishableStringTokenizer::StringSplit(std::string(tokens[3]), ',', shortcode_vector);
            std::vector<std::string> shortcode_set(shortcode_vector.begin(), shortcode_vector.end());

            if (gateway_shortcodes_map.find(exch_gateway_key) == gateway_shortcodes_map.end()) {
              gateway_shortcodes_map[exch_gateway_key] = new GatewayShcMapping(exch, gateway, shortcode_set, yyyymmdd);
            } else {
              if (gateway_shortcodes_map[exch_gateway_key]->validfrom_YYYYMMDD_ < yyyymmdd) {
                delete gateway_shortcodes_map[exch_gateway_key];
                gateway_shortcodes_map[exch_gateway_key] =
                    new GatewayShcMapping(exch, gateway, shortcode_set, yyyymmdd);
              }
            }
          }

        } else if (tokens.size() >= 4 && !IsNumber(tokens[2])) {
          std::cerr << "ValidFromDate entry: " << tokens[2] << ", has chars other than digits. Skipping\n";
        }
      }
      // Populate the shortcode vector
      for (auto& map_entry : gateway_shortcodes_map) {
        shortcodes_in_same_gateway_.push_back(map_entry.second->shortcode_set_);
        delete map_entry.second;
      }
      gateway_shortcodes_map.clear();
    }
  } else {
    std::cerr << "File " << gateway_shc_map_filepath << " not exists or not readable. Exiting\n";
    exit(1);
  }

  return;
}

bool SecurityDelayStats::IsNumber(const std::string& str) {
  for (std::string::const_iterator iter = str.begin(); iter != str.end(); iter++) {
    if (!std::isdigit(*iter)) {
      return false;
    }
  }
  return true;
}

void SecurityDelayStats::GetTimeStampsInRange(ttime_t start_time, ttime_t end_time, std::vector<ttime_t>& t_ts_vec_) {
  for (auto const& pair : ors_message_stats_->GetSeqTimeToOrsOrderMap()) {
    if (pair.first >= start_time && pair.first <= end_time) {
      t_ts_vec_.push_back(pair.first);
    }

    if (pair.first >= end_time) {
      break;
    }
  }
  for (auto const& pair : ors_message_stats_->GetCxlSeqTimeToOrsOrderMap()) {
    if (pair.first >= start_time && pair.first <= end_time) {
      t_ts_vec_.push_back(pair.first);
    }

    if (pair.first >= end_time) {
      break;
    }
  }
}
}
