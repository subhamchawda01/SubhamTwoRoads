/**
   \file CDefCode/exchange_symbol_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#define YFEBM_MAPPING_DIR "/spare/local/files/LIFFE/"

#define TOTAL_DI_SHORTCODES 6
#define DI_VOLUME_DAYS 5

namespace HFSAT {

ExchangeSymbolManager *ExchangeSymbolManager::p_uniqueinstance_ = NULL;

const char CMEMonthCode[] = {
    'F',  // January
    'G',  // February
    'H',  // March
    'J',  // April
    'K',  // May
    'M',  // June
    'N',  // July
    'Q',  // August
    'U',  // September
    'V',  // October
    'X',  // November
    'Z'   // December
};

const char *CME_ZTY = "ZTY";
const char *CME_ZFY = "ZFY";
const char *CME_ZNY = "ZNY";
const char *CME_ZBY = "ZBY";
const char *CME_UBY = "UBY";
const char *CME_GEY = "GEY";
const char *CME_ZT = "ZT";
const char *CME_ZF = "ZF";
const char *CME_ZN = "ZN";
const char *CME_TN = "TN";
const char *CME_ZB = "ZB";
const char *CME_UB = "UB";
const char *CME_NKD = "NKD";
const char *CME_NIY = "NIY";
const char *CME_GE = "GE";
const char *CME_ES = "ES";
const char *CME_NQ = "NQ";
const char *CME_YM = "YM";
const char *CME_6A = "6A";
const char *CME_6B = "6B";
const char *CME_6C = "6C";
const char *CME_6E = "6E";
const char *CME_6J = "6J";
const char *CME_6M = "6M";
const char *CME_6N = "6N";
const char *CME_6S = "6S";
const char *CME_6R = "6R";
const char *CME_SEK = "SEK";
const char *CME_6L = "6L";
const char *CME_SIR = "SIR";
const char *CME_6Z = "6Z";

const char *CME_GC = "GC";
const char *CME_CL = "CL";
const char *CME_QM = "QM";
const char *CME_BZ = "BZ";
const char *CME_SI = "SI";
const char *CME_HG = "HG";
const char *CME_NG = "NG";
const char *CME_NN = "NN";
const char *CME_RB = "RB";
const char *CME_HO = "HO";

const char *CME_EMD = "EMD";
const char *CME_IBV = "IBV";
const char *CME_ZW = "ZW";
const char *CME_KE = "KE";
const char *CME_ZC = "ZC";
const char *CME_ZS = "ZS";
const char *CME_ZL = "ZL";
const char *CME_ZM = "ZM";
const char *CME_XW = "XW";
const char *EUREX_FGBSY = "FGBSY";
const char *EUREX_FGBMY = "FGBMY";
const char *EUREX_FGBLY = "FGBLY";
const char *EUREX_FGBXY = "FGBXY";
const char *EUREX_FBTSY = "FBTSY";
const char *EUREX_FBTPY = "FBTPY";
const char *EUREX_FBTMY = "FBTMY";
const char *EUREX_FOATY = "FOATY";
const char *EUREX_FOAMY = "FOAMY";
const char *EUREX_CONFY = "CONFY";
const char *EUREX_FBONY = "FBONY";
const char *EUREX_FGBS = "FGBS";
const char *EUREX_FGBM = "FGBM";
const char *EUREX_FGBL = "FGBL";
const char *EUREX_FGBX = "FGBX";
const char *EUREX_FBTS = "FBTS";
const char *EUREX_FBTP = "FBTP";
const char *EUREX_FBTM = "FBTM";
const char *EUREX_FOAT = "FOAT";
const char *EUREX_FBON = "FBON";
const char *EUREX_FOAM = "FOAM";
const char *EUREX_CONF = "CONF";
const char *EUREX_FESX = "FESX";
const char *EUREX_FESQ = "FESQ";
const char *EUREX_FDAX = "FDAX";
const char *EUREX_FDXM = "FDXM";
const char *EUREX_FSMI = "FSMI";
const char *EUREX_FEXF = "FEXF";
const char *EUREX_FESB = "FESB";
const char *EUREX_FSTB = "FSTB";
const char *EUREX_FSTS = "FSTS";
const char *EUREX_FSTO = "FSTO";
const char *EUREX_FSTG = "FSTG";
const char *EUREX_FSTI = "FSTI";
const char *EUREX_FSTM = "FSTM";
const char *EUREX_F2MX = "F2MX";
const char *EUREX_FXXP = "FXXP";
const char *EUREX_OKS2 = "OKS2";
const char *EUREX_FEXD = "FEXD";
const char *EUREX_FRDX = "FRDX";
const char *EUREX_FVS = "FVS";  // check if anywhere we use assumption of eurex shortcode size being 4 and enable
const char *EUREX_FEU3 = "FEU3";
const char *EUREX_FCEU = "FCEU";
const char *EUREX_FCPU = "FCPU";
const char *EUREX_FCUF = "FCUF";
const char *TMX_SXF = "SXF";
const char *TMX_CGBY = "CGBY";
const char *TMX_CGB = "CGB";
const char *TMX_EMF = "EMF";
const char *TMX_CGF = "CGF";
const char *TMX_CGZ = "CGZ";
const char *TMX_BAX = "BAX";
const char *BMF_DOL = "BR_DOL";
const char *BMF_IND = "BR_IND";
const char *BMF_WIN = "BR_WIN";
const char *BMF_WDO = "BR_WDO";  // Added from Aug152011
const char *BMF_WEU = "BR_WEU";
const char *BMF_DI = "DI1";
const char *BMF_SP_DI = "SPDI1";
const char *BMF_ISP = "BR_ISP";
const char *BMF_CCM = "BR_CCM";
const char *BMF_ICF = "BR_ICF";
const char *BMF_SJC = "BR_SJC";
const char *LIFFE_JFFCE = "JFFCE";
const char *LIFFE_KFFTI = "KFFTI";
const char *LIFFE_KFMFA = "KFMFA";
const char *LIFFE_JFMFC = "JFMFC";
const char *LIFFE_LFZ = "LFZ";
const char *LIFFE_LFI = "LFI";
const char *LIFFE_LFL = "LFL";
const char *LIFFE_LFR = "LFR";
const char *ICE_LFL = "LFL";
const char *ICE_LFR = "LFR";
const char *ICE_LFS = "LFS";
const char *RTS_Si = "Si";
const char *RTS_RI = "RI";
const char *RTS_BR = "BR";
const char *RTS_ED = "ED";
const char *RTS_GD = "GD";
const char *LIFFE_YFEBM = "YFEBM";
const char *LIFFE_XFW = "XFW";
const char *LIFFE_XFC = "XFC";
const char *LIFFE_XFRC = "XFRC";
const char *TSE_JGB = "GFJGB";
const char *TSE_TPX = "FFTPX";
const char *TSE_MTP = "FFMTP";
const char *TSE_MJG = "GFMJG";
const char *EBS_USDRUB = "USD/RUB";
const char *OSE_NKMF = "NKMF";
const char *OSE_DJI = "DJI";
const char *OSE_JGBL = "JGBL";

const char *SGX_CN = "SGX_CN";
const char *SGX_IN = "SGX_IN";
const char *SGX_NK = "SGX_NK";
const char *SGX_NU = "SGX_NU";
const char *SGX_AJ = "SGX_AJ";
const char *SGX_AU = "SGX_AU";
const char *SGX_US = "SGX_US";
const char *SGX_IU = "SGX_IU";
const char *SGX_KU = "SGX_KU";
const char *SGX_TW = "SGX_TW";
const char *SGX_SG = "SGX_SG";
const char *SGX_NKF = "SGX_NKF";
const char *SGX_INB = "SGX_INB";
const char *SGX_CH = "SGX_CH";
const char *SGX_MD = "SGX_MD";

const boost::gregorian::date_duration one_day_date_duration(1);
const boost::gregorian::date_duration two_day_date_duration(2);
const boost::gregorian::date_duration three_day_date_duration(3);
const boost::gregorian::date_duration eight_day_date_duration(8);
const boost::gregorian::date_duration nine_day_date_duration(9);
const boost::gregorian::date_duration ten_day_date_duration(10);
const boost::gregorian::date_duration eleven_day_date_duration(11);
const boost::gregorian::date_duration sixteen_day_date_duration(16);
const boost::gregorian::date_duration thirty_day_date_duration(30);
const boost::gregorian::date_duration thirty_nine_day_date_duration(39);
const boost::gregorian::date_duration forty_day_date_duration(40);
const boost::gregorian::months one_month(1);
const boost::gregorian::months two_months(1);

// Returns date in yyyymmdd format
inline unsigned int YYYYMMDD_from_date(const boost::gregorian::date &d1) {
  boost::gregorian::date::ymd_type ymd = d1.year_month_day();
  return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
}

// Returns true if given date is a weekend
inline bool IsWeekend(boost::gregorian::date &d1) {
  return ((d1.day_of_week() == boost::gregorian::Saturday) || (d1.day_of_week() == boost::gregorian::Sunday));
}

// Updates the given date to date obtained after subtracting n week days
inline void SubtractNWeekDays(boost::gregorian::date &given_date, int num_days) {
  while (num_days > 0) {
    if (!IsWeekend(given_date)) {
      num_days--;
    }
    given_date -= one_day_date_duration;
  }
}

/// Returns true if for the given symbol this is a day on which trading is happening on CME
inline bool IsCMEExchangeDate(const std::string &_pure_basename_, boost::gregorian::date &d1) {
  // TODO only using weekends as holidays, need to add CME holiday from Quantlib
  return !IsWeekend(d1);
}

ExchangeSymbolManager::ExchangeSymbolManager(const int _YYYYMMDD_)
    : YYYYMMDD_(_YYYYMMDD_),
      orpair_vec_(),
      vol_orpair_vec_(),
      large_es_db_(LARGE_ES_DB_SIZE),
      large_es_db_front_index_(0),
      shortcode_exchange_symbol_map_(),
      bmf_last_trading_dates_vec_() {
  LoadLastBMFTradingDatesOfMonth(YYYYMMDD_);
  LoadRolloverOverrideFile(YYYYMMDD_);
  LoadVolumeBasedSymbolFile(YYYYMMDD_);
  //LoadStaticYFEBMMappingFile(YYYYMMDD_);
  //LoadLIFFESpreadMap(YYYYMMDD_);
  //    LoadMultipleVolumeBasedSymbolFile ( YYYYMMDD_ ) ;
}

void ExchangeSymbolManager::LoadLastBMFTradingDatesOfMonth(
    const int _YYYYMMDD_) {  // will be using for BMF only later on can pass exch as arg

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << BASESYSINFODIR << "RolloverOverride/LastTradingDatesOfMonth_" << (_YYYYMMDD_ / 10000) << ".txt";

  std::string last_trading_dates_of_month_filename_ = HFSAT::FileUtils::AppendHome(t_temp_oss_.str());

  std::ifstream last_trading_dates_of_month_file_;
  last_trading_dates_of_month_file_.open(last_trading_dates_of_month_filename_.c_str(), std::ifstream::in);

  if (!last_trading_dates_of_month_file_.is_open()) {
    return;
  }

  char line_buffer_[1024];
  std::string line_read_ = "";

  while (last_trading_dates_of_month_file_.good()) {
    memset(line_buffer_, 0, 1024);
    line_read_ = "";

    last_trading_dates_of_month_file_.getline(line_buffer_, 1024);
    line_read_ = line_buffer_;

    if (line_read_.find("#") != std::string::npos) continue;  // skip comments

    PerishableStringTokenizer st_(line_buffer_, 1024);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() == 1 && strlen(tokens_[0]) == 8) {
      bmf_last_trading_dates_vec_.push_back(atoi(tokens_[0]));
    }
  }

  last_trading_dates_of_month_file_.close();
}

bool ExchangeSymbolManager::IsLastBMFTradingDateOfMonth(const int _YYYYMMDD_) {
  for (unsigned int last_dates_counter_ = 0; last_dates_counter_ < bmf_last_trading_dates_vec_.size();
       last_dates_counter_++) {
    if (bmf_last_trading_dates_vec_[last_dates_counter_] == _YYYYMMDD_) return true;
  }

  return false;
}

bool ExchangeSymbolManager::IsTSEHoliday(const int _YYYYMMDD_) {
  return HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceTSEStr, _YYYYMMDD_);
}

bool ExchangeSymbolManager::IsCFEHoliday(const int _YYYYMMDD_) {
  return HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceCFEStr, _YYYYMMDD_);
}

void ExchangeSymbolManager::LoadRolloverOverrideFile(int _YYYYMMDD_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << BASESYSINFODIR << "RolloverOverride/RO_" << _YYYYMMDD_ << ".txt";
  std::string _rollover_override_filename_;
  // printf ( "OR filename %s\n", _rollover_override_filename_.c_str() );

  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  if (!strncmp(hostname, "SDV-HK-SRV", 10)) {
    // HK ORS runs from Newedge User which we cannot access to change rollover override file, so changing the filepath
    // to that of dvcinfra user
    _rollover_override_filename_ = std::string("/home/dvcinfra") + "/" + t_temp_oss_.str();
  } else {
    _rollover_override_filename_ = HFSAT::FileUtils::AppendHome(t_temp_oss_.str());
  }

  std::ifstream rollover_override_file_;
  rollover_override_file_.open(_rollover_override_filename_.c_str(), std::ifstream::in);
  if (rollover_override_file_.is_open()) {
    const int kRolloverOverrideLen = 1024;
    char readline_buffer_[kRolloverOverrideLen];
    bzero(readline_buffer_, kRolloverOverrideLen);

    while (rollover_override_file_.good()) {
      bzero(readline_buffer_, kRolloverOverrideLen);
      rollover_override_file_.getline(readline_buffer_, kRolloverOverrideLen);
      PerishableStringTokenizer st_(readline_buffer_, kRolloverOverrideLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      // expect lines of the form :
      // SHORTCODE EXCHSYMBOL
      if (tokens_.size() == 2) {
        ORpair _this_override_pair_;
        _this_override_pair_.shortcode_ = tokens_[0];

        // FOR LIFFE Products
        std::string this_format_string_ = tokens_[1];  // null terminated str
        std::replace(this_format_string_.begin(), this_format_string_.end(), '~', ' ');
        unsigned int max_string_length_to_copy_ =
            std::min((unsigned int)(kSecNameLen - 1), (unsigned int)this_format_string_.length());
        memcpy(_this_override_pair_.exchange_symbol_, this_format_string_.c_str(), max_string_length_to_copy_);
        _this_override_pair_.exchange_symbol_[max_string_length_to_copy_] = '\0';

        // strncpy ( _this_override_pair_.exchange_symbol_, tokens_[1], kSecNameLen ) ;
        // printf ( "New ORPair %s %s\n", _this_override_pair_.shortcode_.c_str(), _this_override_pair_.exchange_symbol_
        // );
        orpair_vec_.push_back(_this_override_pair_);
      }
    }
    rollover_override_file_.close();
  }
}

const char *ExchangeSymbolManager::ConvertToICESymbol(std::string _this_shortcode_) {
  if (_this_shortcode_.substr(0, 3) == "SP_") {  // spreads would have two underlying symbols of form SP_LFIa_LFIb
    std::string basename = _this_shortcode_.substr(3, 3);
    std::size_t t_occurence_ = _this_shortcode_.find("_", 3);
    std::string spread_shortcode_1_ = basename + "_" + _this_shortcode_.substr(6, t_occurence_ - 6);
    std::string spread_shortcode_2_ =
        basename + "_" + _this_shortcode_.substr(t_occurence_ + 4, _this_shortcode_.size() - t_occurence_ - 4);

    std::string symbol_1_ = _GetExchSymbolShortICE(spread_shortcode_1_, true);
    std::string symbol_2_ = _GetExchSymbolShortICE(spread_shortcode_2_, true);

    // search in the map for exchSymbol to spread name
    return _localstore(_this_shortcode_, (symbol_1_ + "-" + symbol_2_).c_str());  // Of the form L1412-L1503
  } else if (_this_shortcode_.substr(0, 2) == "B_") {  // flies would have 3 underlying symbols of form B_LFI_a_b_c
    std::string basename = _this_shortcode_.substr(2, 3);
    std::size_t t_occurence_start_ = 6;
    std::size_t t_occurence_end_ = _this_shortcode_.find("_", 6);
    std::string spread_shortcode_1_ =
        basename + "_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

    t_occurence_end_++;
    t_occurence_start_ = t_occurence_end_;
    t_occurence_end_ = _this_shortcode_.find("_", t_occurence_end_);
    std::string spread_shortcode_2_ =
        basename + "_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

    t_occurence_end_++;
    t_occurence_start_ = t_occurence_end_;
    t_occurence_end_ = _this_shortcode_.find("_", t_occurence_end_);
    std::string spread_shortcode_3_ =
        basename + "_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

    std::string symbol_1_ = _GetExchSymbolShortICE(spread_shortcode_1_, true);
    std::string symbol_2_ = _GetExchSymbolShortICE(spread_shortcode_2_, true);
    std::string symbol_3_ = _GetExchSymbolShortICE(spread_shortcode_3_, true);

    // search in the map for exchSymbol to spread name
    return _localstore(_this_shortcode_,
                       (symbol_1_ + symbol_2_.substr(3) + symbol_3_.substr(3)).c_str());  // Of the form Ld-e-f
  }

  return (_GetExchSymbolICE(_this_shortcode_));
}

/*
  void ExchangeSymbolManager::LoadStaticDIMappingFile ( int _YYYYMMDD_ ) {

    std::ostringstream t_temp_oss_ ;
    t_temp_oss_ << DI_MAPPING_DIR << "static_di_mapping_file_test.txt" ;

    std::string di_static_mapping_filename_ = t_temp_oss_.str() ;

    std::ifstream di_static_mapping_file_;

    di_static_mapping_file_.open ( di_static_mapping_filename_.c_str(), std::ifstream::in ) ;

    if ( di_static_mapping_file_.is_open ( ) )
      {
        const int kRolloverOverrideLen = 1024 ;
        char readline_buffer_ [ kRolloverOverrideLen ];

        std::string line_read_ = "" ;

        memset ( readline_buffer_, 0, kRolloverOverrideLen ) ;

        while ( di_static_mapping_file_.good ( ) )
          {
            memset ( readline_buffer_, 0, kRolloverOverrideLen ) ;

            di_static_mapping_file_.getline ( readline_buffer_, kRolloverOverrideLen ) ;

            if( line_read_.find( "#" ) != std::string::npos ) continue ;  //comments

            PerishableStringTokenizer st_ ( readline_buffer_, kRolloverOverrideLen );
            const std::vector < const char * > & tokens_ = st_.GetTokens ( );

            if ( tokens_.size ( ) == 4 )
              {

                if( _YYYYMMDD_ > atoi( tokens_[ 0 ] ) && _YYYYMMDD_ <= atoi( tokens_[ 1 ] ) ){

                  ORpair _this_override_pair_ ;
                  _this_override_pair_.shortcode_ = tokens_[2];
                  strncpy ( _this_override_pair_.exchange_symbol_, tokens_[3], kSecNameLen ) ;
                  di_orpair_vec_.push_back ( _this_override_pair_ );

                }

              }
          }

        di_static_mapping_file_.close ( ) ;

      }
    else
      {
        fprintf ( stderr , "Could not open file %s\n" , di_static_mapping_filename_.c_str ( ) );
        exit ( -1 );
      }
  }
 */
void ExchangeSymbolManager::LoadStaticYFEBMMappingFile(int _YYYYMMDD_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << YFEBM_MAPPING_DIR << "static_yfebm_mapping_file_test.txt";

  std::string yfebm_static_mapping_filename_ = t_temp_oss_.str();

  std::ifstream yfebm_static_mapping_file_;

  yfebm_static_mapping_file_.open(yfebm_static_mapping_filename_.c_str(), std::ifstream::in);

  if (yfebm_static_mapping_file_.is_open()) {
    const int kRolloverOverrideLen = 1024;
    char readline_buffer_[kRolloverOverrideLen];

    std::string line_read_ = "";

    memset(readline_buffer_, 0, kRolloverOverrideLen);

    while (yfebm_static_mapping_file_.good()) {
      memset(readline_buffer_, 0, kRolloverOverrideLen);

      yfebm_static_mapping_file_.getline(readline_buffer_, kRolloverOverrideLen);

      if (line_read_.find("#") != std::string::npos) continue;  // comments

      PerishableStringTokenizer st_(readline_buffer_, kRolloverOverrideLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 4) {
        if (_YYYYMMDD_ >= atoi(tokens_[0]) && _YYYYMMDD_ <= atoi(tokens_[1])) {
          ORpair _this_override_pair_;
          _this_override_pair_.shortcode_ = tokens_[2];
          strncpy(_this_override_pair_.exchange_symbol_, tokens_[3], kSecNameLen);
          yfebm_orpair_vec_.push_back(_this_override_pair_);
        }
      }
    }

    yfebm_static_mapping_file_.close();

  } else {
    fprintf(stderr, "Could not open file %s\n", yfebm_static_mapping_filename_.c_str());
    exit(-1);
  }
}

