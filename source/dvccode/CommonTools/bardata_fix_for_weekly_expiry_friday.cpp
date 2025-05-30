#include <iostream>
#include <fstream>
#include <stdio.h>
#include <map>
#include <vector>
#include <string.h>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
const boost::gregorian::date_duration one_day_duration_(1);

int ComputeFridayNextExpiryWeeklyOptions(const int t_date_ ) {
  boost::gregorian::date d1((t_date_ / 10000) % 10000, ((t_date_ / 100) % 100), (t_date_ % 100));
  boost::gregorian::greg_weekday gw(boost::gregorian::Friday);
  boost::gregorian::date next_friday = next_weekday(d1, gw);

  boost::gregorian::date current_day = next_friday;
  boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
  int t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
    current_day -= one_day_duration_;
    boost::gregorian::date::ymd_type ymd = current_day.year_month_day();
    t_expiry_date_ = ((ymd.year * 100 + ymd.month) * 100) + ymd.day;
  }

  if (t_expiry_date_ >= t_date_) {
    return t_expiry_date_;
  } else {
    boost::gregorian::date next_friday = next_friday + boost::gregorian::date_duration(3);
    ymd = next_friday.year_month_day();
    return ComputeFridayNextExpiryWeeklyOptions(((ymd.year * 100 + ymd.month) * 100) + ymd.day);
  }
}

using namespace std;
std::vector<std::string> time_to_bar;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "expecting :\n"
              << " BARDATA ---- OUTFILE" << '\n';
    return 0;
  }
  ifstream barFile1, barFile2;
  ofstream barFile3;
  std::string file1, file2, out_file;
  file1 = argv[1];
  out_file = argv[2];
  std::cout << "FILE1: " << file1 << " FILE2: " << file2 << " OUTFILE: " << out_file << std::endl;
  int startdate_ = 20160101;
  int enddate = 20590318;
  //  boost::gregorian::date d1((enddate / 10000) % 10000, ((enddate / 100) % 100), (enddate % 100));
  vector<int> expiry_for_weekly;
  while (startdate_ < enddate) {
    int week_exp = ComputeFridayNextExpiryWeeklyOptions(startdate_);
    startdate_ = HFSAT::DateTime::CalcNextDay(startdate_);
    expiry_for_weekly.push_back(week_exp);
  }
  sort(expiry_for_weekly.begin(), expiry_for_weekly.end());
  expiry_for_weekly.erase(unique(expiry_for_weekly.begin(), expiry_for_weekly.end()), expiry_for_weekly.end());
  barFile1.open(file1, std::ifstream::in);
//   for (auto i : expiry_for_weekly) {
//     cout << i << std::endl;
//   }
  if (!barFile1.is_open()) {
    std::cerr << "UNABLE TO FILE FOR READING :" << file1 << std::endl;
    return 0;
  }
  std::cout << "READING FILE 1 " << file1 << std::endl;
  char line_[2048];
  char buffer_[128];
  int count = 0;
  while (!barFile1.eof()) {
    memset(line_, 0, sizeof(line_));
    barFile1.getline(line_, sizeof(line_));
    string data_com_bar_for_cout = line_;
    std::vector<char *> tokens_;
    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(line_, "\t", tokens_);
    // std::cout << "Token Size " << tokens_.size() << std::endl;
    if (tokens_.size() < 11) {
      std::cerr << "Bar Data File Error " << file1 << " : IGNORING LINE : " << data_com_bar_for_cout << std::endl;
      continue;
    } else if (strcmp(tokens_[9], "0") == 0 && strcmp(tokens_[10], "0") == 0) {
      // std::cerr << "Bar1 Data File Error " << file1 << " : IGNORING LINE : " << data_com_bar_for_cout << std::endl;
      count++;
      continue;
    }

    std::stringstream tmp_shortcode_;
    tmp_shortcode_ << tokens_[0] << "_" << tokens_[1];
    time_t time = (time_t)(atoi(tokens_[0]));
    struct tm tm_curr = *localtime(&time);

    int yyyymmdd = (1900 + tm_curr.tm_year) * 10000 + (1 + tm_curr.tm_mon) * 100 + tm_curr.tm_mday;
    while (yyyymmdd > expiry_for_weekly[0]) expiry_for_weekly.erase(expiry_for_weekly.begin());
    int increment = 0;
    while (atoi(tokens_[4]) > expiry_for_weekly[increment]) increment++;
    if (increment > 2) {
      // std::cout << "Far Week Expiry " << data_com_bar_for_cout << std::endl;
      continue;
    }
    std::vector<char *> tokens_short;
    bzero(buffer_, 128);
    strcpy(buffer_, tokens_[1]);
    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(buffer_, "_", tokens_short);
    char sht_code[50];
    strncpy(sht_code, tokens_short[0], sizeof(sht_code));
    strncat(sht_code, "_", sizeof(sht_code)-1);
    strncat(sht_code, tokens_short[1], sizeof(sht_code)-1);
    strncat(sht_code, "_", sizeof(sht_code)-1);
    strncat(sht_code, tokens_short[2], sizeof(sht_code)-1);
    string tmp = "_" + to_string(increment) + "_W";
    strncat(sht_code, tmp.c_str(), sizeof(sht_code)-1);
    string data_com_bar_tmp = std::string(tokens_[0]) + "\t" + std::string(sht_code) + "\t" + std::string(tokens_[2]) +
                              "\t" + std::string(tokens_[3]) + "\t" + std::string(tokens_[4]) + "\t" +
                              std::string(tokens_[5]) + "\t" + std::string(tokens_[6]) + "\t" +
                              std::string(tokens_[7]) + "\t" + std::string(tokens_[8]) + "\t" +
                              std::string(tokens_[9]) + "\t" + std::string(tokens_[10]);
    // + "\t" +                              std::string(tokens_[11]) + "\t" + std::string(tokens_[12]);

    // cout << "DATA: " << data_com_bar_tmp << std::endl;
    time_to_bar.push_back(data_com_bar_tmp);
  }
  cout << "TOT Lines: " << time_to_bar.size() << " Lines Ignored " << count << std::endl;
  cout << "OUT FILE " << out_file << std::endl;
  barFile3.open(out_file, std::ofstream::out);
  for (auto bardata : time_to_bar) {
    // std::cout << bardata.first << " : " << bardata.second << std::endl;
    barFile3 << bardata << std::endl;
  }

  barFile1.close();
  barFile3.close();
  return 0;
}
