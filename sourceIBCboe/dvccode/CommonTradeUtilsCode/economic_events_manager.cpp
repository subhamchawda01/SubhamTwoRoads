#include <sstream>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

namespace HFSAT {

EconomicEventsManager::EconomicEventsManager(DebugLogger& t_dbglogger_, const Watch& r_watch_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      tradingdate_(r_watch_.YYYYMMDD()),
      events_of_the_day_(),
      traded_events_of_the_day_(),
      allowed_events_of_the_day_(),
      ez_to_current_severity_map_(),
      ez_to_current_tradability_map_(),
      last_economic_times_checked_(0),
      traded_ezone_(""),
      compute_tradability_(false) {
  watch_.subscribe_BigTimePeriod(this);
  ReloadDB();
}

EconomicEventsManager::EconomicEventsManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             std::string _traded_ezone_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      tradingdate_(r_watch_.YYYYMMDD()),
      events_of_the_day_(),
      traded_events_of_the_day_(),
      allowed_events_of_the_day_(),
      ez_to_current_severity_map_(),
      ez_to_current_tradability_map_(),
      last_economic_times_checked_(0),
      traded_ezone_(_traded_ezone_),
      compute_tradability_(false) {
  watch_.subscribe_BigTimePeriod(this);
  ReloadDB();
}

void EconomicEventsManager::ReloadDB() {
  events_of_the_day_.clear();

  std::string economic_events_filename_ = std::string(NEWBASESYSINFODIR) + "BloombergEcoReports/merged_eco_";
  {
    std::stringstream ss;
    ss << watch_.YYYY();
    economic_events_filename_ += ss.str();
  }
  economic_events_filename_ += "_processed.txt";

  std::ifstream economic_events_file_;

  int current_tradingdate_ = watch_.YYYYMMDD();

  economic_events_file_.open(economic_events_filename_.c_str(), std::ifstream::in);
  if (economic_events_file_.is_open()) {
    const int kEconomicEventsFileLineBufferLen = 1024;
    char readline_buffer_[kEconomicEventsFileLineBufferLen];
    bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);

    while (economic_events_file_.good()) {
      bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);
      economic_events_file_.getline(readline_buffer_, kEconomicEventsFileLineBufferLen);
      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kEconomicEventsFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      // unixtime EZone _text_ severity YYYYMMDD_HH:MM:SS_UTC
      // 1293830100 USD Bloomberg_FCI_Monthly 2 20101231_21:15:00_UTC
      if ((tokens_.size() >= 5) && (strlen(tokens_[4]) >= 21)) {
        char text_event_date_[9];
        memcpy(text_event_date_, tokens_[4], 9);
        text_event_date_[8] = '\0';
        if (atoi(text_event_date_) == current_tradingdate_) {
          EventLine event_line_;
          event_line_.event_time_ = atoi(tokens_[0]);
          event_line_.event_mfm_ = 1000 * (event_line_.event_time_ - watch_.last_midnight_sec());
          event_line_.ez_ = GetEZFromStr(tokens_[1]);
          strncpy(event_line_.event_text_, tokens_[2], EVENT_TEXT_MAX_LEN);
          event_line_.severity_ = atoi(tokens_[3]);
          // default
          int start_time_lag_mseconds_ = 5 * 60 * 1000;  // 5 minutes
          int end_time_lag_mseconds_ = 5 * 60 * 1000;    // 5 minutes

          // for GBP products make Boe IRD as 3 degree, so stop +-10mins
          if ((event_line_.ez_ == EZ_GBP) &&
              (strncmp(event_line_.event_text_, "BoE_Interest_Rate_Decision",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("BoE_Interest_Rate_Decision"))) == 0)) {
            event_line_.severity_ = 3;
          }

          // first check for severity 3 events
          if (event_line_.severity_ == 3) {
            start_time_lag_mseconds_ = 10 * 60 * 1000;  // 10 minutes
            end_time_lag_mseconds_ = 10 * 60 * 1000;    // 10 minutes
          }

          // for GBP poducts , stop longer for BoE major events
          if ((event_line_.ez_ == EZ_GBP) &&
              ((strncmp(event_line_.event_text_, "BOE's_Governor_Carney_speech",
                        std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("BOE's_Governor_Carney_speech"))) == 0) ||
               (strncmp(event_line_.event_text_, "Bank_of_England_Quarterly_Inflation_Report",
                        std::min(EVENT_TEXT_MAX_LEN,
                                 (unsigned int)strlen("Bank_of_England_Quarterly_Inflation_Report"))) == 0))) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 10 * 60 * 1000;  // 10 minutes
            end_time_lag_mseconds_ = 45 * 60 * 1000;    // 45 hour 5 minutes
          }

          // then ecb press conference
          if ((strncmp(event_line_.event_text_, "ECB_Trichet's_Speech",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("ECB_Trichet's_Speech"))) == 0) ||
              (strncmp(event_line_.event_text_, "ECB_President_Draghi's_Speech",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("ECB_President_Draghi's_Speech"))) == 0) ||
              (strncmp(event_line_.event_text_, "ECB_Press_Conference",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("ECB_Press_Conference"))) == 0) ||
              (strncmp(event_line_.event_text_, "ECB_Monetary_policy_statement_and_press_conference",
                       std::min(EVENT_TEXT_MAX_LEN,
                                (unsigned int)strlen("ECB_Monetary_policy_statement_and_press_conference"))) == 0)) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 15 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 65 * 60 * 1000;    // 1 hour 5 minutes
          }

          // fed decision 1 hour
          if ((strncmp(event_line_.event_text_, "Fed_Interest_Rate_Decision",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("Fed_Interest_Rate_Decision"))) == 0) ||
              (strncmp(event_line_.event_text_, "FOMC_Rate_Decision",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("FOMC_Rate_Decision"))) == 0)) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 15 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 60 * 60 * 1000;    // 1 hour
          }

          // then fed press conference
          if ((strncmp(event_line_.event_text_, "Fed's_Press_Conference",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("Fed's_Press_Conference"))) == 0) ||
              (strncmp(event_line_.event_text_, "Fed's_Forecasts",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("Fed's_Forecasts"))) == 0) ||
              (strncmp(event_line_.event_text_, "FOMC_Economic_Projections",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("FOMC_Economic_Projections"))) == 0)) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 15 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 60 * 60 * 1000;    // 1 hour
          }

          // BOJ Interest Rate / BOJ press conference
          if ((strncmp(event_line_.event_text_, "BoJ_Interest_Rate_Decision",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("BoJ_Interest_Rate_Decision"))) == 0) ||
              (strncmp(event_line_.event_text_, "BoJ_Monetary_Policy_Statement_and_press_conference",
                       std::min(EVENT_TEXT_MAX_LEN,
                                (unsigned int)strlen("BoJ_Monetary_Policy_Statement_and_press_conference"))) == 0)) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 15 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 60 * 60 * 1000;    // 1 hour
          }

          // humphrey hawkins
          if ((strncmp(event_line_.event_text_, "Fed's_Bernanke_testifies",
                       std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("Fed's_Bernanke_testifies"))) == 0)) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 10 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 120 * 60 * 1000;   // 2 hour
          }

          // one time ECB_LIQUIDITY
          if (strncmp(event_line_.event_text_, "ECB_LIQUIDITY",
                      std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("ECB_LIQUIDITY"))) == 0) {
            event_line_.severity_ = 3;
            start_time_lag_mseconds_ = 15 * 60 * 1000;  // 15 minutes
            end_time_lag_mseconds_ = 30 * 60 * 1000;    // 30 minutes
          }

          event_line_.start_mfm_ = event_line_.event_mfm_ - start_time_lag_mseconds_;
          event_line_.end_mfm_ = event_line_.event_mfm_ + end_time_lag_mseconds_;
          events_of_the_day_.push_back(event_line_);

          if (strncmp(event_line_.event_text_, "ECB_", std::min(EVENT_TEXT_MAX_LEN, (unsigned int)strlen("ECB_"))) ==
              0) {
            // this mainly includes
            // long pause for following 4 events as they are speeches/conf
            // ECB_Press_Conference
            // ECB_Monetary_policy_statement_and_press_conference
            // ECB_President_Draghi's_Speech
            // ECB_Trichet's_Speech
            // normal pause for
            // ECB_Monthly_Report
            // ECB_Interest_Rate_Decision

            event_line_.ez_ = EZ_LFI;
            events_of_the_day_.push_back(event_line_);
          }
        }
      }
    }
  }
  original_events_of_the_day_ = events_of_the_day_;
  EventLine event_line_;
  event_line_.severity_ = 3;

  // WE are ading these breaks as economic times to best handle the automated way of getting flat
  const char* start_str_break_1 = "MSK_1400";  // BREAK 1 - INTRADAY_CLEARING MSK_1400 MSK_1405
  const char* start_str_break_2 = "MSK_1900";  // BREAK 2 - INTRADAY_CLEARING

  // Get the MSK Time Into UTC form
  int32_t start_time_ =
      HFSAT::DateTime::GetUTCHHMMFromTZHHMM(watch_.YYYYMMDD(), atoi(start_str_break_1 + 4), start_str_break_1);
  start_time_ = (start_time_ / 100) * 3600 + (start_time_ % 100) * 60;

  event_line_.event_time_ = (watch_.last_midnight_sec() + start_time_);
  event_line_.event_mfm_ = 1000 * (event_line_.event_time_ - watch_.last_midnight_sec());
  event_line_.start_mfm_ = event_line_.event_mfm_ - 10 * 60 * 1000;
  event_line_.end_mfm_ = event_line_.event_mfm_ + 10 * 60 * 1000;
  event_line_.ez_ = EZ_RTS;
  strncpy(event_line_.event_text_, "INTRADAY_CLEARING", EVENT_TEXT_MAX_LEN);
  events_of_the_day_.push_back(event_line_);

  EventLine event_line_1_;
  event_line_1_.severity_ = 3;

  start_time_ =
      HFSAT::DateTime::GetUTCHHMMFromTZHHMM(watch_.YYYYMMDD(), atoi(start_str_break_2 + 4), start_str_break_2);
  start_time_ = (start_time_ / 100) * 3600 + (start_time_ % 100) * 60;

  event_line_1_.event_time_ = (watch_.last_midnight_sec() + start_time_);
  event_line_1_.event_mfm_ = 1000 * (event_line_1_.event_time_ - watch_.last_midnight_sec());
  event_line_1_.start_mfm_ = event_line_1_.event_mfm_ - 20 * 60 * 1000;
  event_line_1_.end_mfm_ = event_line_1_.event_mfm_ + 5 * 60 * 1000;
  event_line_1_.ez_ = EZ_RTS;
  strncpy(event_line_1_.event_text_, "INTRADAY_CLEARING", EVENT_TEXT_MAX_LEN);
  events_of_the_day_.push_back(event_line_1_);

  EventLine event_line_2_;
  event_line_2_.severity_ = 3;
  const char* start_str_break_3 = "HKT_1200";

  start_time_ =
      HFSAT::DateTime::GetUTCHHMMFromTZHHMM(watch_.YYYYMMDD(), atoi(start_str_break_3 + 4), start_str_break_3);
  start_time_ = (start_time_ / 100) * 3600 + (start_time_ % 100) * 60;
  event_line_2_.event_time_ = (watch_.last_midnight_sec() + start_time_);
  event_line_2_.event_mfm_ = 1000 * (event_line_2_.event_time_ - watch_.last_midnight_sec());
  event_line_2_.start_mfm_ = event_line_2_.event_mfm_ - 5 * 60 * 1000;
  event_line_2_.end_mfm_ = event_line_2_.event_mfm_ + 65 * 60 * 1000;
  event_line_2_.ez_ = EZ_HKD;
  strncpy(event_line_2_.event_text_, "HK_BREAK", EVENT_TEXT_MAX_LEN);
  events_of_the_day_.push_back(event_line_2_);

  // BMF DI1 break
  EventLine event_line_di_;
  event_line_di_.severity_ = 3;
  const char* start_str_break_di_ = "BRT_1600";  // Break Start

  start_time_ =
      HFSAT::DateTime::GetUTCHHMMFromTZHHMM(watch_.YYYYMMDD(), atoi(start_str_break_di_ + 4), start_str_break_di_);

  start_time_ = (start_time_ / 100) * 3600 + (start_time_ % 100) * 60;
  event_line_di_.event_time_ = (watch_.last_midnight_sec() + start_time_);
  event_line_di_.event_mfm_ = 1000 * (event_line_di_.event_time_ - watch_.last_midnight_sec());
  event_line_di_.start_mfm_ = event_line_di_.event_mfm_ - 20 * 60 * 1000;  // getting Flat 20 mins before
  event_line_di_.end_mfm_ = event_line_di_.event_mfm_ + 52 * 60 * 1000;    // BRT_1650 break ends
  event_line_di_.ez_ = EZ_BMFDI;
  strncpy(event_line_di_.event_text_, "DI1_BREAK", EVENT_TEXT_MAX_LEN);
  events_of_the_day_.push_back(event_line_di_);

  sort(events_of_the_day_.begin(), events_of_the_day_.end());
  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    DBGLOG_TIME << " Eco event " << events_of_the_day_[i].toString(watch_.YYYYMMDD()) << DBGLOG_ENDL_FLUSH;
  }
  DBGLOG_DUMP;
}
void EconomicEventsManager::ShowDB() {
  DBGLOG_TIME << "Showing eco events of day " << DBGLOG_ENDL_FLUSH;
  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    DBGLOG_TIME << " Eco event " << events_of_the_day_[i].toString(watch_.YYYYMMDD()) << DBGLOG_ENDL_FLUSH;
  }
  DBGLOG_DUMP;
}

