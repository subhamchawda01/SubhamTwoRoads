/**
   \file SimMarketMaker/sim_config.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_SIMMARKETMAKER_SIM_CONFIG_H
#define BASE_SIMMARKETMAKER_SIM_CONFIG_H

#include <map>
#include <string>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"

#define SIM_CONFIG_FILENAME "baseinfra/OfflineConfigs/SimConfig/sim_config.txt"

namespace HFSAT {
struct SimConfigStruct {
 public:
  SimConfigStruct()
      : shortcode_(),

        use_simple_sim_(false),
        use_accurate_seqd_to_conf_(true),
        use_accurate_cxl_seqd_to_conf_(true),

        using_only_full_mkt_for_sim_(false),

        max_conf_orders_above_best_level_(-1),
        use_no_cxl_from_front_(false),
        use_fgbm_sim_market_maker_(false),
        use_tgt_sim_market_maker_(false),
        use_baseprice_tgt_sim_market_maker_(false),

        tgt_sim_wt1_(0),
        tgt_sim_wt2_(0),
        use_ors_exec_(true),
        use_ors_exec_before_conf_(false),
        low_likelihood_thresh_(0.2),
        retail_order_placing_prob_(1.00),
        retail_max_order_size_to_execute_(10),
        use_order_level_sim_(false),
        order_level_sim_delay_(600),
        order_level_sim_delay_low_(0),
        order_level_sim_delay_high_(250000),
        seq2conf_multiplier_(1),
        // addend is in microseconds
        seq2conf_addend_(0) {}

  SimConfigStruct(std::string t_shortcode_)
      : shortcode_(t_shortcode_),

        use_simple_sim_(false),
        use_accurate_seqd_to_conf_(true),
        use_accurate_cxl_seqd_to_conf_(true),

        using_only_full_mkt_for_sim_(false),

        max_conf_orders_above_best_level_(-1),
        use_no_cxl_from_front_(false),
        use_fgbm_sim_market_maker_(false),
        use_tgt_sim_market_maker_(false),
        use_baseprice_tgt_sim_market_maker_(false),

        tgt_sim_wt1_(0),
        tgt_sim_wt2_(0),
        use_ors_exec_(true),
        use_ors_exec_before_conf_(false),
        low_likelihood_thresh_(0.2),
        retail_order_placing_prob_(1.00),
        retail_max_order_size_to_execute_(10),
        use_order_level_sim_(false),
        order_level_sim_delay_(600),
        order_level_sim_delay_low_(0),
        order_level_sim_delay_high_(250000),
        seq2conf_multiplier_(1),
        // addend is in microseconds
        seq2conf_addend_(0) {}

  SimConfigStruct(std::string t_shortcode_, bool t_use_accurate_seqd_to_conf_, bool t_use_accurate_cxl_seqd_to_conf_,
                  bool t_using_only_full_mkt_for_sim_, const int t_max_conf_orders_above_best_level_,
                  bool t_use_no_cxl_from_front_, bool t_use_fgbm_sim_, bool t_use_tgt_sim_,
                  bool t_use_baseprice_tgt_sim_market_maker_, float t_wt1_, float t_wt2_, bool t_use_ors_exec_,
                  bool t_use_ors_exec_before_conf_, double t_low_likelihood_thresh_,
                  double t_retail_order_placing_prob_, double t_retail_max_order_size_to_execute_,
                  bool t_use_order_level_sim_, int t_order_level_sim_delay_, int t_order_level_sim_delay_low_,
                  int t_order_level_sim_delay_high_, bool t_use_simple_sim_, double t_seq2conf_multiplier_,
                  int t_seq2conf_addend_)
      : shortcode_(t_shortcode_),
        use_simple_sim_(t_use_simple_sim_),
        use_accurate_seqd_to_conf_(t_use_accurate_seqd_to_conf_),
        use_accurate_cxl_seqd_to_conf_(t_use_accurate_cxl_seqd_to_conf_),

        using_only_full_mkt_for_sim_(t_using_only_full_mkt_for_sim_),

        max_conf_orders_above_best_level_(t_max_conf_orders_above_best_level_),

        use_no_cxl_from_front_(t_use_no_cxl_from_front_),
        use_fgbm_sim_market_maker_(t_use_fgbm_sim_),
        use_tgt_sim_market_maker_(t_use_tgt_sim_),
        use_baseprice_tgt_sim_market_maker_(t_use_baseprice_tgt_sim_market_maker_),

        tgt_sim_wt1_(t_wt1_),

        tgt_sim_wt2_(t_wt2_),
        use_ors_exec_(t_use_ors_exec_),
        use_ors_exec_before_conf_(t_use_ors_exec_before_conf_),
        low_likelihood_thresh_(t_low_likelihood_thresh_),
        retail_order_placing_prob_(t_retail_order_placing_prob_),
        retail_max_order_size_to_execute_(t_retail_max_order_size_to_execute_),
        use_order_level_sim_(t_use_order_level_sim_),
        order_level_sim_delay_(t_order_level_sim_delay_),
        order_level_sim_delay_low_(t_order_level_sim_delay_low_),
        order_level_sim_delay_high_(t_order_level_sim_delay_high_),
        seq2conf_multiplier_(t_seq2conf_multiplier_),
        seq2conf_addend_(t_seq2conf_addend_) {}

  std::string ToString() {
    std::ostringstream t_oss_;
    t_oss_ << "[ " << shortcode_ << " "
           << "| "
           << "USS=" << (use_simple_sim_ ? "true " : "false ")
           << "UASTC=" << (use_accurate_seqd_to_conf_ ? "true " : "false ")
           << "UACSTC=" << (use_accurate_cxl_seqd_to_conf_ ? "true " : "false ");

    t_oss_ << "| "
           << "UOFMFS=" << (using_only_full_mkt_for_sim_ ? "true " : "false ");

    t_oss_ << "| "
           << "MCOABL=" << max_conf_orders_above_best_level_;

    t_oss_ << "| "
           << "uncff=" << use_no_cxl_from_front_;

    t_oss_ << "]";

    return t_oss_.str();
  }

 public:
  std::string shortcode_;

  // Theoreticaly should always be true , but sim
  // for some products is tuned to be better with the
  // inaccurate computation.
  bool use_simple_sim_;  // param to signify using a simple sim where we give fills only on price level changes
  bool use_accurate_seqd_to_conf_;
  bool use_accurate_cxl_seqd_to_conf_;

  bool using_only_full_mkt_for_sim_;

  int max_conf_orders_above_best_level_;
  bool use_no_cxl_from_front_;
  bool use_fgbm_sim_market_maker_;
  bool use_tgt_sim_market_maker_;
  bool use_baseprice_tgt_sim_market_maker_;

  float tgt_sim_wt1_, tgt_sim_wt2_;
  bool use_ors_exec_;
  bool use_ors_exec_before_conf_;
  double low_likelihood_thresh_;
  double retail_order_placing_prob_;
  int retail_max_order_size_to_execute_;
  bool use_order_level_sim_;
  int order_level_sim_delay_;
  int order_level_sim_delay_low_;
  int order_level_sim_delay_high_;
  double seq2conf_multiplier_;
  int seq2conf_addend_;
};

class SimConfig {
 public:
  SimConfig(DebugLogger& _dbglogger_, Watch& _watch_)
      : dbglogger_(_dbglogger_), watch_(_watch_), shortcode_to_sim_config_() {}

  void WriteConfigToMap(std::string t_shortcode_, bool t_use_accurate_seqd_to_conf_,
                        bool t_use_accurate_cxl_seqd_to_conf_, bool t_using_only_full_mkt_for_sim_,
                        const int t_max_conf_orders_above_best_level_, bool t_use_no_cxl_front_, bool t_use_fgbm_sim_,
                        bool t_use_tgt_sim_, bool t_use_baseprice_tgt_sim_market_maker_, float t_wt1_, float t_wt2_,
                        bool t_use_ors_exec_, bool t_use_ors_exec_before_conf_, double t_low_likelihood_thresh_,
                        double t_retail_order_placing_prob_, int t_retail_max_order_size_to_execute_,
                        bool t_use_order_level_sim_, int t_order_level_sim_delay_, int t_order_level_sim_delay_low_,
                        int t_order_level_sim_delay_high_, bool t_use_simple_sim_, double t_seq2conf_multiplier_,
                        int t_seq2conf_addend_) {
    SimConfigStruct t_sim_config_(
        t_shortcode_, t_use_accurate_seqd_to_conf_, t_use_accurate_cxl_seqd_to_conf_, t_using_only_full_mkt_for_sim_,
        t_max_conf_orders_above_best_level_, t_use_no_cxl_front_, t_use_fgbm_sim_, t_use_tgt_sim_,
        t_use_baseprice_tgt_sim_market_maker_, t_wt1_, t_wt2_, t_use_ors_exec_, t_use_ors_exec_before_conf_,
        t_low_likelihood_thresh_, t_retail_order_placing_prob_, t_retail_max_order_size_to_execute_,
        t_use_order_level_sim_, t_order_level_sim_delay_, t_order_level_sim_delay_low_, t_order_level_sim_delay_high_,
        t_use_simple_sim_, t_seq2conf_multiplier_, t_seq2conf_addend_);
    if (shortcode_to_sim_config_.find(t_shortcode_) == shortcode_to_sim_config_.end()) {
      // DBGLOG_TIME_CLASS_FUNC_LINE << " Writing " << t_sim_config_.ToString ( ) << DBGLOG_ENDL_FLUSH;
      shortcode_to_sim_config_[t_shortcode_] = t_sim_config_;
    }
  }

  void LoadSimConfigFile(std::string sim_config_file_name_ = "") {
    static bool read_file_ = false;
    if (read_file_) {
      return;
    }

    std::string sim_config_filename_ = FileUtils::AppendHome(SIM_CONFIG_FILENAME);

    if (FileUtils::exists(sim_config_file_name_)) {
      sim_config_filename_ = sim_config_file_name_;
    }

    std::ifstream sim_config_file_;
    // For jenkins support, we need to mention file paths used from basetrade repo relative to WORKDIR.
    const char* work_dir = getenv("WORKDIR");
    if (work_dir != nullptr) {
      const char* deps_install = getenv("DEPS_INSTALL");
      if (deps_install != nullptr) {
        sim_config_filename_ = std::string(deps_install) + "/baseinfra/OfflineConfigs/SimConfig/sim_config.txt";
      }
    }

    sim_config_file_.open(sim_config_filename_.c_str(), std::ifstream::in);

    if (!sim_config_file_.is_open()) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Could not open file " << sim_config_filename_ << DBGLOG_ENDL_FLUSH;
      return;
    }

    char line_[1024];

    std::string shortcode_ = "?";
    bool use_simple_sim_ = false;
    bool use_accurate_seqd_to_conf_ = true;
    bool use_accurate_cxl_seqd_to_conf_ = true;
    bool using_only_full_mkt_for_sim_ = true;
    int max_conf_orders_above_best_level_ = -1;
    bool use_no_cxl_from_front_ = false;
    bool use_fgbm_sim_market_maker_ = false;
    bool use_tgt_sim_market_maker_ = false;
    bool use_baseprice_tgt_sim_market_maker_ = false;
    float tgt_sim_wt1_ = 0, tgt_sim_wt2_ = 0;
    bool use_ors_exec_ = true;
    bool use_ors_exec_before_conf_ = false;
    double low_likelihood_thresh_ = 0.2;
    bool use_order_level_sim_ = false;
    int order_level_sim_delay_ = 600;
    int order_level_sim_delay_low_ = 0;
    int order_level_sim_delay_high_ = 250000;
    double seq2conf_multiplier_ = 1;
    int seq2conf_addend_ = 0;  // addend is in microseconds
    double retail_order_placing_prob_ = 0.99;
    int retail_max_order_size_to_execute_ = 10;

    while (sim_config_file_.good()) {
      memset(line_, 0, sizeof(line_));
      sim_config_file_.getline(line_, sizeof(line_));

      PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
      const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();

      if (tokens_.size() >= 3) {
        if (tokens_[0][0] == '#' || strncmp(tokens_[0], "SIMCONFIG", strlen("SIMCONFIG"))) {
          continue;
        }

        // DBGLOG_TIME_CLASS_FUNC_LINE << line_ << DBGLOG_ENDL_FLUSH;

        if (!strncmp(tokens_[1], "SHORTCODE", strlen("SHORTCODE"))) {
          if (shortcode_.compare("?")) {
            WriteConfigToMap(shortcode_, use_accurate_seqd_to_conf_, use_accurate_cxl_seqd_to_conf_,
                             using_only_full_mkt_for_sim_, max_conf_orders_above_best_level_, use_no_cxl_from_front_,
                             use_fgbm_sim_market_maker_, use_tgt_sim_market_maker_, use_baseprice_tgt_sim_market_maker_,
                             tgt_sim_wt1_, tgt_sim_wt2_, use_ors_exec_, use_ors_exec_before_conf_,
                             low_likelihood_thresh_, retail_order_placing_prob_, retail_max_order_size_to_execute_,
                             use_order_level_sim_, order_level_sim_delay_, order_level_sim_delay_low_,
                             order_level_sim_delay_high_, use_simple_sim_, seq2conf_multiplier_, seq2conf_addend_);
          }

          shortcode_ = tokens_[2];

          use_simple_sim_ = false;
          use_accurate_seqd_to_conf_ = false;
          use_accurate_cxl_seqd_to_conf_ = false;

          using_only_full_mkt_for_sim_ = true;

          max_conf_orders_above_best_level_ = -1;
          use_no_cxl_from_front_ = false;
          use_fgbm_sim_market_maker_ = false;
          use_tgt_sim_market_maker_ = false;
          use_baseprice_tgt_sim_market_maker_ = false;
          tgt_sim_wt2_ = 0;
          tgt_sim_wt2_ = 0;
          use_ors_exec_ = true;
          use_ors_exec_before_conf_ = false;
          low_likelihood_thresh_ = 0.2;
          retail_order_placing_prob_ = 0.99;
          retail_max_order_size_to_execute_ = 10;
          use_order_level_sim_ = false;
          order_level_sim_delay_ = 600;
          order_level_sim_delay_low_ = 0;
          order_level_sim_delay_high_ = 250000;
          seq2conf_multiplier_ = 1;
          seq2conf_addend_ = 0;
        }

        if (!strncmp(tokens_[1], "USE_NO_CXL_FROM_FRONT", strlen("USE_NO_CXL_FROM_FRONT"))) {
          use_no_cxl_from_front_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "USE_SIMPLE_SIM", strlen("USE_SIMPLE_SIM"))) {
          use_simple_sim_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "USE_ACCURATE_SEQD_TO_CONF", strlen("USE_ACCURATE_SEQD_TO_CONF"))) {
          use_accurate_seqd_to_conf_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "USE_ACCURATE_CXL_SEQD_TO_CONF", strlen("USE_ACCURATE_CXL_SEQD_TO_CONF"))) {
          use_accurate_cxl_seqd_to_conf_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "USE_ONLY_FULL_MKT_FOR_SIM", strlen("USE_ONLY_FULL_MKT_FOR_SIM"))) {
          using_only_full_mkt_for_sim_ = (tokens_[2][0] != '0');
        }
        if (!strncmp(tokens_[1], "MAX_CONF_ORDERS_ABOVE_BEST_LEVEL", strlen("MAX_CONF_ORDERS_ABOVE_BEST_LEVEL"))) {
          max_conf_orders_above_best_level_ = atoi(tokens_[2]);
        }
        if (!strncmp(tokens_[1], "USE_FGBM_SIM", strlen("USE_FGBM_SIM"))) {
          use_fgbm_sim_market_maker_ = (tokens_[2][0] != '0');
        }
        if (!strncmp(tokens_[1], "USE_TARGET_PRICE_SIM", strlen("USE_TARGET_PRICE_SIM"))) {
          use_tgt_sim_market_maker_ = (tokens_[2][0] != '0');
        }
        if (!strncmp(tokens_[1], "USE_BASEPRICE_BASED_TARGET_PRICE_SIM",
                     strlen("USE_BASEPRICE_BASED_TARGET_PRICE_SIM"))) {
          use_baseprice_tgt_sim_market_maker_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "WEIGHT_1", strlen("WEIGHT_1"))) {
          tgt_sim_wt1_ = atof(tokens_[2]);
        }
        if (!strncmp(tokens_[1], "WEIGHT_2", strlen("WEIGHT_2"))) {
          tgt_sim_wt2_ = atof(tokens_[2]);
        }

        if (!strncmp(tokens_[1], "USE_ORS_EXEC", strlen("USE_ORS_EXEC"))) {
          use_ors_exec_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "USE_ORS_EXEC_BEFORE_CONF", strlen("USE_ORS_EXEC_BEFORE_CONF"))) {
          use_ors_exec_before_conf_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "LOW_LIKELIHOOD_THRESH", strlen("LOW_LIKELIHOOD_THRESH"))) {
          low_likelihood_thresh_ = atof(tokens_[2]);
        }

        if (!strncmp(tokens_[1], "RETAIL_ORDER_PLACING_PROB",
                     strlen("RETAIL_ORDER_PLACING_PROB"))) {  // retail_max_order_size_to_execute_
          retail_order_placing_prob_ = std::min(1.0, std::max(0.0, atof(tokens_[2])));
        }

        if (!strncmp(tokens_[1], "RETAIL_MAX_ORDER_SIZE_TO_EXECUTE", strlen("RETAIL_MAX_ORDER_SIZE_TO_EXECUTE"))) {
          retail_max_order_size_to_execute_ = std::max(0, atoi(tokens_[2]));
        }

        if (!strncmp(tokens_[1], "USE_ORDER_LEVEL_SIM", strlen("USE_ORDER_LEVEL_SIM"))) {
          use_order_level_sim_ = (tokens_[2][0] != '0');
        }

        if (!strncmp(tokens_[1], "ORDER_LEVEL_SIM_DELAY_NORMAL", strlen("ORDER_LEVEL_SIM_DELAY_NORMAL"))) {
          order_level_sim_delay_ = atoi(tokens_[2]);
        }

        if (!strncmp(tokens_[1], "ORDER_LEVEL_SIM_DELAY_LOW", strlen("ORDER_LEVEL_SIM_DELAY_LOW"))) {
          order_level_sim_delay_low_ = atoi(tokens_[2]);
        }

        if (!strncmp(tokens_[1], "ORDER_LEVEL_SIM_DELAY_HIGH", strlen("ORDER_LEVEL_SIM_DELAY_HIGH"))) {
          order_level_sim_delay_high_ = atoi(tokens_[2]);
        }
        if (!strncmp(tokens_[1], "USE_SEQ2CONF_MULTIPLIER", strlen("USE_SEQ2CONF_MULTIPLIER"))) {
          seq2conf_multiplier_ = atoi(tokens_[2]);
        }
        if (!strncmp(tokens_[1], "USE_SEQ2CONF_ADDEND", strlen("USE_SEQ2CONF_ADDEND"))) {
          seq2conf_addend_ = atoi(tokens_[2]);
        }
      }
    }

    if (shortcode_.compare("?")) {
      WriteConfigToMap(shortcode_, use_accurate_seqd_to_conf_, use_accurate_cxl_seqd_to_conf_,
                       using_only_full_mkt_for_sim_, max_conf_orders_above_best_level_, use_no_cxl_from_front_,
                       use_fgbm_sim_market_maker_, use_tgt_sim_market_maker_, use_baseprice_tgt_sim_market_maker_,
                       tgt_sim_wt1_, tgt_sim_wt2_, use_ors_exec_, use_ors_exec_before_conf_, low_likelihood_thresh_,
                       retail_order_placing_prob_, retail_max_order_size_to_execute_, use_order_level_sim_,
                       order_level_sim_delay_, order_level_sim_delay_low_, order_level_sim_delay_high_, use_simple_sim_,
                       seq2conf_multiplier_, seq2conf_addend_);
    }

    sim_config_file_.close();
    read_file_ = true;
  }

  SimConfigStruct _GetSimConfigsForShortcode(std::string dep_shortcode_, std::string sim_config_file_name_ = "") {
    if (dep_shortcode_.compare(0, 4, "NSE_") == 0) {
      if (shortcode_to_sim_config_.find("NSE") == shortcode_to_sim_config_.end()) {  // may be didnt load file
        LoadSimConfigFile(sim_config_file_name_);
      }
      if (shortcode_to_sim_config_.find("NSE") ==
          shortcode_to_sim_config_.end()) {  // still NSE is missing, add default
        shortcode_to_sim_config_["NSE"] = SimConfigStruct("NSE");
      }

      if (shortcode_to_sim_config_.find(dep_shortcode_) == shortcode_to_sim_config_.end()) {
        shortcode_to_sim_config_[dep_shortcode_] = shortcode_to_sim_config_["NSE"];
      }
    } else {
      if (shortcode_to_sim_config_.find(dep_shortcode_) ==
          shortcode_to_sim_config_.end()) {  // may be we didint load file
        LoadSimConfigFile(sim_config_file_name_);
      }
      if (shortcode_to_sim_config_.find(dep_shortcode_) ==
          shortcode_to_sim_config_.end()) {  // still missing, add default
        shortcode_to_sim_config_[dep_shortcode_] = SimConfigStruct(dep_shortcode_);
      }
    }

    return shortcode_to_sim_config_[dep_shortcode_];
  }

 private:
  DebugLogger& dbglogger_;
  Watch& watch_;

  std::map<std::string, SimConfigStruct> shortcode_to_sim_config_;

 public:
  static SimConfig& GetUniqueInstance(DebugLogger& _dbglogger_, Watch& _watch_) {
    static SimConfig* p_sim_config_ = NULL;

    if (!p_sim_config_) {
      p_sim_config_ = new SimConfig(_dbglogger_, _watch_);
    }

    return (*p_sim_config_);
  }

  static SimConfigStruct GetSimConfigsForShortcode(DebugLogger& _dbglogger_, Watch& _watch_, std::string dep_shortcode_,
                                                   std::string sim_config_file_name_ = "") {
    return GetUniqueInstance(_dbglogger_, _watch_)._GetSimConfigsForShortcode(dep_shortcode_, sim_config_file_name_);
  }
};

struct SimTimeSeriesInfo {
 public:
  SimTimeSeriesInfo(const unsigned int r_num_security_id_)
      : sid_to_high_rejection_periods_(r_num_security_id_),
        sid_to_high_cxl_rejection_periods_(r_num_security_id_),
        sid_to_time_to_seqd_to_conf_times_(r_num_security_id_),
        sid_to_time_to_conf_to_market_delay_times_(r_num_security_id_),
        sid_to_time_to_cxl_to_market_delay_times_(r_num_security_id_),
        sid_to_trade_time_to_price_stats_(r_num_security_id_),
        sid_to_time_to_cxl_seqd_to_conf_times_(r_num_security_id_),
        sid_to_time_to_conf_to_update_times_(r_num_security_id_),

        sid_to_large_price_move_periods_(r_num_security_id_),
        sid_to_newly_formed_levels_(r_num_security_id_),

        sid_to_large_trade_periods_(r_num_security_id_),
        sid_to_price_reversion_periods_(r_num_security_id_),

        sid_to_quote_before_trade_trade_info_(r_num_security_id_),
        sid_to_quote_before_trade_market_update_info_(r_num_security_id_),

        sid_to_trade_adjusted_market_update_info_(r_num_security_id_),

        sid_to_sim_config_() {
    for (auto i = 0u; i < r_num_security_id_; i++) {
      sid_to_time_to_seqd_to_conf_times_[i].clear();  // vector of maps ... so clear
    }
  }

 public:
  std::vector<std::vector<HFSAT::ttime_t> > sid_to_high_rejection_periods_;
  std::vector<std::vector<HFSAT::ttime_t> > sid_to_high_cxl_rejection_periods_;

  std::vector<std::map<ttime_t, ttime_t> > sid_to_time_to_seqd_to_conf_times_;
  std::vector<std::map<ttime_t, ttime_t> > sid_to_time_to_conf_to_market_delay_times_;
  std::vector<std::map<ttime_t, ttime_t> > sid_to_time_to_cxl_to_market_delay_times_;
  std::vector<std::map<ttime_t, int> > sid_to_trade_time_to_price_stats_;
  std::vector<std::map<ttime_t, ttime_t> > sid_to_time_to_cxl_seqd_to_conf_times_;
  std::vector<std::map<ttime_t, ttime_t> > sid_to_time_to_conf_to_update_times_;

  std::vector<std::vector<ttime_t> > sid_to_large_price_move_periods_;
  std::vector<std::map<ttime_t, std::vector<int> > > sid_to_newly_formed_levels_;

  std::vector<std::vector<ttime_t> > sid_to_large_trade_periods_;
  std::vector<std::vector<HFSAT::ttime_t> > sid_to_price_reversion_periods_;

  std::vector<std::map<ttime_t, TradePrintInfo> > sid_to_quote_before_trade_trade_info_;
  std::vector<std::map<ttime_t, MarketUpdateInfo> > sid_to_quote_before_trade_market_update_info_;

  std::vector<std::map<ttime_t, MarketUpdateInfo> > sid_to_trade_adjusted_market_update_info_;

  std::vector<SimConfigStruct> sid_to_sim_config_;

  bool DisableAccurate(unsigned int t_security_id_) {
    if (sid_to_sim_config_.size() > t_security_id_) {
      sid_to_sim_config_[t_security_id_].use_accurate_seqd_to_conf_ = false;
      sid_to_sim_config_[t_security_id_].use_accurate_cxl_seqd_to_conf_ = false;
      return true;
    } else {
      return false;
    }
  }

  void DisableAccurate() {
    for (auto i = 0u; i < sid_to_sim_config_.size(); i++) {
      sid_to_sim_config_[i].use_accurate_seqd_to_conf_ = false;
      sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ = false;
    }
  }
};
}
#endif  // BASE_ORDERROUTING_MARKET_MODEL_MANAGER_H
