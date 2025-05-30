/**
   \file dvccode/CommonTradeUtils/economic_events_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_COMMONTRADEUTILS_ECONOMIC_EVENTS_MANAGER_H
#define BASE_COMMONTRADEUTILS_ECONOMIC_EVENTS_MANAGER_H

#include <map>
#include <vector>
#include <math.h>

#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#define TRADED_EVENTS_INFO_FILE_NAME "TradedEconomicEvents/traded_economic_events.txt"

namespace HFSAT {

typedef enum {
  EZ_ARG,  // 0
  EZ_AUD,  // 1
  EZ_AUS,
  EZ_BLG,
  EZ_BRL,
  EZ_CAD,
  EZ_CLP,
  EZ_CNY,
  EZ_CZK,
  EZ_DMK,
  EZ_EUR,  // 10
  EZ_FIN,
  EZ_FRA,
  EZ_GER,  // 13
  EZ_GRC,
  EZ_HUF,
  EZ_ISK,
  EZ_INR,
  EZ_IDR,
  EZ_IRE,
  EZ_ITA,  // 20
  EZ_JPY,
  EZ_KRW,
  EZ_MXN,
  EZ_DUT,
  EZ_NZD,  // 25
  EZ_NOK,
  EZ_PLN,
  EZ_POR,
  EZ_ROL,
  EZ_RUB,
  EZ_SLV,
  EZ_ZAR,
  EZ_SPA,  // 33
  EZ_SEK,
  EZ_CHF,  // 35
  EZ_TRY,
  EZ_GBP,  // 37
  EZ_USD,  // 38
  EZ_COP,
  EZ_HKD,
  EZ_SGD,
  EZ_TCGB,
  EZ_TDOL,
  EZ_TIND,
  EZ_TFOAT,
  EZ_TFBON,
  EZ_TFBTP,
  EZ_TFGBL,
  EZ_TFGBM,
  EZ_TFGBS,
  EZ_TFESX,
  EZ_TLFR,
  EZ_EBT,
  EZ_RTS,
  EZ_LFI,  // some ECB specific events only
  EZ_BMFDI,
  EZ_MAX
} EconomicZone_t;

typedef std::map<EconomicZone_t, double> EZ2SevMap;
typedef std::map<EconomicZone_t, int> EZ2TrdMap;
typedef std::map<EconomicZone_t, double>::iterator EZ2SevMapIter_t;
typedef std::map<EconomicZone_t, double>::const_iterator EZ2SevMapCiter_t;
typedef std::map<EconomicZone_t, int>::iterator EZ2TrdMapIter_t;
typedef std::map<EconomicZone_t, int>::const_iterator EZ2TrdMapCiter_t;

inline EconomicZone_t GetEZFromStr(std::string ezstr_) {
  if (ezstr_.compare("ARG") == 0) {
    return EZ_ARG;
  } else if (ezstr_.compare("AUD") == 0) {
    return EZ_AUD;
  } else if (ezstr_.compare("AUS") == 0) {
    return EZ_AUS;
  } else if (ezstr_.compare("BLG") == 0) {
    return EZ_BLG;
  } else if (ezstr_.compare("BRL") == 0) {
    return EZ_BRL;
  } else if (ezstr_.compare("CAD") == 0) {
    return EZ_CAD;
  } else if (ezstr_.compare("CLP") == 0) {
    return EZ_CLP;
  } else if (ezstr_.compare("CNY") == 0) {
    return EZ_CNY;
  } else if (ezstr_.compare("CZK") == 0) {
    return EZ_CZK;
  } else if (ezstr_.compare("DMK") == 0) {
    return EZ_DMK;
  } else if (ezstr_.compare("EUR") == 0) {
    return EZ_EUR;
  } else if (ezstr_.compare("FIN") == 0) {
    return EZ_FIN;
  } else if (ezstr_.compare("FRA") == 0) {
    return EZ_FRA;
  } else if (ezstr_.compare("GER") == 0) {
    return EZ_GER;
  } else if (ezstr_.compare("GRC") == 0) {
    return EZ_GRC;
  } else if (ezstr_.compare("HUF") == 0) {
    return EZ_HUF;
  } else if (ezstr_.compare("ISK") == 0) {
    return EZ_ISK;
  } else if (ezstr_.compare("INR") == 0) {
    return EZ_INR;
  } else if (ezstr_.compare("IDR") == 0) {
    return EZ_IDR;
  } else if (ezstr_.compare("IRE") == 0) {
    return EZ_IRE;
  } else if (ezstr_.compare("ITA") == 0) {
    return EZ_ITA;
  } else if (ezstr_.compare("JPY") == 0) {
    return EZ_JPY;
  } else if (ezstr_.compare("KRW") == 0) {
    return EZ_KRW;
  } else if (ezstr_.compare("MXN") == 0) {
    return EZ_MXN;
  } else if (ezstr_.compare("DUT") == 0) {
    return EZ_DUT;
  } else if (ezstr_.compare("NZD") == 0) {
    return EZ_NZD;
  } else if (ezstr_.compare("NOK") == 0) {
    return EZ_NOK;
  } else if (ezstr_.compare("PLN") == 0) {
    return EZ_PLN;
  } else if (ezstr_.compare("POR") == 0) {
    return EZ_POR;
  } else if (ezstr_.compare("ROL") == 0) {
    return EZ_ROL;
  } else if (ezstr_.compare("RUB") == 0) {
    return EZ_RUB;
  } else if (ezstr_.compare("SLV") == 0) {
    return EZ_SLV;
  } else if (ezstr_.compare("ZAR") == 0) {
    return EZ_ZAR;
  } else if (ezstr_.compare("SPA") == 0) {
    return EZ_SPA;
  } else if (ezstr_.compare("SEK") == 0) {
    return EZ_SEK;
  } else if (ezstr_.compare("CHF") == 0) {
    return EZ_CHF;
  } else if (ezstr_.compare("TRY") == 0) {
    return EZ_TRY;
  } else if (ezstr_.compare("GBP") == 0) {
    return EZ_GBP;
  } else if (ezstr_.compare("USD") == 0) {
    return EZ_USD;
  } else if (ezstr_.compare("COP") == 0) {
    return EZ_COP;
  } else if (ezstr_.compare("HKD") == 0) {
    return EZ_HKD;
  } else if (ezstr_.compare("SGD") == 0) {
    return EZ_SGD;
  } else if (ezstr_.compare("TFGBM") == 0) {
    return EZ_TFGBM;
  } else if (ezstr_.compare("TFGBL") == 0) {
    return EZ_TFGBL;
  } else if (ezstr_.compare("TFOAT") == 0) {
    return EZ_TFOAT;
  } else if (ezstr_.compare("TFBON") == 0) {
    return EZ_TFBON;
  } else if (ezstr_.compare("TFBTP") == 0) {
    return EZ_TFBTP;
  } else if (ezstr_.compare("TFGBS") == 0) {
    return EZ_TFGBS;
  } else if (ezstr_.compare("TDOL") == 0) {
    return EZ_TDOL;
  } else if (ezstr_.compare("TIND") == 0) {
    return EZ_TIND;
  } else if (ezstr_.compare("TCGB") == 0) {
    return EZ_TCGB;
  } else if (ezstr_.compare("TLFR") == 0) {
    return EZ_TLFR;
  } else if (ezstr_.compare("TFESX") == 0) {
    return EZ_TFESX;
  } else if (ezstr_.compare("RTS") == 0) {
    return EZ_RTS;
  } else if (ezstr_.compare("LFI") == 0) {
    return EZ_LFI;
  } else if (ezstr_.substr(0, 3) == "EBT") {
    return EZ_EBT;
  } else if (ezstr_.compare("BMFDI") == 0) {
    return EZ_BMFDI;
  }
  if (ezstr_.compare("---") == 0) {
    return EZ_MAX;
  }
  // ExitVerbose ( kEZStrUnknown, ezstr_.c_str() );
  return EZ_MAX;
}

inline const char* GetStrFromEconomicZone(const EconomicZone_t& t_ez_) {
  switch (t_ez_) {
    case EZ_ARG:
      return "ARG";
      break;
    case EZ_AUD:
      return "AUD";
      break;
    case EZ_AUS:
      return "AUS";
      break;
    case EZ_BLG:
      return "BLG";
      break;
    case EZ_BRL:
      return "BRL";
      break;
    case EZ_CAD:
      return "CAD";
      break;
    case EZ_CLP:
      return "CLP";
      break;
    case EZ_CNY:
      return "CNY";
      break;
    case EZ_CZK:
      return "CZK";
      break;
    case EZ_DMK:
      return "DMK";
      break;
    case EZ_EUR:
      return "EUR";
      break;
    case EZ_FIN:
      return "FIN";
      break;
    case EZ_FRA:
      return "FRA";
      break;
    case EZ_GER:
      return "GER";
      break;
    case EZ_GRC:
      return "GRC";
      break;
    case EZ_HUF:
      return "HUF";
      break;
    case EZ_ISK:
      return "ISK";
      break;
    case EZ_INR:
      return "INR";
      break;
    case EZ_IDR:
      return "IDR";
      break;
    case EZ_IRE:
      return "IRE";
      break;
    case EZ_ITA:
      return "ITA";
      break;
    case EZ_JPY:
      return "JPY";
      break;
    case EZ_KRW:
      return "KRW";
      break;
    case EZ_MXN:
      return "MXN";
      break;
    case EZ_DUT:
      return "DUT";
      break;
    case EZ_NZD:
      return "NZD";
      break;
    case EZ_NOK:
      return "NOK";
      break;
    case EZ_PLN:
      return "PLN";
      break;
    case EZ_POR:
      return "POR";
      break;
    case EZ_ROL:
      return "ROL";
      break;
    case EZ_RUB:
      return "RUB";
      break;
    case EZ_SLV:
      return "SLV";
      break;
    case EZ_ZAR:
      return "ZAR";
      break;
    case EZ_SPA:
      return "SPA";
      break;
    case EZ_SEK:
      return "SEK";
      break;
    case EZ_CHF:
      return "CHF";
      break;
    case EZ_TRY:
      return "TRY";
      break;
    case EZ_GBP:
      return "GBP";
      break;
    case EZ_USD:
      return "USD";
      break;
    case EZ_COP:
      return "COP";
      break;
    case EZ_HKD:
      return "HKD";
      break;
    case EZ_SGD:
      return "SGD";
      break;
    case EZ_MAX:
      return "MAX";
      break;
    case EZ_TIND:
      return "TIND";
      break;
    case EZ_TDOL:
      return "TDOL";
      break;
    case EZ_TLFR:
      return "TLFR";
      break;
    case EZ_TCGB:
      return "TCGB";
      break;
    case EZ_TFESX:
      return "TFESX";
      break;
    case EZ_TFGBM:
      return "TFGBM";
      break;
    case EZ_TFGBL:
      return "TFGBL";
      break;
    case EZ_TFOAT:
      return "TFOAT";
      break;
    case EZ_TFBON:  // why ?
      return "TFBON";
      break;
    case EZ_TFBTP:
      return "TFBTP";
      break;
    case EZ_TFGBS:
      return "TFGBS";
      break;
    case EZ_RTS:
      return "RTS";
      break;
    case EZ_EBT:
      return "EBT";
      break;
    case EZ_LFI:
      return "LFI";
      break;
    case EZ_BMFDI:
      return "BMFDI";
      break;
    default:
      return "USD";
  }
}

#define EVENT_TEXT_MAX_LEN 128u

struct EventLine {
  time_t event_time_;
  int event_mfm_;
  EconomicZone_t ez_;

  // generally start time and end time are 5 minutes before and 5 minutes after the
  // economic events except if the text reads special strings like
  // "ECB_Trichet's_Speech"
  // we could also make it a function of the severity ?
  int start_mfm_;
  int end_mfm_;
  int severity_;
  char event_text_[EVENT_TEXT_MAX_LEN];

  EventLine() : event_time_(0), event_mfm_(0), ez_(EZ_USD), start_mfm_(0), end_mfm_(0), severity_(0) {
    bzero(event_text_, EVENT_TEXT_MAX_LEN);
  }

  EventLine(const EventLine& _new_event_line_)
      : event_time_(_new_event_line_.event_time_),
        event_mfm_(_new_event_line_.event_mfm_),
        ez_(_new_event_line_.ez_),
        start_mfm_(_new_event_line_.start_mfm_),
        end_mfm_(_new_event_line_.end_mfm_),
        severity_(_new_event_line_.severity_) {
    memcpy(event_text_, _new_event_line_.event_text_, EVENT_TEXT_MAX_LEN);
  }

  inline bool operator<(const EventLine& b) const { return start_mfm_ < b.start_mfm_; }

  inline std::string toString(const int tradingdate_) const {
    std::ostringstream t_temp_oss_;

    t_temp_oss_
        << event_time_ << ' ' << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(event_time_) << ' '
        << HFSAT::GetStrFromEconomicZone(ez_) << ' ' << severity_ << ' ' << event_text_ << ' '
        << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, start_mfm_))
        << ' ' << start_mfm_ / 1000 << ' ' << event_mfm_ / 1000 << ' ' << end_mfm_ / 1000 << ' '
        << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, end_mfm_))
        << "\n";

    return t_temp_oss_.str();
  }
};

class EconomicEventsManager : public TimePeriodListener {
 public:
  EconomicEventsManager(DebugLogger& t_dbglogger_, const Watch& r_watch_);

  EconomicEventsManager(DebugLogger& t_dbglogger_, const Watch& r_watch_, std::string _traded_ezone_);

  inline void SetComputeTradability(bool t_value_) { compute_tradability_ = true; }

  // /// Needs to be called, only if the tradingdate that watch has in the beginning
  // /// is invalidated by something later. So in practise not needed
  // void SetTradingDate ( int t_tradingdate_ ) ;

  inline void RefreshCautiousMap() {
    if ((last_economic_times_checked_ == 0) || (watch_.msecs_from_midnight() - last_economic_times_checked_ > 500) ||
        (watch_.msecs_from_midnight() - last_economic_times_checked_ <
         0))  // clock was fast forwarded by 24 hours.This case will occur exactly once if it does
    {
      AdjustCautiousMap();
      last_economic_times_checked_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % 15000);
    }
  }

  inline int GetCurrentSeverity(EconomicZone_t t_ez_) const {
    EZ2SevMapCiter_t ez_to_sev_map_citer_ = ez_to_current_severity_map_.find(t_ez_);
    if (ez_to_sev_map_citer_ != ez_to_current_severity_map_.end()) {
      return floor(ez_to_sev_map_citer_->second);
    }
    return 0;
  }

  inline bool AllowedEventsPresent() const {
    if (allowed_events_of_the_day_.size() <= 0) {
      return false;
    } else {
      return true;
    }
  }

  inline bool IsEventHappening() const {
    for (unsigned i = 0; i < original_events_of_the_day_.size(); i++) {
      if (original_events_of_the_day_[i].start_mfm_ <= watch_.msecs_from_midnight() &&
          original_events_of_the_day_[i].end_mfm_ >= watch_.msecs_from_midnight())  // can check for sev  > 0 too
      {
        return true;
      }
    }
    return false;
  }

  inline bool GetCurrentTradability() const {
    EconomicZone_t t_ez_ = GetEZFromStr("EBT_");  // proxy for all traded_ezones
    EZ2TrdMapCiter_t ez_to_trd_map_citer_ = ez_to_current_tradability_map_.find(t_ez_);
    if (ez_to_trd_map_citer_ != ez_to_current_tradability_map_.end()) {
      if (ez_to_trd_map_citer_->second == 1) {
        return true;
      } else {
        return false;
      }
    }
    return false;
  }

  inline const std::vector<EventLine>& events_of_the_day() const { return events_of_the_day_; }
  inline const std::vector<EventLine>& allowed_events_of_the_day() const { return allowed_events_of_the_day_; }
  inline const std::vector<EventLine>& traded_events_of_the_day() const { return traded_events_of_the_day_; }

  /// currently called by watch ... TODO ... call this on select timeout in livetrading
  inline void OnTimePeriodUpdate(const int num_pages_to_add_) { RefreshCautiousMap(); }

  void GetTradedEventsForToday();
  void AllowEconomicEventsFromList(const std::string& shortcode_);
  /// for case where exch || shc && sev = sev > k ?  k : 0, call after
  void AdjustSeverity(const std::string& r_shortcode_, const ExchSource_t& r_exchange_);

  /// Does the heavy lifting of reading the file and storing a vector of economic events for today.
  void ReloadDB();

  /// Display the list of presently loaded economic events
  void ShowDB();

 protected:
 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  int tradingdate_;

  std::vector<EventLine> events_of_the_day_;
  std::vector<EventLine> traded_events_of_the_day_;
  std::vector<EventLine> allowed_events_of_the_day_;
  std::vector<EventLine> original_events_of_the_day_;
  EZ2SevMap ez_to_current_severity_map_;  ///< Computed in AdjustCautiousMap
  EZ2TrdMap ez_to_current_tradability_map_;

  int last_economic_times_checked_;
  std::string traded_ezone_;
  bool compute_tradability_;

  /// Based on current msecs from midnight, it goes through all events and
  /// for whichever event the mfm is not more than 5 minutes in future and 6 minutes in the past,
  /// say for "USD","_text", severity
  /// adds to map as
  void AdjustCautiousMap();
};
}

#endif  // BASE_COMMONTRADEUTILS_ECONOMIC_EVENTS_MANAGER_H
