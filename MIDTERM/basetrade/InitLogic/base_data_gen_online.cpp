/**
   \file InitLogic/base_data_gen_online.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "basetrade/InitLogic/base_data_gen.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
// #define CCPROFILING

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define SAMPLE_DATA_DAYS 250

HFSAT::DebugLogger *global_dbglogger_ = nullptr;            //(4 * 1024 * 1024, 256 * 1024);  // 4MB
HFSAT::BulkFileWriter bulk_file_writer_(32 * 1024 * 1024);  // 32MB .. increasing this makes the program very slow //
                                                            // should think of making this a function of the number of
                                                            // indicators

// For use in termination_handler
unsigned int global_progid_ = 0;
std::string global_output_filename_ = "";
unsigned int global_msecs_to_wait_to_print_again_ = 0;
unsigned int global_l1events_timeout_ = 0;
unsigned int global_num_trades_to_wait_print_again_ = 0;
unsigned int global_to_print_on_economic_times_ = 0;
unsigned int global_use_sample_shortcodes_ = 0;
std::string global_traded_ezone_ = "";
std::string global_modelfilename_ = "";
int global_tradingdate_ = 0;
int global_datagen_start_utc_hhmm_ = 0;
int global_datagen_start_utc_yymmdd_ = 0;
int global_datagen_end_utc_hhmm_ = 0;
int global_datagen_end_utc_yymmdd_ = 0;
unsigned int global_regime_mode_to_print_ = 0;
std::string global_command_line_args_ = "";
std::map<int, pair<std::string, std::string>> stats_map_;
void termination_handler(int signum) {
  if (global_dbglogger_ != nullptr) {
    global_dbglogger_->Close();
  }
  bulk_file_writer_.Close();

  if (signum == SIGSEGV || signum == SIGILL ||
      signum == SIGFPE) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string newline = "\n";
    std::string job_desc = "UNKNOWN";

    // Send slack notifications to #AWS-issues channel if running on EC2, otherwise send mails
    newline = "\\n";  // slack will need extra escaping
    // Fetch parent identifier (job in scheduler), as we may need to remove it from the scheduler
    char *job_id_ptr = getenv("DVC_JOB_ID");
    if (job_id_ptr != NULL) {
      job_desc = std::string(job_id_ptr);
    }

    std::string email_string_ = "", subject_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "Online datagen received " << SimpleSignalString(signum) << " on " << hostname_ << newline << newline;
      t_oss_ << "Parent job identifier: " << job_desc << newline;
      t_oss_ << " progid_=" << global_progid_ << " tradingdate_=" << global_tradingdate_
             << " datagen_start_utc_hhmm_=" << global_datagen_start_utc_hhmm_
             << " datagen_end_utc_hhmm_=" << global_datagen_end_utc_hhmm_ << " modelfilename_=" << global_modelfilename_
             << " output_filename_=" << global_output_filename_
             << " msecs_to_wait_to_print_again_=" << global_msecs_to_wait_to_print_again_
             << " num_trades_to_wait_print_again_=" << global_num_trades_to_wait_print_again_
             << " to_print_on_economic_times_=" << global_to_print_on_economic_times_
             << " use_sample_shortcodes_=" << global_use_sample_shortcodes_ << newline;

      subject_string_ = t_oss_.str();

      // to dump contents of model file to email body
      std::ifstream fin;
      fin.open(global_modelfilename_.data(), std::ifstream::in);
      char email_body[1024] = "Model file not found";
      t_oss_ << "<br />" << newline << "CommandLine: " << global_command_line_args_ << newline;
      while (fin) {
        fin.getline(email_body, 1024);
        t_oss_ << email_body << newline;
      }
      fin.close();

      email_string_ = t_oss_.str();
    }

    HFSAT::SlackManager slack_manager(AWSISSUES);
    slack_manager.sendNotification(email_string_);
    abort();
  }

  exit(0);
}

std::string CreateIlistFromConfig(std::string config_name) {
  std::string shc_;
  std::string fname_ = "";
  std::vector<std::string> features_;
  std::vector<std::string> source_features_;
  std::vector<std::string> expr_div_features_;
  std::map<std::string, std::vector<int>> features2durations_;
  int DEFAULT_DURATION = 300;
  std::vector<int> durations_;

  std::map<std::string, std::string> raw_indicators2keys_;
  std::map<std::string, std::string> replace_string_;
  std::map<std::string, std::string> inverse_replace_string_;

  replace_string_["RollingAvgL1Size"] = "L1SizeTrend";
  replace_string_["RollingAvgOrdSize"] = "L1OrderTrend";
  replace_string_["RollingSumTrades"] = "RecentSimpleTradesMeasure";
  replace_string_["RollingSumVolume"] = "RecentSimpleVolumeMeasure";
  replace_string_["RollingAvgTradeSize"] = "MovingAvgTradeSize";
  replace_string_["RollingStdev"] = "SlowStdevCalculator";
  replace_string_["RollingCorrelation"] = "SlowCorrCalculator";
  replace_string_["AvgPrice"] = "SimplePriceType";
  replace_string_["AvgPriceImpliedVol"] = "SimplePriceTypeImpliedVol";
  replace_string_["RollingTrendStdev"] = "SlowStdevTrendCalculator";
  replace_string_["StdevL1Bias"] = "StdevL1Bias";

  for (auto &it_ : replace_string_) {
    inverse_replace_string_[it_.second] = it_.first;
  }

  if (HFSAT::FileUtils::exists(config_name)) {
    std::ifstream config_fstream_;
    config_fstream_.open(config_name.c_str(), std::ifstream::in);
    if (config_fstream_.is_open()) {
      const int buf_len_ = 1024;
      char readline_buffer_[buf_len_];

      while (config_fstream_.good()) {
        bzero(readline_buffer_, buf_len_);
        config_fstream_.getline(readline_buffer_, buf_len_);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, buf_len_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) continue;
        if (tokens_.size() >= 4 && strcmp(tokens_[0], "SHC") == 0) {
          shc_ = std::string(tokens_[1]);
        } else if (tokens_.size() > 1 && strcmp(tokens_[0], "DURATIONS") == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            durations_.push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "FEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          if (std::find(features_.begin(), features_.end(), t_feature_) == features_.end()) {
            features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 3 && strcmp(tokens_[0], "SOURCEFEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          t_feature_ = t_feature_ + " " + std::string(tokens_[2]);
          if (std::find(source_features_.begin(), source_features_.end(), t_feature_) == source_features_.end()) {
            source_features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 3; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "RAW_INDICATOR") == 0) {
          std::stringstream raw_indicator_;
          std::stringstream key_;
          raw_indicator_ << tokens_[1];
          key_ << tokens_[1];
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            raw_indicator_ << " " << tokens_[i];
            key_ << "_" << tokens_[i];
          }
          raw_indicators2keys_[key_.str()] = raw_indicator_.str();
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "RATIOEXPR_FEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          t_feature_ = t_feature_ + " " + std::string(tokens_[2]);
          if (std::find(expr_div_features_.begin(), expr_div_features_.end(), t_feature_) == expr_div_features_.end()) {
            expr_div_features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 3; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        }
      }

      if (durations_.empty()) {
        durations_.push_back(DEFAULT_DURATION);
      }
      unsigned int indicator_idx_ = 0;

      std::vector<int> *t_durations_;
      for (const std::string &t_feature_ : features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::stringstream t_indicator_;
          if (t_feature_.compare("SimplePriceType") == 0) {
            t_indicator_ << "SimplePriceType " << shc_ << " MidPrice";
          } else if (t_feature_.compare("SimplePriceTypeImpliedVol") == 0) {
            t_indicator_ << "SimplePriceType " << shc_ << " ImpliedVol";
          } else if (t_feature_.compare("SlowStdevTrendCalculator") == 0) {
            t_indicator_ << "SlowStdevTrendCalculator " << shc_ << " 900 " << t_duration_ << " MidPrice";
          } else if (t_feature_.compare("StdevL1Bias") == 0) {
            t_indicator_ << "StdevL1Bias " << shc_ << " " << t_duration_ << " MktSizeWPrice";
          } else if (t_feature_.compare("SimpleTrendMax") == 0) {
            t_indicator_ << "SimpleTrend " << shc_ << " " << t_duration_ << " MktSizeWPrice";
          } else {
            t_indicator_ << t_feature_ << " " << shc_ << " " << t_duration_ << " MidPrice";
          }
          if (t_feature_.compare("SimpleTrendMax") == 0) {
            std::string key_ = std::string("SimpleTrendMax") + std::to_string(t_duration_);
            stats_map_[indicator_idx_] = std::make_pair(key_, t_indicator_.str());
          } else {
            std::string key_ = ((inverse_replace_string_.find(t_feature_) == inverse_replace_string_.end())
                                    ? t_feature_
                                    : inverse_replace_string_[t_feature_]) +
                               std::to_string(t_duration_);
            stats_map_[indicator_idx_] = make_pair(key_, t_indicator_.str());
          }
          indicator_idx_ += 1;
        }
      }
      for (const std::string &t_feature_ : source_features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::vector<std::string> t_tokens_;
          boost::split(t_tokens_, t_feature_, boost::is_any_of(" "));
          std::string &t_feature_word_ = t_tokens_[0];
          std::string &t_indep_ = t_tokens_[1];

          std::stringstream t_indicator_;
          t_indicator_ << t_feature_word_ << " " << shc_ << " " << t_indep_ << " " << t_duration_ << " MidPrice";
          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             std::to_string(t_duration_) + "_" + t_indep_;
          stats_map_[indicator_idx_++] = make_pair(key_, t_indicator_.str());
        }
      }
      for (const std::string &t_feature_ : expr_div_features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::vector<std::string> t_tokens_;
          boost::split(t_tokens_, t_feature_, boost::is_any_of(" "));
          std::string &t_feature_word_ = t_tokens_[0];
          std::string &t_indep_ = t_tokens_[1];

          std::stringstream t_indicator_;
          t_indicator_ << "Expression DIV 3 1.0 " << t_feature_word_ << " " << t_indep_ << " " << t_duration_
                       << " MidPrice 3 1.0 " << t_feature_word_ << " " << shc_ << " " << t_duration_ << " MidPrice";
          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             "_Ratio_" + std::to_string(t_duration_) + "_" + t_indep_;
          stats_map_[indicator_idx_++] = make_pair(key_, t_indicator_.str());
        }
      }
      for (auto &t_indc_ : raw_indicators2keys_) {
        stats_map_[indicator_idx_++] = make_pair(t_indc_.first, t_indc_.second);
      }

      std::ofstream fhandle_;

      fname_ = "/spare/local/tradeinfo/ilist_" + shc_ + "_" + std::to_string(global_tradingdate_) + ".ilist";
      HFSAT::FileUtils::MkdirEnclosing(fname_);
      fhandle_.open(fname_);
      fhandle_ << "MODELINIT DEPBASE " << shc_ << " MidPrice MidPrice\n";
      fhandle_ << "MODELMATH LINEAR CHANGE\n";
      fhandle_ << "INDICATORSTART\n";

      for (auto indx = 0u; indx < indicator_idx_; indx++) {
        std::string key;
        pair<std::string, std::string> t_set = stats_map_[indx];
        key = t_set.first;
        if (key.find("SimpleTrendMax") == 0)
          fhandle_ << "INDICATOR "
                   << "-1.00"
                   << " " << t_set.second << "\n";
        else
          fhandle_ << "INDICATOR "
                   << "1.00"
                   << " " << t_set.second << "\n";
      }
      fhandle_ << "INDICATOREND\n";
      fhandle_.close();
    }
  }
  return fname_;
}

void ParseCommandLineParams(int argc, char **argv, std::string &modelfilename_, int &tradingdate_,
                            int &datagen_start_utc_hhmm_, int &datagen_end_utc_hhmm_, unsigned int &progid_,
                            std::string &output_filename_, unsigned int &msecs_to_wait_to_print_again_,
                            unsigned long long &l1events_timeout_, unsigned int &num_trades_to_wait_print_again_,
                            unsigned int &to_print_on_economic_times_, bool &use_fake_faster_data_,
                            unsigned int &use_sample_shortcodes_, std::vector<std::string> &sampling_shortcodes_,
                            std::map<std::string, double> &c3_sampling_cutoffs_, unsigned int &regime_mode_to_print_,
                            std::string &traded_ezone_, std::vector<std::string> &dbg_code_vec_, bool &stats_only_,
                            HFSAT::IndicatorLogger::DatagenStats_t &samples_print_, int &type_, bool &use_l1_data_) {
  // expect :
  // 1. $datagenexec INDICATORLISTFILENAME TRADINGDATE UTC_STARTHHMM UTC_ENDHHMM PROGID
  // OUTPUTFILENAME/"STATS"/"STATS_SAMPLES" MSEC_PRINT
  // EVENT_PRINT NUM_TRADES_PRINT [ SAMPLE_USING_CORE_SHC 0/1 ] [ ADD_DBG_CODE DBG_CODE1 DBG_CODE2 ... ]
  if (argc < 11) {
    HFSAT::ExitVerbose(HFSAT::kDataGenCommandLineLessArgs,
                       "Usage: datagen INDICATORLISTFILENAME TRADINGDATE UTC_STARTHHMM UTC_ENDHHMM PROGID "
                       "OUTPUTFILENAME/\"STATS\"/\"STATS_SAMPLES\" MSECS_PRINT "
                       "l1EVENTS_PRINT/SAMPLE_USING_CORE_SHC(c1/c2/c3) NUM_TRADES_PRINT "
                       "ECO_MODE [ USE_FAKE_FASTER_DATA 0/1 = 1 ] [ ADD_SAMPLING_CODES ZN_0[0.5] 6M_0 [0.5] 6E_0 "
                       "[0.5]... -1 ] [ REGIME 1 -1 ] [ TRADED_EZONE ] [ ADD_DBG_CODE DBG_CODE1 DBG_CODE2 ... ]");
  } else {
    // for sending email.
    char tmp_str_command_args_[10240] = "";
    for (int i = 0; i < argc; i++) sprintf(tmp_str_command_args_, "%s %s", tmp_str_command_args_, argv[i]);

    modelfilename_ = argv[1];
    global_modelfilename_ = std::string(modelfilename_);

    if (!HFSAT::FileUtils::readable(modelfilename_)) {
      std::cerr << "modelfilename_ " << modelfilename_ << " not readable" << std::endl;
      exit(1);
    }

    tradingdate_ = atoi(argv[2]);
    if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
      std::cerr << "tradingdate_ " << tradingdate_ << " out of range [ " << MIN_YYYYMMDD << " " << MAX_YYYYMMDD << " ] "
                << std::endl;
      exit(1);
    }

    {
      char *start_hhmm_ = argv[3];

      char *tz_ = NULL;
      bool is_prev_ = false;
      int t_tradingdate_ = tradingdate_;
      if (strncmp(start_hhmm_, "PREV_", 5) == 0) {
        is_prev_ = true;
        start_hhmm_ = start_hhmm_ + 5;
      }
      if ((strncmp(start_hhmm_, "EST_", 4) == 0) || (strncmp(start_hhmm_, "CST_", 4) == 0) ||
          (strncmp(start_hhmm_, "CET_", 4) == 0) || (strncmp(start_hhmm_, "BRT_", 4) == 0) ||
          (strncmp(start_hhmm_, "UTC_", 4) == 0) || (strncmp(start_hhmm_, "KST_", 4) == 0) ||
          (strncmp(start_hhmm_, "HKT_", 4) == 0) || (strncmp(start_hhmm_, "MSK_", 4) == 0) ||
          (strncmp(start_hhmm_, "IST_", 4) == 0) || (strncmp(start_hhmm_, "JST_", 4) == 0) ||
          (strncmp(start_hhmm_, "BST_", 4) == 0) || (strncmp(start_hhmm_, "AST_", 4) == 0)) {
        tz_ = start_hhmm_;
        start_hhmm_ = start_hhmm_ + 4;
        int hhmmss = atoi(start_hhmm_);
        if (hhmmss < 10000) {
          hhmmss *= 100;
        }
        if ((strncmp(tz_, "CST_", 4) == 0) && (hhmmss >= 170000)) {
          is_prev_ = true;
        }
        if ((strncmp(tz_, "AST_", 4) == 0) && (hhmmss >= 0) && (hhmmss <= 82900)) {
          t_tradingdate_ = HFSAT::DateTime::CalcNextDay(t_tradingdate_);
        }
      }
      if (is_prev_) {
        t_tradingdate_ = HFSAT::DateTime::CalcPrevDay(t_tradingdate_);
      }

      if (tz_ != NULL) {
        datagen_start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(t_tradingdate_, atoi(start_hhmm_), tz_);
        global_datagen_start_utc_yymmdd_ =
            HFSAT::DateTime::GetUTCYYMMDDFromTZHHMMSS(t_tradingdate_, atoi(start_hhmm_), tz_);
      } else {
        datagen_start_utc_hhmm_ = atoi(start_hhmm_);
        global_datagen_start_utc_yymmdd_ = t_tradingdate_;
      }
    }
    {
      char *end_hhmm_ = argv[4];

      char *tz_ = NULL;
      bool is_prev_ = false;
      int t_tradingdate_ = tradingdate_;

      // but please do not put the data in this format.It is laughably ridiculous
      if (strncmp(end_hhmm_, "PREV_", 5) == 0) {
        is_prev_ = true;
        end_hhmm_ = end_hhmm_ + 5;
      }
      if ((strncmp(end_hhmm_, "EST_", 4) == 0) || (strncmp(end_hhmm_, "CST_", 4) == 0) ||
          (strncmp(end_hhmm_, "CET_", 4) == 0) || (strncmp(end_hhmm_, "BRT_", 4) == 0) ||
          (strncmp(end_hhmm_, "UTC_", 4) == 0) || (strncmp(end_hhmm_, "KST_", 4) == 0) ||
          (strncmp(end_hhmm_, "HKT_", 4) == 0) || (strncmp(end_hhmm_, "MSK_", 4) == 0) ||
          (strncmp(end_hhmm_, "IST_", 4) == 0) || (strncmp(end_hhmm_, "JST_", 4) == 0) ||
          (strncmp(end_hhmm_, "BST_", 4) == 0) || (strncmp(end_hhmm_, "AST_", 4) == 0)) {
        tz_ = end_hhmm_;
        end_hhmm_ = end_hhmm_ + 4;
        int hhmmss = atoi(end_hhmm_);
        if (hhmmss < 10000) {
          hhmmss *= 100;
        }
        if ((strncmp(tz_, "CST_", 4) == 0) && (hhmmss >= 170000)) {
          is_prev_ = true;
        }
        if ((strncmp(tz_, "AST_", 4) == 0) && (hhmmss >= 0) && (hhmmss <= 82900)) {
          t_tradingdate_ = HFSAT::DateTime::CalcNextDay(t_tradingdate_);
        }
      }
      if (is_prev_) {
        t_tradingdate_ = HFSAT::DateTime::CalcPrevDay(t_tradingdate_);
      }

      if (tz_ != NULL) {
        datagen_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(t_tradingdate_, atoi(end_hhmm_), tz_);
        global_datagen_end_utc_yymmdd_ =
            HFSAT::DateTime::GetUTCYYMMDDFromTZHHMMSS(t_tradingdate_, atoi(end_hhmm_), tz_);
      } else {
        datagen_end_utc_hhmm_ = atoi(end_hhmm_);
        global_datagen_end_utc_yymmdd_ = t_tradingdate_;
      }
    }

    progid_ = atoi(argv[5]);

    // Comparing argv[6] ( typically used for datagen output file ) against other options STATS, STAT_SAMPLES,
    // PNL_BASED_STATS
    // to output different stats based on the target application
    if (strcmp(argv[6], "STATS") == 0) {
      stats_only_ = true;
      samples_print_ = HFSAT::IndicatorLogger::kIndStats;
      output_filename_ = argv[6];
    } else if (strcmp(argv[6], "STATS_SAMPLES") == 0) {
      stats_only_ = true;
      samples_print_ = HFSAT::IndicatorLogger::kSampleStats;
      output_filename_ = argv[6];
    } else if (strcmp(argv[6], "PNL_BASED_STATS") == 0) {
      stats_only_ = true;
      samples_print_ = HFSAT::IndicatorLogger::kPnlBasedStats;
      output_filename_ = argv[6];
    } else if (strncmp(argv[6], "SUM_VARS@", 9) == 0) {
      type_ = 1;
      strtok(argv[6], "@");
      output_filename_ = strtok(NULL, "@");
      //      std::cout << output_filename_ << "\n";
    } else if (strncmp(argv[6], "WEIGHTED@", 9) == 0) {
      type_ = 2;
      strtok(argv[6], "@");
      output_filename_ = strtok(NULL, "@");
      //      std::cout << output_filename_ << "\n";
    } else if (strncmp(argv[6], "PBAT_BIAS@", 10) == 0) {
      type_ = 3;
      strtok(argv[6], "@");
      output_filename_ = strtok(NULL, "@");
    } else {
      stats_only_ = false;
      output_filename_ = argv[6];
    }
    msecs_to_wait_to_print_again_ = atoi(argv[7]);

    if (strcmp(argv[8], "c1") == 0) {
      use_sample_shortcodes_ = 1u;
    } else if (strcmp(argv[8], "c2") == 0) {
      use_sample_shortcodes_ = 2u;
    } else if (strcmp(argv[8], "c3") == 0) {
      use_sample_shortcodes_ = 3u;
    } else if (strcmp(argv[8], "e1") == 0) {
      use_sample_shortcodes_ = 4u;
    } else {
      use_sample_shortcodes_ = 0u;
      l1events_timeout_ = atoi(argv[8]);
    }

    if (strcmp(argv[9], "t1") == 0) {
      use_sample_shortcodes_ = 5u;
      if (strcmp(argv[8], "e1") == 0) {
        use_sample_shortcodes_ = 6u;
      }
    } else if (strcmp(argv[9], "ts1") == 0) {
      use_sample_shortcodes_ = 7u;
      if (strcmp(argv[8], "e1") == 0) {
        use_sample_shortcodes_ = 8u;
      }
    } else {
      // use_sample_shortcodes_ = 0u;
      num_trades_to_wait_print_again_ = atoi(argv[9]);
    }

    to_print_on_economic_times_ = atoi(argv[10]);

    int _current_processed_index_ = 10;

    use_fake_faster_data_ = true;
    if (argc >= _current_processed_index_ + 3 &&
        (strcmp(argv[_current_processed_index_ + 1], "USE_FAKE_FASTER_DATA") == 0)) {
      use_fake_faster_data_ = (atoi(argv[_current_processed_index_ + 2]) > 0);
      _current_processed_index_ += 2;
    }

#define SAMPLINGCODESARG_START_INDEX (_current_processed_index_ + 1)
    if (argc >= SAMPLINGCODESARG_START_INDEX + 1) {
      if (strcmp(argv[SAMPLINGCODESARG_START_INDEX], "ADD_SAMPLING_CODES") == 0) {
        _current_processed_index_ = SAMPLINGCODESARG_START_INDEX + 1;
        while (strcmp(argv[_current_processed_index_], "-1") != 0) {
          if (_current_processed_index_ >
              argc)  // we saw end of the arguments without encountering -1, enforcing strict format
          {
            std::cerr << "when given ADD_SAMPLING_CODES datagen expects -1 to intrepret end of sampling shortcodes "
                         "sequence , -1 is missing " << std::endl;
            exit(1);
          } else {
            sampling_shortcodes_.push_back(std::string(argv[_current_processed_index_]));
            _current_processed_index_++;
            if (use_sample_shortcodes_ == 3u) {
              if (atof(argv[_current_processed_index_]) <= 1 && atof(argv[_current_processed_index_]) > 0) {
                c3_sampling_cutoffs_[std::string(argv[_current_processed_index_ - 1])] =
                    atof(argv[_current_processed_index_]);
                _current_processed_index_++;
              } else {
                std::cerr
                    << "when given ADD_SAMPLING_CODES for c3,  datagen expects c3_sampling_cutoffs between 0 and 1 "
                    << std::endl;
                exit(1);
              }
            }
          }
        }
        _current_processed_index_++;
      }
    }
#undef SAMPLINGCODESARG_START_INDEX

    if (argc >= _current_processed_index_ + 4) {
      if (strcmp(argv[_current_processed_index_ + 1], "REGIME") == 0) {
        _current_processed_index_++;
        regime_mode_to_print_ =
            atoi(argv[_current_processed_index_ + 1]) > 0 ? atoi(argv[_current_processed_index_ + 1]) : 0;
        _current_processed_index_++;
        if (strcmp(argv[_current_processed_index_ + 1], "-1") != 0) {
          std::cerr << "expecting end ( -1 ) for REGIME argument" << std::endl;
          exit(1);
        } else {
          _current_processed_index_++;
        }
      }
    }

    while (argc >= _current_processed_index_ + 2 && strcmp(argv[_current_processed_index_ + 1], "ADD_DBG_CODE") != 0) {
      if (!strcmp(argv[_current_processed_index_ + 1], "USEL1DATA")) {
        use_l1_data_ = true;
      } else {
        traded_ezone_ = argv[_current_processed_index_ + 1];
      }
      _current_processed_index_++;
    }

#define DBGCODEARG_START_INDEX (_current_processed_index_ + 1)
    if (argc >= (DBGCODEARG_START_INDEX + 1)) {  // expect ADD_DBG_CODE DBG_CODE1 DBG_CODE2
      if (strcmp(argv[DBGCODEARG_START_INDEX], "ADD_DBG_CODE") == 0) {
        for (int i = (DBGCODEARG_START_INDEX + 1); i < argc; i++) {
          dbg_code_vec_.push_back(std::string(argv[i]));
        }
      }
    }
#undef DBGCODEARG_START_INDEX
  }

  global_progid_ = progid_;
  global_output_filename_ = output_filename_;
  global_msecs_to_wait_to_print_again_ = msecs_to_wait_to_print_again_;
  global_l1events_timeout_ = l1events_timeout_;
  global_num_trades_to_wait_print_again_ = num_trades_to_wait_print_again_;
  global_to_print_on_economic_times_ = to_print_on_economic_times_;
  global_traded_ezone_ = traded_ezone_;
  global_modelfilename_ = modelfilename_;
  global_tradingdate_ = tradingdate_;
  global_datagen_start_utc_hhmm_ = datagen_start_utc_hhmm_;
  global_datagen_end_utc_hhmm_ = datagen_end_utc_hhmm_;
  global_use_sample_shortcodes_ = use_sample_shortcodes_;
  global_regime_mode_to_print_ = regime_mode_to_print_;
}

/**
 * @brief Main Initialization
 *
 * parse command line paramters
 * build the list of source_shortcode_vec_ to initialize
 * initialize sec_name_indexer
 * initialize Adapters for each source
 * initialize the marketdata processing market_view_managers
 * initialize IndicatorLogger
 * make IndicatorLogger subscribe to all the market books of interest i.e for the securities of interest as a
 *OnReadyListener ..
 * An OnReadyListener is updated after all the variables have been updated
 * An OnReadyListener is not updated with market data .. but with the notification that now every market data listener
 *has the right picture of data.
 * Only on OnReadyListener does the IndicatorLogger think about printing stuff
 *
 * start event loop
 */
