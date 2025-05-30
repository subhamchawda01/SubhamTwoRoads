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

  // start stats ::

  // each strategy ( everything here ) should address STRENGTH and CONSISTENCY.

  // strategy 0 ::
  // sort by mean :: pick whose mean > cutoff1 && whose sign_bias > cutoff2

  // strategy 1 ::
  // use sign_bias to filter :: use mean_ex_outliers to sort :: pick whose mean_ex_outlier > cutoff //

  // strategy 2 ::
  // use mean_rewarded_sharpe to sort :: pick whose mean > cutoff

  // strategy 3 ::
  // use sharpe to sort :: pick whose mean > cutoff

  // currently adding only proven strategies

  double mean_;
  double median_;
  double stdev_;
  double sharpe_;

  /* penalized by stdev and promoted by mean */
  double mean_rewarded_sharpe_;

  // a = +ve / sum ( +ve + -ve ) , max ( a , 1 - a ) [ 0 ,1 ]  1 suggests high significance
  double sign_bias_;
  double sign_rewarded_mean_;
  double sign_rewarded_sharpe_;

  /* 2 / sqrt ( n )  // dynamic cutoff ex: abs ( mean ) > 2 / sqrt ( n )
     no mathematical significance ( all models are wrong ... ) in this context, however in a data series of n items the
     correlation is significant iff that correlation > 2 / sqrt ( n )*/
  double significant_corr_;

  // how far is median from mean in sig units, lesser the better
  double skew_penalized_mean_;  // abs ( mu - md ) / sig

  // simply strip-off anything greater than mu - 2*sig and recalculate mean
  double mean_ex_outliers_;

  // :: end stats

  IndicatorRecVec(int id)
      : indicatorId(id),
        data(),
        raw_values(),
        mean_(0),
        median_(0),
        stdev_(0),
        sharpe_(0),
        mean_rewarded_sharpe_(0),
        sign_bias_(0),
        significant_corr_(0),
        skew_penalized_mean_(0),
        mean_ex_outliers_(0) {}

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

    while (it != data.end() && (*it).date < rec.date) {
      it++;
    }

    data.insert(it, dvp);  // also works when the loops is unable to find i
    numDaysDataPresent = std::max(numDaysDataPresent, (int)data.size());
  }

  /* basic */
  void all_stats() {
    // stats101
    unsigned int n = data.size();
    raw_values.resize(n);
    int npvals_ = 0, nnvals_ = 0;

    for (auto i = 0u; i < n; ++i) {
      if (data[i].value > 0) {
        npvals_++;
      } else if (data[i].value < 0) {
        nnvals_++;
      }
      raw_values[i] = data[i].value;
    }
    std::vector<double> t_raw_copy = raw_values;
    std::sort(t_raw_copy.begin(), t_raw_copy.end());
    if (n < 0) {
      std::cerr << "no data for indicator " << indicatorVector[indicatorId] << "\n";
      return;
    }
    sign_bias_ = std::max(npvals_, nnvals_) / (double)std::max(1, (npvals_ + nnvals_));
    //    std::cout << sign_bias_ << " " << npvals_ << " " << nnvals_ << "\n" ;
    sign_rewarded_mean_ = sign_bias_ * mean_;

    median_ = t_raw_copy[t_raw_copy.size() / 2];
    stdev_ = HFSAT::VectorUtils::GetStdevAndMean(raw_values, mean_);
    sharpe_ = ((stdev_ > 0) ? (mean_ / stdev_) : (mean_));

    if (stdev_ > 0) {
      mean_rewarded_sharpe_ = mean_ * std::min(2.5, (fabs(mean_) / stdev_));
      sign_rewarded_sharpe_ = sign_bias_ * std::min(2.5, (fabs(mean_) / stdev_));
    } else {
      mean_rewarded_sharpe_ = mean_;
      sign_rewarded_sharpe_ = sign_bias_ * mean_;
    }

    //    sb_penalized_mean_ = sign_bias_ * std::min ( 2.5, ( fabs ( mean_ ) / stdev_ ) ) ;

    significant_corr_ = n > 0 ? 2 / sqrt(n) : 1;
    skew_penalized_mean_ = (stdev_ > 0) ? fabs(mean_ - median_) / stdev_ : -1;

    double rco_ = mean_ + 2 * stdev_;
    double lco_ = mean_ - 2 * stdev_;
    int rn_ = 0;

    for (auto i = 0u; i < n; ++i) {
      if (data[i].value < rco_ && data[i].value > lco_) {
        mean_ex_outliers_ += data[i].value;
        rn_++;
      }
    }
    mean_ex_outliers_ = rn_ > 0 ? mean_ex_outliers_ / rn_ : 0;
  }
  // x*stability + y*mean + z*stdev
  /* scoring functions ends here */
};