//  void ExchangeSymbolManager::LoadMultipleVolumeBasedSymbolFile ( int _YYYYMMDD_ )
//  {
//
//    int this_YYYYMMDD_ = _YYYYMMDD_ ;
//    std::string _volume_symbol_filename_ = "";
//
//    // Try to load most recent file in the last 30 days
//    std::map < std::string, unsigned int > symbol_volume_map_ ;
//    std::map < std::string, std::vector< std::string > > shortcode_multi_symbol_map_ ;
//    std::vector < std::string > volume_based_symbols_ ;
//
//    int no_of_days_ = DI_VOLUME_DAYS ;
//
//    int data_available_counter_ = 0 ;
//
//    while( no_of_days_ > 0 ){
//
//      for ( unsigned int ii = 0; ii < 30; ii++ )
//	{
//	  std::ostringstream t_temp_oss_ ;
//	  t_temp_oss_ << "/spare/local/" << "VolumeBasedSymbol/VOSymbol_" << this_YYYYMMDD_ << ".txt" ;
//	  _volume_symbol_filename_ = t_temp_oss_ .str ( ) ;
//
//	  this_YYYYMMDD_ = DateTime::CalcPrevDay ( this_YYYYMMDD_ ) ;
//
//	  if ( FileUtils::exists ( _volume_symbol_filename_ ) )  { data_available_counter_ ++; break ; }
//	}
//
//      if ( !FileUtils::exists ( _volume_symbol_filename_ ) )
//	{
//	  return ; //if no data available 30 days back then return, logical mapping
//	}
//
//      std::ifstream volume_symbol_file_;
//      volume_symbol_file_.open ( _volume_symbol_filename_.c_str(), std::ifstream::in ) ;
//      if ( volume_symbol_file_.is_open ( ) )
//	{
//
//	  const int kVolSymbolLen = 1024 ;
//	  char readline_buffer_ [ kVolSymbolLen ];
//	  bzero ( readline_buffer_, kVolSymbolLen );
//
//	  while ( volume_symbol_file_.good ( ) )
//	    {
//	      bzero ( readline_buffer_, kVolSymbolLen );
//	      volume_symbol_file_.getline ( readline_buffer_, kVolSymbolLen ) ;
//	      PerishableStringTokenizer st_ ( readline_buffer_, kVolSymbolLen );
//	      const std::vector < const char * > & tokens_ = st_.GetTokens ( );
//	      // expect lines of the form :
//	      // SHORTCODE EXCHSYMBOL VOLUME
//	      if ( tokens_.size ( ) == 3 )
//		{
//		  ORpair _this_volsymbol_pair_ ;
//		  // Load for Brazilian products
//		  // BR_DOL_0 is actually DOL_0 according to volume file
//		  if ( std::string(tokens_[0]).compare(0, 2, "DI")== 0 )
//		    {
//		      if( symbol_volume_map_.find( tokens_[1] ) != symbol_volume_map_.end() ){
//
//			if( atoi( tokens_[2] ) == -1 ) { symbol_volume_map_[ tokens_[1] ] += 0 ; }
//			else { symbol_volume_map_[ tokens_[1] ] += atoi( tokens_[2] ) ; }
//
//		      }else{
//
//			if( atoi( tokens_[2] ) == -1 ) { symbol_volume_map_[ tokens_[1] ] = 0 ; }
//			else { symbol_volume_map_[ tokens_[1] ] = atoi( tokens_[2] ) ; }
//
//		      }
//
//		      if( shortcode_multi_symbol_map_.find( tokens_[0] ) != shortcode_multi_symbol_map_.end( ) ){
//
//			shortcode_multi_symbol_map_[ tokens_[0] ].push_back( tokens_[1] ) ;
//
//		      }else{
//
//			std::vector < std::string > symbols_per_shortcode_ ;
//
//			symbols_per_shortcode_.push_back( tokens_[1] ) ;
//			shortcode_multi_symbol_map_[ tokens_[0] ] = symbols_per_shortcode_ ;
//
//		      }
//
//		    }
//
//		}
//	    }
//	  volume_symbol_file_.close ( ) ;
//	}
//
//      no_of_days_ -- ;
//
//    }
//
//    if( data_available_counter_ > 0 ) {
//
//      std::map < std::string, unsigned int > :: iterator itr_ = symbol_volume_map_.begin() ;
//      std::vector < unsigned int > volumes_sorted_ ;
//
//      while( itr_ != symbol_volume_map_.end() ){
//
//	volumes_sorted_.push_back( itr_ -> second ) ;
//	itr_ ++ ;
//
//      }
//
//      std::sort ( volumes_sorted_.begin(), volumes_sorted_.end() ) ;
//      reverse( volumes_sorted_.begin(), volumes_sorted_.end() ) ;
//
//      for( unsigned int i=0; i < volumes_sorted_.size(); i++ ) {
//
//	itr_ = symbol_volume_map_.begin() ;
//
//	while( itr_ != symbol_volume_map_.end() ){
//
//	  if( itr_ -> second == volumes_sorted_[ i ] ){
//
//	    volume_based_symbols_.push_back( itr_ -> first );
//	    symbol_volume_map_.erase( itr_->first );
//	    break ;
//
//	  }
//
//	  itr_ ++ ;
//
//	}
//
//      }
//
//
//      for( unsigned int symbol_counter_ = 0; symbol_counter_ < TOTAL_DI_SHORTCODES ; symbol_counter_ ++ ){
//
//	std::ostringstream t_temp_oss_ ;
//	t_temp_oss_ << "BR_DI_" << symbol_counter_ ;
//
//	ORpair _this_volsymbol_pair_ ;
//
//	_this_volsymbol_pair_.shortcode_ = t_temp_oss_.str() ;
//
//	strncpy ( _this_volsymbol_pair_.exchange_symbol_, volume_based_symbols_[ symbol_counter_ ].c_str(), kSecNameLen
//) ;
//	vol_orpair_vec_.push_back ( _this_volsymbol_pair_ );
//
//      }
//
//    }
//
//  }
//