void EconomicEventsManager::AllowEconomicEventsFromList(const std::string& shortcode_) {
  allowed_events_of_the_day_.clear();
  std::string allowed_economic_events_filename_ =
      std::string(NEWBASESYSINFODIR) + "AllowedEconomicEvents/allowed_economic_events.txt";
  std::ifstream allowed_economic_events_file_;
  allowed_economic_events_file_.open(allowed_economic_events_filename_.c_str(), std::ifstream::in);
  std::string shortcode_t = shortcode_;
  if (shortcode_.find("SP_GE") == 0 || shortcode_.find("GE_") == 0) {
    shortcode_t = "GE";
  } else if (shortcode_.find("SP_VX") == 0 || shortcode_.find("VX_") == 0) {
    shortcode_t = "VX";
  }

  if (allowed_economic_events_file_.is_open()) {
    const int kEconomicEventsFileLineBufferLen = 1024;
    char readline_buffer_[kEconomicEventsFileLineBufferLen];
    bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);

    while (allowed_economic_events_file_.good()) {
      bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);
      allowed_economic_events_file_.getline(readline_buffer_, kEconomicEventsFileLineBufferLen);

      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kEconomicEventsFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if ((tokens_.size() >= 3)) {
        std::string t_event_name_ = (std::string)tokens_[2];
        EconomicZone_t t_ez_ = GetEZFromStr(tokens_[1]);
        std::string t_shortcode_ = (std::string)tokens_[0];
        unsigned int num_events_today_ = events_of_the_day_.size();

        if (tokens_.size() >= 5) {
          int t_stop_time_lag_seconds_ = atoi(tokens_[3]);
          int t_start_time_lag_seconds_ = atoi(tokens_[4]);

          if (t_shortcode_.compare(shortcode_t) == 0) {
            for (auto i = 0u; i < num_events_today_; i++) {
              if ((t_event_name_.compare((std::string)events_of_the_day_[i].event_text_) == 0) &&
                  (t_ez_ == events_of_the_day_[i].ez_)) {
                EventLine allowed_event_line_;
                allowed_event_line_.event_time_ = events_of_the_day_[i].event_mfm_;
                allowed_event_line_.event_mfm_ = events_of_the_day_[i].event_mfm_;
                allowed_event_line_.ez_ = events_of_the_day_[i].ez_;
                strncpy(allowed_event_line_.event_text_, events_of_the_day_[i].event_text_, EVENT_TEXT_MAX_LEN);
                allowed_event_line_.start_mfm_ = events_of_the_day_[i].event_mfm_ - t_stop_time_lag_seconds_ * 1000;
                allowed_event_line_.end_mfm_ = events_of_the_day_[i].event_mfm_ - t_start_time_lag_seconds_ * 1000;
                allowed_event_line_.severity_ = 0;    // this is not used
                events_of_the_day_[i].severity_ = 0;  // changed severity of the event, so that the default
                allowed_events_of_the_day_.push_back(allowed_event_line_);
              }
            }
          }
        }

        if (t_shortcode_.compare(shortcode_t) == 0) {
          for (auto i = 0u; i < num_events_today_; i++) {
            if ((t_event_name_.compare((std::string)events_of_the_day_[i].event_text_) == 0) &&
                t_ez_ == events_of_the_day_[i].ez_) {
              events_of_the_day_[i].severity_ = 0;
            }
          }
        }
      }
    }

    sort(allowed_events_of_the_day_.begin(), allowed_events_of_the_day_.end());

    allowed_economic_events_file_.close();
  }
}

