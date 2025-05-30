/**
    \file CDefCode/currency_convertor.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/email_utils.hpp"

namespace HFSAT {

CurrencyConvertor* CurrencyConvertor::p_uniqueinstance_ = NULL;

Currency_t GetCurrencyFromString(const char* cc_str_) {
  if (strcmp(cc_str_, "USD") == 0)
    return kCurrencyUSD;
  else if (strcmp(cc_str_, "BRL") == 0)
    return kCurrencyBRL;
  else if (strcmp(cc_str_, "EUR") == 0)
    return kCurrencyEUR;
  else if (strcmp(cc_str_, "JPY") == 0)
    return kCurrencyJPY;
  else if (strcmp(cc_str_, "HKD") == 0)
    return kCurrencyHKD;
  else if (strcmp(cc_str_, "SGD") == 0)
    return kCurrencySGD;
  else if (strcmp(cc_str_, "MXN") == 0)
    return kCurrencyMXN;
  else if (strcmp(cc_str_, "INR") == 0)
    return kCurrencyINR;
  else if (strcmp(cc_str_, "GBP") == 0)
    return kCurrencyGBP;
  else if (strcmp(cc_str_, "CAD") == 0)
    return kCurrencyCAD;
  else if (strcmp(cc_str_, "RUB") == 0)
    return kCurrencyRUB;
  else if (strcmp(cc_str_, "AUD") == 0)
    return kCurrencyAUD;
  else if (strcmp(cc_str_, "CHF") == 0)
    return kCurrencyCHF;
  else if (strcmp(cc_str_, "KRW") == 0)
    return kCurrencyKRW;
  return kCurrencyUSD;
}

CurrencyPair_t GetCurrencyPairFromString(const char* cc_str_) {
  if (strcmp(cc_str_, "USDBRL") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyBRL);
  }
  if (strcmp(cc_str_, "BRLUSD") == 0) {
    return CurrencyPair_t(kCurrencyBRL, kCurrencyUSD);
  }

  if (strcmp(cc_str_, "EURUSD") == 0) {
    return CurrencyPair_t(kCurrencyEUR, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDEUR") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyEUR);
  }

  if (strcmp(cc_str_, "USDJPY") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyJPY);
  }
  if (strcmp(cc_str_, "JPYUSD") == 0) {
    return CurrencyPair_t(kCurrencyJPY, kCurrencyUSD);
  }

  if (strcmp(cc_str_, "USDHKD") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyHKD);
  }
  if (strcmp(cc_str_, "HKDUSD") == 0) {
    return CurrencyPair_t(kCurrencyHKD, kCurrencyUSD);
  }

  if (strcmp(cc_str_, "SGDUSD") == 0) {
    return CurrencyPair_t(kCurrencySGD, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDSGD") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencySGD);
  }

  if (strcmp(cc_str_, "MXNUSD") == 0) {
    return CurrencyPair_t(kCurrencyMXN, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDMXN") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyMXN);
  }

  if (strcmp(cc_str_, "INRUSD") == 0) {
    return CurrencyPair_t(kCurrencyINR, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDINR") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyINR);
  }

  if (strcmp(cc_str_, "GBPUSD") == 0) {
    return CurrencyPair_t(kCurrencyGBP, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDGBP") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyGBP);
  }

  if (strcmp(cc_str_, "USDCAD") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyCAD);
  }
  if (strcmp(cc_str_, "CADUSD") == 0) {
    return CurrencyPair_t(kCurrencyCAD, kCurrencyUSD);
  }

  if (strcmp(cc_str_, "RUBUSD") == 0) {
    return CurrencyPair_t(kCurrencyRUB, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDRUB") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyRUB);
  }

  if (strcmp(cc_str_, "AUDUSD") == 0) {
    return CurrencyPair_t(kCurrencyAUD, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDAUD") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyAUD);
  }

  if (strcmp(cc_str_, "CHFUSD") == 0) {
    return CurrencyPair_t(kCurrencyCHF, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDCHF") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyCHF);
  }
  if (strcmp(cc_str_, "KRWUSD") == 0) {
    return CurrencyPair_t(kCurrencyKRW, kCurrencyUSD);
  }
  if (strcmp(cc_str_, "USDKRW") == 0) {
    return CurrencyPair_t(kCurrencyUSD, kCurrencyKRW);
  }

  Currency_t base_currency_ = GetCurrencyFromString(cc_str_);
  Currency_t counter_currency_ = GetCurrencyFromString(cc_str_ + 3);

  return CurrencyPair_t(base_currency_, counter_currency_);  // error case
}

bool CurrencyConvertor::LoadCurrencyDataFromReports() {
  std::string t_currency_info_filename_ = "NO_FILE_RET";
  int this_yyyymmdd_ = date_;

  for (unsigned int days_counter_ = 0; days_counter_ < 360; days_counter_++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << CURRENCY_RATES_REPORTS_DIR << "currency_rates_" << this_yyyymmdd_ << ".txt";

    t_currency_info_filename_ = t_temp_oss_.str();

    if (FileUtils::ExistsAndReadable(t_currency_info_filename_)) {
      break;
    } else {
      this_yyyymmdd_ = DateTime::CalcPrevDay(this_yyyymmdd_);
    }
  }

  if (!FileUtils::ExistsAndReadable(t_currency_info_filename_)) {
    return false;
  }

  std::ifstream currency_rates_file_;

  currency_rates_file_.open(t_currency_info_filename_.c_str(), std::ifstream::in);

  if (!currency_rates_file_.is_open()) {
    std::cerr << " Error Opening Currency Rates File\n";
    return false;
  }

  const int kSpreadInfoLineLen = 1024;
  char readline_buffer_[kSpreadInfoLineLen];

  while (currency_rates_file_.good()) {
    memset(readline_buffer_, 0, kSpreadInfoLineLen);

    currency_rates_file_.getline(readline_buffer_, kSpreadInfoLineLen);
    PerishableStringTokenizer st_(readline_buffer_, kSpreadInfoLineLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if (tokens_.size() == 2) {
      const char* t_this_cc_pair_str_ = tokens_[0];
      double t_this_currency_val_ = atof(tokens_[1]);

      if (t_this_currency_val_ == 0) {  // format error ... value cannot be 0
        // currency_rates_file_.close( ) ;
        // return false ;
        continue;
      }

      CurrencyPair_t ccpair_ = GetCurrencyPairFromString(t_this_cc_pair_str_);

      if (ccpair_ != CurrencyPair_t(kCurrencyUSD, kCurrencyUSD)) {
        one_in_[ccpair_] = t_this_currency_val_;
        one_in_[ccpair_.Invert()] = 1.00 / t_this_currency_val_;
      } else {
        // currency_rates_file_.close () ;
        // return false ;
        continue;
      }
    }
  }

  currency_rates_file_.close();
  return true;
}

void CurrencyConvertor::LoadOfflineCurrencyData() {
  if (LoadCurrencyDataFromReports()) return;  // report available

  std::string t_currency_info_filename_ = "NO_FILE_RET";

  int this_yyyymmdd_ = date_;
  for (auto i = 0u; i < 360; i++) {  // try 360 days back from this day to get the last time currency_info was updated
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << BASESYSINFODIR << "CurrencyInfo/currency_info_" << this_yyyymmdd_ << ".txt";
    t_currency_info_filename_ = HFSAT::FileUtils::AppendHome(t_temp_oss_.str());
    if (FileUtils::ExistsAndReadable(t_currency_info_filename_)) {
      break;
    } else {
      this_yyyymmdd_ = DateTime::CalcPrevDay(this_yyyymmdd_);
    }
  }

  bool file_readable = true;
  {
    if (!FileUtils::ExistsAndReadable(t_currency_info_filename_)) {  // setting to default file
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << BASESYSINFODIR << "CurrencyInfo/currency_info_default.txt";
      t_currency_info_filename_ = HFSAT::FileUtils::AppendHome(t_temp_oss_.str());
      if (!(FileUtils::ExistsAndReadable(t_currency_info_filename_))) {
        fprintf(stderr, "Default CurrenyInfoFile %s doesn't exist. Continuing with default values !\n",
                t_currency_info_filename_.c_str());
        HFSAT::Email email_;
        email_.addSender("nseall@tworoads.co.in");
        email_.addRecepient("nseall@tworoads.co.in");
        email_.setSubject("CurrenyInfoFile not readable");
        char host_name[256];
        gethostname(host_name, 256);
        email_.content_stream << "Using default currency values: server " << host_name
                              << "; File: " << t_currency_info_filename_;
        email_.sendMail();
        // exit ( 0 );
        file_readable = false;  // File is not readable=> don't try to read it
      }
    }
  }

  {
    std::ifstream currency_info_file_;
    if (file_readable) {
      currency_info_file_.open(t_currency_info_filename_.c_str(), std::ifstream::in);
    }
    if (file_readable && currency_info_file_.is_open()) {
      const int kSpreadInfoLineLen = 1024;
      char readline_buffer_[kSpreadInfoLineLen];
      bzero(readline_buffer_, kSpreadInfoLineLen);

      while (currency_info_file_.good()) {
        bzero(readline_buffer_, kSpreadInfoLineLen);
        currency_info_file_.getline(readline_buffer_, kSpreadInfoLineLen);
        PerishableStringTokenizer st_(readline_buffer_, kSpreadInfoLineLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 2) {
          const char* t_this_cc_pair_str_ = tokens_[0];
          double t_this_currency_val_ = atof(tokens_[1]);
          CurrencyPair_t ccpair_ = GetCurrencyPairFromString(t_this_cc_pair_str_);
          if (ccpair_ != CurrencyPair_t(kCurrencyUSD, kCurrencyUSD)) {
            one_in_[ccpair_] = t_this_currency_val_;
            one_in_[ccpair_.Invert()] = 1.00 / t_this_currency_val_;
          }
        }
      }
      currency_info_file_.close();
    } else {
      // File was not accessible => initialize with default values
      /*
       * EURUSD 1.106900
              USDJPY 120.245731
              GBPUSD 1.526548
              USDHKD 7.755624
              USDCAD 1.244015
              USDBRL 2.982112
              USDRUB 61.080043
              USDINR 62.254766
              USDAUD 1.283314
              USDSEK 8.324149
              USDZAR 11.759238
              USDSGD 1.388479
              USDMXN 15.485528
              USDCHF 1.007189
              USDKRW 1143.45
       */
      // Need to update all the below 3 variables when adding a new currency
      const std::string currency_pair_strings[] = {"EURUSD", "USDJPY", "GBPUSD", "USDHKD", "USDCAD",
                                                   "USDBRL", "USDRUB", "USDINR", "USDAUD", "USDSEK",
                                                   "USDZAR", "USDSGD", "USDMXN", "USDCHF", "USDKRW"};
      const double currency_pair_values[] = {1.106900,  120.245731, 1.526548,  7.755624, 1.244015,
                                             2.982112,  61.080043,  62.254766, 1.283314, 8.324149,
                                             11.759238, 1.388479,   15.485528, 1.007189, 1143.45};
      int array_size = 15;

      for (int ctr = 0; ctr < array_size; ctr++) {
        CurrencyPair_t ccpair_ = GetCurrencyPairFromString(currency_pair_strings[ctr].c_str());
        if (ccpair_ != CurrencyPair_t(kCurrencyUSD, kCurrencyUSD)) {
          one_in_[ccpair_] = currency_pair_values[ctr];
          one_in_[ccpair_.Invert()] = 1.00 / currency_pair_values[ctr];
        }
      }
    }
  }
}
}