void ExchangeSymbolManager::LoadVolumeBasedSymbolFile(int _YYYYMMDD_) {
  int this_YYYYMMDD_ = _YYYYMMDD_;
  std::string _volume_symbol_filename_ = "";
  // Try to load most recent file in the last 30 days
  for (unsigned int ii = 0; ii < 30; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/"
                << "VolumeBasedSymbol/VOSymbol_" << this_YYYYMMDD_ << ".txt";
    _volume_symbol_filename_ = t_temp_oss_.str();

    if (FileUtils::exists(_volume_symbol_filename_))
      break;
    else {
      // Try previous day
      this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
  }

  if (!FileUtils::exists(_volume_symbol_filename_)) {
    //	fprintf(stderr, "No Volume based Symbol File in last 30 days : Looking into %s\n",
    //	"/spare/local/VolumeBasedSymbol/VOSymbol_YYYYMMDD");
    // exit ( 0 );
  }

  std::ifstream volume_symbol_file_;
  volume_symbol_file_.open(_volume_symbol_filename_.c_str(), std::ifstream::in);
  if (volume_symbol_file_.is_open()) {
    const int kVolSymbolLen = 1024;
    char readline_buffer_[kVolSymbolLen];
    bzero(readline_buffer_, kVolSymbolLen);

    while (volume_symbol_file_.good()) {
      bzero(readline_buffer_, kVolSymbolLen);
      volume_symbol_file_.getline(readline_buffer_, kVolSymbolLen);
      PerishableStringTokenizer st_(readline_buffer_, kVolSymbolLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      // expect lines of the form :
      // SHORTCODE EXCHSYMBOL VOLUME
      if (tokens_.size() == 3) {
        ORpair _this_volsymbol_pair_;
        // Load for Brazilian products
        // BR_DOL_0 is actually DOL_0 according to volume file
        if (std::string(tokens_[0]).compare(0, 3, "DOL") == 0 || std::string(tokens_[0]).compare(0, 3, "WIN") == 0 ||
            std::string(tokens_[0]).compare(0, 3, "IND") == 0 || std::string(tokens_[0]).compare(0, 3, "WDO") == 0) {
          std::string new_br_prod = "BR_" + std::string(tokens_[0]);
          _this_volsymbol_pair_.shortcode_ = new_br_prod;
        } else {
          _this_volsymbol_pair_.shortcode_ = std::string(tokens_[0]);
        }
        strncpy(_this_volsymbol_pair_.exchange_symbol_, tokens_[1], kSecNameLen);
        vol_orpair_vec_.push_back(_this_volsymbol_pair_);
      }
    }
    volume_symbol_file_.close();
  }
}

const char *ExchangeSymbolManager::fromRolloverOverrideFile(const std::string &_shortcode_) const {
  for (auto i = 0u; i < orpair_vec_.size(); i++) {
    if (orpair_vec_[i].shortcode_.compare(_shortcode_) == 0) {
      return orpair_vec_[i].exchange_symbol_;
    }
  }

  return NULL;
}

const char *ExchangeSymbolManager::fromStaticDIMappingFile(const std::string &_shortcode_) const {
  for (auto i = 0u; i < di_orpair_vec_.size(); i++) {
    if (di_orpair_vec_[i].shortcode_.compare(_shortcode_) == 0) {
      return di_orpair_vec_[i].exchange_symbol_;
    }
  }

  return NULL;
}

const char *ExchangeSymbolManager::fromStaticYFEBMMappingFile(const std::string &_shortcode_) const {
  for (auto i = 0u; i < yfebm_orpair_vec_.size(); i++) {
    if (yfebm_orpair_vec_[i].shortcode_.compare(_shortcode_) == 0) {
      return yfebm_orpair_vec_[i].exchange_symbol_;
    }
  }

  return NULL;
}

const char *ExchangeSymbolManager::fromVolumeSymbolFile(const std::string &_shortcode_) const {
  for (auto i = 0u; i < vol_orpair_vec_.size(); i++) {
    if (vol_orpair_vec_[i].shortcode_.compare(_shortcode_) == 0) {
      return vol_orpair_vec_[i].exchange_symbol_;
    }
  }

  return NULL;
}

char *ExchangeSymbolManager::_GetSpreadNameForProdLIFFE(const std::string &_symbol1_, const std::string &_symbol2_) {
  char *ret_val = NULL;
  for (uint32_t i = 0; i < liffe_spread_vec_.size(); i++) {
    if (_symbol1_.compare(liffe_spread_vec_[i].symbol1_) == 0 && _symbol2_.compare(liffe_spread_vec_[i].symbol2_) == 0)
      return liffe_spread_vec_[i].sp_symbol_;
  }
  return ret_val;
}

char *ExchangeSymbolManager::_GetFlyNameForProdLIFFE(const std::string &_symbol1_, const std::string &_symbol2_,
                                                     const std::string &_symbol3_) {
  char *ret_val = NULL;
  for (uint32_t i = 0; i < liffe_fly_vec_.size(); i++) {
    if (_symbol1_.compare(liffe_fly_vec_[i].symbol1_) == 0 && _symbol2_.compare(liffe_fly_vec_[i].symbol2_) == 0 &&
        _symbol3_.compare(liffe_fly_vec_[i].symbol3_) == 0)
      return liffe_fly_vec_[i].fly_symbol_;
  }
  return ret_val;
}

/** @brief to see what symbol the exchange is using for what we depict by this shortcode
 * see if symbol already computed in the map
 * else compute it and insert into map
 */

const bool ExchangeSymbolManager::_CheckIfContractSpecExists(const std::string &_shortcode_) {
  std::string _this_shortcode_ = _shortcode_;
  return SecurityDefinitions::CheckIfContractSpecExists(_this_shortcode_, YYYYMMDD_);
}

bool ExchangeSymbolManager::IsCombinedShortcode(const std::string &_this_shortcode_) {
  return (
      // ICE LIFFE
      std::string::npos != _this_shortcode_.find("C_LFI") || std::string::npos != _this_shortcode_.find("C_LFL") ||
      std::string::npos != _this_shortcode_.find("C_LFI_SP") ||
      std::string::npos != _this_shortcode_.find("C_LFL_SP") || std::string::npos != _this_shortcode_.find("C_YFEBM") ||

      // OSE
      std::string::npos != _this_shortcode_.find("C_NK") || std::string::npos != _this_shortcode_.find("C_JGBL") ||
      std::string::npos != _this_shortcode_.find("C_TOPIX") ||

      // BMF
      std::string::npos != _this_shortcode_.find("C_BOVESPA") || std::string::npos != _this_shortcode_.find("C_DOL") ||
      std::string::npos != _this_shortcode_.find("C_IND") || std::string::npos != _this_shortcode_.find("C_DI") ||

      // TMX
      std::string::npos != _this_shortcode_.find("C_BAX") || std::string::npos != _this_shortcode_.find("C_BAX_SP") ||

      // CFE
      std::string::npos != _this_shortcode_.find("C_VX") || std::string::npos != _this_shortcode_.find("C_VX_SP") ||

      // CME
      std::string::npos != _this_shortcode_.find("C_GE") || std::string::npos != _this_shortcode_.find("C_GE_SP") ||
      std::string::npos != _this_shortcode_.find("C_ZT") || std::string::npos != _this_shortcode_.find("C_ZF") ||
      std::string::npos != _this_shortcode_.find("C_ZN") || std::string::npos != _this_shortcode_.find("C_ZB") ||
      std::string::npos != _this_shortcode_.find("C_UB") || std::string::npos != _this_shortcode_.find("C_TN") ||
      std::string::npos != _this_shortcode_.find("C_ES") || std::string::npos != _this_shortcode_.find("C_NQ") ||
      std::string::npos != _this_shortcode_.find("C_YM") || std::string::npos != _this_shortcode_.find("C_6A") ||
      std::string::npos != _this_shortcode_.find("C_6B") || std::string::npos != _this_shortcode_.find("C_6C") ||
      std::string::npos != _this_shortcode_.find("C_6E") || std::string::npos != _this_shortcode_.find("C_6J") ||
      std::string::npos != _this_shortcode_.find("C_6R") || std::string::npos != _this_shortcode_.find("C_6Z") ||
      std::string::npos != _this_shortcode_.find("C_6M") || std::string::npos != _this_shortcode_.find("C_6N") ||
      std::string::npos != _this_shortcode_.find("C_CL") || std::string::npos != _this_shortcode_.find("C_BZ") ||
      std::string::npos != _this_shortcode_.find("C_RB") || std::string::npos != _this_shortcode_.find("C_NG") ||
      std::string::npos != _this_shortcode_.find("C_HO") || std::string::npos != _this_shortcode_.find("C_QM") ||
      std::string::npos != _this_shortcode_.find("C_GC") || std::string::npos != _this_shortcode_.find("C_SI") ||
      std::string::npos != _this_shortcode_.find("C_HG") || std::string::npos != _this_shortcode_.find("C_ZC") ||
      std::string::npos != _this_shortcode_.find("C_ZW") || std::string::npos != _this_shortcode_.find("C_KE") ||
      std::string::npos != _this_shortcode_.find("C_ZS") ||

      // EUREX
      std::string::npos != _this_shortcode_.find("C_FVS") || std::string::npos != _this_shortcode_.find("C_FVS_SP") ||
      std::string::npos != _this_shortcode_.find("C_FGBS") || std::string::npos != _this_shortcode_.find("C_FGBM") ||
      std::string::npos != _this_shortcode_.find("C_FGBL") || std::string::npos != _this_shortcode_.find("C_FGBX") ||
      std::string::npos != _this_shortcode_.find("C_FOAT") || std::string::npos != _this_shortcode_.find("C_FBTP") ||
      std::string::npos != _this_shortcode_.find("C_FBTS") || std::string::npos != _this_shortcode_.find("C_FESX") ||
      std::string::npos != _this_shortcode_.find("C_FDAX") || std::string::npos != _this_shortcode_.find("C_FDXM") ||
      std::string::npos != _this_shortcode_.find("C_FXXP") || std::string::npos != _this_shortcode_.find("C_FEU3") ||
      // ASX
      std::string::npos != _this_shortcode_.find("C_IR") || std::string::npos != _this_shortcode_.find("C_SGX_NK") ||
      std::string::npos != _this_shortcode_.find("C_SGX_CN"));
}
void ExchangeSymbolManager::_AddExchSymbol(const std::string& _shortcode_, const std::string& _exchange_symbol_){
  _addlocalstore(_shortcode_,_exchange_symbol_);
}
const char *ExchangeSymbolManager::_GetExchSymbol(const std::string &_shortcode_) {
  std::string _this_shortcode_ = _shortcode_;

  if (IsCombinedShortcode(_this_shortcode_)) {  // Combined Security

    return _localstore(_this_shortcode_, _shortcode_.c_str());
  }

  const char *_fls_str_ = _findlocalstore(_this_shortcode_);
  if (_fls_str_ != NULL) {
    return _fls_str_;
  }

  const char *_or_str_ = fromRolloverOverrideFile(_this_shortcode_);
  if (_or_str_ != NULL) {
    return _localstore(_this_shortcode_, _or_str_);
  }

  const ExchSource_t _this_exch_source_ = SecurityDefinitions::GetContractExchSource(_this_shortcode_, YYYYMMDD_);

  switch (_this_exch_source_) {
    case kExchSourceCME: {
      if ((_this_shortcode_.substr(0, 5) == "SP_GE") || (_this_shortcode_.substr(0, 5) == "SP_CL") ||
          (_this_shortcode_.substr(0, 5) == "SP_RB") || (_this_shortcode_.substr(0, 5) == "SP_NG") ||
          (_this_shortcode_.substr(0, 5) == "SP_HO")) {  // spreads would have two underlying symbols

        // this can be of type SP_GE1_GE2 or SP_GE12_GE13 , so handle 12 and 13 properly
        size_t first_leg_digits_ = _this_shortcode_.rfind('_') - 5;
        size_t second_leg_digits_ = _this_shortcode_.size() - (8 + first_leg_digits_);
        std::string spread_shortcode_1_ =
            _this_shortcode_.substr(3, 2) + "_" + _this_shortcode_.substr(5, first_leg_digits_);
        std::string spread_shortcode_2_ =
            _this_shortcode_.substr(3, 2) + "_" + _this_shortcode_.substr(8 + first_leg_digits_, second_leg_digits_);
        std::string symbol_1_ = _GetExchSymbolCME(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolCME(spread_shortcode_2_);
        // spread GEM1-GEM2 , outright GEM1 and GEM2
        std::string spread_exch_sym_ = symbol_1_ + "-" + symbol_2_;
        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
      }
      return (_GetExchSymbolCME(_this_shortcode_));
    } break;
    case kExchSourceEUREX: {
      if (_this_shortcode_.substr(0, 3).compare("SP_") == 0) {  // spread contract
        // currently set for fvs only
        std::string first_leg = _this_shortcode_.substr(3, 3) + "_" + _this_shortcode_.substr(6, 1);
        std::string second_leg = _this_shortcode_.substr(8, 3) + "_" + _this_shortcode_.substr(11, 1);
        std::string symbol_first_leg = _GetExchSymbolEUREX(first_leg, true);
        std::string symbol_second_leg = _GetExchSymbolEUREX(second_leg, true);
        std::string exchange_symbol = symbol_first_leg + symbol_second_leg.substr(3, symbol_second_leg.size());
        return (_localstore(_this_shortcode_, exchange_symbol.c_str()));
      } else {
        return (_GetExchSymbolEUREX(_this_shortcode_, false));
      }
    } break;
    case kExchSourceBMF: {
      std::string shortcode_basename_ = _this_shortcode_.substr(0, 3);
      if (shortcode_basename_ == "DR1" || shortcode_basename_ == "IR1") {
        std::string base_shortcode_ = "";

        if (shortcode_basename_ == "DR1") {
          base_shortcode_ = "BR_DOL";
        } else if (shortcode_basename_ == "IR1") {
          base_shortcode_ = "BR_IND";
        }

        std::string spread_shortcode_1_ = base_shortcode_ + "_0";
        std::string spread_shortcode_2_ = base_shortcode_ + "_1";

        std::string symbol_1_ = _GetExchSymbolBMF(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolBMF(spread_shortcode_2_);

        if (IsLastBMFTradingDateOfMonth(YYYYMMDD_) && shortcode_basename_ == "DR1") {
          spread_shortcode_1_ = "BR_WIN_0";
          spread_shortcode_2_ = "BR_DOL_0";

          symbol_1_ = _GetExchSymbolBMF(spread_shortcode_1_);
          symbol_2_ = _GetExchSymbolBMF(spread_shortcode_2_);
        }

        std::string spread_exch_sym_ = shortcode_basename_ + symbol_1_.substr(3, 3) + symbol_2_.substr(3, 3);

        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
      }

      return (_GetExchSymbolBMF(_this_shortcode_));
    } break;
    case kExchSourceBMFEQ:
    case kExchSourceMICEX:
    case kExchSourceMICEX_EQ:
    case kExchSourceMICEX_CR:
    case kExchSourceNASDAQ:
    case kExchSourceHYB: {
      if (_this_shortcode_ == "AMBV4" || _this_shortcode_ == "ABEV3") {
        if (YYYYMMDD_ < 20140301) {
          return _localstore(_this_shortcode_, "AMBV4");
        } else {
          return _localstore(_this_shortcode_, "ABEV3");
        }
      }

      if (_this_shortcode_ == "ALLL3" || _this_shortcode_ == "RUMO3") {
        if (YYYYMMDD_ < 20150401) {
          return _localstore(_this_shortcode_, "ALLL3");
        } else {
          return _localstore(_this_shortcode_, "RUMO3");
        }
      }

      return _localstore(_this_shortcode_, _this_shortcode_.c_str());
      break;
    }
    case kExchSourceBATSCHI: {
      return (_GetExchSymbolBATSCHI(_this_shortcode_));
      break;
    }
    case kExchSourceTMX: {
      if (_this_shortcode_.substr(0, 3) == "SP_") {  // spreads would have two underlying symbols

        std::string spread_shortcode_1_ = _this_shortcode_.substr(3, 3) + "_" + _this_shortcode_.substr(6, 1);
        std::string spread_shortcode_2_ = _this_shortcode_.substr(8, 3) + "_" + _this_shortcode_.substr(11, 1);

        std::string symbol_1_ = _GetExchSymbolTMX(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolTMX(spread_shortcode_2_);

        // spreads uses BAXM12 instead of BAXM2(fut.)
        std::string spread_exch_sym_ = symbol_1_.substr(0, 4) + "1" + symbol_1_.substr(4, 1) + symbol_2_.substr(0, 4) +
                                       "1" + symbol_2_.substr(4, 1);

        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());

      } else if (_this_shortcode_.substr(0, 4) == "FLY_") {
        std::string fly_shortcode_1_ = _this_shortcode_.substr(4, 3) + "_" + _this_shortcode_.substr(7, 1);
        std::string fly_shortcode_2_ = _this_shortcode_.substr(9, 3) + "_" + _this_shortcode_.substr(12, 1);
        std::string fly_shortcode_3_ = _this_shortcode_.substr(14, 3) + "_" + _this_shortcode_.substr(17, 1);
        std::string symbol_1_ = _GetExchSymbolTMX(fly_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolTMX(fly_shortcode_2_);
        std::string symbol_3_ = _GetExchSymbolTMX(fly_shortcode_3_);
        std::string fly_exch_sym_ = symbol_1_.substr(0, 3) + "+" + symbol_1_.substr(3, 2) + "-2" +
                                    symbol_2_.substr(3, 2) + "+" + symbol_3_.substr(3, 2);
        return _localstore(_this_shortcode_, fly_exch_sym_.c_str());
      }

      return (_GetExchSymbolTMX(_this_shortcode_));

    } break;

    case kExchSourceEBS: {
      return _GetExchSymbolEBS(_this_shortcode_);
      break;
    }

    case kExchSourceICE: {
      return ConvertToICESymbol(_this_shortcode_);
    } break;
    case kExchSourceLIFFE: {
      if (YYYYMMDD_ >= 20141020 &&
          (_this_shortcode_.find("LFR") != std::string::npos || _this_shortcode_.find("LFL") != std::string::npos)) {
        return ConvertToICESymbol(_this_shortcode_);
      }

      if (YYYYMMDD_ >= 20141103 && (_this_shortcode_.find("LFI") != std::string::npos)) {
        return ConvertToICESymbol(_this_shortcode_);
      }

      if (YYYYMMDD_ >= 20141117 && (_this_shortcode_.find("LFZ") != std::string::npos)) {
        return ConvertToICESymbol(_this_shortcode_);
      }

      if (_this_shortcode_.substr(0, 6) == "SP_LFI") {  // spreads would have two underlying symbols of form
                                                        // SP_LFIa_LFIb
        std::size_t t_occurence_ = _this_shortcode_.find("_", 3);
        std::string spread_shortcode_1_ = "LFI_" + _this_shortcode_.substr(6, t_occurence_ - 6);
        std::string spread_shortcode_2_ =
            "LFI_" + _this_shortcode_.substr(t_occurence_ + 4, _this_shortcode_.size() - t_occurence_ - 4);

        std::string symbol_1_ = _GetExchSymbolLIFFE(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolLIFFE(spread_shortcode_2_);

        // search in the map for exchSymbol to spread name
        return _localstore(_this_shortcode_, _GetSpreadNameForProdLIFFE(symbol_1_, symbol_2_));
      }
      if (_this_shortcode_.substr(0, 6) == "SP_LFL") {  // spreads would have two underlying symbols of form
                                                        // SP_LFIa_LFIb
        std::size_t t_occurence_ = _this_shortcode_.find("_", 3);
        std::string spread_shortcode_1_ = "LFL_" + _this_shortcode_.substr(6, t_occurence_ - 6);
        std::string spread_shortcode_2_ =
            "LFL_" + _this_shortcode_.substr(t_occurence_ + 4, _this_shortcode_.size() - t_occurence_ - 4);

        std::string symbol_1_ = _GetExchSymbolLIFFE(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolLIFFE(spread_shortcode_2_);

        // search in the map for exchSymbol to spread name
        return _localstore(_this_shortcode_, _GetSpreadNameForProdLIFFE(symbol_1_, symbol_2_));
      } else if (_this_shortcode_.substr(0, 6) ==
                 "B_LFI_") {  // flies would have 3 underlying symbols of form B_LFI_a_b_c

        std::size_t t_occurence_start_ = 6;
        std::size_t t_occurence_end_ = _this_shortcode_.find("_", 6);
        std::string spread_shortcode_1_ =
            "LFI_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

        t_occurence_end_++;
        t_occurence_start_ = t_occurence_end_;
        t_occurence_end_ = _this_shortcode_.find("_", t_occurence_end_);
        std::string spread_shortcode_2_ =
            "LFI_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

        t_occurence_end_++;
        t_occurence_start_ = t_occurence_end_;
        t_occurence_end_ = _this_shortcode_.find("_", t_occurence_end_);
        std::string spread_shortcode_3_ =
            "LFI_" + _this_shortcode_.substr(t_occurence_start_, t_occurence_end_ - t_occurence_start_);

        std::string symbol_1_ = _GetExchSymbolLIFFE(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolLIFFE(spread_shortcode_2_);
        std::string symbol_3_ = _GetExchSymbolLIFFE(spread_shortcode_3_);

        // search in the map for exchSymbol to spread name
        return _localstore(_this_shortcode_, _GetFlyNameForProdLIFFE(symbol_1_, symbol_2_, symbol_3_));
      }
      return (_GetExchSymbolLIFFE(_this_shortcode_));

    } break;

    case kExchSourceRTS: {
      return (_GetExchSymbolRTS(_this_shortcode_));
    } break;
    case kExchSourceJPY: {
      return (_GetExchSymbolOSE(_this_shortcode_));
    } break;
    case kExchSourceHONGKONG: {
      return (_GetExchSymbolHKEX(_this_shortcode_));
    }
    case kExchSourceTSE: {
      return (_GetExchSymbolTSE(_this_shortcode_));
    } break;
    case kExchSourceESPEED: {
      return (_GetExchSymbolESPEED(_this_shortcode_));
    } break;
    case kExchSourceCFE: {
      // support for shortcode which are about to rollover
      // naming the rolling over VX_0 as VXE_0 and SP_VX0_VX1 as SP_VXE0_VXE1
      if (_this_shortcode_.find("VXE") != std::string::npos) {
        if (_this_shortcode_.substr(0, 3) == "SP_") {
          std::string spread_shortcode_1_ = _this_shortcode_.substr(3, 2) + "_" + _this_shortcode_.substr(6, 1);
          std::string spread_shortcode_2_ = _this_shortcode_.substr(8, 2) + "_" + _this_shortcode_.substr(11, 1);

          std::string symbol_1_ = _GetExchSymbolCFE(spread_shortcode_1_, true, true);
          std::string symbol_2_ = _GetExchSymbolCFE(spread_shortcode_2_, true, true);

          std::string spread_exch_sym_ = symbol_1_ + "_" + symbol_2_;
          return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
        }

        return (_GetExchSymbolCFE(_this_shortcode_, false, true));
      }

      if (_this_shortcode_.substr(0, 3) == "SP_") {  // spreads would have two underlying symbols

        std::string spread_shortcode_1_ = _this_shortcode_.substr(3, 2) + "_" + _this_shortcode_.substr(5, 1);
        std::string spread_shortcode_2_ = _this_shortcode_.substr(7, 2) + "_" + _this_shortcode_.substr(9, 1);

        std::string symbol_1_ = _GetExchSymbolCFE(spread_shortcode_1_, true);
        std::string symbol_2_ = _GetExchSymbolCFE(spread_shortcode_2_, true);

        std::string spread_exch_sym_ = symbol_1_ + "_" + symbol_2_;

        if (_this_shortcode_.length() > 11) {  // butterfly

          std::string spread_shortcode_3_ = _this_shortcode_.substr(11, 2) + "_" + _this_shortcode_.substr(13, 1);
          std::string symbol_3_ = _GetExchSymbolCFE(spread_shortcode_3_, true);

          spread_exch_sym_ += std::string("_" + symbol_3_);
        }

        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
      }

      return (_GetExchSymbolCFE(_this_shortcode_));
    }
    case kExchSourceASX: {
      if (_this_shortcode_.substr(0, 3) == "SP_") {  // spreads would have two underlying symbols

        std::string spread_shortcode_1_ = _this_shortcode_.substr(3, 2) + "_" + _this_shortcode_.substr(5, 1);
        std::string spread_shortcode_2_ = _this_shortcode_.substr(7, 2) + "_" + _this_shortcode_.substr(9, 1);

        std::string symbol_1_ = _GetExchSymbolASX(spread_shortcode_1_, true);
        std::string symbol_2_ = _GetExchSymbolASX(spread_shortcode_2_, true);

        std::string spread_exch_sym_ = symbol_1_ + "-" + symbol_2_;

        if (_this_shortcode_.length() > 11) {  // butterfly

          std::string spread_shortcode_3_ = _this_shortcode_.substr(11, 2) + "_" + _this_shortcode_.substr(13, 1);
          std::string symbol_3_ = _GetExchSymbolCFE(spread_shortcode_3_, true);

          spread_exch_sym_ += std::string("_" + symbol_3_);
        }

        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
      }
      return (_GetExchSymbolASX(_this_shortcode_));
    } break;
    case kExchSourceNSE: {
      // note - current system works on a single day assumption, so this is logically consistent
      return (_localstore(_this_shortcode_, NSESecurityDefinitions::GetExchSymbolNSE(_this_shortcode_).c_str()));
    }
    case kExchSourceBSE: {
      return (_localstore(
          _this_shortcode_,
          BSESecurityDefinitions::GetUniqueInstance(YYYYMMDD_).GetExchSymbolBSE(_this_shortcode_).c_str()));
    }
    case kExchSourceCBOE: {
      // note - current system works on a single day assumption, so this is logically consistent
      return (_localstore(_this_shortcode_, CBOESecurityDefinitions::GetExchSymbolCBOE(_this_shortcode_).c_str()));
    }
    case kExchSourceSGX: {
      if (_this_shortcode_.substr(0, 3) == "SP_") {
        std::string exchange_prefix_ = _this_shortcode_.substr(3, 3);
        std::string spread_shortcode_1_ =
            exchange_prefix_ + "_" + _this_shortcode_.substr(7, 2) + "_" + _this_shortcode_.substr(9, 1);
        std::string spread_shortcode_2_ =
            exchange_prefix_ + "_" + _this_shortcode_.substr(11, 2) + "_" + _this_shortcode_.substr(13, 1);

        std::string symbol_1_ = _GetExchSymbolSGX(spread_shortcode_1_);
        std::string symbol_2_ = _GetExchSymbolSGX(spread_shortcode_2_);

        std::string spread_exch_sym_ = symbol_1_ + "-" + symbol_2_;

        return _localstore(_this_shortcode_, spread_exch_sym_.c_str());
      }

      return (_GetExchSymbolSGX(_this_shortcode_));
    }
    case kExchSourceKRX: {
      return (_GetExchSymbolKRX(_this_shortcode_));
    }
    default:
      break;
  }

  return NULL;
}

inline void break_CME_shortcode_into_base_number(const std::string &_shortcode_, std::string &_pure_basename_,
                                                 std::string &_expiry_number_str_) {
  std::string key("_");
  size_t found = _shortcode_.rfind(key);
  if (found != std::string::npos) {
    _pure_basename_ = _shortcode_.substr(0, found);
    _expiry_number_str_ = _shortcode_.substr(found + key.length());
  } else {
    _pure_basename_ = _shortcode_;
    _expiry_number_str_ = "0";
  }
}

const char *ExchangeSymbolManager::_GetExchSymbolCME(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = "";
  std::string _expiry_number_str_ = "";
  break_CME_shortcode_into_base_number(_shortcode_, _pure_basename_, _expiry_number_str_);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());
  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsCMEMonth(_pure_basename_, current_month_)) {  // check if we are before rollover date
      int next_cme_month_ = current_month_;
      int next_cme_year_ = current_year_;
      int next_last_trading_date_ = GetCMELastTradingDateYYYYMM(_pure_basename_, next_cme_month_, next_cme_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date
        SetToNextCMEMonth(_pure_basename_, next_cme_month_, next_cme_year_);
        next_last_trading_date_ = GetCMELastTradingDateYYYYMM(_pure_basename_, next_cme_month_, next_cme_year_);
      }

      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_cme_month_ = current_month_;
      int next_cme_year_ = current_year_;
      SetToNextCMEMonth(_pure_basename_, next_cme_month_, next_cme_year_);

      int next_last_trading_date_ = GetCMELastTradingDateYYYYMM(_pure_basename_, next_cme_month_, next_cme_year_);

      // Last day of trading: First notice date: E.g 30thSept, 31stMay, 29th Feb etc
      // So if today is 30th Sept ZF_0 should point to first contract in next year
      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date
        SetToNextCMEMonth(_pure_basename_, next_cme_month_, next_cme_year_);
        next_last_trading_date_ = GetCMELastTradingDateYYYYMM(_pure_basename_, next_cme_month_, next_cme_year_);
      } else {
        // since it is guaranteed to be > current_min_last_trading_date
        // we can set current_min_last_trading_date to next_last_trading_date_
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  // fprintf ( stdout, " _pure_basename = %s , current_min_trading_date = %d\n\n",     _pure_basename_.c_str(),
  // current_min_last_trading_date );

  std::string full_contract_symbol_ = GetCMESymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolKRX(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = "";
  std::string _expiry_number_str_ = "";
  break_CME_shortcode_into_base_number(_shortcode_, _pure_basename_, _expiry_number_str_);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsKRXMonth(_pure_basename_, current_month_)) {
      int next_krx_month_ = current_month_;
      int next_krx_year_ = current_year_;
      int next_last_trading_date_ = GetKRXLastTradingDateYYYYMM(_pure_basename_, next_krx_month_, next_krx_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextKRXMonth(_pure_basename_, next_krx_month_, next_krx_year_);
        next_last_trading_date_ = GetKRXLastTradingDateYYYYMM(_pure_basename_, next_krx_month_, next_krx_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_krx_month_ = current_month_;
      int next_krx_year_ = current_year_;
      SetToNextKRXMonth(_pure_basename_, next_krx_month_, next_krx_year_);
      int next_last_trading_date_ = GetKRXLastTradingDateYYYYMM(_pure_basename_, next_krx_month_, next_krx_year_);

      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetKRXSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolSGX(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = "";
  std::string _expiry_number_str_ = "";
  break_CME_shortcode_into_base_number(_shortcode_, _pure_basename_, _expiry_number_str_);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  //  std::cout << _pure_basename_ << " : " << _expiry_number_str_ << "\n";

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  //  std::cout << "Date: " << current_min_last_trading_date << " : " << current_month_ << " : " << current_year_ <<
  //  "\n";

  do {
    if (IsSGXMonth(_pure_basename_, current_month_)) {  // check if we are before rollover date
      int next_sgx_month_ = current_month_;
      int next_sgx_year_ = current_year_;
      int next_last_trading_date_ = GetSGXLastTradingDateYYYYMM(_pure_basename_, next_sgx_month_, next_sgx_year_);

      //      std::cout << "LastTradingDay: " << next_last_trading_date_ << "\n";

      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date

        SetToNextSGXMonth(_pure_basename_, next_sgx_month_, next_sgx_year_);
        next_last_trading_date_ = GetSGXLastTradingDateYYYYMM(_pure_basename_, next_sgx_month_, next_sgx_year_);
      }

      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_sgx_month_ = current_month_;
      int next_sgx_year_ = current_year_;
      SetToNextSGXMonth(_pure_basename_, next_sgx_month_, next_sgx_year_);

      int next_last_trading_date_ = GetSGXLastTradingDateYYYYMM(_pure_basename_, next_sgx_month_, next_sgx_year_);

      // Last day of trading: First notice date: E.g 30thSept, 31stMay, 29th Feb etc
      // So if today is 30th Sept ZF_0 should point to first contract in next year
      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date
        SetToNextSGXMonth(_pure_basename_, next_sgx_month_, next_sgx_year_);
        next_last_trading_date_ = GetSGXLastTradingDateYYYYMM(_pure_basename_, next_sgx_month_, next_sgx_year_);
      } else {
        // since it is guaranteed to be > current_min_last_trading_date
        // we can set current_min_last_trading_date to next_last_trading_date_
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  // fprintf ( stdout, " _pure_basename = %s , current_min_trading_date = %d\n\n",     _pure_basename_.c_str(),
  // current_min_last_trading_date );

  std::string full_contract_symbol_ = GetSGXSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolTSE(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = "";
  std::string _expiry_number_str_ = "";

  // Utlilizing same functions as CME
  break_CME_shortcode_into_base_number(_shortcode_, _pure_basename_, _expiry_number_str_);

  if (_pure_basename_ == "JGB") {
    _pure_basename_ = "GFJGB";
  } else if (_pure_basename_ == "TPX") {
    _pure_basename_ = "FFTPX";
  } else if (_pure_basename_ == "MTP") {
    _pure_basename_ = "FFMTP";
  } else if (_pure_basename_ == "MJG") {
    _pure_basename_ = "GFMJG";
  }

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsTSEMonth(_pure_basename_, current_month_)) {  // check if we are before rollover date
      int next_tse_month_ = current_month_;
      int next_tse_year_ = current_year_;
      int next_last_trading_date_ = GetTSELastTradingDateYYYYMM(_pure_basename_, next_tse_month_, next_tse_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date

        SetToNextTSEMonth(_pure_basename_, next_tse_month_, next_tse_year_);
        next_last_trading_date_ = GetTSELastTradingDateYYYYMM(_pure_basename_, next_tse_month_, next_tse_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_tse_month_ = current_month_;
      int next_tse_year_ = current_year_;
      SetToNextTSEMonth(_pure_basename_, next_tse_month_, next_tse_year_);

      int next_last_trading_date_ = GetTSELastTradingDateYYYYMM(_pure_basename_, next_tse_month_, next_tse_year_);

      // Last day of trading: First notice date: E.g 30thSept, 31stMay, 29th Feb etc
      // So if today is 30th Sept ZF_0 should point to first contract in next year
      if (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date
        SetToNextTSEMonth(_pure_basename_, next_tse_month_, next_tse_year_);
        next_last_trading_date_ = GetTSELastTradingDateYYYYMM(_pure_basename_, next_tse_month_, next_tse_year_);
      } else {
        // since it is guaranteed to be > current_min_last_trading_date
        // we can set current_min_last_trading_date to next_last_trading_date_
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetTSESymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolEBS(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  return _shortcode_.c_str();
}

const char *ExchangeSymbolManager::_GetExchSymbolBATSCHI(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string this_shortcode = _shortcode_;

  if (_shortcode_ == "ULp" || _shortcode_ == "ULa") {
    if (YYYYMMDD_ <= 20130227)
      this_shortcode = "ULp";
    else
      this_shortcode = "ULa";
  } else if (_shortcode_ == "FTEp" || _shortcode_ == "ORAp") {
    if (YYYYMMDD_ <= 20130628)
      this_shortcode = "FTEp";
    else
      this_shortcode = "ORAp";
  }

  return _localstore(_shortcode_, this_shortcode.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolESPEED(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  return _localstore(_shortcode_, _shortcode_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolCFE(const std::string &_shortcode_, bool _is_spread_,
                                                     bool _is_expiry_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  if (0 == _pure_basename_.compare("VXE")) {
    _pure_basename_ = std::string("VX");
  }
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1, 1);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  if (_pure_basename_.compare("VXWEEK"))  // shortcode which expire monthly
  {
    do {
      if (IsCFEMonth(_pure_basename_, current_month_)) {
        int next_cfe_month_ = current_month_;
        int next_cfe_year_ = current_year_;
        int next_last_trading_date_ = GetCFELastTradingDateYYYYMM(_pure_basename_, next_cfe_month_, next_cfe_year_);
        if (next_last_trading_date_ < current_min_last_trading_date) {
          SetToNextCFEMonth(_pure_basename_, next_cfe_month_, next_cfe_year_);
          next_last_trading_date_ = GetCFELastTradingDateYYYYMM(_pure_basename_, next_cfe_month_, next_cfe_year_);
        }
        current_min_last_trading_date = next_last_trading_date_;
        current_month_ = ((current_min_last_trading_date / 100) % 100);
        current_year_ = (current_min_last_trading_date / 10000);
      } else {
        int next_cfe_month_ = current_month_;
        int next_cfe_year_ = current_year_;
        SetToNextCFEMonth(_pure_basename_, next_cfe_month_, next_cfe_year_);
        int next_last_trading_date_ = GetCFELastTradingDateYYYYMM(_pure_basename_, next_cfe_month_, next_cfe_year_);

        // since it is guaranteed to be > current_min_last_trading_date
        // we can set current_min_last_trading_date to next_last_trading_date_
        current_min_last_trading_date = next_last_trading_date_;
        current_month_ = ((current_min_last_trading_date / 100) % 100);
        current_year_ = (current_min_last_trading_date / 10000);
      }
      if (_expiry_number_ > 0) {
        current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
        current_month_ = ((current_min_last_trading_date / 100) % 100);
        current_year_ = (current_min_last_trading_date / 10000);
      }
      _expiry_number_--;
    } while (_expiry_number_ >= 0);

    std::string full_contract_symbol_ =
        GetCFESymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date, _is_spread_, _is_expiry_);

    if (_is_spread_) {
      return strdup(full_contract_symbol_.c_str());
    } else {
      return _localstore(_shortcode_, full_contract_symbol_.c_str());
    }
  }

  else  // shortcodes which expire weekly
  {
    std::string date_string = std::to_string(YYYYMMDD_);
    boost::gregorian::date current_date = boost::gregorian::from_undelimited_string(date_string);

    boost::gregorian::date output_date;

    if (current_date.day_of_week() != boost::gregorian::Wednesday)
      current_date =
          boost::date_time::next_weekday(current_date, boost::gregorian::greg_weekday(boost::date_time::Wednesday));

    int current_week, current_year;

    do {
      current_week = current_date.week_number();
      current_year = boost::lexical_cast<int>(current_date.year());

      output_date = current_date;
      current_date = current_date + one_day_date_duration;
      current_date =
          boost::date_time::next_weekday(current_date, boost::gregorian::greg_weekday(boost::date_time::Wednesday));

      // if a week is not a cfe week, then next week has to be a cfe week
      if (!IsCFEWeek(_pure_basename_, current_week, current_year)) continue;  // skip this week

      if (_expiry_number_ == 0) {
        while (IsCFEHoliday(YYYYMMDD_from_date(output_date))) output_date = output_date - one_day_date_duration;
      }
      _expiry_number_--;
    } while (_expiry_number_ >= 0);

    std::stringstream symbol_stream("");
    current_week = current_date.week_number();
    current_year = boost::lexical_cast<int>(current_date.year());

    boost::gregorian::date week_check_day(current_year, boost::gregorian::Jan, 1);

    if (week_check_day.day_of_week() > boost::gregorian::greg_weekday(boost::date_time::Wednesday)) current_week--;

    int current_month = current_date.month().as_number();

    symbol_stream << "VX";

    if (current_week < 10) symbol_stream << "0";
    symbol_stream << current_week << current_year;

    if (current_month < 10) symbol_stream << "0";
    symbol_stream << current_month;

    std::string full_contract_symbol_ = symbol_stream.str();
    return _localstore(_shortcode_, full_contract_symbol_.c_str());
  }
}

const char *ExchangeSymbolManager::_GetExchSymbolASX(const std::string &_shortcode_, bool _is_spread_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }

  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1, 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  do {
    if (IsASXMonth(_pure_basename_, current_month_)) {
      int next_asx_month_ = current_month_;
      int next_asx_year_ = current_year_;
      int next_last_trading_date_ = GetASXLastTradingDateYYYYMM(_pure_basename_, next_asx_month_, next_asx_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextASXMonth(_pure_basename_, next_asx_month_, next_asx_year_);
        next_last_trading_date_ = GetASXLastTradingDateYYYYMM(_pure_basename_, next_asx_month_, next_asx_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_asx_month_ = current_month_;
      int next_asx_year_ = current_year_;
      SetToNextASXMonth(_pure_basename_, next_asx_month_, next_asx_year_);
      int next_last_trading_date_ = GetASXLastTradingDateYYYYMM(_pure_basename_, next_asx_month_, next_asx_year_);

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ =
      GetASXSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date, _is_spread_);

  if (_is_spread_) {
    return strdup(full_contract_symbol_.c_str());
  } else {
    return _localstore(_shortcode_, full_contract_symbol_.c_str());
  }
}

const char *ExchangeSymbolManager::_GetExchSymbolEUREX(const std::string &_shortcode_, bool is_spread) {
  // Input date validity with respect to product and exchange start dates.

  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }

  // Assumption - shortcodes are always in the form _?, not putting any checks
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsEUREXMonth(_pure_basename_, current_month_)) {
      int next_eurex_month_ = current_month_;
      int next_eurex_year_ = current_year_;
      int next_last_trading_date_ = GetEUREXLastTradingDateYYYYMM(_pure_basename_, next_eurex_month_, next_eurex_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextEUREXMonth(_pure_basename_, next_eurex_month_, next_eurex_year_);
        next_last_trading_date_ = GetEUREXLastTradingDateYYYYMM(_pure_basename_, next_eurex_month_, next_eurex_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_eurex_month_ = current_month_;
      int next_eurex_year_ = current_year_;
      SetToNextEUREXMonth(_pure_basename_, next_eurex_month_, next_eurex_year_);
      int next_last_trading_date_ = GetEUREXLastTradingDateYYYYMM(_pure_basename_, next_eurex_month_, next_eurex_year_);

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ =
      GetEUREXSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date, is_spread);

  // This is to ignore FVS_0 -> FVSZ16 like entries being created from spread
  if (is_spread) {
    return full_contract_symbol_.c_str();
  }
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

void ExchangeSymbolManager::LoadLIFFESpreadMap(int date_) {
  char spread_map_file_[1000] = {0};
  sprintf(spread_map_file_, "/spare/local/files/LIFFE/liffe-london-spreads-mapping-%d.txt", date_);

  if (!(FileUtils::ExistsAndReadable(std::string(
          spread_map_file_)))) {  // TODO we should probably look at a few previous dates like we do in other files.
    sprintf(spread_map_file_, "/spare/local/files/LIFFE/liffe-london-spreads-mapping-default.txt");
  }

  if (FileUtils::ExistsAndReadable(std::string(spread_map_file_))) {
    std::ifstream bfr(spread_map_file_);
    if (bfr.is_open()) {
      char line_[1000] = {0};

      char t_symbol1_[kSecNameLen];
      char t_symbol2_[kSecNameLen];
      char t_symbol3_[kSecNameLen];
      char t_symbol4_[kSecNameLen];

      strcpy(t_symbol1_, "");  // init
      strcpy(t_symbol2_, "");  // init
      strcpy(t_symbol3_, "");  // init
      strcpy(t_symbol4_, "");  // init

      while (bfr.good()) {
        std::string line_str;
        getline(bfr, line_str);
        if (line_str.empty()) {
          break;
        }

        std::strncpy(line_, line_str.c_str(), 999);
        line_[999] = '\0';

        // TODO no error checking ... might be best to use tokenizer ?
        // following code is prone to memory corruption and accessing memory not accessible.
        strcpy(t_symbol1_, strtok(line_, "~"));
        strcpy(t_symbol2_, strtok(NULL, "~"));
        strcpy(t_symbol3_, strtok(NULL, "~"));
        char *str = strtok(NULL, "~");
        if (str != NULL) {
          strcpy(t_symbol4_, str);
          FlySymbol this_fly_symbol_;
          strcpy(this_fly_symbol_.symbol1_, t_symbol2_);
          strcpy(this_fly_symbol_.symbol2_, t_symbol3_);
          strcpy(this_fly_symbol_.symbol3_, t_symbol4_);
          strcpy(this_fly_symbol_.fly_symbol_, t_symbol1_);
          liffe_fly_vec_.push_back(this_fly_symbol_);
        } else {
          //	std::cerr<"Tokenising failed\n";
          SpreadSymbol this_spread_symbol_;
          strcpy(this_spread_symbol_.symbol1_, t_symbol2_);
          strcpy(this_spread_symbol_.symbol2_, t_symbol3_);
          strcpy(this_spread_symbol_.sp_symbol_, t_symbol1_);
          liffe_spread_vec_.push_back(this_spread_symbol_);
        }
      }
      bfr.close();
    }
  }
}

const char *ExchangeSymbolManager::_GetExchSymbolRTS(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  if (_pure_basename_.compare("NT") == 0) {
    _pure_basename_ = "NK";
  }  // Novateck ticker is same as NK, so using NT_0 as shc and this is a hack to get the correct exchange symbol
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  do {
    if (IsRTSMonth(_pure_basename_, current_month_)) {
      int next_rts_month_ = current_month_;
      int next_rts_year_ = current_year_;
      int next_last_trading_date_ = GetRTSLastTradingDateYYYYMM(_pure_basename_, next_rts_month_, next_rts_year_);

      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextRTSMonth(_pure_basename_, next_rts_month_, next_rts_year_);
        next_last_trading_date_ = GetRTSLastTradingDateYYYYMM(_pure_basename_, next_rts_month_, next_rts_year_);
      }

      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_rts_month_ = current_month_;
      int next_rts_year_ = current_year_;
      SetToNextRTSMonth(_pure_basename_, next_rts_month_, next_rts_year_);
      int next_last_trading_date_ = GetRTSLastTradingDateYYYYMM(_pure_basename_, next_rts_month_, next_rts_year_);

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetRTSSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

/*    NOT REQUIRED NOW
        bool ExchangeSymbolManager::IsWheatCloseToExpiry ( std::string _pure_basename_, const int current_month_, const
   int current_year_ )
        {
        if ( IsLIFFEMonth ( _pure_basename_, current_month_ ) )
        {
        int next_liffe_month_ = current_month_;
        int next_liffe_year_ = current_year_;
        int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM ( _pure_basename_, next_liffe_month_,
   next_liffe_year_ );

        while ( next_last_trading_date_ < YYYYMMDD_ )
        {

        SetToNextLIFFEMonth ( _pure_basename_, next_liffe_month_, next_liffe_year_ );
        next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM ( _pure_basename_, next_liffe_month_, next_liffe_year_
   );
        }

        if ( ( YYYYMMDD_ <= next_last_trading_date_ ) && ( YYYYMMDD_ > ( next_last_trading_date_ - 3 ) ) )
        {
        return true ;
        }
        }
        return false ;
        }
 */

const char *ExchangeSymbolManager::_GetExchSymbolLIFFE(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }

  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  // To use static mapping for YFEBM _0 and _1 if present in static file
  if (strncmp(_pure_basename_.c_str(), LIFFE_YFEBM, strlen(LIFFE_YFEBM)) == 0) {
    const char *_or_str_ = fromStaticYFEBMMappingFile(_shortcode_);
    if (_or_str_ != NULL) {
      return _localstore(_shortcode_, _or_str_);
    }
  }

  do {
    if (IsLIFFEMonth(_pure_basename_, current_month_, current_year_)) {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      while (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
        next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      if ((_pure_basename_ == LIFFE_LFR) || (_pure_basename_ == LIFFE_XFW) || (_pure_basename_ == LIFFE_XFC) ||
          (_pure_basename_ == LIFFE_XFRC) || (_pure_basename_ == LIFFE_YFEBM)) {
        while (next_last_trading_date_ < current_min_last_trading_date) {
          SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
          next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
        }
      }

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetLIFFESymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolICE(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  do {
    // Keeping date calculations for ICE same as LIFFE
    if (IsLIFFEMonth(_pure_basename_, current_month_)) {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      while (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
        next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      if ((_pure_basename_ == LIFFE_LFR) || (_pure_basename_ == LIFFE_XFW) || (_pure_basename_ == LIFFE_XFC) ||
          (_pure_basename_ == LIFFE_XFRC) || (_pure_basename_ == LIFFE_YFEBM)) {
        while (next_last_trading_date_ < current_min_last_trading_date) {
          SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
          next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
        }
      }

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetICESymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolShortICE(const std::string &_shortcode_, bool _is_spread_) {
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);

  do {
    // Keeping date calculations for ICE same as LIFFE
    if (IsLIFFEMonth(_pure_basename_, current_month_)) {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      while (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
        next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_liffe_month_ = current_month_;
      int next_liffe_year_ = current_year_;
      SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
      int next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);

      if ((_pure_basename_ == LIFFE_LFR) || (_pure_basename_ == LIFFE_XFW) || (_pure_basename_ == LIFFE_XFC) ||
          (_pure_basename_ == LIFFE_XFRC) || (_pure_basename_ == LIFFE_YFEBM)) {
        while (next_last_trading_date_ < current_min_last_trading_date) {
          SetToNextLIFFEMonth(_pure_basename_, next_liffe_month_, next_liffe_year_);
          next_last_trading_date_ = GetLIFFELastTradingDateYYYYMM(_pure_basename_, next_liffe_month_, next_liffe_year_);
        }
      }

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ =
      GetICEShortSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);

  if (_is_spread_) {
    return strdup(full_contract_symbol_.c_str());
  } else {
    return _localstore(_shortcode_, full_contract_symbol_.c_str());
  }
}

inline void break_BMF_shortcode_into_base_number(const std::string &_shortcode_, std::string &_pure_basename_,
                                                 std::string &_expiry_number_str_) {
  std::string key("_");
  size_t found = _shortcode_.rfind(key);
  if (found != std::string::npos) {
    _pure_basename_ = _shortcode_.substr(0, found);
    _expiry_number_str_ = _shortcode_.substr(found + key.length());
  } else {
    _pure_basename_ = _shortcode_;
    _expiry_number_str_ = "0";
  }
}

const char *ExchangeSymbolManager::_GetExchSymbolBMF(const std::string &_shortcode_) {
  // equity stokcs
  if (_shortcode_.find("BR_") == std::string::npos && _shortcode_.find("DI1") == std::string::npos) {
    return _localstore(_shortcode_, _shortcode_.c_str());
  }

  std::string _pure_basename_ = "";
  std::string _expiry_number_str_ = "";
  break_BMF_shortcode_into_base_number(_shortcode_, _pure_basename_, _expiry_number_str_);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  if (strncmp(_pure_basename_.c_str(), BMF_DI, strlen(BMF_DI)) == 0 ||
      strncmp(_pure_basename_.c_str(), BMF_SP_DI, strlen(BMF_SP_DI)) == 0) {
    const char *_or_str_ = _shortcode_.c_str();
    if (_or_str_ != NULL) {
      return _localstore(_shortcode_, _or_str_);
    }
  }
  // // If not found in the volume based file then use the contract specification logic

  // this products shift volumes on last trading day of each month otherwise follows logical mapping
  if (_pure_basename_.compare(BMF_DOL) == 0 || _pure_basename_.compare(BMF_WDO) == 0) {
    if (IsLastBMFTradingDateOfMonth(YYYYMMDD_)) _expiry_number_++;  // forward series by 1
  }

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsBMFMonth(_pure_basename_, current_month_)) {  // check if we are before rollover date
      int next_bmf_month_ = current_month_;
      int next_bmf_year_ = current_year_;
      int next_last_trading_date_ = GetBMFLastTradingDateYYYYMM(_pure_basename_, next_bmf_month_, next_bmf_year_);
      while (next_last_trading_date_ < current_min_last_trading_date) {  // past rollover date
        SetToNextBMFMonth(_pure_basename_, next_bmf_month_, next_bmf_year_);
        next_last_trading_date_ = GetBMFLastTradingDateYYYYMM(_pure_basename_, next_bmf_month_, next_bmf_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_bmf_month_ = current_month_;
      int next_bmf_year_ = current_year_;
      SetToNextBMFMonth(_pure_basename_, next_bmf_month_, next_bmf_year_);
      int next_last_trading_date_ = GetBMFLastTradingDateYYYYMM(_pure_basename_, next_bmf_month_, next_bmf_year_);

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetBMFSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolTMX(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string key("_");
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.rfind(key));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());

  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  do {
    if (IsTMXMonth(current_month_)) {
      int next_tmx_month_ = current_month_;
      int next_tmx_year_ = current_year_;
      int next_last_trading_date_ = GetTMXLastTradingDateYYYYMM(_pure_basename_, next_tmx_month_, next_tmx_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextTMXMonth(next_tmx_month_, next_tmx_year_);
        next_last_trading_date_ = GetTMXLastTradingDateYYYYMM(_pure_basename_, next_tmx_month_, next_tmx_year_);
      }
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      int next_tmx_month_ = current_month_;
      int next_tmx_year_ = current_year_;
      SetToNextTMXMonth(next_tmx_month_, next_tmx_year_);
      int next_last_trading_date_ = GetTMXLastTradingDateYYYYMM(_pure_basename_, next_tmx_month_, next_tmx_year_);

      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextTMXMonth(next_tmx_month_, next_tmx_year_);
        next_last_trading_date_ = GetTMXLastTradingDateYYYYMM(_pure_basename_, next_tmx_month_, next_tmx_year_);
      } else {
      }

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    if (_expiry_number_ > 0) {
      current_min_last_trading_date = HFSAT::DateTime::CalcNextDay(3, current_min_last_trading_date);
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }
    _expiry_number_--;
  } while (_expiry_number_ >= 0);

  std::string full_contract_symbol_ = GetTMXSymbolFromLastTradingDate(_pure_basename_, current_min_last_trading_date);
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

const char *ExchangeSymbolManager::_GetExchSymbolOSE(const std::string &_shortcode_) {
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.length() - 2);

  if (YYYYMMDD_ >= USING_OSE_ITCH_FROM && _pure_basename_ == "JP400") {
    _pure_basename_ = "JN400";
  }

  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.length() - 1);
  int _expiry_number_ = atoi(_expiry_number_str_.c_str());
  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  int next_ose_month_ = current_month_;
  int next_ose_year_ = current_year_;

  // NKM volumes follow NK expiries , using same exch-symbol names for both.

  // ///NK mini - monthly contracts

  {
    if (IsOSEMonth(_pure_basename_, current_month_)) {
      int next_last_trading_date_ = GetOSELastTradingDateYYYYMM(_pure_basename_, next_ose_month_, next_ose_year_);
      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextOSEMonth(_pure_basename_, next_ose_month_, next_ose_year_);
        next_last_trading_date_ = GetOSELastTradingDateYYYYMM(_pure_basename_, next_ose_month_, next_ose_year_);
      }

      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    } else {
      SetToNextOSEMonth(_pure_basename_, next_ose_month_, next_ose_year_);
      int next_last_trading_date_ = GetOSELastTradingDateYYYYMM(_pure_basename_, next_ose_month_, next_ose_year_);

      if (next_last_trading_date_ < current_min_last_trading_date) {
        SetToNextOSEMonth(_pure_basename_, next_ose_month_, next_ose_year_);
        next_last_trading_date_ = GetOSELastTradingDateYYYYMM(_pure_basename_, next_ose_month_, next_ose_year_);
      }

      // since it is guaranteed to be > current_min_last_trading_date
      // we can set current_min_last_trading_date to next_last_trading_date_
      current_min_last_trading_date = next_last_trading_date_;
      current_month_ = ((current_min_last_trading_date / 100) % 100);
      current_year_ = (current_min_last_trading_date / 10000);
    }

    while (_expiry_number_ > 0) {
      SetToNextOSEMonth(_pure_basename_, current_month_, current_year_);
      _expiry_number_--;
    }

    next_ose_year_ = current_year_;
    next_ose_month_ = current_month_;
  }

  std::stringstream ss;
  ss << std::setfill('0');
  if (_pure_basename_.compare("NKM") == 0 || OSE_NKMF == _pure_basename_) {
    ss << "NKM" << (int)(next_ose_year_ % 100) << std::setw(2) << (int)next_ose_month_;
  } else if (_pure_basename_ == OSE_DJI || _pure_basename_ == "NK" || _pure_basename_ == "DJI" ||
             _pure_basename_ == "TOPIX" || _pure_basename_ == "TOPIXM" || _pure_basename_ == "TPX30" ||
             _pure_basename_ == "JP400" || _pure_basename_ == "JN400" || _pure_basename_ == "JGBMY" ||
             _pure_basename_ == "JGBLY" || _pure_basename_ == "JGBLMY" || _pure_basename_ == "JGBSLY" ||
             _pure_basename_ == "JGBM" || _pure_basename_ == "JGBL" || _pure_basename_ == "JGBLM" ||
             _pure_basename_ == "JGBSL") {
    ss << _pure_basename_ << (int)(next_ose_year_ % 100) << std::setw(2) << (int)next_ose_month_;
  }

  std::string full_contract_symbol_ = ss.str();
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}
const char *ExchangeSymbolManager::_GetExchSymbolHKEX(const std::string &_shortcode_) {
  //    std::string _pure_basename_ = _shortcode_.substr ( 0, _shortcode_.length()-2 );
  //    std::string _expiry_number_str_ = _shortcode_.substr ( _shortcode_.length()-1  );
  // HK Equities
  if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(_shortcode_) > YYYYMMDD_) {
    std::cerr << "Product Start Date for " << _shortcode_ << " greater than current date " << YYYYMMDD_
              << ". Cannot find Exchange Symbol. Exiting." << std::endl;
    exit(1);
  }
  if (_shortcode_.substr(0, 3) == "HK_") {
    return _localstore(_shortcode_, HKStocksSecurityDefinitions::GetExchSymbolHKStocks(_shortcode_).c_str());
  }

  std::string _pure_basename_ = _shortcode_.substr(0, _shortcode_.find("_"));
  std::string _expiry_number_str_ = _shortcode_.substr(_shortcode_.find("_") + 1);

  int _expiry_number_ = atoi(_expiry_number_str_.c_str());
  int current_min_last_trading_date = YYYYMMDD_;
  int current_month_ = ((current_min_last_trading_date / 100) % 100);
  int current_year_ = (current_min_last_trading_date / 10000);
  int current_day_ = current_min_last_trading_date % 100;
  int next_hkex_month_ = current_month_;
  int next_hkex_year_ = current_year_;
  int next_last_trading_day_ = GetHKEXLastTradingDay(_pure_basename_, next_hkex_month_, next_hkex_year_);
  if (current_day_ > next_last_trading_day_) {
    if (next_hkex_month_ == 12) {
      next_hkex_month_ = 1;
      next_hkex_year_++;
    } else {
      next_hkex_month_++;
    }
  }

  for (int i = 0; i < _expiry_number_; i++) {
    if (i < 1) {
      if (next_hkex_month_ == 12) {
        next_hkex_month_ = 1;
        next_hkex_year_++;
      } else {
        next_hkex_month_++;
      }
    } else {
      // for expiry number >1, get the next quarter month
      next_hkex_month_ = ((next_hkex_month_ / 3 + 1) * 3);
      if (next_hkex_month_ > 12) {
        next_hkex_month_ -= 12;
        next_hkex_year_++;
      }
    }
  }

  std::stringstream ss;
  ss << _pure_basename_ << CMEMonthCode[next_hkex_month_ - 1] << (next_hkex_year_ % 10);
  std::string full_contract_symbol_ = ss.str();
  return _localstore(_shortcode_, full_contract_symbol_.c_str());
}

bool ExchangeSymbolManager::IsCMEMonth(const std::string &_pure_basename_, const int _this_month_) {
  if ((_pure_basename_.compare(CME_CL) == 0) || (_pure_basename_.compare(CME_BZ) == 0) ||
      (_pure_basename_.compare(CME_NG) == 0) || (_pure_basename_.compare(CME_RB) == 0) ||
      (_pure_basename_.compare(CME_QM) == 0) || (_pure_basename_.compare(CME_HO) == 0) ||
      (_pure_basename_.compare(CME_NN) == 0))
    return true;
  if (_pure_basename_.compare(CME_HG) == 0)
    return (((_this_month_ == 11) || ((_this_month_ % 2) == 0)) && (_this_month_ != 10) && (_this_month_ != 12));
  if (_pure_basename_.compare(CME_GC) == 0) return (((_this_month_ + 1) % 2) == 0 && (_this_month_ != 9));
  if (_pure_basename_.compare(CME_SI) == 0)
    return (((_this_month_ == 11) || ((_this_month_ % 2) == 0)) && (_this_month_ != 10) && (_this_month_ != 12));
  if (_pure_basename_.compare(CME_IBV) == 0) return ((_this_month_ % 2) == 0);
  if ((_pure_basename_.compare(CME_ZW) == 0) || (_pure_basename_.compare(CME_KE) == 0) ||
      (_pure_basename_.compare(CME_XW) == 0) || (_pure_basename_.compare(CME_ZC) == 0)) {
    if ((_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 7) || (_this_month_ == 9) ||
        (_this_month_ == 12))
      return true;
    else
      return false;
  }
  if (_pure_basename_.compare(CME_ZM) == 0 || _pure_basename_.compare(CME_ZL) == 0) {
    if ((_this_month_ == 1) || (_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 7) ||
        (_this_month_ == 8) || (_this_month_ == 9) || (_this_month_ == 10) || (_this_month_ == 12))
      return true;
    else
      return false;
  }
  if (_pure_basename_.compare(CME_ZS) == 0) {
    if ((_this_month_ == 1) || (_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 7) ||
        (_this_month_ == 8) || (_this_month_ == 9) || (_this_month_ == 11))
      return true;
    else
      return false;
  }

  return ((_this_month_ % 3) == 0);
}

bool ExchangeSymbolManager::IsSGXMonth(const std::string &_pure_basename_, const int _this_month_) {
  // 2 serial front months
  if (_pure_basename_.compare(SGX_NKF) == 0) {
    //    std::cout << (_pure_basename_.compare(SGX_CNF) == 0) << " : " << (_pure_basename_.compare(SGX_INF)) << "\n";
    if ((_this_month_ % 3) != 0) {
      return true;
    } else
      return false;
  } else if ((_pure_basename_.compare(SGX_NK) == 0) || (_pure_basename_.compare(SGX_NU) == 0)) {
    // quarterly expiries in general
    if ((_this_month_ % 3) == 0) {
      return true;
    } else
      return false;
  } else if (_pure_basename_.compare(SGX_AJ) == 0 || _pure_basename_.compare(SGX_AU) == 0 ||
             _pure_basename_.compare(SGX_US) == 0 || _pure_basename_.compare(SGX_IU) == 0 ||
             _pure_basename_.compare(SGX_KU) == 0 || _pure_basename_.compare(SGX_CN) == 0 ||
             _pure_basename_.compare(SGX_IN) == 0 || _pure_basename_.compare(SGX_TW) == 0 ||
             _pure_basename_.compare(SGX_SG) == 0 || _pure_basename_.compare(SGX_CH) == 0 ||
             _pure_basename_.compare(SGX_MD) == 0 || _pure_basename_.compare(SGX_INB) == 0) {
    return true;
  }

  return false;
}

bool ExchangeSymbolManager::IsTSEMonth(const std::string &_pure_basename_, const int _this_month_) {
  // 3 calendar contracts
  if (_pure_basename_.compare(TSE_JGB) == 0 || _pure_basename_.compare(TSE_MJG) == 0) return ((_this_month_ % 3) == 0);

  // 5 calendar contracts
  if (_pure_basename_.compare(TSE_TPX) == 0 || _pure_basename_.compare(TSE_MTP) == 0) return ((_this_month_ % 3) == 0);

  return ((_this_month_ % 3) == 0);
}

void ExchangeSymbolManager::SetToNextCMEMonth(const std::string &_pure_basename_, int &_next_cme_month_,
                                              int &_next_cme_year_) {
  if (_pure_basename_.compare(CME_IBV) == 0) {
    if (_next_cme_month_ == 12) {
      _next_cme_month_ = 1;
      _next_cme_year_++;
      while (!IsCMEMonth(_pure_basename_, _next_cme_month_)) {
        _next_cme_month_++;
      }
    } else {
      do {
        _next_cme_month_++;
      } while (!IsCMEMonth(_pure_basename_, _next_cme_month_));
    }

    return;
  }

  if ((_pure_basename_.compare(CME_CL) == 0) || (_pure_basename_.compare(CME_BZ) == 0) ||
      (_pure_basename_.compare(CME_RB) == 0) || (_pure_basename_.compare(CME_NG) == 0) ||
      (_pure_basename_.compare(CME_QM) == 0) || (_pure_basename_.compare(CME_HO) == 0) ||
      (_pure_basename_.compare(CME_NN) == 0)) {
    if (_next_cme_month_ == 12) {
      _next_cme_month_ = 1;
      _next_cme_year_++;

    } else {
      _next_cme_month_++;
    }

    return;
  }

  if (_pure_basename_.compare(CME_GC) == 0 || _pure_basename_.compare(CME_SI) == 0 ||
      _pure_basename_.compare(CME_HG) == 0) {
    do {
      if (_next_cme_month_ >= 12) {
        _next_cme_month_ = 1;
        _next_cme_year_++;

      } else {
        _next_cme_month_++;
      }
    } while (!IsCMEMonth(_pure_basename_, _next_cme_month_));

    return;
  }

  if (_pure_basename_.compare(CME_ZS) == 0) {
    if (_next_cme_month_ >= 11) {
      _next_cme_month_ = 1;  // 1 is ofcourse the ZS delivery month
      _next_cme_year_++;

    } else {
      do {
        _next_cme_month_++;

      } while (!IsCMEMonth(_pure_basename_, _next_cme_month_));
    }

    return;
  }

  if (_next_cme_month_ == 12) {
    _next_cme_month_ = 3;
    _next_cme_year_++;
    while (!IsCMEMonth(_pure_basename_, _next_cme_month_)) {
      _next_cme_month_++;
    }
  } else {
    do {
      _next_cme_month_++;
    } while (!IsCMEMonth(_pure_basename_, _next_cme_month_));
  }
}

void ExchangeSymbolManager::SetToNextSGXMonth(const std::string &_pure_basename_, int &_next_sgx_month_,
                                              int &_next_sgx_year_) {
  if (_pure_basename_.compare(SGX_NKF) == 0) {
    if (_next_sgx_month_ == 12 || _next_sgx_month_ == 11) {
      _next_sgx_month_ = 1;
      _next_sgx_year_++;
    } else {
      do {
        _next_sgx_month_++;
      } while (!IsSGXMonth(_pure_basename_, _next_sgx_month_));
    }
    return;
  } else if (_pure_basename_.compare(SGX_NK) == 0 || _pure_basename_.compare(SGX_NU) == 0) {
    if (_next_sgx_month_ == 12) {
      _next_sgx_month_ = 3;
      _next_sgx_year_++;
    } else {
      do {
        _next_sgx_month_++;
      } while (!IsSGXMonth(_pure_basename_, _next_sgx_month_));
    }
    return;
  } else if (_pure_basename_.compare(SGX_AJ) == 0 || _pure_basename_.compare(SGX_AU) == 0 ||
             _pure_basename_.compare(SGX_US) == 0 || _pure_basename_.compare(SGX_IU) == 0 ||
             _pure_basename_.compare(SGX_KU) == 0 || _pure_basename_.compare(SGX_CN) == 0 ||
             _pure_basename_.compare(SGX_IN) == 0 || _pure_basename_.compare(SGX_SG) == 0 ||
             _pure_basename_.compare(SGX_TW) == 0 || _pure_basename_.compare(SGX_CH) == 0 ||
             _pure_basename_.compare(SGX_MD) == 0 || _pure_basename_.compare(SGX_INB) == 0) {
    if (_next_sgx_month_ == 12) {
      _next_sgx_month_ = 1;
      _next_sgx_year_++;
    } else {
      do {
        _next_sgx_month_++;
      } while (!IsSGXMonth(_pure_basename_, _next_sgx_month_));
    }
  }
}

void ExchangeSymbolManager::SetToNextTSEMonth(const std::string &_pure_basename_, int &_next_tse_month_,
                                              int &_next_tse_year_) {
  if (_next_tse_month_ == 12) {
    _next_tse_month_ = 3;
    _next_tse_year_++;
    while (!IsTSEMonth(_pure_basename_, _next_tse_month_)) {
      _next_tse_month_++;
    }
  } else {
    do {
      _next_tse_month_++;
    } while (!IsTSEMonth(_pure_basename_, _next_tse_month_));
  }
}

bool ExchangeSymbolManager::IsEUREXMonth(const std::string &_pure_basename_, const int _this_month_) {
  if (_pure_basename_.compare(EUREX_FVS) == 0) return true;  // fvs is a monthly contract

  return ((_this_month_ % 3) == 0);
}

bool ExchangeSymbolManager::IsCFEMonth(const std::string &_pure_basename_, const int _this_month_) { return true; }

// if monthly expiry happens for a product in a week, then weekly expiry doesn't happen
bool ExchangeSymbolManager::IsCFEWeek(const std::string &pure_basename, int this_week, int this_year) {
  boost::gregorian::date d(this_year, boost::gregorian::Jan, 1);
  int curWeekDay = d.day_of_week();
  d += boost::gregorian::date_duration((this_week - 1) * 7) + boost::gregorian::date_duration(3 - curWeekDay);
  boost::gregorian::greg_month this_month = d.month();

  if (this_month == 12) {
    this_month = 1;
    this_year++;
  } else {
    this_month = this_month + 1;
  }

  boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                     boost::gregorian::Friday, this_month);
  boost::gregorian::date d1 = ndm.get_date(this_year);

  while (IsCFEHoliday(YYYYMMDD_from_date(d1))) {
    d1 = d1 - one_day_date_duration;
  }

  boost::gregorian::date monthly_expiry_date = d1 - thirty_day_date_duration;

  // even is the wednesday under consideration is a holiday
  if (this_week == monthly_expiry_date.week_number()) return false;

  return true;
}

bool ExchangeSymbolManager::IsASXMonth(const std::string &_pure_basename_, const int _this_month_) {
  //    if ( _pure_basename_.compare ( "AP" ) == 0 ||
  //        _pure_basename_.compare ( "IR" ) == 0 )
  //      {
  //        return true;
  //      }
  if (_pure_basename_.compare("XT") == 0 || _pure_basename_.compare("YT") == 0 || _pure_basename_.compare("XTE") == 0 ||
      _pure_basename_.compare("YTE") == 0 || _pure_basename_.compare("XX") == 0 ||
      _pure_basename_.compare("YTY") == 0 || _pure_basename_.compare("XTY") == 0 ||
      _pure_basename_.compare("AP") == 0 || _pure_basename_.compare("IR") == 0 || _pure_basename_.compare("IB") == 0) {
    if ((_this_month_ % 3) == 0) {
      return true;
    }
  }

  return false;
}

bool ExchangeSymbolManager::IsKRXMonth(const std::string &_pure_basename_, const int _this_month_) {
  if ((_this_month_ % 3) == 0) {
    return true;
  }
  return false;
}

void ExchangeSymbolManager::SetToNextCFEMonth(const std::string &_pure_basename_, int &_next_cfe_month_,
                                              int &_next_cfe_year_) {
  if (_next_cfe_month_ == 12) {
    _next_cfe_month_ = 1;
    _next_cfe_year_++;
  } else {
    do {
      _next_cfe_month_++;
    } while (!IsCFEMonth(_pure_basename_, _next_cfe_month_));
  }
}

void ExchangeSymbolManager::SetToNextASXMonth(const std::string &_pure_basename_, int &_next_asx_month_,
                                              int &_next_asx_year_) {
  //    if ( _pure_basename_.compare ( "AP" ) == 0 ||
  //        _pure_basename_.compare ( "IR" ) == 0 )
  //      {
  //        if ( _next_asx_month_ == 12 )
  //          {
  //            _next_asx_month_ = 1;
  //            _next_asx_year_++;
  //          }
  //        else
  //          {
  //            _next_asx_month_++;
  //          }
  //      }
  if (_pure_basename_.compare("XT") == 0 || _pure_basename_.compare("XTE") == 0 || _pure_basename_.compare("YT") == 0 ||
      _pure_basename_.compare("YTE") == 0 || _pure_basename_.compare("XTY") == 0 ||
      _pure_basename_.compare("XX") == 0 || _pure_basename_.compare("YTY") == 0 || _pure_basename_.compare("AP") == 0 ||
      _pure_basename_.compare("IR") == 0 || _pure_basename_.compare("IB") == 0) {
    if (_next_asx_month_ == 12) {
      _next_asx_month_ = 3;
      _next_asx_year_++;
    } else {
      do {
        _next_asx_month_++;
      } while (!IsASXMonth(_pure_basename_, _next_asx_month_));
    }
  }
}
void ExchangeSymbolManager::SetToNextKRXMonth(const std::string &_pure_basename_, int &_next_krx_month_,
                                              int &_next_krx_year_) {
  if (_next_krx_month_ == 12) {
    _next_krx_month_ = 3;
    _next_krx_year_++;
  } else {
    do {
      _next_krx_month_++;
    } while (!IsKRXMonth(_pure_basename_, _next_krx_month_));
  }
}

bool ExchangeSymbolManager::IsLIFFEMonth(const std::string &_pure_basename_, const int _this_month_,
                                         const int _this_year_ /*=-1*/) {
  if (_pure_basename_.compare(LIFFE_LFR) == 0 || _pure_basename_.compare(LIFFE_LFZ) == 0 ||
      _pure_basename_.compare(LIFFE_LFI) == 0 || _pure_basename_.compare(LIFFE_LFL) == 0)
    return ((_this_month_ % 3) == 0);
  if (_pure_basename_.compare(LIFFE_YFEBM) ==
      0) {  // Jan/March/May/Nov till May 2015 and March/May/Sep/Dec from Sept 2015
    if ((_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 1 && _this_year_ <= 2015) ||
        (_this_month_ == 11 && _this_year_ < 2015) ||
        (((_this_month_ == 9) || (_this_month_ == 12)) && (_this_year_ >= 2015)))
      return (true);
    else
      return (false);
  }
  if (_pure_basename_.compare(LIFFE_XFC) == 0) {  // March/May/July/Sept/Dec
    if ((_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 7) || (_this_month_ == 9) ||
        (_this_month_ == 12))
      return (true);
    else
      return (false);
  }
  if (_pure_basename_.compare(LIFFE_XFW) == 0) {  // March/May/Aug/Oct/Dec
    if ((_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 8) || (_this_month_ == 10) ||
        (_this_month_ == 12))
      return (true);
    else
      return (false);
  }
  if (_pure_basename_.compare(LIFFE_XFRC) == 0) {  // Jan/March/May/July/Sept/Nov
    if ((_this_month_ == 1) || (_this_month_ == 3) || (_this_month_ == 5) || (_this_month_ == 7) ||
        (_this_month_ == 9) || (_this_month_ == 11))
      return (true);
    else
      return (false);
  }

  // FCE, LFI, LFL are monthly contracts
  return true;
}

void ExchangeSymbolManager::SetToNextLIFFEMonth(const std::string &_pure_basename_, int &_next_liffe_month_,
                                                int &_next_liffe_year_) {
  if (_pure_basename_.compare(LIFFE_LFR) == 0 || _pure_basename_.compare(LIFFE_LFZ) == 0 ||
      _pure_basename_.compare(LIFFE_LFI) == 0 || _pure_basename_.compare(LIFFE_LFL) == 0) {
    if (_next_liffe_month_ == 12) {
      _next_liffe_month_ = 3;
      _next_liffe_year_++;

    } else {
      do {
        _next_liffe_month_++;

      } while (!IsLIFFEMonth(_pure_basename_, _next_liffe_month_, _next_liffe_year_));
    }

  } else if (_pure_basename_.compare(LIFFE_YFEBM) == 0 || _pure_basename_.compare(LIFFE_XFC) == 0 ||
             _pure_basename_.compare(LIFFE_XFW) == 0 || _pure_basename_.compare(LIFFE_XFRC) == 0) {
    do {
      if (_next_liffe_month_ == 12) {
        _next_liffe_month_ = 1;
        _next_liffe_year_++;
      } else {
        _next_liffe_month_++;
      }
    } while (!IsLIFFEMonth(_pure_basename_, _next_liffe_month_, _next_liffe_year_));

  } else {
    if (_next_liffe_month_ == 12) {
      _next_liffe_month_ = 1;
      _next_liffe_year_++;

    } else {
      _next_liffe_month_++;
    }
  }
}

void ExchangeSymbolManager::SetToNextEUREXMonth(const std::string &_pure_basename_, int &_next_eurex_month_,
                                                int &_next_eurex_year_) {
  if (_pure_basename_.compare(EUREX_FVS) == 0) {  // FVS is monthly

    if (_next_eurex_month_ == 12) {
      _next_eurex_month_ = 1;
      _next_eurex_year_++;

    } else {
      _next_eurex_month_++;
    }

  } else {
    if (_next_eurex_month_ == 12) {
      _next_eurex_month_ = 3;
      _next_eurex_year_++;
    } else {
      do {
        _next_eurex_month_++;
      } while (!IsEUREXMonth(_pure_basename_, _next_eurex_month_));
    }
  }
}

bool ExchangeSymbolManager::IsBMFMonth(const std::string &_pure_basename_, const int _this_month_) {
  if (!_pure_basename_.compare(BMF_WIN) || !_pure_basename_.compare(BMF_IND)) {
    // IND & WIN are even numbered months.
    return (_this_month_ % 2 == 0);
  } else if (_pure_basename_.compare(BMF_ISP) == 0) {
    return (_this_month_ % 3 == 0);
  } else if (_pure_basename_.compare(BMF_CCM) == 0 || _pure_basename_.compare(BMF_SJC) == 0) {
    if (_this_month_ == 1 || _this_month_ == 3 || _this_month_ == 5 || _this_month_ == 7 || _this_month_ == 8 ||
        _this_month_ == 9 || _this_month_ == 11) {
      return true;
    } else {
      return false;
    }
  } else if (_pure_basename_.compare(BMF_ICF) == 0) {
    if (_this_month_ == 3 || _this_month_ == 5 || _this_month_ == 7 || _this_month_ == 9 || _this_month_ == 12) {
      return true;
    } else {
      return false;
    }
  }

  return true;  // DOL, WDO ALL
}

void ExchangeSymbolManager::SetToNextBMFMonth(const std::string &_pure_basename_, int &_next_bmf_month_,
                                              int &_next_bmf_year_) {
  if (_next_bmf_month_ == 12) {
    _next_bmf_month_ = 1;
    _next_bmf_year_++;
    if (!IsBMFMonth(_pure_basename_, _next_bmf_month_)) {
      SetToNextBMFMonth(_pure_basename_, _next_bmf_month_, _next_bmf_year_);
    }
  } else {
    _next_bmf_month_++;
    if (!IsBMFMonth(_pure_basename_, _next_bmf_month_)) {
      SetToNextBMFMonth(_pure_basename_, _next_bmf_month_, _next_bmf_year_);
    }
  }
}

bool ExchangeSymbolManager::IsTMXMonth(const int _this_month_) { return ((_this_month_ % 3) == 0); }

void ExchangeSymbolManager::SetToNextTMXMonth(int &_next_eurex_month_, int &_next_eurex_year_) {
  if (_next_eurex_month_ == 12) {
    _next_eurex_month_ = 3;
    _next_eurex_year_++;
  } else {
    do {
      _next_eurex_month_++;
    } while (!IsTMXMonth(_next_eurex_month_));
  }
}

void ExchangeSymbolManager::SetToNextOSEMonth(const std::string &_pure_basename_, int &_next_ose_month_,
                                              int &_next_ose_year_) {
  if (OSE_NKMF == _pure_basename_) {
    if (_next_ose_month_ >= 11) {
      _next_ose_month_ = 1;
      _next_ose_year_++;
    } else {
      do {
        _next_ose_month_++;
      } while (!IsOSEFrontMonth(_pure_basename_, _next_ose_month_));
    }

  } else {
    if (_next_ose_month_ == 12) {
      _next_ose_month_ = 3;
      _next_ose_year_++;
    } else {
      do {
        _next_ose_month_++;
      } while (!IsOSEMonth(_pure_basename_, _next_ose_month_));
    }
  }
}

bool ExchangeSymbolManager::IsRTSMonth(const std::string &_pure_basename_, const int _this_month_) {
  if (_pure_basename_.compare(RTS_BR) == 0) return true;
  return ((_this_month_ % 3) == 0);
}

void ExchangeSymbolManager::SetToNextRTSMonth(const std::string &_pure_basename_, int &_next_rts_month_,
                                              int &_next_rts_year_) {
  if (_pure_basename_.compare(RTS_BR) == 0) {
    if (_next_rts_month_ == 12) {
      _next_rts_month_ = 1;
      _next_rts_year_++;

    } else {
      _next_rts_month_++;
    }

    return;
  }

  if (_next_rts_month_ == 12) {
    _next_rts_month_ = 3;
    _next_rts_year_++;
  } else {
    do {
      _next_rts_month_++;
    } while (!IsRTSMonth(_pure_basename_, _next_rts_month_));
  }
}

/// Get Last Trading day of the given contract
int ExchangeSymbolManager::GetCMELastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_cme_month_,
                                                       const int next_cme_year_) {
  /// ES, NQ, YM
  /// Settlement Date:
  /// The 3rd Friday of IMM month
  /// Last Trading Day :
  /// Same as Settlement Date

  /// OBSERVATION: the volume shifts towards the next future contract one
  /// week before i.e 2nd Friday
  std::string shortcode = _pure_basename_ + std::string("_0");

  if ((_pure_basename_.compare(CME_ES) == 0) || (_pure_basename_.compare(CME_NQ) == 0) ||
      (_pure_basename_.compare(CME_NKD) == 0) || (_pure_basename_.compare(CME_NIY) == 0) ||
      (_pure_basename_.compare(CME_EMD) == 0) || (_pure_basename_.compare(CME_YM) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Friday, next_cme_month_);
    boost::gregorian::date d1 = ndm.get_date(next_cme_year_);
    if ((_pure_basename_.compare(CME_NKD) == 0) || (_pure_basename_.compare(CME_NIY) == 0)) {
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
    }

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  /// ZT, ZF, ZN, ZB, UB : first notice date is the last day of the previous month
  /// we expect to stop trading a day before
  if ((_pure_basename_.compare(CME_ZT) == 0) || (_pure_basename_.compare(CME_ZF) == 0) ||
      (_pure_basename_.compare(CME_ZN) == 0) || (_pure_basename_.compare(CME_TN) == 0) ||
      (_pure_basename_.compare(CME_ZB) == 0) || (_pure_basename_.compare(CME_UB) == 0) ||
      (_pure_basename_.compare(CME_ZTY) == 0) || (_pure_basename_.compare(CME_ZFY) == 0) ||
      (_pure_basename_.compare(CME_ZNY) == 0) || (_pure_basename_.compare(CME_ZBY) == 0) ||
      (_pure_basename_.compare(CME_UBY) == 0) || (_pure_basename_.compare(CME_ZW) == 0) ||
      (_pure_basename_.compare(CME_KE) == 0) || (_pure_basename_.compare(CME_ZL) == 0) ||
      (_pure_basename_.compare(CME_ZM) == 0) || (_pure_basename_.compare(CME_ZC) == 0) ||
      (_pure_basename_.compare(CME_ZS) == 0) || (_pure_basename_.compare(CME_XW) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 1);

    // 1 day subtracted to get last day of previous month.
    // One day rolled over to be safe.
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
  }

  /// CME FX futures
  /// 6C, BRL and rouble have different last trading day
  if ((_pure_basename_.compare(CME_6A) == 0) || (_pure_basename_.compare(CME_6B) == 0) ||
      (_pure_basename_.compare(CME_6E) == 0) || (_pure_basename_.compare(CME_6J) == 0) ||
      (_pure_basename_.compare(CME_6M) == 0) || (_pure_basename_.compare(CME_6N) == 0) ||
      (_pure_basename_.compare(CME_6S) == 0) || (_pure_basename_.compare(CME_6C) == 0) ||
      (_pure_basename_.compare(CME_SEK) == 0) || (_pure_basename_.compare(CME_6L) == 0) ||
      (_pure_basename_.compare(CME_SIR) == 0) || (_pure_basename_.compare(CME_6Z) == 0))

  {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_cme_month_);
    boost::gregorian::date d1 = ndm.get_date(next_cme_year_);

    // Instead of Monday (Third Wednesday -2 business day ), the volume starts shifting on the previous Friday itself
    // Noted for Sept 16th (rollover) & also noted for Dec 16th (rollover ). We should make one day before (Thursday) as
    // last trading date. This observation holds true for all 6's
    // TODO {} See for M12 rollover
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 4);
  }

  // TODO VERIFY, ONLY ADDED CURRENTLY FOR COMBINED WRITER PURPOSE
  if (_pure_basename_.compare(CME_6R) == 0) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 15);
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  // if ( _pure_basename_.compare ( CME_6C ) == 0 )
  //   {
  // 	boost::gregorian::nth_day_of_the_week_in_month ndm ( boost::gregorian::nth_day_of_the_week_in_month::third,
  // boost::gregorian::Wednesday, next_cme_month_ );
  // 	boost::gregorian::date d1 = ndm.get_date ( next_cme_year_ );

  // 	d1 -= one_day_date_duration;
  //     return YYYYMMDD_from_date( d1 );
  //   }

  /// GE : second London bank business day prior to the third Wednesday of the contract expiry month
  if (_pure_basename_.compare(CME_GE) == 0 || _pure_basename_.compare(CME_GEY) == 0) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_cme_month_);
    boost::gregorian::date d1 = ndm.get_date(next_cme_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }

  // volumes shifts on 18th
  if ((_pure_basename_.compare(CME_CL) == 0) || (_pure_basename_.compare(CME_RB) == 0) ||
      (_pure_basename_.compare(CME_QM) == 0) || (_pure_basename_.compare(CME_HO) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 18);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(CME_BZ) == 0)) {
    // Expiry rules changed March 2016 onwards
    if (YYYYMMDD_ < 20160115) {
      boost::gregorian::date d1(next_cme_year_, next_cme_month_, 15);
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);

    } else {
      // Get First Day of the month and then go to end of the month
      boost::gregorian::date d1(next_cme_year_, next_cme_month_, 1);
      d1 = d1.end_of_month();
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
    }
  }

  // last trading day is 3 business days before the 1st of delivery month
  // volume rolls over roughly 3 days prior to that
  if ((_pure_basename_.compare(CME_NG) == 0) || (_pure_basename_.compare(CME_NN) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 1);
    d1 = d1.end_of_month();
    d1 += one_day_date_duration;
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 6);
  }
  // volumes shifts on second last trading day of expiry month
  if ((_pure_basename_.compare(CME_GC) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 1);
    boost::gregorian::date d2 = d1.end_of_month();

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 2);
  }

  if ((_pure_basename_.compare(CME_SI) == 0) || (_pure_basename_.compare(CME_HG) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 1);
    boost::gregorian::date d2 = d1.end_of_month();

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 2);
  }

  if ((_pure_basename_.compare(CME_IBV) == 0)) {
    boost::gregorian::date d1(next_cme_year_, next_cme_month_, 15);
    // Assuming closest means strictly <= as well.
    while (d1.day_of_week() != boost::gregorian::Wednesday) {
      d1 -= one_day_date_duration;
    }

    int d1_day_ = YYYYMMDD_from_date(d1) % 100;

    // bugfix : docs mentions last trading date as wednesday closest to 15the of expiry month, not necessarily
    // being prior to 15th

    // get wednesday after 15th
    boost::gregorian::date d2(next_cme_year_, next_cme_month_, 15);

    while (d2.day_of_week() != boost::gregorian::Wednesday) {
      d2 += one_day_date_duration;
    }

    int d2_day_ = YYYYMMDD_from_date(d2) % 100;

    // last trading date is post 15th
    if ((d2_day_ - 15) < (15 - d1_day_)) {
      d2 -= one_day_date_duration;
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 1);
    }
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  ExitVerbose(kExchangeSymbolManagerUnhandledCase, _pure_basename_.c_str());

  // default day ... 10th day of the month
  boost::gregorian::date d1(next_cme_year_, next_cme_month_, 10);
  return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
}

int ExchangeSymbolManager::GetDateNBusinessDaysBefore(std::string shortcode, int current_YYYYMMDD,
                                                      int num_bussiness_days) {
  if (num_bussiness_days <= 0) {
    return current_YYYYMMDD;
  }

  int YYYY = (current_YYYYMMDD / 10000) % 10000;
  int MM = (current_YYYYMMDD / 100) % 100;
  int DD = current_YYYYMMDD % 100;

  boost::gregorian::date d1(YYYY, MM, DD);

  HolidayManager &holiday_manager = HolidayManager::GetUniqueInstance();
  int product_start_date = holiday_manager.GetProductStartDate(shortcode);

  int days_to_substract = num_bussiness_days;
  int result_YYYYMMDD = YYYYMMDD_from_date(d1);
  while (days_to_substract) {
    try {
      if (!holiday_manager.IsProductHoliday(shortcode, result_YYYYMMDD, true)) {
        days_to_substract--;
      }
    } catch (...) {
      // In case of exception, assuming it a holiday
    }

    d1 -= one_day_date_duration;
    result_YYYYMMDD = YYYYMMDD_from_date(d1);
    if (result_YYYYMMDD <= product_start_date) {
      return result_YYYYMMDD;  // Error Case: Last Bussiness Day not possible as all days prior to start day are
                               // holidays. Return result_YYYYMMDD (the best we can do).
    }
  }

  return result_YYYYMMDD;
}

int ExchangeSymbolManager::GetSGXLastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_sgx_month_,
                                                       const int next_sgx_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");
  if (_pure_basename_.compare(SGX_CN) == 0 || _pure_basename_.compare(SGX_SG) == 0 ||
      _pure_basename_.compare(SGX_TW) == 0 || _pure_basename_.compare(SGX_CH) == 0) {
    // Last Trading Day: Second last business day of the contract month
    boost::gregorian::date d1(next_sgx_year_, next_sgx_month_, 1);
    boost::gregorian::date d2 = d1.end_of_month();

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 2);
  }

  if (_pure_basename_.compare(SGX_IN) == 0 || _pure_basename_.compare(SGX_MD) == 0 ||
      _pure_basename_.compare(SGX_INB) == 0) {
    // Last Trading Day: last Thursday of the month
    boost::gregorian::date d1(next_sgx_year_, next_sgx_month_, 1);
    boost::gregorian::date d2 = d1.end_of_month();

    std::string day_of_week_ = d2.day_of_week().as_long_string();

    while (day_of_week_ != "Thursday") {
      d2 -= one_day_date_duration;
      day_of_week_ = d2.day_of_week().as_long_string();
    }

    // Rollover 1 day before
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 1);
  }

  // Day before 2nd Friday for each month
  if ((_pure_basename_.compare(SGX_NK) == 0) || (_pure_basename_.compare(SGX_NKF) == 0) ||
      (_pure_basename_.compare(SGX_NU) == 0)) {
    boost::gregorian::date d1(next_sgx_year_, next_sgx_month_, 1);
    std::string day_of_week_ = d1.day_of_week().as_long_string();
    short fri_count = 0;
    while (1) {
      day_of_week_ = d1.day_of_week().as_long_string();
      if (day_of_week_ == "Friday") {
        fri_count++;
        if (fri_count == 2) {
          break;
        }
      }
      d1 += one_day_date_duration;
    }
    d1 -= one_day_date_duration;  // Day before 2nd Friday

    // Rollover 1 Day before
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  // 2 Business days before 3rd Wed
  if ((_pure_basename_.compare(SGX_AU) == 0) || (_pure_basename_.compare(SGX_AJ) == 0) ||
      (_pure_basename_.compare(SGX_KU) == 0) || (_pure_basename_.compare(SGX_US) == 0)) {
    boost::gregorian::date d1(next_sgx_year_, next_sgx_month_, 1);
    std::string day_of_week_ = d1.day_of_week().as_long_string();
    short wed_count = 0;
    while (1) {
      day_of_week_ = d1.day_of_week().as_long_string();
      if (day_of_week_ == "Wednesday") {
        wed_count++;
        if (wed_count == 3) {
          break;
        }
      }
      d1 += one_day_date_duration;
    }

    // 3 days before (2 bussiness days and 1 Day Rollover Early)
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }
  // 2 Business days prior last bussiness day
  if (_pure_basename_.compare(SGX_IU) == 0) {
    boost::gregorian::date d1(next_sgx_year_, next_sgx_month_, 1);
    boost::gregorian::date d2 = d1.end_of_month();

    std::string day_of_week_ = d2.day_of_week().as_long_string();

    // 3 days before (2 Business Days and 1 Day Early Rollover)
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 3);
  }

  // Need to change to default expiry value
  return -1;
}

int ExchangeSymbolManager::GetTSELastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_tse_month_,
                                                       const int next_tse_year_) {
  // just a placeholder
  boost::gregorian::date dummy(next_tse_year_, next_tse_month_, 10);
  std::string shortcode = _pure_basename_ + std::string("_0");
  // volumes shifts on a week before the delivery date
  if ((_pure_basename_.compare(TSE_JGB) == 0) || (_pure_basename_.compare(TSE_MJG) == 0)) {
    boost::gregorian::date d1(next_tse_year_, next_tse_month_, 20);
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 8);
  }

  // 2nd Friday of each contract month, don't know when volumes starts shifting //TODO
  if ((_pure_basename_.compare(TSE_TPX) == 0) || (_pure_basename_.compare(TSE_MTP))) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Friday, next_tse_month_);
    boost::gregorian::date d1 = ndm.get_date(next_tse_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }
  // not expected
  return YYYYMMDD_from_date(dummy);
}

std::string ExchangeSymbolManager::GetCMESymbolFromLastTradingDate(const std::string &_pure_basename_,
                                                                   const int current_min_last_trading_date) {
  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000) % 10;

  /// ZT, ZF, ZN, ZB, UB : first notice date is the last day of the previous month
  /// we expect to stop trading a day before
  if ((_pure_basename_.compare(CME_ZT) == 0) || (_pure_basename_.compare(CME_ZF) == 0) ||
      (_pure_basename_.compare(CME_ZN) == 0) || (_pure_basename_.compare(CME_ZB) == 0) ||
      (_pure_basename_.compare(CME_UB) == 0) || (_pure_basename_.compare(CME_ZTY) == 0) ||
      (_pure_basename_.compare(CME_ZFY) == 0) || (_pure_basename_.compare(CME_ZNY) == 0) ||
      (_pure_basename_.compare(CME_ZBY) == 0) || (_pure_basename_.compare(CME_UBY) == 0) ||
      (_pure_basename_.compare(CME_ZW) == 0) || (_pure_basename_.compare(CME_KE) == 0) ||
      (_pure_basename_.compare(CME_ZL) == 0) || (_pure_basename_.compare(CME_ZM) == 0) ||
      (_pure_basename_.compare(CME_ZC) == 0) || (_pure_basename_.compare(CME_ZS) == 0) ||
      (_pure_basename_.compare(CME_XW) == 0) || (_pure_basename_.compare(CME_TN) == 0) ||
      (_pure_basename_.compare(CME_CL) == 0) || (_pure_basename_.compare(CME_RB) == 0) ||
      (_pure_basename_.compare(CME_NG) == 0) || (_pure_basename_.compare(CME_QM) == 0) ||
      (_pure_basename_.compare(CME_HO) == 0) || (_pure_basename_.compare(CME_NN) == 0)) {
    if (current_min_last_trading_date_mm == 12) {
      current_min_last_trading_date_mm = 1;
      current_min_last_trading_date_yy++;
    } else {
      current_min_last_trading_date_mm++;
    }
  }

  if ((_pure_basename_.compare(CME_BZ) == 0)) {
    // expiry changed for contracts after March 2016.
    if (YYYYMMDD_ > 20160115) {
      if (current_min_last_trading_date_mm == 11) {
        current_min_last_trading_date_mm = 1;
        current_min_last_trading_date_yy++;
      } else if (current_min_last_trading_date_mm == 12) {
        current_min_last_trading_date_mm = 2;
        current_min_last_trading_date_yy++;
      } else {
        current_min_last_trading_date_mm += 2;
      }
    } else {
      if (current_min_last_trading_date_mm == 12) {
        current_min_last_trading_date_mm = 1;
        current_min_last_trading_date_yy++;
      } else {
        current_min_last_trading_date_mm++;
      }
    }
  }

  if ((_pure_basename_.compare(CME_GC) == 0)) {
    if (current_min_last_trading_date_mm % 2 != 0) current_min_last_trading_date_mm++;
  }

  if ((_pure_basename_.compare(CME_HG) == 0) || (_pure_basename_.compare(CME_SI) == 0)) {
    if ((current_min_last_trading_date_mm == 2) || (current_min_last_trading_date_mm == 4) ||
        (current_min_last_trading_date_mm == 6) || (current_min_last_trading_date_mm == 8) ||
        (current_min_last_trading_date_mm == 11)) {
      current_min_last_trading_date_mm++;
    }
  }

  std::stringstream ss;
  ss << _pure_basename_ << CMEMonthCode[current_min_last_trading_date_mm - 1] << current_min_last_trading_date_yy;
  return ss.str();
}

std::string ExchangeSymbolManager::GetSGXSymbolFromLastTradingDate(const std::string &_pure_basename_,
                                                                   const int current_min_last_trading_date) {
  //  std::cout << "Symbol: " << _pure_basename_ << " : " << current_min_last_trading_date << "\n";

  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000) % 100;

  //  std::cout << "MM: " << current_min_last_trading_date_mm << " YY: " << current_min_last_trading_date_yy << "\n";
  std::string pure_basename = _pure_basename_;
  if (_pure_basename_.substr(0, 4) == "SGX_") {
    pure_basename = _pure_basename_.substr(4);
  }
  if (pure_basename.substr(0, 2) == "SG" && (YYYYMMDD() > 20160601)) {
    pure_basename = "SGP";
  }
  std::stringstream ss;
  if (_pure_basename_.compare(SGX_NKF) == 0) {
    // pure_basename.substr(0, 2) == _pure_basename_.substr(0, 6). We only want to remove last F.
    ss << pure_basename.substr(0, 2) << CMEMonthCode[current_min_last_trading_date_mm - 1]
       << current_min_last_trading_date_yy;
  } else {
    ss << pure_basename << CMEMonthCode[current_min_last_trading_date_mm - 1] << current_min_last_trading_date_yy;
  }
  return ss.str();
}

inline bool IsEUREXExchangeDate(const std::string &_pure_basename_, boost::gregorian::date &d1) {
  // TODO only using weekends as holidays, need to add EUREX holiday from Quantlib
  return ((d1.day_of_week() != boost::gregorian::Saturday) && (d1.day_of_week() != boost::gregorian::Sunday));
}

/// FGBS,M,L,X
/// Delivery Day :
/// The tenth calendar day of the respective quarterly month, if this day is an exchange day; otherwise, the exchange
/// day immediately succeeding that day.
/// Last Trading Day :
/// Two exchange days prior to the Delivery Day of the relevant maturity month. Close of trading in the maturing futures
/// on the Last Trading Day is at 12:30 CET.
///
/// FESX,FDAX,FSMI
/// Last Trading Day = Final Settlement Day is the third Friday, if this is an exchange day; otherwise the exchange day
/// immediately preceding that day.
///
/// FVS
/// Last Trading Day = Tuesday preceding 2nd last Friday of the month, if this is an exchange day; otherwise the
/// exchange day immediately preceding that day.
/// FX FUTURES
/// Last trading day and final settlement day is the third Wednesday of each maturity month if this is an exchange day;
/// otherwise the exchange day immediately preceding that day. Close of trading in the maturing FX Futures on the last
/// trading day is at 15:00 CET.

int ExchangeSymbolManager::GetEUREXLastTradingDateYYYYMM(std::string _pure_basename_, const int next_eurex_month_,
                                                         const int next_eurex_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");

  if ((_pure_basename_.compare(EUREX_FESX) == 0) || (_pure_basename_.compare(EUREX_FEXD) == 0) ||
      (_pure_basename_.compare(EUREX_FDAX) == 0) || (_pure_basename_.compare(EUREX_FRDX) == 0) ||
      (_pure_basename_.compare(EUREX_FDXM) == 0) || (_pure_basename_.compare(EUREX_FEXF) == 0) ||
      (_pure_basename_.compare(EUREX_FESB) == 0) || (_pure_basename_.compare(EUREX_FSTB) == 0) ||
      (_pure_basename_.compare(EUREX_FSTS) == 0) || (_pure_basename_.compare(EUREX_FSTO) == 0) ||
      (_pure_basename_.compare(EUREX_FSTG) == 0) || (_pure_basename_.compare(EUREX_FSTI) == 0) ||
      (_pure_basename_.compare(EUREX_FSTM) == 0) || (_pure_basename_.compare(EUREX_F2MX) == 0) ||
      (_pure_basename_.compare(EUREX_FXXP) == 0) || (_pure_basename_.compare(EUREX_FSMI) == 0) ||
      (_pure_basename_.compare(EUREX_FESQ) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(EUREX_FVS) == 0)) {
    boost::gregorian::date d1 = boost::gregorian::date(next_eurex_year_, next_eurex_month_, 1).end_of_month();
    while (d1.day_of_week() != boost::gregorian::Friday) {
      d1 = d1 - one_day_date_duration;
    }
    d1 = d1 - boost::gregorian::date_duration(10);
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(EUREX_FEU3) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }

  if ((_pure_basename_.compare(EUREX_OKS2) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Thursday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(EUREX_FEU3) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
  }

  if ((_pure_basename_.compare(EUREX_FGBS) == 0) || (_pure_basename_.compare(EUREX_FGBM) == 0) ||
      (_pure_basename_.compare(EUREX_FGBL) == 0) || (_pure_basename_.compare(EUREX_FGBX) == 0) ||
      (_pure_basename_.compare(EUREX_FBTS) == 0) || (_pure_basename_.compare(EUREX_FBTP) == 0) ||
      (_pure_basename_.compare(EUREX_FBTM) == 0) || (_pure_basename_.compare(EUREX_CONF) == 0) ||
      (_pure_basename_.compare(EUREX_FOAT) == 0) || (_pure_basename_.compare(EUREX_FOAM) == 0) ||
      (_pure_basename_.compare(EUREX_FBON) == 0) || (_pure_basename_.compare(EUREX_FBON) == 0) ||
      (_pure_basename_.compare(EUREX_FGBSY) == 0) || (_pure_basename_.compare(EUREX_FGBMY) == 0) ||
      (_pure_basename_.compare(EUREX_FGBLY) == 0) || (_pure_basename_.compare(EUREX_FGBXY) == 0) ||
      (_pure_basename_.compare(EUREX_FBTSY) == 0) || (_pure_basename_.compare(EUREX_FBTPY) == 0) ||
      (_pure_basename_.compare(EUREX_FBTMY) == 0) || (_pure_basename_.compare(EUREX_CONFY) == 0) ||
      (_pure_basename_.compare(EUREX_FOATY) == 0) || (_pure_basename_.compare(EUREX_FOAMY) == 0)) {
    boost::gregorian::date d1(next_eurex_year_, next_eurex_month_, 10);  ///< tenth calendar day of the contract month

    // if this isn't an exchange day then return the next exchange day after it
    while (!IsEUREXExchangeDate(_pure_basename_, d1)) {
      d1 += one_day_date_duration;
    }

    // Observation: Making it 4 days from 3 days based on Volume observation manually
    // Docs says Last trading day is 2 days before delivery day
    // 2+1 exchange days prior to Delivery Day
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 4);
  }

  if ((_pure_basename_.compare(EUREX_FCEU) == 0) || (_pure_basename_.compare(EUREX_FCPU) == 0) ||
      (_pure_basename_.compare(EUREX_FCUF) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_eurex_month_);
    boost::gregorian::date d1 = ndm.get_date(next_eurex_year_);  ///< 3rd wednesday of the contract month
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  ExitVerbose(kExchangeSymbolManagerUnhandledCase, _pure_basename_.c_str());

  // default day 1
  boost::gregorian::date d1(next_eurex_year_, next_eurex_month_, 10);
  // d1 -= one_day_date_duration;
  return YYYYMMDD_from_date(d1);
}
// FCE, FTI and LFZ last trading day is the 3rd Friday on the delivery month
// LFL 3rd Wed.
// LFI 2days prior to the 3rd Wed
// LFR -- TODO
int ExchangeSymbolManager::GetLIFFELastTradingDateYYYYMM(std::string _pure_basename_, const int next_liffe_month_,
                                                         const int next_liffe_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");

  if ((_pure_basename_.compare(LIFFE_JFFCE) == 0) || (_pure_basename_.compare(LIFFE_KFFTI) == 0) ||
      (_pure_basename_.compare(LIFFE_LFZ) == 0) || (_pure_basename_.compare(LIFFE_KFMFA) == 0) ||
      (_pure_basename_.compare(LIFFE_JFMFC) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, next_liffe_month_);
    boost::gregorian::date d1 = ndm.get_date(next_liffe_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(LIFFE_LFL) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_liffe_month_);
    boost::gregorian::date d1 = ndm.get_date(next_liffe_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);
  }

  if ((_pure_basename_.compare(LIFFE_LFI) == 0)) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_liffe_month_);
    boost::gregorian::date d1 = ndm.get_date(next_liffe_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }

  if ((_pure_basename_.compare(LIFFE_LFR) == 0)) {
    boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 1);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }

  if ((_pure_basename_.compare(LIFFE_XFC) == 0)) {
    boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 1);
    d1 = d1.end_of_month();

    int last_trading_day_ = GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 11);

    boost::gregorian::date d2(last_trading_day_ / 10000, (last_trading_day_ % 10000) / 100, last_trading_day_ % 100);
    d2 -= thirty_nine_day_date_duration;
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d2), 1);
  }

  if ((_pure_basename_.compare(LIFFE_XFW) == 0)) {
    boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 1);
    // d1 = d1.end_of_month();
    d1 -= sixteen_day_date_duration;  // actual
    d1 -= forty_day_date_duration;    // based on volume shift pattern

    std::string day_of_week_ = d1.day_of_week().as_long_string();

    // last day is Sunday hence shift back two more days
    if (day_of_week_ == "Sunday") d1 -= two_day_date_duration;

    // last day Saturday hence 1
    if (day_of_week_ == "Saturday") d1 -= one_day_date_duration;
    return YYYYMMDD_from_date(d1);
  }

  if ((_pure_basename_.compare(LIFFE_XFRC) == 0)) {
    boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 1);
    d1 = d1.end_of_month();         // actual
    d1 -= forty_day_date_duration;  // based on volume shift pattern
    // last day is Sunday hence shift back two more days
    std::string day_of_week_ = d1.day_of_week().as_long_string();
    if (day_of_week_ == "Sunday") d1 -= two_day_date_duration;

    // last day Saturday hence 1
    if (day_of_week_ == "Saturday") d1 -= one_day_date_duration;
    return YYYYMMDD_from_date(d1);
  }

  if ((_pure_basename_.compare(LIFFE_YFEBM) == 0)) {
    // stop trading before one day of expiry i.e 10th
    boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 9);
    return YYYYMMDD_from_date(d1);
  }
  // default day 1
  boost::gregorian::date d1(next_liffe_year_, next_liffe_month_, 10);
  return YYYYMMDD_from_date(d1);
}

std::string ExchangeSymbolManager::GetEUREXSymbolFromLastTradingDate(std::string _pure_basename_,
                                                                     const int current_min_last_trading_date,
                                                                     bool _is_spread_) {
  int current_min_last_trading_date_tmp = current_min_last_trading_date / 100;
  std::stringstream ss;
  if (_is_spread_) {
    ss << _pure_basename_ << CMEMonthCode[current_min_last_trading_date_tmp % 100 - 1]
       << (current_min_last_trading_date_tmp / 100) % 100;
  } else {
    ss << _pure_basename_ << current_min_last_trading_date_tmp;
  }
  return ss.str();
}

std::string ExchangeSymbolManager::GetASXSymbolFromLastTradingDate(std::string _pure_basename_,
                                                                   const int current_min_last_trading_date,
                                                                   bool _is_spread_) {
  int current_min_last_trading_date_tmp = current_min_last_trading_date / 100;
  std::stringstream ss;
  if (_is_spread_) {
    ss << _pure_basename_ << CMEMonthCode[(current_min_last_trading_date_tmp % 100) - 1]
       << (current_min_last_trading_date_tmp / 100) % 10;
  } else {
    if (_pure_basename_.compare("XTE") == 0 || _pure_basename_.compare("YTE") == 0) {
      std::string _temp_pure_basename_ = _pure_basename_.substr(0, _pure_basename_.find("E"));
      ss << _temp_pure_basename_ << current_min_last_trading_date_tmp;
    } else {
      ss << _pure_basename_ << current_min_last_trading_date_tmp;
    }
  }
  return ss.str();
}

std::string ExchangeSymbolManager::GetKRXSymbolFromLastTradingDate(std::string _pure_basename_,
                                                                   const int current_min_last_trading_date) {
  int current_min_last_trading_date_tmp = current_min_last_trading_date / 100;
  std::stringstream ss;
  ss << _pure_basename_ << CMEMonthCode[(current_min_last_trading_date_tmp % 100) - 1]
     << (current_min_last_trading_date_tmp / 100) % 100;
  return ss.str();
}

std::string ExchangeSymbolManager::GetTSESymbolFromLastTradingDate(std::string _pure_basename_,
                                                                   const int current_min_last_trading_date) {
  std::string fut_code_ = "00000F";

  int this_local_trade_date_ = current_min_last_trading_date;

  std::ostringstream expiry_oss_;
  expiry_oss_ << this_local_trade_date_;

  char exch_symbol_[16];

  memset(exch_symbol_, 0, 16);
  //    memset( exch_symbol_, ' ', 15 ) ;

  int expiry_offset_ = 5;
  int future_code_offset_ = 9;

  memcpy(exch_symbol_, _pure_basename_.c_str(), _pure_basename_.length());
  memcpy(exch_symbol_ + expiry_offset_, (expiry_oss_.str()).substr(2, 4).c_str(), 4);
  memcpy(exch_symbol_ + future_code_offset_, fut_code_.c_str(), 6);

  return exch_symbol_;
}

std::string ExchangeSymbolManager::GetLIFFESymbolFromLastTradingDate(std::string _pure_basename_,
                                                                     const int current_min_last_trading_date) {
  std::string fut_code_ = "00000F";

  int this_local_trade_date_ = current_min_last_trading_date;

  //    if ( ( _pure_basename_ == LIFFE_LFR ) || ( _pure_basename_ == LIFFE_XFC ) || ( _pure_basename_ == LIFFE_XFRC )
  //    || ( _pure_basename_ == LIFFE_YFEBM ) ) {
  if ((_pure_basename_ == LIFFE_LFR) || (_pure_basename_ == LIFFE_XFC) || (_pure_basename_ == LIFFE_XFRC)) {
    int current_month_ = ((current_min_last_trading_date / 100) % 100);
    int current_year_ = (current_min_last_trading_date / 10000);

    // Incrementing the Year is not required here, since the expiry based last trading date function will always
    // return
    // the correct Date from which we extract this expiry
    current_month_++;
    if (current_month_ == 13) {
      current_month_ = 1;
      current_year_++;
    }
    boost::gregorian::date d1(current_year_, current_month_, 1);

    this_local_trade_date_ = YYYYMMDD_from_date(d1);
  }
  if (_pure_basename_ == LIFFE_XFW) {
    int current_month_ = ((current_min_last_trading_date / 100) % 100);
    int current_year_ = (current_min_last_trading_date / 10000);

    // Incrementing the Year is not required here, since the expiry based last trading date function will always
    // return
    // the correct Date from which we extract this expiry
    current_month_ += 2;
    boost::gregorian::date d1(current_year_, current_month_, 1);
    this_local_trade_date_ = YYYYMMDD_from_date(d1);
  }

  std::ostringstream expiry_oss_;
  expiry_oss_ << this_local_trade_date_;

  char exch_symbol_[16];

  memset(exch_symbol_, 0, 16);
  memset(exch_symbol_, ' ', 15);

  int expiry_offset_ = 5;
  int future_code_offset_ = 9;

  memcpy(exch_symbol_, _pure_basename_.c_str(), _pure_basename_.length());
  memcpy(exch_symbol_ + expiry_offset_, (expiry_oss_.str()).substr(2, 4).c_str(), 4);
  memcpy(exch_symbol_ + future_code_offset_, fut_code_.c_str(), 6);

  return exch_symbol_;
}

std::string ExchangeSymbolManager::GetICEShortSymbolFromLastTradingDate(std::string _pure_basename_,
                                                                        const int current_min_last_trading_date) {
  int this_local_trade_date_ = current_min_last_trading_date;

  //    if ( ( _pure_basename_ == LIFFE_LFR ) || ( _pure_basename_ == LIFFE_XFC ) || ( _pure_basename_ == LIFFE_XFRC )
  //    || ( _pure_basename_ == LIFFE_YFEBM ) ) {
  if (_pure_basename_ == ICE_LFR) {
    int current_month_ = ((current_min_last_trading_date / 100) % 100);
    int current_year_ = (current_min_last_trading_date / 10000);

    // Incrementing the Year is not required here, since the expiry based last trading date function will always
    // return
    // the correct Date from which we extract this expiry
    current_month_++;
    if (current_month_ == 13) {
      current_month_ = 1;
      current_year_++;
    }
    boost::gregorian::date d1(current_year_, current_month_, 1);

    this_local_trade_date_ = YYYYMMDD_from_date(d1);
  }
  std::ostringstream expiry_oss_;
  expiry_oss_ << ((this_local_trade_date_ / 100) % 10000);  // YYMM

  char exch_symbol_[16];

  memset(exch_symbol_, 0, 16);
  // memset( exch_symbol_, ' ', 15 ) ;

  int expiry_offset_ = 3;

  memcpy(exch_symbol_, _pure_basename_.c_str(), _pure_basename_.length());
  memcpy(exch_symbol_ + expiry_offset_, expiry_oss_.str().c_str(), 4);

  return exch_symbol_;
}

std::string ExchangeSymbolManager::GetICESymbolFromLastTradingDate(std::string _pure_basename_,
                                                                   const int current_min_last_trading_date) {
  int this_local_trade_date_ = current_min_last_trading_date;

  //    if ( ( _pure_basename_ == LIFFE_LFR ) || ( _pure_basename_ == LIFFE_XFC ) || ( _pure_basename_ == LIFFE_XFRC )
  //    || ( _pure_basename_ == LIFFE_YFEBM ) ) {
  if (_pure_basename_ == ICE_LFR) {
    int current_month_ = ((current_min_last_trading_date / 100) % 100);
    int current_year_ = (current_min_last_trading_date / 10000);

    // Incrementing the Year is not required here, since the expiry based last trading date function will always
    // return
    // the correct Date from which we extract this expiry
    current_month_++;
    if (current_month_ == 13) {
      current_month_ = 1;
      current_year_++;
    }
    boost::gregorian::date d1(current_year_, current_month_, 1);

    this_local_trade_date_ = YYYYMMDD_from_date(d1);
  }
  std::ostringstream expiry_oss_;
  expiry_oss_ << "FM" << CMEMonthCode[((this_local_trade_date_ / 100) % 100) - 1] << "00"
              << ((this_local_trade_date_ / 10000) % 100) << "!";  // FMM00YY

  char exch_symbol_[16];

  memset(exch_symbol_, 0, 16);
  memset(exch_symbol_, ' ', 4);

  int expiry_offset_ = 4;

  memcpy(exch_symbol_, _pure_basename_.substr(2).c_str(), _pure_basename_.substr(2).length());
  memcpy(exch_symbol_ + expiry_offset_, expiry_oss_.str().c_str(), 8);

  return exch_symbol_;
}

/// IND, WIN
/// Last Trading Day : The wednesday closest to the 15th calendar day of the contract month. SHould this day be a
/// holiday or a nontrading day
/// then the last trading day will be the day following that
/// DOL
/// Last Trading Day : The last business day of the month preceeding the contract month
/// DI
/// Last Trading Day : The last business day of the month preceeding the fist business day of the contract month
/// ISP
/// Last Trading Day : The last business day before the third Friday ( if not business day, use the following )  of
/// the
/// contract month
/// CCM
/// Last Trading Day : The fifteenthday of the delivery month. If that day is a holiday or is not a trading day
/// atBM&FBOVESPA, the last trading day shall be the following business day.
/// SJC
/// Last Trading Day : The second business day preceding the contract month.
/// ICF
/// Last Trading Day : 6th business day preceding the last day of thedelivery month
int ExchangeSymbolManager::GetBMFLastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_bmf_month_,
                                                       const int next_bmf_year_) {
  if (_pure_basename_.compare(BMF_ISP) == 0) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, next_bmf_month_);
    boost::gregorian::date d1 = ndm.get_date(next_bmf_year_);
    do {
      d1 -= one_day_date_duration;
    } while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday);

    d1 -= one_day_date_duration;  // one more day to be safe
    return YYYYMMDD_from_date(d1);
  }
  if (_pure_basename_.compare(BMF_CCM) == 0) {
    boost::gregorian::date d1(next_bmf_year_, next_bmf_month_, 15);

    d1 -= one_day_date_duration;  // just to be safe
    while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday) {
      d1 -= one_day_date_duration;  // for safety
    }
    return YYYYMMDD_from_date(d1);
  }
  if (_pure_basename_.compare(BMF_SJC) == 0) {
    boost::gregorian::date d1(next_bmf_year_, next_bmf_month_, 1);
    do {
      d1 -= one_day_date_duration;
    } while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday);
    do {
      d1 -= one_day_date_duration;
    } while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday);

    d1 -= one_day_date_duration;  // one more day to be safe
    return YYYYMMDD_from_date(d1);
  }
  if (_pure_basename_.compare(BMF_ICF) == 0) {
    boost::gregorian::date d1(next_bmf_year_ + (next_bmf_month_ / 12), (next_bmf_month_ + 1) % 12, 1);
    d1 -= one_day_date_duration;

    for (unsigned int i = 1; i <= 6; i++) {
      do {
        d1 -= one_day_date_duration;
      } while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday);
    }

    d1 -= one_day_date_duration;  // one more day to be safe
    return YYYYMMDD_from_date(d1);
  }
  if ((_pure_basename_.compare(BMF_IND) == 0) || (_pure_basename_.compare(BMF_WIN) == 0)) {
    boost::gregorian::date d1(next_bmf_year_, next_bmf_month_, 15);
    // Assuming closest means strictly <= as well.
    while (d1.day_of_week() != boost::gregorian::Wednesday) {
      d1 -= one_day_date_duration;
    }

    int d1_day_ = YYYYMMDD_from_date(d1) % 100;

    // bugfix : docs mentions last trading date as wednesday closest to 15the of expiry month, not necessarily
    // being prior to 15th

    // get wednesday after 15th
    boost::gregorian::date d2(next_bmf_year_, next_bmf_month_, 15);

    while (d2.day_of_week() != boost::gregorian::Wednesday) {
      d2 += one_day_date_duration;
    }

    int d2_day_ = YYYYMMDD_from_date(d2) % 100;

    // last trading date is post 15th
    if ((d2_day_ - 15) < (15 - d1_day_)) {
      d2 -= one_day_date_duration;
      return YYYYMMDD_from_date(d2);
    }

    // TODO -- Need to look at the holiday thing
    d1 -= one_day_date_duration;  // one more day to be safe
    return YYYYMMDD_from_date(d1);
  }

  if ((_pure_basename_.compare(BMF_WDO) == 0) || (_pure_basename_.compare(BMF_DI) == 0) ||
      (_pure_basename_.compare(BMF_SP_DI) == 0) || (_pure_basename_.compare(BMF_DOL) == 0)) {
    boost::gregorian::date d1(next_bmf_year_, next_bmf_month_, 1);

    do {
      d1 -= one_day_date_duration;
    } while (d1.day_of_week() == boost::gregorian::Sunday || d1.day_of_week() == boost::gregorian::Saturday);

    d1 -= one_day_date_duration;  // one more day to be safe
    return YYYYMMDD_from_date(d1);
  }

  ExitVerbose(kExchangeSymbolManagerUnhandledCase, _pure_basename_.c_str());

  // default day
  boost::gregorian::date d1(next_bmf_year_, next_bmf_month_, 1);
  d1 -= one_day_date_duration;
  // TODO -- Need to look at the holiday thing
  d1 -= one_day_date_duration;  // one more day to be safe
  return YYYYMMDD_from_date(d1);
}
bool ExchangeSymbolManager::IsOSEMonth(const std::string &_pure_basename_, const int _this_month_) {
  // Front Month Mini
  if (OSE_NKMF == _pure_basename_ && (_this_month_ % 3) != 0) {
    return true;
  }

  return ((_this_month_ % 3) == 0);
}

