/**
    \file Tools/make_indicator_list.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

std::vector<std::string> indicatorVector;
std::map<std::string, int> indicator2id;

int numDaysDataPresent = 0;

struct SingleIndRecord {
  int date;
  double value;
  int indicatorId;
};

struct DateValuePair {
  int date;
  double value;

  DateValuePair(const SingleIndRecord& rec) : date(rec.date), value(rec.value) {}
};

struct IndicatorRecVec {
  int indicatorId;
  std::vector<DateValuePair> data;
  std::vector<double> raw_values;

  double mean_;
  double median_;
  double stdev_;
  double std_penalized_mean_;
  double sharpe_;

  IndicatorRecVec(int id)
      : indicatorId(id), data(), raw_values(), mean_(0), median_(0), stdev_(0), std_penalized_mean_(0), sharpe_(0) {}

  // insert at a location which makes the data sorted on date in increasing order
  void add(const SingleIndRecord& rec) {
    if (rec.indicatorId != indicatorId) {
      std::cerr << "incorrect indicator value attempted to add\n";
      return;
    }

    DateValuePair dvp(rec);
    if (data.size() == 0 || data.back().date < rec.date) {
      data.push_back(dvp);
      return;
    }
    // find and insert at that point
    std::vector<DateValuePair>::iterator it = data.begin();
    while (it != data.end() && (*it).date < rec.date) it++;
    data.insert(it, dvp);  // also works when the loops is unable to find i
    numDaysDataPresent = std::max(numDaysDataPresent, (int)data.size());
  }

  void normalize() {
    raw_values.resize(data.size());

    for (auto i = 0u; i < data.size(); ++i) raw_values[i] = data[i].value;

    std::sort(raw_values.begin(), raw_values.end());
    if (raw_values.size() == 0) {
      std::cerr << "no data for indicator " << indicatorVector[indicatorId] << "\n";
      return;
    }

    median_ = raw_values[raw_values.size() / 2];

    for (auto i = 0u; i < data.size(); ++i) raw_values[i] = data[i].value;
    stdev_ = HFSAT::VectorUtils::GetStdevAndMean(raw_values, mean_);
    if (stdev_ > 0) {
      std_penalized_mean_ = mean_ * std::min(2.5, (fabs(mean_) / stdev_));
    } else {
      std_penalized_mean_ = mean_;
    }
    sharpe_ = ((stdev_ > 0) ? (mean_ / stdev_) : (mean_));
    HFSAT::VectorUtils::NormalizeData(raw_values, mean_, stdev_);
  }
};
std::vector<IndicatorRecVec> indicator_records;

bool compareIndicatorRecVec(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.median_) > fabs(r2.median_);
}

bool compareIndicatorRecVecByAbsMean(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.mean_) > fabs(r2.mean_);
}

bool compareIndicatorRecVecStdPenPNL(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.std_penalized_mean_) > fabs(r2.std_penalized_mean_);
}

bool compareIndicatorSharpe(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.sharpe_) > fabs(r2.sharpe_);
}

bool parseFromLine(char* line, SingleIndRecord* in, int startDate, int endDate) {
  HFSAT::PerishableStringTokenizer tokenizer(line, 1024);

  const std::vector<const char*> tokens = tokenizer.GetTokens();
  if (tokens.size() <= 3) return false;

  if (strcmp(tokens[1], "INDICATOR") != 0) return false;

  in->date = atoi(tokens[0]);

  if (in->date < startDate || in->date > endDate) return false;  // invalid date

  in->value = atof(tokens[2]);

  std::string indicatorString;
  for (unsigned int i = 3; i < tokens.size(); ++i) indicatorString += std::string(tokens[i]) + " ";

  if (indicator2id.find(indicatorString) == indicator2id.end()) {
    in->indicatorId = indicator2id.size();
    indicator2id[indicatorString] = in->indicatorId;
    indicatorVector.push_back(indicatorString);
    if (in->indicatorId == 0) indicator_records = std::vector<IndicatorRecVec>();
    indicator_records.push_back(IndicatorRecVec(in->indicatorId));
  } else
    in->indicatorId = indicator2id[indicatorString];

  return true;
}

bool parseFromLine(char* line, SingleIndRecord* in, std::vector<int> dates_vec) {
  HFSAT::PerishableStringTokenizer tokenizer(line, 1024);

  const std::vector<const char*> tokens = tokenizer.GetTokens();
  if (tokens.size() <= 3) return false;

  if (strcmp(tokens[1], "INDICATOR") != 0) return false;

  in->date = atoi(tokens[0]);

  if (std::find(dates_vec.begin(), dates_vec.end(), in->date) == dates_vec.end()) {
    return false;  // invalid date
  }

  in->value = atof(tokens[2]);

  std::string indicatorString;
  for (unsigned int i = 3; i < tokens.size(); ++i) indicatorString += std::string(tokens[i]) + " ";

  if (indicator2id.find(indicatorString) == indicator2id.end()) {
    in->indicatorId = indicator2id.size();
    indicator2id[indicatorString] = in->indicatorId;
    indicatorVector.push_back(indicatorString);
    if (in->indicatorId == 0) indicator_records = std::vector<IndicatorRecVec>();
    indicator_records.push_back(IndicatorRecVec(in->indicatorId));
  } else
    in->indicatorId = indicator2id[indicatorString];

  return true;
}

void interpretDateArg(char* ptr, int& date) {
  static const std::string TODAY = "TODAY";
  char* tmp = strstr(ptr, TODAY.c_str());
  if (tmp == NULL) {
    date = atoi(ptr);
    return;
  }
  date = atoi(ptr + TODAY.length());

  HFSAT::ttime_t t = HFSAT::GetTimeOfDay();
  t.tv_sec = t.tv_sec + 86400 * date;
  date = HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(t);
  //  std::cerr << "date " << ptr << " interpreted as " << date << "\n";
}

void removeOtherDependentOnes(int i, bool* selected, float corr_cutoff) {
  for (unsigned int j = 0; j < indicator_records.size(); ++j) {
    if (selected[j]) continue;
    double corr_i_j = HFSAT::GetCorrelation(indicator_records[i].raw_values, indicator_records[j].raw_values);
    if (fabs(corr_i_j) > corr_cutoff) {
      std::cerr << "rejected " << indicatorVector[indicator_records[i].indicatorId]
                << " --since its correlation with-- " << indicatorVector[indicator_records[j].indicatorId] << " --is-- "
                << corr_i_j << "\n";
      selected[j] = true;
    }
  }
  return;
}

void AddPriceType(std::string& indicator_line_string_, std::string _base_price_type_) {
  std::vector<std::string> indicator_list_ = {
      "BidAskToPay", "BidAskToPayCutoff", "DiffEDAvgTPxBasepx", "DiffEDSizeAvgTPxBasepx", "DiffPriceL1",
      "DiffTDAvgTPxBasepx", "DiffTDAvgTPxBasepxLvl", "DiffTDAvgTPxBasepxOneLvl", "DiffTDSizeAvgTPxBasepx",
      "DiffTDSizeAvgTPxBasepxLvl", "DiffTDSizeAvgTPxBasepxOneLvl", "DiffTRAvgTPxBasepx", "DiffTRSizeAvgTPxBasepx",
      "EventOwpDiffPriceL1", "FutureToSpotPricing", "MoorePenrose", "MultMidOrderPrice", "MultMidPrice",
      "MultMktComplexOrderPrice", "MultMktComplexPrice", "MultMktComplexPriceShortAvg", "MultMktComplexPriceTopOff",
      "MultMktOrderPrice", "MultMktOrderPriceTopOff", "MultMktPerOrderComplexPrice", "MultMktPrice", "OwpDiffPriceL1",
      "SizeWDiffTDSizeAvgTPxBasepx", "TimeOwpDiffPriceL1", "TradeBookAdjustedPrice"};
  for (unsigned i = 0; i < indicator_list_.size(); i++) {
    char _buffer_[512];
    memcpy(_buffer_, indicator_line_string_.c_str(), indicator_line_string_.size());
    HFSAT::PerishableStringTokenizer st_(_buffer_, indicator_line_string_.size());
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() < 2) {
      return;
    }
    if (indicator_list_[i].compare(std::string(tokens_[0])) != 0) {
      continue;
    }
    std::string price_type_ = std::string(tokens_[tokens_.size() - 1]);
    if (indicator_list_[i].compare("FutureToSpotPricing") == 0) {
      price_type_ = std::string(tokens_[tokens_.size() - 2]);
    }
    HFSAT::PriceType_t pt_ = HFSAT::StringToPriceType_t(price_type_);
    if (pt_ == HFSAT::kPriceTypeMax) {
      indicator_line_string_ = indicator_line_string_ + _base_price_type_;
      return;
    };
  }
}

int main(int argc, char** argv) {
  if (argc < 7) {
    std::cerr << argv[0] << " SHORTCODE INDICATOR_RECORD_FILE DEPBASE DEPPRED START_YYYYMMDD END_YYMMDD "
                            "MAX_CORR_THRESH -a <algorithm for comparision(0,1,2)> -i [definately choose nth (0,1..]] "
                            "-e [exclude_filename] -d [ set of dates to make indicator list on ]\n";
    std::cerr << "  YYYYMMDD can be a negative number to indicate offset from today\n";
    std::cerr << "  algo_options: 1 - std penalized mean. \t\t2 - sharpe ratio \n";

    exit(1);
  }

  char* shortcode = argv[1];
  char* indicator_record_file = argv[2];
  char* dep_base = argv[3];
  char* dep_pred = argv[4];

  bool date_vector_given = false;
  int start_date;
  int end_date;
  std::vector<int> dates_vec;
  interpretDateArg(argv[5], start_date);
  interpretDateArg(argv[6], end_date);

  double corr_cutoff = atof(argv[7]);

  int choice_comp_algo = 1;
  int rn = 0;
  char* exclude_indicators_file = NULL;
  int i = 8;

  while (i < argc) {
    if (strncmp(argv[i], "-a", 2) == 0) {
      choice_comp_algo = atoi(argv[++i]);
    }

    else if (strncmp(argv[i], "-i", 2) == 0) {
      rn = atoi(argv[++i]);
    }

    else if (strncmp(argv[i], "-e", 2) == 0) {
      exclude_indicators_file = argv[++i];
    }

    else if (strncmp(argv[i], "-d", 2) == 0) {
      date_vector_given = true;
      int j = i + 1;
      for (; j < argc; j++) {
        dates_vec.push_back(atoi(argv[j]));
        i++;
      }
    }

    else {
      std::cout << "Unknown parameter options included" << argv[i];
      return -1;
    }

    i++;
  }

  HFSAT::BulkFileReader reader;
  reader.open(indicator_record_file);
  if (!reader.is_open()) {
    std::cout << "Cannot Open File: " << indicator_record_file << std::endl;
    return -1;
  }

  // std::ifstream reader;
  // reader.open ( indicator_record_file, std::ifstream::in ) ;

  char line[1024] = {0};
  SingleIndRecord indicator;
  indicator.date = 0;
  indicator.value = 0;
  indicator.indicatorId = -1;  // to have segfault
  while (reader.is_open()) {
    int len_Read = reader.GetLine(line, 1024);
    if (len_Read <= 0) break;

    bool readSuccess = false;

    if (date_vector_given) {
      readSuccess = parseFromLine(line, &indicator, dates_vec);
    } else {
      readSuccess = parseFromLine(line, &indicator, start_date, end_date);
    }

    if (!readSuccess) continue;
    indicator_records[indicator.indicatorId].add(indicator);
  }
  reader.close();

  if (indicatorVector.size() == 0) {
    std::cerr << "No data was read!!\n";
    exit(1);
  }

  static const int CUTOFF_NUM_DAYS_PERCENT = 80;

  for (auto i = 0u; i < indicator_records.size(); ++i) indicator_records[i].normalize();

  if (corr_cutoff >= 2) {
    // A hack to be used in generate_comb_indicator_stats.pl to that individual indicator records file can be computed

    switch (choice_comp_algo) {
      case 0:
        std::sort(indicator_records.begin(), indicator_records.end(), compareIndicatorRecVec);  // sort on median_
        break;
      case 1:
        std::sort(indicator_records.begin(), indicator_records.end(),
                  compareIndicatorRecVecStdPenPNL);  // sort on std_penalized_mean_
        break;
      case 2:
        std::sort(indicator_records.begin(), indicator_records.end(), compareIndicatorSharpe);  // sort on sharpe ratio_
        break;
    }
    //      std::sort(indicator_records.begin(), indicator_records.end(), compareIndicatorSharpe );//sort on median_

    for (auto i = 0u; i < indicator_records.size(); ++i) {
      if ((int)indicator_records[i].data.size() < (int)(CUTOFF_NUM_DAYS_PERCENT * numDaysDataPresent / 100.0))
        continue;  // not enough num days data for that indicator
      printf("INDICATOR %.4f %.4f %s\n", indicator_records[i].mean_, indicator_records[i].stdev_,
             indicatorVector[indicator_records[i].indicatorId].c_str());
    }
    // print mean and stdev
    return 0;
  }

  switch (choice_comp_algo) {
    case 0:
      std::sort(indicator_records.begin(), indicator_records.end(), compareIndicatorRecVec);  // sort on median_
      break;
    case 1:
      std::sort(indicator_records.begin(), indicator_records.end(),
                compareIndicatorRecVecStdPenPNL);  // sort on std_penalized_mean_
      break;
    case 2:
      std::sort(indicator_records.begin(), indicator_records.end(), compareIndicatorSharpe);  // sort on sharpe ratio_
      break;
    default:
      std::cerr << "The comp algo not found.\n";
      break;
  }

  // print indicator file header
  printf("MODELINIT DEPBASE %s %s %s\n", shortcode, dep_base, dep_pred);
  printf("MODELMATH LINEAR CHANGE\n");
  printf("INDICATORSTART\n");

  bool* selected = (bool*)calloc(indicatorVector.size(), sizeof(bool));
  std::map<std::string, int> m_exclude_list;

  if (exclude_indicators_file) {
    HFSAT::BulkFileReader ex_reader;
    ex_reader.open(exclude_indicators_file);
    if (!ex_reader.is_open()) {
      std::cout << "Cannot Open File: " << exclude_indicators_file << std::endl;
      return -1;
    }

    while (ex_reader.is_open()) {
      int iLength = ex_reader.GetLine(line, 1024);
      if (iLength <= 0) break;

      HFSAT::PerishableStringTokenizer tokenizer(line, 1024);
      const std::vector<const char*> tokens = tokenizer.GetTokens();

      if (tokens.size() <= 2 || strcmp(tokens[0], "INDICATOR") != 0) continue;

      std::string sKey = "";
      for (unsigned int i = 2; i < tokens.size(); ++i) {
        if (strcmp(tokens[i], "#") == 0) break;
        sKey += std::string(tokens[i]) + " ";
      }

      std::map<std::string, int>::iterator it_ = indicator2id.find(sKey);

      if (it_ != indicator2id.end()) {
        m_exclude_list[sKey] = indicator2id[sKey];
        std::cerr << "skipping indicator " << sKey << "\n";
      }
    }
    ex_reader.close();
  }

  // pick randomly from top 5 and then proceed.

  selected[rn] = true;
  AddPriceType(indicatorVector[indicator_records[rn].indicatorId], dep_base);
  printf("INDICATOR 1.00 %s # %.4f %.4f %.4f\n", indicatorVector[indicator_records[rn].indicatorId].c_str(),
         indicator_records[rn].std_penalized_mean_, indicator_records[rn].mean_, indicator_records[rn].stdev_);
  removeOtherDependentOnes(rn, selected, corr_cutoff);

  for (auto i = 0u; i < indicator_records.size(); i++) {
    // choose from the top 5 and remove all other with very high correlation
    if (selected[i]) {
      // std::cout << "id is not selected " <<  i << "\n";
      continue;
    }
    if ((fabs(indicator_records[i].median_) < 0.01) || (fabs(indicator_records[i].mean_) < 0.01) ||
        (fabs(indicator_records[i].std_penalized_mean_) < 0.003) ||
        (int)indicator_records[i].data.size() < (int)(CUTOFF_NUM_DAYS_PERCENT * numDaysDataPresent / 100.0) ||
        (m_exclude_list.find(indicatorVector[indicator_records[i].indicatorId]) != m_exclude_list.end())) {
      selected[i] = true;
      continue;
    }
    AddPriceType(indicatorVector[indicator_records[i].indicatorId], dep_base);
    printf("INDICATOR 1.00 %s # %.4f %.4f %.4f\n", indicatorVector[indicator_records[i].indicatorId].c_str(),
           indicator_records[i].std_penalized_mean_, indicator_records[i].mean_, indicator_records[i].stdev_);
    // double corr_i_j = HFSAT::VectorUtils::CalcDotProduct(indicator_records[i].raw_values,
    // indicator_records[j].raw_values) / (indicator_records[j].raw_values.size() -1 );
    selected[i] = true;
    removeOtherDependentOnes(i, selected, corr_cutoff);
  }
  // indicators end
  printf("INDICATOREND\n");
}
