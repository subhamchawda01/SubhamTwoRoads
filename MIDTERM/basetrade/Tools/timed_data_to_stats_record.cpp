/**
    \file Tools/timed_data_to_stats_record.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <signal.h>
#include <algorithm>

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/buffered_vec.hpp"

#include "basetrade/Tools/td2rd_utils.hpp"
#include "basetrade/Tools/timed_data_to_reg_data.hpp"
#include "basetrade/Tools/timed_data_to_multiple_reg_data.hpp"
#include "basetrade/Tools/simple_line_bwriter.hpp"

#include "basetrade/MTools/data_processing.hpp"

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

    email_address_ = "kp@circulumvite.com";

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.sendMail();

    abort();
  }

  exit(0);
}

struct idx_value {
  int idx_;
  double value_;

  idx_value(int i, double v) : idx_(i), value_(v) {}
};

bool compareIdxValueRec(const idx_value& r1, const idx_value& r2) { return (r1.value_ < r2.value_); }

void ParseCommandLineParams(const int argc, const char** argv, std::string& model_name,
                            std::string& timed_data_filename_, double& min_px_increment,
                            HFSAT::NormalizingAlgo& normalizing_algo_, std::vector<std::string>& filters,
                            std::vector<int>& counters_to_predict_, std::string& daily_volume_file) {
  // expect :
  // 1. $dat_to_reg MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME
  std::string usage =
      "Usage: <model-file> <timed-data-file> <min_price_increment> <norm-algo> <num_filters> <all-filters-except-fv> "
      "<num-pred-period> <all-period-list> [if fv filter, then daily_volume_file_path should be provided]\n";
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
  int num_pred_intervals = atoi(argv[arg_indx++]);

  for (int i = 0; i < num_pred_intervals; ++i) {
    if (argc <= arg_indx) {
      std::cerr << usage << "\n";
      exit(0);
    }
    counters_to_predict_.push_back(std::max(1, atoi(argv[arg_indx++])));
  }

  if (argc > arg_indx) daily_volume_file = argv[arg_indx++];
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

  std::string name;
};
#undef MAX_DUMMY
#undef MIN_DUMMY

class InMemRegData : public HFSAT::SimpleLineProcessor {
  std::string model_filename_;

  int num_words_in_line;
  int num_lines;
  std::vector<double> arr;

  /* stats */

 public:
  InMemRegData() : num_words_in_line(0), num_lines(0) {}

  InMemRegData(std::string m_fn_) : model_filename_(m_fn_), num_words_in_line(0), num_lines(0) {}

  void AddWord(double _item_) { arr.push_back(_item_); }
  void FinishLine() {
    if (num_lines == 0) {
      num_words_in_line = arr.size();
    }
    num_lines++;
  }
  void Close() {}  // nothing to do

  int NumLines() { return num_lines; }

  // I want to understand what this function is doing.
  // The code is not so nice. It reuses variables and
  // there is array access without checking validity of
  // index.
  // TODO Check if memcpy is a better way. But that will
  // need us to allocate a block of memor and not
  // use a vector
  // std::memcpy(dest, source, how_much_to_copy);
  std::vector<double> GetRow(int idx_) {
    std::vector<double> row(num_words_in_line, 0);

    for (int i = 0, idx_ = idx_ * num_words_in_line; i < num_words_in_line; i++, idx_++) {
      row[i] = arr[idx_];
    }

    return row;
  }
  std::vector<double> GetColumn(int idx_) {
    std::vector<double> column(num_lines, 0);
    int _max_ = idx_ + (num_lines - 1) * num_words_in_line;

    for (int i = 0; idx_ <= _max_; idx_ = idx_ + num_words_in_line, i++) {
      column[i] = arr[idx_];
    }

    return column;
  }

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

  std::vector<double> CalculateCorrelation(bool* filter) {
    std::vector<double> corr_record;

    if ((num_words_in_line < 1) || (num_words_in_line > 100000)) {
      std::cerr << " too many/less indicators " << num_words_in_line
                << " change the limit in the timed_data_to_multple_stats.cpp file or debug \n";
      exit(0);
    }

    corr_record.reserve((num_words_in_line)-1);

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

      for (int j = 0; j < num_words_in_line; ++j) {
        mean[j] += arr[indx++];
      }

      filtered_row_count++;
    }

    for (int j = 0; j < num_words_in_line; ++j) {
      mean[j] /= filtered_row_count;
    }

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
      if (std::isnan(corr)) corr = 0;
      corr_record.push_back(corr);
    }
    return corr_record;
  }

  void PrintMultipleStats(bool* _filtered_) {
    for (int idx_ = 1; idx_ < num_words_in_line; idx_++) {
      int NO_BINS = 9;

      std::vector<idx_value> _indep_sorted_;
      _indep_sorted_.reserve(num_lines);

      std::vector<double> _dep_;
      _dep_.reserve(num_lines);

      int _max_ = idx_ + (num_lines - 1) * num_words_in_line;

      for (int indep_idx_ = idx_, dep_idx_ = 0, r_line_no = 0, w_line_no = 0; indep_idx_ <= _max_;
           indep_idx_ = indep_idx_ + num_words_in_line, dep_idx_ = dep_idx_ + num_words_in_line, r_line_no++) {
        if ((_filtered_ != NULL && !_filtered_[r_line_no])) {
          continue;
        }

        _dep_.push_back(arr[dep_idx_]);
        _indep_sorted_.push_back(idx_value(w_line_no, arr[indep_idx_]));
        w_line_no++;
      }

      //	std::cerr << "total remaining lines " << _dep_.size ( ) << "\n" ;

      std::sort(_indep_sorted_.begin(), _indep_sorted_.end(), compareIdxValueRec);

      NO_BINS = std::min(NO_BINS, (int)_indep_sorted_.size() / 3);

      int i = 0;
      int j = 0;

      std::vector<double> q_beta(NO_BINS, 0);
      std::vector<double> q_rho(NO_BINS, 0);
      std::vector<double> q_imu(NO_BINS, 0);
      std::vector<double> q_isig(NO_BINS, 0);
      std::vector<double> q_dmu(NO_BINS, 0);
      std::vector<double> q_dsig(NO_BINS, 0);

      /*
      std::cout << "\n\n" ;
      std::cout << std::left ;
      std::cout << std::setw ( 15 ) << "IND" ;
      std::cout << std::setw ( 15 ) << "BIN" ;
      std::cout << std::setw ( 15 ) << "B_SIZE" ;
      std::cout << std::setw ( 15 ) << "B_MIN" ;
      std::cout << std::setw ( 15 ) << "B_MAX" ;
      std::cout << std::setw ( 15 ) << "DEP_MU" ;
      std::cout << std::setw ( 15 ) << "INDEP_MU" ;
      std::cout << std::setw ( 15 ) << "DEP_SIG" ;
      std::cout << std::setw ( 15 ) << "INDEP_SIG"  ;
      std::cout << std::setw ( 15 ) << "RHO" ;
      std::cout << std::setw ( 15 ) << "BETA" << "\n" ; */

      // std::string indicator_string_ ;
      // HFSAT::GetIndicatorString ( model_filename_ , idx_ , indicator_string_ ) ;
      // std::cout << indicator_string_ << "," ;

      for (; i < NO_BINS; i++) {
        int j_limit = ((i + 1) * _indep_sorted_.size()) / NO_BINS;

        std::vector<double> _dep_bin_;
        std::vector<double> _indep_bin_;

        _dep_bin_.reserve(j_limit - j);
        _indep_bin_.reserve(j_limit - j);

        for (; j < j_limit; j++) {
          _dep_bin_.push_back(_dep_[_indep_sorted_[j].idx_]);
          _indep_bin_.push_back(_indep_sorted_[j].value_);
        }

        q_dsig[i] = HFSAT::VectorUtils::GetStdevAndMean(_dep_bin_, q_dmu[i]);
        q_isig[i] = HFSAT::VectorUtils::GetStdevAndMean(_indep_bin_, q_imu[i]);
        q_rho[i] = HFSAT::GetCorrelation(_dep_bin_, _indep_bin_);
        q_beta[i] = HFSAT::GetSLRCoeff(_dep_bin_, _indep_bin_);

        std::cout << "|" << i << ",";
        std::cout << _indep_bin_.size() << ",";
        std::cout << *std::min_element(_indep_bin_.begin(), _indep_bin_.end()) << ",";
        std::cout << *std::max_element(_indep_bin_.begin(), _indep_bin_.end()) << ",";
        std::cout << q_dmu[i] << ",";
        std::cout << q_imu[i] << ",";
        std::cout << q_dsig[i] << ",";
        std::cout << q_isig[i] << ",";
        std::cout << q_rho[i] << ",";
        std::cout << q_beta[i] << "|";

        /*
        std::cout << std::setw ( 15 ) << indicator_string_ ;
        std::cout << std::setw ( 15 ) << i ;
        std::cout << std::setw ( 15 ) << _indep_bin_.size ( ) ;
        std::cout << std::setw ( 15 ) << *std::min_element ( _indep_bin_.begin ( ) , _indep_bin_.end ( ) ) ;
        std::cout << std::setw ( 15 ) << *std::max_element ( _indep_bin_.begin ( ) , _indep_bin_.end ( ) ) ;
        std::cout << std::setw ( 15 ) << q_dmu [ i ] ;
        std::cout << std::setw ( 15 ) << q_imu [ i ] ;
        std::cout << std::setw ( 15 ) << q_dsig [ i ] ;
        std::cout << std::setw ( 15 ) << q_isig [ i ] ;
        std::cout << std::setw ( 15 ) << q_rho [ i ] ;
        std::cout << std::setw ( 15 ) << q_beta [ i ] << "\n";
        */
      }
      std::cout << "\n";
    }
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
  HFSAT::NormalizingAlgo normalizing_algo_ = HFSAT::na_t1;
  std::string daily_volume_file;

  bool is_returns_based_ = true;

  ParseCommandLineParams(argc, (const char**)argv, model_filename_, timed_data_filename_, min_px_increment,
                         normalizing_algo_, filters_names, counters_to_predict_, daily_volume_file);

  if (!HFSAT::GetDepCalcParams(model_filename_, is_returns_based_)) {
    std::cerr << "GetDepCalcParams did not find a line like MODELMATH CART CHANGE 0.5 na_t1" << std::endl;
    exit(0);
  }
  int size = 0;
  bool isFVRequired = (daily_volume_file != "" && HFSAT::FileUtils::ExistsWithSize(daily_volume_file, size) &&
                       daily_volume_file.compare("INVALIDFILE") != 0);

  if (daily_volume_file != "" && size <= 0 && daily_volume_file.compare("INVALIDFILE") != 0) {
    std::cerr << "note that the supplied daily_volume_file is of empty size " << daily_volume_file << "\n";
  }

  std::vector<Filter> filters;
  for (size_t i = 0; i < filters_names.size(); ++i) {
    filters.push_back(Filter(filters_names[i], min_px_increment));
  }

  std::vector<HFSAT::SimpleLineProcessor*> lineProcessors;
  size_t num_files = counters_to_predict_.size();
  if (isFVRequired) num_files *= 2;

  for (size_t i = 0; i < num_files; ++i) lineProcessors.push_back(new InMemRegData(model_filename_));

  HFSAT::BufferedDataSource<HFSAT::TD2RD_UTILS::TimedData>* src =
      new HFSAT::TD2RD_UTILS::TimedDataFileSource<HFSAT::TD2RD_UTILS::TimedData>(timed_data_filename_);
  HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData> td_vec = HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData>(src);

  DailyTradeVolumeReader* tvr = NULL;
  if (isFVRequired) {
    tvr = new DailyTradeVolumeReader(daily_volume_file);
  }

  // std::cout << "multiple td 2 rd processing \n" ;
  HFSAT::MultipleTd2RdProcessing(td_vec, lineProcessors, is_returns_based_, counters_to_predict_, normalizing_algo_,
                                 isFVRequired, tvr, false);

  // std::cout << "multiple td 2 rd processing done \n" ;

  int max_lines = 0;
  for (size_t i = 0; i < lineProcessors.size(); ++i) {
    max_lines = std::max(max_lines, ((InMemRegData*)lineProcessors[i])->NumLines());
  }
  bool* filtered_lines = new bool[max_lines];

  // std::cout << "stats :: max no of lines in timed data " << max_lines << "\n" ;

  if (max_lines > 0) {
    for (size_t i = 0; i < lineProcessors.size(); ++i) {
      for (size_t j = 0; j < filters.size(); ++j) {
        ((InMemRegData*)lineProcessors[i])->ApplyFilter(filters[j], filtered_lines);
        ((InMemRegData*)lineProcessors[i])->PrintMultipleStats(filtered_lines);
      }

      if (isFVRequired) {
        i++;
        ((InMemRegData*)lineProcessors[i])->PrintMultipleStats(NULL);
      }
    }
  } else {
    //      std::cerr << "stats :: max no of lines in timed data " << max_lines << "\n" ;
  }

  return 0;
}