bool ExchangeSymbolManager::IsOSEFrontMonth(const std::string &_pure_basename_, const int _this_month_) {
  return ((_this_month_ % 3) != 0);
}

std::string ExchangeSymbolManager::GetBMFSymbolFromLastTradingDate(const std::string &_pure_basename_,
                                                                   const int current_min_last_trading_date) {
  std::string basename_minus_BR_ = _pure_basename_.substr(3);  // first three are expected to be BR_

  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000) % 100;
  if ((_pure_basename_.compare(BMF_DOL) == 0) ||
      (_pure_basename_.compare(BMF_WDO) == 0)) {  // weirdness since DOL expires in the month befoe the BMF_month
    if (current_min_last_trading_date_mm == 12) {
      current_min_last_trading_date_mm = 1;
      current_min_last_trading_date_yy++;
    } else {
      current_min_last_trading_date_mm++;
    }
  }

  std::stringstream ss;
  ss << basename_minus_BR_ << CMEMonthCode[current_min_last_trading_date_mm - 1] << current_min_last_trading_date_yy;
  return ss.str();
}

/// Retuns true if the given product will be traded on this day
inline bool IsTMXExchangeDate(const std::string &_pure_basename_, const boost::gregorian::date &d1) {
  // TODO only using weekends as holidays, need to add TMX holiday from Quantlib
  return ((d1.day_of_week() != boost::gregorian::Saturday) && (d1.day_of_week() != boost::gregorian::Sunday));
}

