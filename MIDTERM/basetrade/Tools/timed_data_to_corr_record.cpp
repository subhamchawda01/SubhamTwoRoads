/**
    \file Tools/timed_data_to_multiple_reg_data.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <signal.h>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/buffered_vec.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "basetrade/Tools/td2rd_utils.hpp"
#include "basetrade/Tools/timed_data_to_reg_data.hpp"
#include "basetrade/Tools/timed_data_to_multiple_reg_data.hpp"
#include "basetrade/Tools/simple_line_bwriter.hpp"

std::string input_string_ = "";

void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "timed_data_to_stats received SIGSEGV on " << hostname_ << "\n";
      t_oss_ << "exec_cmd_ " << input_string_ << "\n";

      email_string_ = t_oss_.str();
    }

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    email_address_ = "nseall@tworoads.co.in";

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.sendMail();

    abort();
  }

  exit(0);
}

void ParseCommandLineParams(const int argc, const char** argv, std::string& model_name,
                            std::string& timed_data_filename_, double& min_px_increment,
                            HFSAT::NormalizingAlgo& normalizing_algo_, std::vector<std::string>& filters,
                            std::vector<int>& counters_to_predict_, std::string& daily_volume_file, bool& fsudm_filter_,
                            std::vector<double>& tails_) {
  // expect :
  // 1. $dat_to_reg MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME
  std::string usage =
      "Usage: <model-file> <timed-data-file> <min_price_increment> <norm-algo> <num_filters> <all-filters-except-fv & "
      "fsudm> <num-pred-period> <all-period-list> [if fv filter, then daily_volume_file_path should be provided] [0/1 "
      "fsudm ?] <num_tails_for tail correlation> <stdev filter for indicator-tails> \n";
  if (argc <= 6) {
    std::cerr << usage << "\n";
    exit(0);
  }

  for (int i = 0; i < argc; i++) {
    input_string_ += " ";
    input_string_ += argv[i];
  }

  int arg_indx = 1;
  model_name = argv[arg_indx++];
  timed_data_filename_ = argv[arg_indx++];
  min_px_increment = atof(argv[arg_indx++]);
  normalizing_algo_ = HFSAT::StringToNormalizingAlgo(argv[arg_indx++]);
  int num_filters = atoi(argv[arg_indx++]);

  for (int i = 0; i < num_filters; ++i) {
    if (argc <= arg_indx) {
      std::cerr << usage << "\n";
      exit(0);
    }
    filters.push_back(argv[arg_indx++]);
  }

  if (argc <= arg_indx) {
    std::cerr << usage << "\n";
    exit(0);
  }
  int num_pred_intervals = std::max(1, atoi(argv[arg_indx++]));

  for (int i = 0; i < num_pred_intervals; ++i) {
    if (argc <= arg_indx) {
      std::cerr << usage << "\n";
      exit(0);
    }
    counters_to_predict_.push_back(atoi(argv[arg_indx++]));
  }

  if (argc > arg_indx) {
    daily_volume_file = argv[arg_indx++];
  }

  if (argc > arg_indx) {
    fsudm_filter_ = (atoi(argv[arg_indx++]) > 0 ? true : false);
  }

  int num_tails = 0;
  if (argc > arg_indx) {
    num_tails = atoi(argv[arg_indx++]);
  }

  for (int i = 0; i < num_tails; ++i) {
    if (argc <= arg_indx) {
      std::cerr << usage << "\n";
      exit(0);
    }
    tails_.push_back(atof(argv[arg_indx++]));
  }
}

// 2 billion, seems fair?
#define MAX_DUMMY 2000000000.0
#define MIN_DUMMY -2000000000.0

class Filter {
  void set(bool absolute_, double low_limit_, double up_limit_) {
    absolute = absolute_;
    lower_limit = low_limit_;
    upper_limit = up_limit_;
  }

 public:
  bool absolute;  // Or based on sdev
  double upper_limit;
  double lower_limit;
  std::string name;
  Filter(std::string name_, double min_price_incement)
      : absolute(true), upper_limit(MAX_DUMMY), lower_limit(MIN_DUMMY), name(name_) {
    if (name_ == "f0")
      set(true, MIN_DUMMY, MAX_DUMMY);
    else if (name_ == "fst.5")
      set(true, min_price_incement * 0.5, MAX_DUMMY);
    else if (name_ == "fst1")
      set(true, min_price_incement * 1.0, MAX_DUMMY);
    else if (name_ == "fst2")
      set(true, min_price_incement * 2.0, MAX_DUMMY);

    else if (name_ == "fsl2")
      set(false, MIN_DUMMY, 2.0);
    else if (name_ == "fsl3")
      set(false, MIN_DUMMY, 3.0);

    else if (name_ == "fsg.5")
      set(false, 0.5, MAX_DUMMY);
    else if (name_ == "fsg1")
      set(false, 1.0, MAX_DUMMY);
    else if (name_ == "fsg2")
      set(false, 2.0, MAX_DUMMY);

    else if (name_ == "fsr1_5")
      set(false, 1.0, 5.0);
    else if (name_ == "fsr.5_3")
      set(false, 0.5, 3.0);

    else
      set(true, MIN_DUMMY, MAX_DUMMY);  // ALL PASS FILTER
  }
};
#undef MAX_DUMMY
#undef MIN_DUMMY

class InMemRegData : public HFSAT::SimpleLineProcessor {
  int num_words_in_line;
  int num_lines;
  std::vector<double> arr;

 public:
  InMemRegData() : num_words_in_line(0), num_lines(0) {}

  void AddWord(double _item_) { arr.push_back(_item_); }
  void FinishLine() {
    if (num_lines == 0) num_words_in_line = arr.size();
    num_lines++;
  }
  void Close() {}  // nothing to do

  int NumLines() { return num_lines; }

  double Item(int i, int j) {
    return arr[i * num_words_in_line + j];  // no checks for dimensions
  }

  void ApplyFilter(Filter& f, bool* filtered_rows) {
    if (f.absolute) {
      for (int i = 0, indx = 0; i < num_lines; ++i, indx += num_words_in_line) {
        double val = fabs(arr[indx]);
        filtered_rows[i] = val <= f.upper_limit && val >= f.lower_limit;
      }
    } else {
      // computation of 0-mean sdev as done in the script
      if (num_lines <= 100) {
        for (int i = 0; i < num_lines; ++i) {
          filtered_rows[i] = true;
        }
      } else {
        double l2_dep_sum = 0;

        for (int i = 0, indx = 0; i < num_lines; ++i, indx += num_words_in_line) {
          l2_dep_sum += arr[indx] * arr[indx];
        }
        double nomean_stdev_ = sqrt(l2_dep_sum / (num_lines - 1));
        double upper_limit = f.upper_limit * nomean_stdev_;
        double lower_limit = f.lower_limit * nomean_stdev_;

        for (int i = 0, indx = 0; i < num_lines; ++i, indx += num_words_in_line) {
          double val = fabs(arr[indx]);
          filtered_rows[i] = val <= upper_limit && val >= lower_limit;
        }
      }
    }
  }

  void ApplyIndicatorTail(std::vector<double>& tails_, std::vector<bool**>& tail_filtered_lines_) {
    for (size_t i = 0; i < tails_.size(); ++i) {
      tail_filtered_lines_.push_back(new bool* [num_words_in_line - 1]);
      for (size_t j = 0; j < (size_t)num_words_in_line - 1; ++j) {
        tail_filtered_lines_[i][j] = new bool[num_lines];
      }
    }
    for (int32_t indc_col_ = 0; indc_col_ < num_words_in_line - 1; ++indc_col_) {
      std::vector<double> indc_vec_;
      for (int i = 0; i < num_lines; ++i) {
        double val = arr[i * num_words_in_line + indc_col_ + 1];
        indc_vec_.push_back(val);
      }
      double mean = HFSAT::VectorUtils::GetMean(indc_vec_);
      double stdev = HFSAT::VectorUtils::GetStdev(indc_vec_, mean);

      for (size_t i = 0; i < tails_.size(); ++i) {
        double lower_limit_ = -1 * tails_[i] * stdev;
        double upper_limit_ = tails_[i] * stdev;

        for (size_t j = 0; j < indc_vec_.size(); ++j) {
          if (indc_vec_[j] <= lower_limit_ || indc_vec_[j] >= upper_limit_) {
            tail_filtered_lines_[i][indc_col_][j] = true;
          } else {
            tail_filtered_lines_[i][indc_col_][j] = false;
          }
        }
      }
    }
  }

  std::vector<double> CalculateCorrelation(bool* filter) {
    std::vector<double> corr_record;

    try {
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    } catch (std::exception& e) {
      std::cerr << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map using "
                << num_words_in_line << " WHAT :: " << e.what() << "\n";

      HFSAT::Email email_;
      email_.setSubject("Subject : DEBUG ");
      email_.addRecepient("kp@circulumvite.com,gchak@circulumvite.com");
      email_.addSender("vector_reserve@timed_data");
      email_.content_stream << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map of size "
                            << num_words_in_line << " WHAT :: " << e.what() << "\n";
      email_.sendMail();
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    }
    corr_record.reserve(std::min(10000, std::max(1, num_words_in_line) - 1));

    int filtered_row_count = 0;
    double* mean = (double*)calloc(num_words_in_line, sizeof(double));
    double* sxy = (double*)calloc(num_words_in_line, sizeof(double));
    double* syy = (double*)calloc(num_words_in_line, sizeof(double));

    int indx = 0;
    for (int i = 0; i < num_lines; i++) {
      if (filter != NULL && !filter[i]) {
        indx += num_words_in_line;
        continue;
      }

      for (int j = 0; j < num_words_in_line; ++j) mean[j] += arr[indx++];
      filtered_row_count++;
    }

    for (int j = 0; j < num_words_in_line; ++j) mean[j] /= filtered_row_count;

    indx = 0;
    double xt = 0;  // x-mean(x)
    double yt = 0;  // y-mean(y)
    double sxx = 0;
    for (int i = 0; i < num_lines; i++) {
      if (filter != NULL && !filter[i]) {
        indx += num_words_in_line;
        continue;
      }

      xt = arr[indx++] - mean[0];
      sxx += xt * xt;
      for (int j = 1; j < num_words_in_line; ++j) {  // note j begins from 1
        yt = arr[indx++] - mean[j];
        syy[j] += yt * yt;
        sxy[j] += xt * yt;
      }
    }
    // note previously sxy and syy is not computed for 0 index
    for (int j = 1; j < num_words_in_line; ++j) {
      double corr = sxy[j] / sqrt(sxx * syy[j]);
      if (std::isnan(corr)) {
        corr = NAN;
      }
      corr_record.push_back(corr);
    }
    return corr_record;
  }

  std::vector<double> CalculateCorrelationNoMean(bool* filter) {
    std::vector<double> corr_record;

    try {
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    } catch (std::exception& e) {
      std::cerr << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map using "
                << num_words_in_line << " WHAT :: " << e.what() << "\n";

      HFSAT::Email email_;
      email_.setSubject("Subject : DEBUG ");
      email_.addRecepient("kp@circulumvite.com,gchak@circulumvite.com");
      email_.addSender("vector_reserve@timed_data");
      email_.content_stream << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map of size "
                            << num_words_in_line << " WHAT :: " << e.what() << "\n";
      email_.sendMail();
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    }
    corr_record.reserve(std::min(10000, std::max(1, num_words_in_line) - 1));

    double* sxy = (double*)calloc(num_words_in_line, sizeof(double));
    double* syy = (double*)calloc(num_words_in_line, sizeof(double));

    int indx = 0;
    double xt = 0;
    double yt = 0;
    double sxx = 0;
    for (int i = 0; i < num_lines; i++) {
      if (filter != NULL && !filter[i]) {
        indx += num_words_in_line;
        continue;
      }

      xt = arr[indx++];
      sxx += xt * xt;
      for (int j = 1; j < num_words_in_line; ++j) {  // note j begins from 1
        yt = arr[indx++];
        syy[j] += yt * yt;
        sxy[j] += xt * yt;
      }
    }
    // note previously sxy and syy is not computed for 0 index
    for (int j = 1; j < num_words_in_line; ++j) {
      double corr = sxy[j] / sqrt(sxx * syy[j]);
      if (std::isnan(corr)) {
        corr = NAN;
      }
      corr_record.push_back(corr);
    }
    return corr_record;
  }

  std::vector<double> CalculateTailedCorrelation(bool* filter, bool** tailed_filter_) {
    std::vector<double> corr_record;

    try {
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    } catch (std::exception& e) {
      std::cerr << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map using "
                << num_words_in_line << " WHAT :: " << e.what() << "\n";

      HFSAT::Email email_;
      email_.setSubject("Subject : DEBUG ");
      email_.addRecepient("kp@circulumvite.com,gchak@circulumvite.com");
      email_.addSender("vector_reserve@timed_data");
      email_.content_stream << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map of size "
                            << num_words_in_line << " WHAT :: " << e.what() << "\n";
      email_.sendMail();
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    }
    int min_rows = (int)(0.08 * num_lines);
    corr_record.reserve(std::min(10000, std::max(1, num_words_in_line) - 1));

    for (int indc_ = 0; indc_ < num_words_in_line - 1; ++indc_) {
      double ux_ = 0, uxx_ = 0, uy_ = 0, uyy_ = 0, uxy_ = 0, filtered_row_count = 0;

      for (int i = 0, base_idx_ = 0; i < num_lines; i++, base_idx_ += num_words_in_line) {
        if ((filter != NULL && !filter[i]) || (!tailed_filter_[indc_][i])) {
          continue;
        }
        uy_ += arr[base_idx_];
        uyy_ += arr[base_idx_] * arr[base_idx_];

        ux_ += arr[base_idx_ + indc_ + 1];
        uxx_ += arr[base_idx_ + indc_ + 1] * arr[base_idx_ + indc_ + 1];

        uxy_ += arr[base_idx_] * arr[base_idx_ + indc_ + 1];

        filtered_row_count++;
      }

      if (filtered_row_count > min_rows) {
        uy_ /= filtered_row_count;
        uyy_ /= filtered_row_count;
        ux_ /= filtered_row_count;
        uxx_ /= filtered_row_count;
        uxy_ /= filtered_row_count;

        double corr = (uxy_ - ux_ * uy_) / (sqrt(uxx_ - ux_ * ux_) * sqrt(uyy_ - uy_ * uy_));
        //        std::cout << "means: " << ux_ << " " << uxx_ << " " << uy_ << " " << uyy_ << " " << corr << std::endl;
        if (std::isnan(corr)) {
          corr = NAN;
        }
        corr_record.push_back(corr);
      } else {
        corr_record.push_back(NAN);
      }
    }
    return corr_record;
  }

  std::vector<double> CalculateTailedCorrelationNoMean(bool* filter, bool** tailed_filter_) {
    std::vector<double> corr_record;

    try {
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    } catch (std::exception& e) {
      std::cerr << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map using "
                << num_words_in_line << " WHAT :: " << e.what() << "\n";

      HFSAT::Email email_;
      email_.setSubject("Subject : DEBUG ");
      email_.addRecepient("kp@circulumvite.com,gchak@circulumvite.com");
      email_.addSender("vector_reserve@timed_data");
      email_.content_stream << "DEBUG::timed_data_to_corr_record tried to use reserve with definition map of size "
                            << num_words_in_line << " WHAT :: " << e.what() << "\n";
      email_.sendMail();
      corr_record.reserve(std::max(1, num_words_in_line) - 1);
    }
    int min_rows = (int)(0.08 * num_lines);
    corr_record.reserve(std::min(10000, std::max(1, num_words_in_line) - 1));

    for (int indc_ = 0; indc_ < num_words_in_line - 1; ++indc_) {
      double uxx_ = 0, uyy_ = 0, uxy_ = 0, filtered_row_count = 0;

      for (int i = 0, base_idx_ = 0; i < num_lines; i++, base_idx_ += num_words_in_line) {
        if ((filter != NULL && !filter[i]) || (!tailed_filter_[indc_][i])) {
          continue;
        }
        uyy_ += arr[base_idx_] * arr[base_idx_];
        uxx_ += arr[base_idx_ + indc_ + 1] * arr[base_idx_ + indc_ + 1];
        uxy_ += arr[base_idx_] * arr[base_idx_ + indc_ + 1];
        filtered_row_count++;
      }

      if (filtered_row_count > min_rows) {
        uyy_ /= filtered_row_count;
        uxx_ /= filtered_row_count;
        uxy_ /= filtered_row_count;

        double corr = uxy_ / (sqrt(uxx_) * sqrt(uyy_));
        //        std::cout << "means: " << ux_ << " " << uxx_ << " " << uy_ << " " << uyy_ << " " << corr << std::endl;
        if (std::isnan(corr)) {
          corr = NAN;
        }
        corr_record.push_back(corr);
      } else {
        corr_record.push_back(NAN);
      }
    }
    return corr_record;
  }
};

/// input arguments : model_filename input_data_filename counters_to_predict_ normalizing_algo_ outputdatafile
/// model_filename is used to find out the parameters of computation of returns / change
/// inputdatafile format :
/// [ msecs_from_midnight event_count_from_start predprice baseprice var1 var2 ... varn ]
/// outputdatafile format :
/// [ ( predprice - baseprice ) var1 ... varn ]
int main(int argc, char** argv) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);

  std::string model_filename_;
  std::string timed_data_filename_;
  double min_px_increment;
  std::vector<std::string> filters_names;
  std::vector<int> counters_to_predict_;  // = 5000; ///< could be msecs_to_predict or events_to_predict
  std::vector<double> tails_;
  HFSAT::NormalizingAlgo normalizing_algo_ = HFSAT::na_t1;
  std::string daily_volume_file;
  bool fsudm_filter_ = false;

  bool is_returns_based_ = true;

  ParseCommandLineParams(argc, (const char**)argv, model_filename_, timed_data_filename_, min_px_increment,
                         normalizing_algo_, filters_names, counters_to_predict_, daily_volume_file, fsudm_filter_,
                         tails_);

  if (!HFSAT::GetDepCalcParams(model_filename_, is_returns_based_)) {
    std::cerr << "GetDepCalcParams did not find a line like MODELMATH CART CHANGE 0.5 na_t1" << std::endl;
    exit(0);
  }

  bool isFVRequired = (daily_volume_file != "" && daily_volume_file.compare("INVALIDFILE") != 0 &&
                       daily_volume_file.compare("NULL") != 0);
  std::vector<Filter> filters;
  for (size_t i = 0; i < filters_names.size(); ++i) {
    filters.push_back(Filter(filters_names[i], min_px_increment));
  }

  std::vector<HFSAT::SimpleLineProcessor*> lineProcessors;
  size_t num_files = counters_to_predict_.size();

  if (isFVRequired && fsudm_filter_)
    num_files *= 3;
  else if (isFVRequired)
    num_files *= 2;
  else if (fsudm_filter_)
    num_files *= 2;

  for (size_t i = 0; i < num_files; ++i) lineProcessors.push_back(new InMemRegData());

  HFSAT::BufferedDataSource<HFSAT::TD2RD_UTILS::TimedData>* src =
      new HFSAT::TD2RD_UTILS::TimedDataFileSource<HFSAT::TD2RD_UTILS::TimedData>(timed_data_filename_);
  HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData> td_vec = HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData>(src);

  DailyTradeVolumeReader* tvr = NULL;
  if (isFVRequired) tvr = new DailyTradeVolumeReader(daily_volume_file);

  HFSAT::MultipleTd2RdProcessing(td_vec, lineProcessors, is_returns_based_, counters_to_predict_, normalizing_algo_,
                                 isFVRequired, tvr, fsudm_filter_);

  int max_lines = 0;
  for (size_t i = 0; i < lineProcessors.size(); ++i) {
    max_lines = std::max(max_lines, ((InMemRegData*)lineProcessors[i])->NumLines());
  }
  bool* filtered_lines = new bool[max_lines];

  //  std::cerr << "max no of lines in timed data " << max_lines << "\n" ;

  int tmp = isFVRequired ? 2 : 1;
  if (fsudm_filter_) {
    tmp++;
  }

  if (tails_.empty()) {
    for (size_t i = 0; i < lineProcessors.size(); ++i) {
      for (size_t j = 0; j < filters.size(); ++j) {
        ((InMemRegData*)lineProcessors[i])->ApplyFilter(filters[j], filtered_lines);
        std::vector<double> corr = ((InMemRegData*)lineProcessors[i])->CalculateCorrelationNoMean(filtered_lines);
        std::cout << counters_to_predict_[i / tmp] << " " << filters[j].name;
        for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
        std::cout << "\n";
      }

      if (isFVRequired) {
        i++;
        std::vector<double> corr = ((InMemRegData*)lineProcessors[i])->CalculateCorrelationNoMean(NULL);
        std::cout << counters_to_predict_[i / tmp] << " fv";
        for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
        std::cout << "\n";
      }

      if (fsudm_filter_) {
        i++;
        std::vector<double> corr = ((InMemRegData*)lineProcessors[i])->CalculateCorrelationNoMean(NULL);
        std::cout << counters_to_predict_[i / tmp] << " fsudm";
        for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
        std::cout << "\n";
      }
    }
  } else {
    for (size_t i = 0; i < lineProcessors.size(); ++i) {
      std::vector<bool**> tails_filtered_lines_;
      ((InMemRegData*)lineProcessors[i])->ApplyIndicatorTail(tails_, tails_filtered_lines_);

      for (size_t j = 0; j < filters.size(); ++j) {
        ((InMemRegData*)lineProcessors[i])->ApplyFilter(filters[j], filtered_lines);
        for (size_t k = 0; k < tails_.size(); ++k) {
          std::vector<double> corr = ((InMemRegData*)lineProcessors[i])
                                         ->CalculateTailedCorrelationNoMean(filtered_lines, tails_filtered_lines_[k]);
          std::cout << "tail_" << tails_[k] << " " << counters_to_predict_[i / tmp] << " " << filters[j].name;
          for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
          std::cout << "\n";
        }
      }

      if (isFVRequired) {
        i++;
        for (size_t k = 0; k < tails_.size(); ++k) {
          std::vector<double> corr =
              ((InMemRegData*)lineProcessors[i])->CalculateTailedCorrelationNoMean(NULL, tails_filtered_lines_[k]);
          std::cout << "tail_" << tails_[k] << " " << counters_to_predict_[i / tmp] << " fv";
          for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
          std::cout << "\n";
        }
      }

      if (fsudm_filter_) {
        i++;
        for (size_t k = 0; k < tails_.size(); ++k) {
          std::vector<double> corr =
              ((InMemRegData*)lineProcessors[i])->CalculateTailedCorrelationNoMean(NULL, tails_filtered_lines_[k]);
          std::cout << "tail_" << tails_[k] << " " << counters_to_predict_[i / tmp] << " fsudm";
          for (size_t k = 0; k < corr.size(); ++k) std::cout << " " << corr[k];
          std::cout << "\n";
        }
      }
      for (size_t k = 0; k < tails_.size(); ++k) {
        delete[] tails_filtered_lines_[k];
      }
    }
  }
  return 0;
}