void EconomicEventsManager::GetTradedEventsForToday() {
  traded_events_of_the_day_.clear();

  std::ostringstream t_oss_;
  t_oss_ << std::string(BASETRADEINFODIR) + std::string(TRADED_EVENTS_INFO_FILE_NAME);
  std::string traded_economic_events_filename_ = t_oss_.str();
  std::ifstream traded_economic_events_file_;
  traded_economic_events_file_.open(traded_economic_events_filename_.c_str(), std::ifstream::in);

  if (traded_economic_events_file_.is_open()) {
    const int kEconomicEventsFileLineBufferLen = 1024;
    char readline_buffer_[kEconomicEventsFileLineBufferLen];
    bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);

    while (traded_economic_events_file_.good()) {
      bzero(readline_buffer_, kEconomicEventsFileLineBufferLen);
      traded_economic_events_file_.getline(readline_buffer_, kEconomicEventsFileLineBufferLen);
      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kEconomicEventsFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      // ETrdZone OrigEZone _text_ SECS-BEFORE SECS_AFTER
      if ((tokens_.size() >= 5)) {
        std::string t_event_name_ = (std::string)tokens_[2];
        EconomicZone_t t_ez_ = GetEZFromStr(tokens_[1]);
        unsigned int num_events_today_ = events_of_the_day_.size();
        bool ignore_all_ = false;
        if (t_event_name_.compare("IGNORE_ALL") == 0) {
          ignore_all_ = true;
        }
        if (traded_ezone_.compare(tokens_[0]) == 0) {
          for (auto i = 0u; i < num_events_today_; i++) {
            if (ignore_all_) {
              events_of_the_day_[i].severity_ = 0;
            }

            if ((t_event_name_.compare((std::string)events_of_the_day_[i].event_text_) == 0) &&
                t_ez_ == events_of_the_day_[i].ez_) {
              DBGLOG_TIME << "Considering " << t_event_name_ << "to trade" << DBGLOG_ENDL_FLUSH;
              int t_start_time_lag_secs_ = atoi(tokens_[3]);
              int t_end_time_lag_secs_ = atoi(tokens_[4]);
              EventLine traded_event_line_;
              traded_event_line_.event_time_ = events_of_the_day_[i].event_time_;
              traded_event_line_.event_mfm_ = events_of_the_day_[i].event_mfm_;
              traded_event_line_.ez_ = GetEZFromStr(tokens_[0]);
              strncpy(traded_event_line_.event_text_, events_of_the_day_[i].event_text_, EVENT_TEXT_MAX_LEN);
              traded_event_line_.start_mfm_ = events_of_the_day_[i].event_mfm_ - t_start_time_lag_secs_ * 1000;
              traded_event_line_.end_mfm_ = events_of_the_day_[i].event_mfm_ + t_end_time_lag_secs_ * 1000;
              events_of_the_day_[i].severity_ = 0;
              traded_event_line_.severity_ = 0;
              traded_events_of_the_day_.push_back(traded_event_line_);
            }
          }
        }
      }
    }
    traded_economic_events_file_.close();
  }
}