/// SXF
/// Settlement Day :
/// Third Friday of the respective quarterly month, if this day is an exchange day; otherwise, 1st preceeding day.
/// Last Trading Day :
/// One trading day prior to the Settlement Day of the relevant maturity month.
///
/// CGB
/// Settlement, Last Trading Day:
/// Trading finishes at 1 pm Montreal Time on 7th business day preceding last trading day of delivery month.
///
/// BAX
/// Settlement, Last Trading Day:
/// Trading ceases at 10 am on the 2nd UK business day preceding third Wednesday of contract month; in case of holiday
/// previous business day
int ExchangeSymbolManager::GetTMXLastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_tmx_month_,
                                                       const int next_tmx_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");

  if (_pure_basename_.compare(TMX_SXF) == 0 || _pure_basename_.compare(TMX_EMF) == 0) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, next_tmx_month_);
    boost::gregorian::date d1 = ndm.get_date(next_tmx_year_);

    // last trading day is one day prior
    int last_trading_date_ = GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 1);

    // Roll two more days before last trading day based on observation
    return GetDateNBusinessDaysBefore(shortcode, last_trading_date_, 2);
  }

  if ((_pure_basename_.compare(TMX_CGB) == 0) || (_pure_basename_.compare(TMX_CGBY) == 0) ||
      (_pure_basename_.compare(TMX_CGF) == 0) || (_pure_basename_.compare(TMX_CGZ) == 0)) {
    /// get last trading day of month
    // boost::gregorian::date d1 ( ( next_tmx_month_ == 12 )?( next_tmx_year_ + 1 ):next_tmx_year_, ( next_tmx_month_
    // ==
    // 12 )?1:(next_tmx_month_ + 1), 1 );

    boost::gregorian::date d1(next_tmx_year_, next_tmx_month_, 1);
    // First bussiness day of the month
    while (!IsTMXExchangeDate(_pure_basename_, d1)) {
      d1 += one_day_date_duration;
    }

    // d1 -= one_day_date_duration;
    // while ( ! IsTMXExchangeDate ( _pure_basename_, d1 ) )
    //   {
    //     d1 -= one_day_date_duration;
    //   }

    // for ( unsigned int i = 0 ; i < 7 ; i ++ )
    //   {
    // 3 business days prior to the first business day of the delivery month

    // 2 more days
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 5);
  }

  if (_pure_basename_.compare(TMX_BAX) == 0) {
    // third wednesday
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Wednesday, next_tmx_month_);
    boost::gregorian::date d1 = ndm.get_date(next_tmx_year_);
    // 2nd london banking day prior+last trading day is one day prior
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 3);
  }

  ExitVerbose(kExchangeSymbolManagerUnhandledCase, _pure_basename_.c_str());

  boost::gregorian::date d1(next_tmx_year_, next_tmx_month_, 5);
  return YYYYMMDD_from_date(d1);
}