int main(int argc, char **argv) {
  std::string do_not_run_studies_file_ = "/home/dvctrader/.DO_NOT_RUN_STUDIES";
  if (HFSAT::FileUtils::exists(do_not_run_studies_file_)) {
    std::cerr << "File: " << do_not_run_studies_file_ << " exists." << std::endl;
    std::cerr << "Either you're not supposed to run this exec on this machine or remove this file." << std::endl;
    std::cerr << "Exiting now." << std::endl;
    exit(-1);
  }

  // Signal Handling, Interrupts and Segmentation Faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGILL, termination_handler);
  signal(SIGFPE, termination_handler);

  int tradingdate_ = 0;
  int datagen_start_utc_hhmm_ = 0;
  int datagen_end_utc_hhmm_ = 0;
  unsigned int progid_ = 0;
  std::string modelfilename_ = "";
  std::string output_filename_ = "";
  std::string traded_ezone_ = "";
  unsigned int msecs_to_wait_to_print_again_ = 1000;
  unsigned long long l1events_timeout_ = 15;
  unsigned int num_trades_to_wait_print_again_ = 0;
  // 0 means don't, 1 means print all the time, 2 means only print during this time
  unsigned int to_print_on_economic_times_ = 0u;
  bool use_fake_faster_data_ = true;
  // 0 means don't sample using core_shortcodes 1 means use core shortcode l1events together with dep l1 events
  unsigned int use_sample_shortcodes_ = 0u;
  // TODO ... { 2 means have seperate timeout for each core_shc and dep ( getl1eventsTimeOut ( std::string & shc_ ) )
  // 3/4 use core trade/price change events only ..make_finer.... }
  // user supplied sampling shortcodes_
  std::vector<std::string> sampling_shortcodes_;
  std::map<std::string, double> c3_sampling_cutoffs_;
  unsigned int regime_mode_to_print_ = 0;
  bool use_l1_data_ = false;

  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  ///< vector of all sources which we need (i)data for (ii)are trading
  std::vector<std::string> source_shortcode_vec_;
  ///< vector of all sources which we need ORS messages for, to build indicators
  std::vector<std::string> ors_needed_by_indicators_vec_;

  std::vector<std::string> dbg_code_vec_;  ///< set of codes that we want the logger to print on

  bool stats_only_ = false;
  // Print Indicator Stats by default
  HFSAT::IndicatorLogger::DatagenStats_t samples_print_ = HFSAT::IndicatorLogger::kIndStats;

  int type_ = 0;

#ifdef CCPROFILING
  // Cpu Cycle Profiling
  HFSAT::CpucycleProfiler &cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(4);
  cpucycle_profiler_.SetTag(0, " ProcessAllEvents to PLMVMV start ");
  cpucycle_profiler_.SetTag(1, " PLMVMV start to SMV onl1change ");
  cpucycle_profiler_.SetTag(2, " SMV to OnIndicatorUpdate ");
#endif  // CCPROFILING

  // Parse all the command line parameters
  ParseCommandLineParams(argc, argv, modelfilename_, tradingdate_, datagen_start_utc_hhmm_, datagen_end_utc_hhmm_,
                         progid_, output_filename_, msecs_to_wait_to_print_again_, l1events_timeout_,
                         num_trades_to_wait_print_again_, to_print_on_economic_times_, use_fake_faster_data_,
                         use_sample_shortcodes_, sampling_shortcodes_, c3_sampling_cutoffs_, regime_mode_to_print_,
                         traded_ezone_, dbg_code_vec_, stats_only_, samples_print_, type_, use_l1_data_);

  modelfilename_ = CreateIlistFromConfig(modelfilename_);
  // Setup the LogFile name
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/datalogs/log." << HFSAT::DateTime::GetCurrentIsoDateUTC() << "." << progid_;
  std::string logfilename = t_temp_oss_.str();

  // Make an object of CommonSMVSource and use it as an API
  std::vector<std::string> dummy_shc_list;
  CommonSMVSource *common_smv_source = new CommonSMVSource(dummy_shc_list, tradingdate_);
  // Get the dbglogger and watch after creating the source
  HFSAT::Watch &watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger &dbglogger_ = common_smv_source->getLogger();
  global_dbglogger_ = &dbglogger_;

  // Create all the unique instances of required managers
  std::ostringstream temp_oss_;
  temp_oss_ << datagen_start_utc_hhmm_;
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, temp_oss_.str());
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_, traded_ezone_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::PcaWeightsManager::SetUniqueInstance(tradingdate_);

  // This call parses the model file and gets a list of:
  // 1) source_shortcode_vec_ -> all the shortcodes for which the model requires data
  // 2) ors_needed_by_indicators_vec_ -> list of shortcodes which require ors data
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();

  std::string dep_shortcode_ = HFSAT::ModelCreator::CollectShortCodes(
      dbglogger_, watch_, modelfilename_, source_shortcode_vec_, ors_needed_by_indicators_vec_, true);

  int datagen_start_mfm_ = (3600 * (datagen_start_utc_hhmm_ / 100) + 60 * (datagen_start_utc_hhmm_ % 100)) * 1000;

  // Get the offline mix mms weights filename from the model
  std::string offline_mix_mms_wts_filename_ =
      HFSAT::ModelCreator::GetOfflineMixMMSWtsFileName(modelfilename_, datagen_start_mfm_, source_shortcode_vec_[0]);

  std::string online_mix_price_consts_filename_ =
      HFSAT::ModelCreator::GetOnlinePriceConstFilename(modelfilename_, datagen_start_mfm_, source_shortcode_vec_[0]);
  std::string online_beta_kalman_consts_filename_ = HFSAT::ModelCreator::GetOnlineBetaKalmanConstFilename(
      modelfilename_, datagen_start_mfm_, source_shortcode_vec_[0]);

  for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
    // go through all source_shortcodes and check for option
    if (strncmp(source_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      if (HFSAT::NSESecurityDefinitions::IsOption(source_shortcode_vec_[i])) {
        HFSAT::VectorUtils::UniqueVectorAdd(
            source_shortcode_vec_,
            HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(
                source_shortcode_vec_[i]));  // Adding future shortcode as source in case of options
      }
    }
  }

  // Set all the parameters in the common_smv_source
  common_smv_source->SetSourceShortcodes(source_shortcode_vec_);
  common_smv_source->SetSourcesNeedingOrs(ors_needed_by_indicators_vec_);
  common_smv_source->SetDepShortcode(dep_shortcode_);
  common_smv_source->SetDbgloggerFileName(logfilename);
  common_smv_source->SetOfflineMixMMSFilename(offline_mix_mms_wts_filename_);
  common_smv_source->SetOnlineMixPriceFilename(online_mix_price_consts_filename_);
  common_smv_source->SetOnlineBetaKalmanFileName(online_beta_kalman_consts_filename_);
  common_smv_source->SetFakeFasterData(use_fake_faster_data_);
  common_smv_source->SetStartEndTime(datagen_start_utc_hhmm_, datagen_end_utc_hhmm_);
  common_smv_source->SetStartEndUTCDate(global_datagen_start_utc_yymmdd_, global_datagen_end_utc_yymmdd_);

  common_smv_source->SetNSEL1Mode(use_l1_data_);
  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the security id to smv ptr from common source
  HFSAT::SecurityMarketViewPtrVec sid_to_smv_ptr_map_ = common_smv_source->getSMVMap();

  // Initialize the market update manager. This guy subscribes itself to all the security market views.
  // Indicators subscribe to this market update manager for data interrupted cases.
  HFSAT::MarketUpdateManager &market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, tradingdate_));

  //-----------------------------------------------------------------------------------------------------

  // -----Indicator Stats is derived from Indicator Logger, we don't need to have different pointers for them ----
  HFSAT::IndicatorLogger *indicator_logger_ = nullptr;

  HFSAT::CommonIndicator::set_global_start_mfm(HFSAT::GetMsecsFromMidnightFromHHMM(datagen_start_utc_hhmm_));
  HFSAT::CommonIndicator::set_global_end_mfm(HFSAT::GetMsecsFromMidnightFromHHMM(datagen_end_utc_hhmm_) +
                                             (global_datagen_end_utc_yymmdd_ - global_datagen_start_utc_yymmdd_) *
                                                 86400000);

  // ------------------------------------------SAMPLING------------------------------------------
  std::vector<HFSAT::SecurityMarketView *> sample_shc_smv_vec_;
  std::vector<double> c3_required_cutoffs_;
  if (strcmp(argv[8], "c1") == 0 || strcmp(argv[8], "c2") == 0 || strcmp(argv[8], "c3") == 0) {
    if (sampling_shortcodes_.empty())  // if not already supplied by user
    {
      HFSAT::GetSamplingShortcodes(dep_shortcode_, sampling_shortcodes_,
                                   HFSAT::CommonIndicator::global_trading_start_mfm_);
      for (auto i = 0u; i < sampling_shortcodes_.size(); i++) {
        c3_sampling_cutoffs_[sampling_shortcodes_[i]] = 0.5;
      }
    }

    l1events_timeout_ = 0.0;
    if (!HFSAT::VectorUtils::LinearSearchValue(sampling_shortcodes_, dep_shortcode_)) {
      sampling_shortcodes_.push_back(dep_shortcode_);
      c3_sampling_cutoffs_[dep_shortcode_] = 0.5;
    }

    // CONSIDER: changing the for loop to
    // for ( auto && t_source_shortcode_ : source_shortcode_vec_ )
    for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
      if (HFSAT::VectorUtils::LinearSearchValue(sampling_shortcodes_,
                                                source_shortcode_vec_[i]))  // both in model and core
      {
        sample_shc_smv_vec_.push_back(
            HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(source_shortcode_vec_[i]));
        if (strcmp(argv[8], "c3") != 0) {
          l1events_timeout_ +=
              HFSAT::SampleDataUtil::GetAvgForPeriod(source_shortcode_vec_[i], tradingdate_, SAMPLE_DATA_DAYS,
                                                     HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                     HFSAT::CommonIndicator::global_trading_end_mfm_, "L1EVPerSec");
        }
        // l1events_timeout_ += HFSAT::AvgEventCounter::GetAvgL1EventCount(source_shortcode_vec_[i]);
        c3_required_cutoffs_.push_back(c3_sampling_cutoffs_[source_shortcode_vec_[i]]);
      }
    }
  }

  if (use_sample_shortcodes_ == 4u || use_sample_shortcodes_ == 5u || use_sample_shortcodes_ == 6u ||
      use_sample_shortcodes_ == 7u || use_sample_shortcodes_ == 8u) {
    if (use_sample_shortcodes_ == 4u || use_sample_shortcodes_ == 6u || use_sample_shortcodes_ == 8u) {
      l1events_timeout_ = std::max(1.0, (msecs_to_wait_to_print_again_ / 1000) *
                                            HFSAT::SampleDataUtil::GetAvgForPeriod(
                                                dep_shortcode_, tradingdate_, SAMPLE_DATA_DAYS,
                                                HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                HFSAT::CommonIndicator::global_trading_end_mfm_, "L1EVPerSec"));
    }
    if (use_sample_shortcodes_ == 5u || use_sample_shortcodes_ == 6u) {
      num_trades_to_wait_print_again_ = std::max(
          1.0, (msecs_to_wait_to_print_again_ *
                HFSAT::SampleDataUtil::GetAvgForPeriod(dep_shortcode_, tradingdate_, SAMPLE_DATA_DAYS,
                                                       HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                       HFSAT::CommonIndicator::global_trading_end_mfm_, "TRADES")) /
                   (1000 * 300.0));
    }
    if (use_sample_shortcodes_ == 7u || use_sample_shortcodes_ == 8u) {
      num_trades_to_wait_print_again_ =
          std::max(1.0, (msecs_to_wait_to_print_again_ * HFSAT::SampleDataUtil::GetAvgForPeriod(
                                                             dep_shortcode_, tradingdate_, SAMPLE_DATA_DAYS,
                                                             HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                             HFSAT::CommonIndicator::global_trading_end_mfm_, "VOL")) /
                            (1000 * 300.0));
    }
    if (use_sample_shortcodes_ == 4u || use_sample_shortcodes_ == 5u || use_sample_shortcodes_ == 6u) {
      use_sample_shortcodes_ = 0u;
    }
    msecs_to_wait_to_print_again_ = 1000000;  // making it high so that sampling wont be done on the basis of time
  }

  if ((strcmp(argv[8], "c1") == 0 || strcmp(argv[8], "c2") == 0) &&
      (strcmp(argv[9], "t1") == 0 || strcmp(argv[9], "ts1") == 0)) {
    l1events_timeout_ = 0;
    num_trades_to_wait_print_again_ = 0;
    if (strcmp(argv[9], "t1") == 0) {
      for (auto i = 0u; i < sample_shc_smv_vec_.size(); i++) {
        num_trades_to_wait_print_again_ +=
            HFSAT::SampleDataUtil::GetAvgForPeriod(sample_shc_smv_vec_[i]->shortcode(), tradingdate_, SAMPLE_DATA_DAYS,
                                                   HFSAT::CommonIndicator::global_trading_start_mfm_,
                                                   HFSAT::CommonIndicator::global_trading_end_mfm_, "TRADES");
      }
      use_sample_shortcodes_ = 9;
      if (strcmp(argv[8], "c2") == 0) use_sample_shortcodes_ = 10;
    } else {
      for (auto i = 0u; i < sample_shc_smv_vec_.size(); i++) {
        num_trades_to_wait_print_again_ += HFSAT::SampleDataUtil::GetAvgForPeriod(
            sample_shc_smv_vec_[i]->shortcode(), tradingdate_, SAMPLE_DATA_DAYS,
            HFSAT::CommonIndicator::global_trading_start_mfm_, HFSAT::CommonIndicator::global_trading_end_mfm_, "VOL");
      }
      use_sample_shortcodes_ = 11;
      if (strcmp(argv[8], "c2") == 0) use_sample_shortcodes_ = 12;
    }
    num_trades_to_wait_print_again_ = std::max(1u, (atoi(argv[7]) * num_trades_to_wait_print_again_) / 300000);
    if (use_sample_shortcodes_ == 10u || use_sample_shortcodes_ == 12u) {
      msecs_to_wait_to_print_again_ = atoi(argv[7]);
    }
  }

  // In the below calls of CreateIndicatorStats/Logger :
  // 1) All Indicators are instantiated
  // 2) IndicatorsStats object is created and subscribed to listen all the indicators.
  // 3) Indicators subscribe to smv inside their own constructor/initialization.
  if (stats_only_) {
    indicator_logger_ = HFSAT::ModelCreator::CreateIndicatorStats(
        dbglogger_, watch_, bulk_file_writer_, economic_events_manager_, modelfilename_, output_filename_,
        msecs_to_wait_to_print_again_, l1events_timeout_, num_trades_to_wait_print_again_, to_print_on_economic_times_,
        use_sample_shortcodes_, sample_shc_smv_vec_, c3_required_cutoffs_, regime_mode_to_print_, samples_print_, true,
        stats_map_);
  } else if (type_ == 1) {
    indicator_logger_ = HFSAT::ModelCreator::CreateLogger(
        dbglogger_, watch_, bulk_file_writer_, economic_events_manager_, modelfilename_, output_filename_,
        msecs_to_wait_to_print_again_, l1events_timeout_, num_trades_to_wait_print_again_, to_print_on_economic_times_,
        use_sample_shortcodes_, sample_shc_smv_vec_, c3_required_cutoffs_, regime_mode_to_print_);
  } else if (type_ == 2) {
    indicator_logger_ = HFSAT::ModelCreator::CreateIndicatorLogger(
        dbglogger_, watch_, bulk_file_writer_, economic_events_manager_, modelfilename_, output_filename_,
        msecs_to_wait_to_print_again_, l1events_timeout_, num_trades_to_wait_print_again_, to_print_on_economic_times_,
        use_sample_shortcodes_, sample_shc_smv_vec_, c3_required_cutoffs_, regime_mode_to_print_, true);
  } else if (type_ == 3) {
    indicator_logger_ = HFSAT::ModelCreator::CreateIndicatorLogger(
        dbglogger_, watch_, bulk_file_writer_, economic_events_manager_, modelfilename_, output_filename_,
        msecs_to_wait_to_print_again_, l1events_timeout_, num_trades_to_wait_print_again_, to_print_on_economic_times_,
        use_sample_shortcodes_, sample_shc_smv_vec_, c3_required_cutoffs_, regime_mode_to_print_, true, true);
  } else {
    indicator_logger_ = HFSAT::ModelCreator::CreateIndicatorLogger(
        dbglogger_, watch_, bulk_file_writer_, economic_events_manager_, modelfilename_, output_filename_,
        msecs_to_wait_to_print_again_, l1events_timeout_, num_trades_to_wait_print_again_, to_print_on_economic_times_,
        use_sample_shortcodes_, sample_shc_smv_vec_, c3_required_cutoffs_, regime_mode_to_print_);
  }

  // subscribes all indicators to market data interrupts
  indicator_logger_->SubscribeMarketInterrupts(market_update_manager_);
  indicator_logger_->setStartTM(datagen_start_utc_hhmm_);  // this is checked in IndicatorLogger::PrintVals
  indicator_logger_->setEndTM(global_datagen_start_utc_yymmdd_, global_datagen_end_utc_yymmdd_, datagen_end_utc_hhmm_);

  // Subscribe to all smvs
  market_update_manager_.start();

  HFSAT::ModelCreator::LinkupIndicatorLoggerToOnReadySources(indicator_logger_, source_shortcode_vec_,
                                                             ors_needed_by_indicators_vec_);

  // ----------------------------------------------------------------------------------------------------------------------

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  // Initialize MarketViewManagers for different exchangescommon_smv_source->
  HFSAT::IndexedLiffePriceLevelMarketViewManager *indexed_liffe_price_level_market_view_manager_ =
      common_smv_source->indexed_liffe_price_level_market_view_manager();
  HFSAT::IndexedNtpMarketViewManager *indexed_ntp_market_view_manager_ =
      common_smv_source->indexed_ntp_market_view_manager();
  HFSAT::IndexedEobiPriceLevelMarketViewManager *indexed_eobi_price_level_market_view_manager_ =
      common_smv_source->indexed_eobi_price_level_market_view_manager();
  HFSAT::IndexedRtsMarketViewManager *indexed_rts_market_view_manager_ =
      common_smv_source->indexed_rts_market_view_manager();
  HFSAT::IndexedCmeMarketViewManager *indexed_cme_market_view_manager_ =
      common_smv_source->indexed_cme_market_view_manager();
  HFSAT::IndexedMicexMarketViewManager *indexed_micex_market_view_manager_ =
      common_smv_source->indexed_micex_market_view_manager();
  HFSAT::IndexedCfeMarketViewManager *indexed_cfe_market_view_manager_ =
      common_smv_source->indexed_cfe_market_view_manager();
  HFSAT::IndexedTmxMarketViewManager *indexed_tmx_market_view_manager_ =
      common_smv_source->indexed_tmx_market_view_manager();
  HFSAT::IndexedIceMarketViewManager *indexed_ice_market_view_manager_ =
      common_smv_source->indexed_ice_market_view_manager();
  HFSAT::IndexedAsxMarketViewManager *indexed_asx_market_view_manager_ =
      common_smv_source->indexed_asx_market_view_manager();
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager *indexed_hkomd_price_level_market_view_manager =
      common_smv_source->hkomd_price_level_market_view_manager();
  HFSAT::IndexedNSEMarketViewManager *indexed_nse_market_view_manager_ =
      common_smv_source->indexed_nse_market_view_manager();
  HFSAT::GenericL1DataMarketViewManager *generic_l1_data_market_view_manager_ =
      common_smv_source->generic_l1_data_market_view_manager();

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();

  bool use_nse_l1 = false;

  if (curr_location_ == HFSAT::kTLocSPR) {
    use_nse_l1 = true;
  }

  std::vector<HFSAT::ExchSource_t> exch_source_vec_;
  for (unsigned int i = 0; i < source_shortcode_vec_.size(); i++) {
    std::string _this_shortcode_ = source_shortcode_vec_[i];
    HFSAT::ExchSource_t t_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, tradingdate_);

    if (HFSAT::UseEOBIData(curr_location_, tradingdate_, _this_shortcode_)) {
      t_exch_source_ = HFSAT::kExchSourceEOBI;
    } else if (HFSAT::UseHKOMDData(curr_location_, tradingdate_, t_exch_source_)) {
      t_exch_source_ = HFSAT::kExchSourceHKOMDPF;
    }

    if (std::find(exch_source_vec_.begin(), exch_source_vec_.end(), t_exch_source_) == exch_source_vec_.end()) {
      exch_source_vec_.push_back(t_exch_source_);
    };
  }

  for (unsigned int i = 0; i < exch_source_vec_.size(); i++) {
    switch (exch_source_vec_[i]) {
      case HFSAT::kExchSourceCME: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::CME, (void *)((HFSAT::PriceLevelGlobalListener *)indexed_cme_market_view_manager_),
            &watch_);
      } break;
      case HFSAT::kExchSourceEOBI: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::EOBI_PF,
            (void *)((HFSAT::PriceLevelGlobalListener *)indexed_eobi_price_level_market_view_manager_), &watch_);
      } break;
      case HFSAT::kExchSourceNSE: {
        if (use_nse_l1) {
          combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
              HFSAT::MDS_MSG::NSE_L1, (void *)((HFSAT::L1DataListener *)(generic_l1_data_market_view_manager_)),
              &watch_);
        } else {
          combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
              HFSAT::MDS_MSG::NSE, (void *)((HFSAT::OrderGlobalListenerNSE *)(indexed_nse_market_view_manager_)),
              &watch_);
        }
      } break;

      case HFSAT::kExchSourceBMF:
      case HFSAT::kExchSourceNTP: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::NTP, (void *)((HFSAT::NTPPriceLevelGlobalListener *)(indexed_ntp_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceBMFEQ: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::BMF_EQ, (void *)((HFSAT::NTPPriceLevelGlobalListener *)(indexed_ntp_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceTMX: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::TMX, (void *)((HFSAT::FullBookGlobalListener *)(indexed_tmx_market_view_manager_)),
            &watch_);

        // TODO: Add a date check or remove the older one after the move to OBF
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::TMX_OBF, (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_ice_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceICE: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::ICE, (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_ice_market_view_manager_)),
            &watch_);
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::ICE_LS, (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_ice_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceASX: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::ASX, (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_asx_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceSGX: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::SGX, (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_ice_market_view_manager_)),
            &watch_);
      } break;
      case HFSAT::kExchSourceLIFFE: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::LIFFE_LS,
            (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_liffe_price_level_market_view_manager_)), &watch_);
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::LIFFE,
            (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_liffe_price_level_market_view_manager_)), &watch_);
      } break;

      case HFSAT::kExchSourceHONGKONG:
      case HFSAT::kExchSourceHKOMDCPF:
      case HFSAT::kExchSourceHKOMDPF: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::HKOMDPF,
            (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_hkomd_price_level_market_view_manager)), &watch_);
      } break;
      case HFSAT::kExchSourceJPY: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::OSE_ITCH_PF,
            (void *)((HFSAT::PriceLevelGlobalListener *)(indexed_ice_market_view_manager_)), &watch_);
      } break;
      case HFSAT::kExchSourceRTS: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::RTS, (void *)((HFSAT::PriceLevelGlobalListener *)indexed_rts_market_view_manager_),
            &watch_);
      } break;

      case HFSAT::kExchSourceMICEX_EQ: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::MICEX, (void *)((HFSAT::PriceLevelGlobalListener *)indexed_micex_market_view_manager_),
            &watch_);
      } break;
      case HFSAT::kExchSourceMICEX_CR: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::MICEX, (void *)((HFSAT::PriceLevelGlobalListener *)&indexed_micex_market_view_manager_),
            &watch_);
      } break;

      case HFSAT::kExchSourceCFE: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::CSM, (void *)((HFSAT::CFEPriceLevelGlobalListener *)(indexed_cfe_market_view_manager_)),
            &watch_);
      } break;

      case HFSAT::kExchSourceMEFF:
      case HFSAT::kExchSourceIDEM:
      case HFSAT::kExchSourceREUTERS:
      default: { } break; }
  };

  combined_mds_messages_shm_processor_.RunLiveShmSource();

  // ------------- CLEANUP-----------------
  // indicator_logger_
  if (indicator_logger_ != nullptr) {
    indicator_logger_->PrintErrors();
    delete indicator_logger_;
  }
  common_smv_source->cleanup();
// --------------------------------------

#ifdef CCPROFILING

  std::cout << "Printing the results of CPU Profiler " << std::endl;
  const std::vector<HFSAT::CpucycleProfilerSummaryStruct> prof_summary =
      HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummary();

  for (unsigned int ii = 0; ii < prof_summary.size(); ii++) {
    if (prof_summary[ii].total_occurrence_ > 0) {
      std::cout << prof_summary[ii].tag_name_ << " " << prof_summary[ii].fifty_percentile_ << " "
                << ((double)prof_summary[ii].ninetyfive_percentile_ / (double)prof_summary[ii].fifty_percentile_) << ' '
                << prof_summary[ii].total_occurrence_ << std::endl;
    }
  }

#endif  // CCPROFILING
}