void EconomicEventsManager::AdjustCautiousMap() {
  ez_to_current_severity_map_.clear();
  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    if ((events_of_the_day_[i].start_mfm_ <= watch_.msecs_from_midnight()) &&
        (events_of_the_day_[i].end_mfm_ >= watch_.msecs_from_midnight())) {
      if (ez_to_current_severity_map_.find(events_of_the_day_[i].ez_) == ez_to_current_severity_map_.end()) {
        ez_to_current_severity_map_[events_of_the_day_[i].ez_] = 0;
      }
      if (events_of_the_day_[i].severity_ >= ez_to_current_severity_map_[events_of_the_day_[i].ez_]) {
        ez_to_current_severity_map_[events_of_the_day_[i].ez_] = events_of_the_day_[i].severity_;
      }
    }
  }

  if (compute_tradability_) {
    ez_to_current_tradability_map_.clear();
    for (auto i = 0u; i < traded_events_of_the_day_.size(); i++) {
      if ((traded_events_of_the_day_[i].start_mfm_ <= watch_.msecs_from_midnight()) &&
          (traded_events_of_the_day_[i].end_mfm_ >= watch_.msecs_from_midnight())) {
        if (ez_to_current_tradability_map_.find(traded_events_of_the_day_[i].ez_) ==
            ez_to_current_tradability_map_.end()) {
          ez_to_current_tradability_map_[traded_events_of_the_day_[i].ez_] = 1;
        }
      }
    }
  }
}