std::string ExchangeSymbolManager::GetTMXSymbolFromLastTradingDate(const std::string &_pure_basename_,
                                                                   const int current_min_last_trading_date) {
  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000) % 10;

  /// For CGB : first notice date is the 3rd last day of the previous month
  /// we expect to stop trading a 2 day before and rollover
  if ((_pure_basename_.compare(TMX_CGB) == 0) || (_pure_basename_.compare(TMX_CGBY) == 0) ||
      (_pure_basename_.compare(TMX_CGF) == 0) || (_pure_basename_.compare(TMX_CGZ) == 0)) {
    if (current_min_last_trading_date_mm == 12) {
      current_min_last_trading_date_mm = 1;
      current_min_last_trading_date_yy++;
    } else {
      current_min_last_trading_date_mm++;
    }
  }

  std::stringstream ss;
  ss << _pure_basename_ << CMEMonthCode[current_min_last_trading_date_mm - 1] << current_min_last_trading_date_yy;
  return ss.str();
}

int ExchangeSymbolManager::GetRTSLastTradingDateYYYYMM(const std::string _pure_basename_, const int next_rts_month_,
                                                       const int next_rts_year_) {
  // last trading day is 15th but volume shifts one day before that, so using 14th
  // For BR_0 last trading day is 1st working day => we stop trading 2 working days before this
  std::string shortcode = _pure_basename_ + std::string("_0");

  if (_pure_basename_.compare(RTS_BR) == 0) {
    int temp_month = next_rts_month_;
    int temp_year = next_rts_year_;
    SetToNextRTSMonth(_pure_basename_, temp_month, temp_year);
    boost::gregorian::date d1(temp_year, temp_month, 1);  // 1st day of next month
    // stop trading two days before last trading day
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
  }

  boost::gregorian::date d1(next_rts_year_, next_rts_month_, 14);
  return YYYYMMDD_from_date(d1);
}