std::vector<IndicatorRecVec> indicator_records;

bool compareByAbsMedian(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.median_) > fabs(r2.median_);
}

bool compareByAbsMean(const IndicatorRecVec& r1, const IndicatorRecVec& r2) { return fabs(r1.mean_) > fabs(r2.mean_); }

bool compareByMeanRSharpe(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.mean_rewarded_sharpe_) > fabs(r2.mean_rewarded_sharpe_);
}

bool compareBySharpe(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.sharpe_) > fabs(r2.sharpe_);
}

bool compareByMeanExOutliers(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.mean_ex_outliers_) > fabs(r2.mean_ex_outliers_);
}

bool compareBySignRMean(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.sign_rewarded_mean_) > fabs(r2.sign_rewarded_mean_);
}

bool compareBySignRSharpe(const IndicatorRecVec& r1, const IndicatorRecVec& r2) {
  return fabs(r1.sign_rewarded_sharpe_) > fabs(r2.sign_rewarded_sharpe_);
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

void removeOtherDependentOnes(int i, bool* filtered, float corr_cutoff) {
  for (unsigned int j = 0; j < indicator_records.size(); ++j) {
    if (filtered[j]) continue;
    double corr_i_j = HFSAT::GetCorrelation(indicator_records[i].raw_values, indicator_records[j].raw_values);
    if (fabs(corr_i_j) > corr_cutoff) {
      std::cerr << "rejected " << indicatorVector[indicator_records[i].indicatorId]
                << " --since its correlation with-- " << indicatorVector[indicator_records[j].indicatorId] << " --is-- "
                << corr_i_j << "\n";
      filtered[j] = true;
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
                            "MAX_CORR_THRESH -m MAX_NO -a <algorithm for comparision(0,1,2,3,4,5,6)> -i [definately "
                            "choose nth (0,1..]] -e [exclude_filename] -d [ set of dates to make indicator list on ]\n";
    std::cerr << "  YYYYMMDD can be a negative number to indicate offset from today\n";
    std::cerr << "  algo_options: 0-median_ \t\t3-mean_ \t\t2-sharpe_ \t\t3-mean_rewarded_sharpe_ "
                 "\t\t4-mean_ex_outliers_ \t\t5-sign_rewarded_mean_ \t\t6-sign_rewarded_sharpe_ \n";

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
  int max_indicators_ = 10000;

  int choice_comp_algo = 1;
  int rn = -1;
  char* exclude_indicators_file = NULL;
  int i = 8;

  while (i < argc) {
    if (strncmp(argv[i], "-a", 2) == 0) {
      choice_comp_algo = atoi(argv[++i]);
    } else if (strncmp(argv[i], "-m", 2) == 0) {
      max_indicators_ = atoi(argv[++i]);
    } else if (strncmp(argv[i], "-i", 2) == 0) {
      rn = atoi(argv[++i]);
    } else if (strncmp(argv[i], "-e", 2) == 0) {
      exclude_indicators_file = argv[++i];
    } else if (strncmp(argv[i], "-d", 2) == 0) {
      date_vector_given = true;
      int j = i + 1;
      for (; j < argc; j++) {
        dates_vec.push_back(atoi(argv[j]));
        i++;
      }
    } else {
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
    if (!readSuccess) {
      continue;
    }
    indicator_records[indicator.indicatorId].add(indicator);
  }

  reader.close();

  static const int CUTOFF_NUM_DAYS_PERCENT = 80;

  // compute stats
  for (auto i = 0u; i < indicator_records.size(); ++i) {
    indicator_records[i].all_stats();
  }

  // sort indicators using stats on their correlation data
  switch (choice_comp_algo) {
    case 0:
      std::sort(indicator_records.begin(), indicator_records.end(), compareByAbsMedian);  // sort on median_
      break;
    case 1:
      std::sort(indicator_records.begin(), indicator_records.end(), compareByAbsMean);  // sort on mean_
      break;
    case 2:
      std::sort(indicator_records.begin(), indicator_records.end(), compareBySharpe);  // sort on sharpe_
      break;
    case 3:
      std::sort(indicator_records.begin(), indicator_records.end(),
                compareByMeanRSharpe);  // sort on mean_rewarded_sharpe_
      break;
    case 4:
      std::sort(indicator_records.begin(), indicator_records.end(),
                compareByMeanExOutliers);  // sort on mean_ex_outliers_
      break;
    case 5:
      std::sort(indicator_records.begin(), indicator_records.end(), compareBySignRMean);  // sort on sign_rewarded_mean_
      break;
    case 6:
      std::sort(indicator_records.begin(), indicator_records.end(),
                compareBySignRSharpe);  // sort on sign_rewarded_sharpe_
      break;
  }

  if (corr_cutoff >= 2) {
    // A hack to be used in generate_comb_indicator_stats.pl to that individual indicator records file can be computed

    for (auto i = 0u; i < indicator_records.size(); ++i) {
      if ((int)indicator_records[i].data.size() < (int)(CUTOFF_NUM_DAYS_PERCENT * numDaysDataPresent / 100.0)) {
        continue;  // not enough num days data for that indicator
      }
      AddPriceType(indicatorVector[indicator_records[i].indicatorId], dep_base);
      printf("INDICATOR %.4f %.4f %s # sharpe %.4f mrs %.4f meo %.4f srm %.4f srs %.4f \n", indicator_records[i].mean_,
             indicator_records[i].stdev_, indicatorVector[indicator_records[i].indicatorId].c_str(),
             indicator_records[i].sharpe_, indicator_records[i].mean_rewarded_sharpe_,
             indicator_records[i].mean_ex_outliers_, indicator_records[i].sign_rewarded_mean_,
             indicator_records[i].sign_rewarded_sharpe_);
    }
    // print mean and stdev
    return 0;
  }

  // print indicator file header
  printf("MODELINIT DEPBASE %s %s %s\n", shortcode, dep_base, dep_pred);
  printf("MODELMATH LINEAR CHANGE\n");
  printf("INDICATORSTART\n");

  bool* filtered = (bool*)calloc(indicatorVector.size(), sizeof(bool));
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
  int pick_count_ = 0;

  if (rn > -1) {
    filtered[rn] = true;
    printf("INDICATOR 1.00 %s # %.4f %.4f %.4f\n", indicatorVector[indicator_records[rn].indicatorId].c_str(),
           indicator_records[rn].mean_rewarded_sharpe_, indicator_records[rn].mean_, indicator_records[rn].stdev_);
    removeOtherDependentOnes(rn, filtered, corr_cutoff);
    pick_count_++;
  }

  for (auto i = 0u; i < indicator_records.size() && pick_count_ <= max_indicators_; i++) {
    // choose from the top 5 and remove all other with very high correlation
    if (filtered[i]) {
      // std::cout << "id is not filtered " <<  i << "\n";
      continue;
    }
    if ((fabs(indicator_records[i].median_) < 0.01) || (fabs(indicator_records[i].mean_) < 0.01) ||
        (fabs(indicator_records[i].mean_rewarded_sharpe_) < 0.003) || indicator_records[i].sign_bias_ < 0.75 ||
        (int)indicator_records[i].data.size() < (int)(CUTOFF_NUM_DAYS_PERCENT * numDaysDataPresent / 100.0) ||
        (m_exclude_list.find(indicatorVector[indicator_records[i].indicatorId]) != m_exclude_list.end())) {
      filtered[i] = true;
      continue;
    }
    AddPriceType(indicatorVector[indicator_records[i].indicatorId], dep_base);
    printf("INDICATOR 1.00 %s # %.4f %.4f %.4f %.4f\n", indicatorVector[indicator_records[i].indicatorId].c_str(),
           indicator_records[i].mean_rewarded_sharpe_, indicator_records[i].mean_, indicator_records[i].stdev_,
           indicator_records[i].sign_rewarded_sharpe_);
    // double corr_i_j = HFSAT::VectorUtils::CalcDotProduct(indicator_records[i].raw_values,
    // indicator_records[j].raw_values) / (indicator_records[j].raw_values.size() -1 );
    filtered[i] = true;
    pick_count_++;
    removeOtherDependentOnes(i, filtered, corr_cutoff);
  }
  // indicators end
  printf("INDICATOREND\n");
}