/**
 * This function overrides merged_eco file event severities for specific set of exchanges/shortcodes
 * Gets called from exec-logic constructor
 *
 * @param r_shortcode_
 * @param r_exchange_
 */
void EconomicEventsManager::AdjustSeverity(const std::string& r_shortcode_, const ExchSource_t& r_exchange_) {
  // run  OSE for all non 3 degree events in EU and US hours
  if (r_exchange_ == kExchSourceJPY) {
    for (auto&& this_event : events_of_the_day_) {
      if ((this_event.ez_ == EZ_USD || this_event.ez_ == EZ_EUR) && this_event.severity_ < 3) {
        this_event.severity_ = 0;
      }
      if ((this_event.ez_ == EZ_USD || this_event.ez_ == EZ_EUR) && this_event.severity_ >= 3) {
        this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
        this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
      }
    }
  }

  // run EUREX for GBP < 3 degree events
  // run EUREX for USD <= 1 degree events
  // FBTS, run it for all non 3 degree events
  if (r_exchange_ == kExchSourceEUREX) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ == EZ_GBP) {
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        } else {
          this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
          this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
        }
      } else if (this_event.ez_ == EZ_USD) {
        if (this_event.severity_ <= 1) {
          this_event.severity_ = 0;
        }
      }

      if (r_shortcode_.compare("FBTS_0") == 0) {
        // only stopping for 3 degree event
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        }
      }
    }
  }

  if (r_exchange_ == kExchSourceCME) {
    for (auto&& this_event : events_of_the_day_) {
      // Run eurodollars through all < 3 degree events

      if (r_shortcode_.substr(0, 3).compare("GE_") == 0 || r_shortcode_.substr(0, 5).compare("SP_GE") == 0) {
        if (this_event.ez_ == EZ_USD) {
          if (this_event.severity_ < 3) {
            this_event.severity_ = 0;
          }
        }
      }

      // This was added after observation that on 2 degree events our strategies tend to do worse for longer period of
      // time,
      if ((r_shortcode_.substr(0, 3).compare("ZF_")) == 0 || (r_shortcode_.substr(0, 3).compare("ZN_") == 0) ||
          (r_shortcode_.substr(0, 3).compare("ZB_") == 0) || (r_shortcode_.substr(0, 3).compare("UB_") == 0)) {
        if (this_event.ez_ == EZ_USD) {
          if (this_event.severity_ == 2) {
            this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_ + 15 * 60 * 1000;
          }
        }
      }

      // Again currencies which are lagging, no need to stop on < 3 degree vents
      if ((r_shortcode_.compare("6M_0") == 0) || (r_shortcode_.compare("6N_0") == 0)) {
        if (this_event.ez_ == EZ_USD) {
          if (this_event.severity_ < 3) {
            this_event.severity_ = 0;
          } else if (this_event.severity_ == 3) {
            this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_;
          }
        } else {
          this_event.severity_ = 0;
        }
      }

      if (r_shortcode_.compare("6M_0") == 0) {
        if (this_event.ez_ == EZ_MXN) {
          if (this_event.severity_ < 2) {
            this_event.severity_ = 0;
          }
        }
      }

      // 6C not stopping on < 2 USD events
      if (r_shortcode_.compare("6C_0") == 0) {
        if (this_event.ez_ == EZ_USD) {
          if (this_event.severity_ < 3) {
            this_event.severity_ = 0;
          }
        } else if (this_event.ez_ != EZ_CAD) {
          // Not sure if this is needed because anyways we have added USD and CAD in shortcode_ezone_vec.hpp
          this_event.severity_ = 0;
        }
      }

      // AUD/USD stopping for 2 and 3 degree events only
      if (r_shortcode_.compare("6A_0") == 0) {
        if (this_event.ez_ == EZ_USD) {
          if (this_event.severity_ < 2) {
            this_event.severity_ = 0;
          }
        }
      }

      // For wheat products, apart from 3 degree events, they are mostly unaffected
      // Other event which we need to take care of is monthly production which is not there on fxstreet
      if (r_shortcode_.compare("KE_0") == 0 || r_shortcode_.compare("KE_1") == 0 || r_shortcode_.compare("ZW_0") == 0 ||
          r_shortcode_.compare("ZW_1") == 0) {
        if (this_event.severity_ < 2) {
          this_event.severity_ = 0;
        }
      }

      // Not sure why this was done
      if (this_event.ez_ == EZ_GBP) {
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        } else {
          this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
          this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
        }
      }

      // Again, can lead to issues
      // Chinese events tend to have smaller numbers compared to their actual impacts
      if (this_event.ez_ == EZ_CNY) {
        if (this_event.severity_ < 2) {
          this_event.severity_ = 0;
        } else {
          this_event.start_mfm_ = this_event.event_mfm_ - 10 * 60 * 1000;
          this_event.end_mfm_ = this_event.event_mfm_ + 10 * 60 * 1000;
        }
      }
    }
  }
  if (r_exchange_ == kExchSourceRTS) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ != EZ_RTS && this_event.ez_ != EZ_RUB) {
        // stop only for very small duration at 3-degre non-russia events to get rid of positions
        if (this_event.severity_ >= 3) {
          this_event.start_mfm_ = this_event.event_mfm_ - 2 * 60 * 1000;
          this_event.end_mfm_ = this_event.event_mfm_ + 20 * 1000;
        } else {
          this_event.severity_ = 0;
        }
      }
    }
  }

  if (r_exchange_ == kExchSourceTMX) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ == EZ_USD && this_event.severity_ < 3) {
        this_event.severity_ = 0;
      } else if (this_event.ez_ == EZ_CAD) {
        if ((this_event.severity_ < 3) && !r_shortcode_.compare(0, 3, "SXF")) {
          // Not stopping sxf on < 3 degree events
          this_event.severity_ = 0;
        } else if ((this_event.severity_ == 2) && (r_shortcode_.find("BAX") == std::string::npos)) {
          // in TMX, for all Canadian 2 degree events, apart from bax, treat them as 3 degree events (mostly CGB)
          this_event.severity_ = 3;
        }
      }
    }
  }

  // c++ 17 syntax
  // reference for auto&& http://en.cppreference.com/w/cpp/language/range-for
  if (!r_shortcode_.compare(0, 3, "FVS")) {
    for (auto&& this_event_ : events_of_the_day_) {
      if (this_event_.severity_ < 3) {
        this_event_.severity_ = 0;
      } else {
        this_event_.start_mfm_ = this_event_.event_mfm_ - 10 * 60 * 1000;
        this_event_.end_mfm_ = this_event_.event_mfm_ + 1 * 60 * 1000;
      }
    }
  }

  if (r_exchange_ == kExchSourceNSE) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ != EZ_INR) {
        this_event.severity_ = 0;
      }
    }
  }

  if (r_exchange_ == kExchSourceASX) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ == EZ_CNY || this_event.ez_ == EZ_JPY) {
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        } else {
          if (!r_shortcode_.compare(0, 2, "YT") || !r_shortcode_.compare(0, 2, "IR")) {
            this_event.start_mfm_ = this_event.event_mfm_ - 30 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
          } else {
            this_event.start_mfm_ = this_event.event_mfm_ - 15 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
          }
        }
      } else if (this_event.ez_ == EZ_AUD) {
        if (this_event.severity_ < 3) {  // Removing AUD 1 and 2 degree events from here.Will add any relevant event in
                                         // this category in allowed eco file.
          this_event.severity_ = 0;
        } else {
          if (!r_shortcode_.compare(0, 2, "YT") || !r_shortcode_.compare(0, 2, "IR")) {
            this_event.start_mfm_ = this_event.event_mfm_ - 45 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
          } else {
            this_event.start_mfm_ = this_event.event_mfm_ - 15 * 60 * 1000;
            this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
          }
        }
      }
    }
  }
  if ((r_exchange_ == kExchSourceRTS) || (r_exchange_ == kExchSourceMICEX_CR)) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ == EZ_RUB) {
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        } else {
          this_event.start_mfm_ = this_event.event_mfm_ - 10 * 60 * 1000;
          this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
        }
      }
    }
  }
  if (r_exchange_ == kExchSourceSGX) {
    for (auto&& this_event : events_of_the_day_) {
      if (this_event.ez_ == EZ_USD && this_event.severity_ < 3) {
        this_event.severity_ = 0;
      }
      if (this_event.ez_ == EZ_USD && this_event.severity_ >= 3) {
        this_event.start_mfm_ = this_event.event_mfm_ - 5 * 60 * 1000;
        this_event.end_mfm_ = this_event.event_mfm_ + 5 * 60 * 1000;
      }
      if (this_event.ez_ == EZ_HKD) {
        if (this_event.severity_ < 3) {
          this_event.severity_ = 0;
        } else {
          if (r_shortcode_.compare("SGX_CN_0") == 0 || (r_shortcode_.compare("SGX_TW_0") == 0)) {
            this_event.severity_ = 3;
          } else {
            this_event.severity_ = 0;
          }
        }
      }
    }
  }
}
}