std::string ExchangeSymbolManager::GetRTSSymbolFromLastTradingDate(const std::string _pure_basename_,
                                                                   const int current_min_last_trading_date) {
  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000) % 10;

  int contract_month_ = current_min_last_trading_date_mm;
  int contract_year_ = current_min_last_trading_date_yy;

  // Expiry happens 2 days before the current month for BR
  if (_pure_basename_.compare(RTS_BR) == 0) {
    SetToNextRTSMonth(_pure_basename_, contract_month_, contract_year_);
  }

  std::stringstream ss;
  ss << _pure_basename_ << CMEMonthCode[contract_month_ - 1] << contract_year_;
  return ss.str();
}

// mapping shifts a day prior to 2nd Friday, this doens't mean 2nd Thursday, consider 1st day of the month is Friday
int ExchangeSymbolManager::GetOSELastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_tmx_month_,
                                                       const int next_tmx_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");

  int local_ose_month = next_tmx_month_;
  int local_ose_year = next_tmx_year_;

  if (OSE_NKMF == _pure_basename_ && (!IsOSEFrontMonth(_pure_basename_, local_ose_month))) {
    SetToNextOSEMonth(_pure_basename_, local_ose_month, local_ose_year);

    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Friday, local_ose_month);
    boost::gregorian::date d1 = ndm.get_date(local_ose_year);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
  }

  if (std::string(OSE_JGBL) == _pure_basename_) {
    // 20the day of each contract month, move down when it is not a business day
    boost::gregorian::date d1(next_tmx_year_, next_tmx_month_, 20);

    if (YYYYMMDD_ > JGBL_LAST_TRADING_DATE_CHANGE) {
      // There will be 1 weekend in this
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 6);

    } else {
      std::string day_of_week_ = d1.day_of_week().as_long_string();

      // There will be 1 weekend in this
      d1 -= eleven_day_date_duration;

      if ("Sunday" == day_of_week_) {
        // 3 wekend days to skip
        d1 -= one_day_date_duration;

      } else if ("Monday" == day_of_week_ || "Tuesday" == day_of_week_ || "Wednesday" == day_of_week_) {
        // double weekend
        d1 -= two_day_date_duration;
      }
    }
    return YYYYMMDD_from_date(d1);
  }

  boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                     boost::gregorian::Friday, next_tmx_month_);
  boost::gregorian::date d1 = ndm.get_date(next_tmx_year_);

  return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(d1), 2);
}
// returns the second last day of the month
int ExchangeSymbolManager::GetHKEXLastTradingDay(const std::string &_pure_basename_, const int next_tmx_month_,
                                                 const int next_tmx_year_) {
  // the last trading day will be the third last business day of the month
  std::string shortcode = _pure_basename_ + std::string("_0");
  int day = boost::gregorian::gregorian_calendar::end_of_month_day(next_tmx_year_, next_tmx_month_);
  boost::gregorian::date last_rading_date_(next_tmx_year_, next_tmx_month_, day);
  return (GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(last_rading_date_), 2)) % 100;
}

int ExchangeSymbolManager::GetCFELastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_cfe_month_,
                                                       const int next_cfe_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");

  boost::gregorian::date last_trading_date_;

  if (_pure_basename_.substr(0, 3) == "VSW") {
    // VSW: The final settlement date for a VXST futures contract is on the Wednesday of the week of the month
    //      denoted in the ticker symbol of the contract ("Final Settlement Date"). If the Wednesday is a CBOE holiday
    //      or if the Friday in the business week following the Wednesday (i.e., nine days away) is a CBOE holiday,
    //      then the Final Settlement Date shall be the business day immediately preceding the Wednesday.

    int week_number_ = atoi(_pure_basename_.substr(3, 1).c_str());

    // boost::gregorian::nth_day_of_the_week_in_month ndm ( boost::gregorian::nth_day_of_the_week_in_month::first,
    // boost::gregorian::Wednesday, next_cfe_month_ );
    boost::gregorian::nth_day_of_the_week_in_month ndm(
        (boost::gregorian::nth_day_of_the_week_in_month::week_num)week_number_, boost::gregorian::Wednesday,
        next_cfe_month_);
    boost::gregorian::date settlement_date_ = ndm.get_date(next_cfe_year_);

    if (IsCFEHoliday(YYYYMMDD_from_date(settlement_date_)) ||
        IsCFEHoliday(YYYYMMDD_from_date(settlement_date_ + nine_day_date_duration))) {
      return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(settlement_date_), 2);
    }

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(settlement_date_), 1);

  } else {
    // VX futures: Settlement date: 30 days prior to the third Friday of the calendar month following the current
    // month.
    // Note that this has to be a Wednesday.
    // VX futures: Last trading date: settlement date - 1

    int local_cfe_month = next_cfe_month_;
    int local_cfe_year = next_cfe_year_;

    if (local_cfe_month == 12) {
      local_cfe_month = 1;
      local_cfe_year++;
    } else {
      local_cfe_month++;
    }

    // VX_ securities
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Friday, local_cfe_month);
    boost::gregorian::date d1 = ndm.get_date(local_cfe_year);

    // If this is a holiday, skip to the preceding business day
    while (IsCFEHoliday(YYYYMMDD_from_date(d1))) {
      d1 = d1 - one_day_date_duration;
    }

    boost::gregorian::date settlement_date_ = d1 - thirty_day_date_duration;
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(settlement_date_), 2);
  }

  return YYYYMMDD_from_date(last_trading_date_);
}

std::string ExchangeSymbolManager::GetCFESymbolFromLastTradingDate(std::string _pure_basename_,
                                                                   const int current_min_last_trading_date,
                                                                   bool _is_spread_, bool _is_expiry_) {
  int current_min_last_trading_date_mm = (current_min_last_trading_date / 100) % 100;
  int current_min_last_trading_date_yy = (current_min_last_trading_date / 10000);

  int contract_month_ = current_min_last_trading_date_mm;
  int contract_year_ = current_min_last_trading_date_yy;

  if (_is_expiry_) {
    if (contract_month_ == 1) {
      contract_month_ = 12;
      contract_year_--;
    } else {
      contract_month_--;
    }
  }

  if (!_is_spread_) {
    std::stringstream ss;
    ss << std::setfill('0');
    ss << _pure_basename_ << contract_year_ << std::setw(2) << contract_month_;
    return ss.str();

  } else {
    std::stringstream ss;
    ss << _pure_basename_ << CMEMonthCode[contract_month_ - 1] << current_min_last_trading_date_yy % 10;
    return ss.str();
  }
}

int ExchangeSymbolManager::GetASXLastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_asx_month_,
                                                       const int next_asx_year_) {
  boost::gregorian::date last_trading_date_;
  std::string shortcode = _pure_basename_ + std::string("_0");

  if (_pure_basename_.compare("XT") == 0 || _pure_basename_.compare("YT") == 0 || _pure_basename_.compare("XTE") == 0 ||
      _pure_basename_.compare("YTE") == 0 || _pure_basename_.compare("YTY") == 0 ||
      _pure_basename_.compare("XTY") == 0 || _pure_basename_.compare("XX") == 0) {
    // Officially 15th day of the last trading month
    // we don't roll back since officially it is rolled over to next business day in future

    int last_trading_date = (next_asx_year_ * 10000 + next_asx_month_ * 100 + 15);
    return GetDateNBusinessDaysBefore(shortcode, last_trading_date, 2);

  } else if (_pure_basename_.compare("AP") == 0) {
    // Officially third Thursday is the last trading day
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::third,
                                                       boost::gregorian::Thursday, next_asx_month_);
    boost::gregorian::date last_trading_date_ = ndm.get_date(next_asx_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(last_trading_date_), 2);

  } else if (_pure_basename_.compare("IR") == 0 ||
             _pure_basename_.compare("IB") == 0)  // TODO: this isn't correct - fix this
  {
    // Officially second Thrusday is the last trading day ( Friday is the settlement day )
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Thursday, next_asx_month_);
    boost::gregorian::date last_trading_date_ = ndm.get_date(next_asx_year_);

    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(last_trading_date_), 2);
  }

  return 19700101;
}

int ExchangeSymbolManager::GetKRXLastTradingDateYYYYMM(const std::string &_pure_basename_, const int next_krx_month_,
                                                       const int next_krx_year_) {
  std::string shortcode = _pure_basename_ + std::string("_0");
  boost::gregorian::date last_trading_date_;
  if (_pure_basename_.compare("KOSPI") == 0) {
    boost::gregorian::nth_day_of_the_week_in_month ndm(boost::gregorian::nth_day_of_the_week_in_month::second,
                                                       boost::gregorian::Thursday, next_krx_month_);
    boost::gregorian::date last_trading_date_ = ndm.get_date(next_krx_year_);
    return GetDateNBusinessDaysBefore(shortcode, YYYYMMDD_from_date(last_trading_date_), 2);
  }
  return 19700101;
}
}  // namespace HFSAT
