#include <string.h>
#include <getopt.h>
#include <math.h>
#include <algorithm>

#include "baseinfra/EventDispatcher/assumption.hpp"
#include "dvctrade/SpreadTrading/kalman_regression.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define FILE_PATH_PREFIX "/NAS1/data/NSEBarData/FUT_BarData/"
#define FILE_PATH_PREFIX_ADJUSTED "/NAS1/data/NSEBarData/FUT_BarData_Adjusted/"

// kalman train period is 1 year prior to test period
void print_usage(const char* prg_name) {
  printf(" This is the Kalman Exec \n");
  printf(
      " Usage:%s --start_date <start_YYYYMMDD> --end_date <end_YYYYMMDD> --logfile <lfile> --instrument_1 <ticker> "
      "--instrument_2 <ticker2> --mode<0/1> --sampling_time <secs> --rolling_reg_period <in minutes> --norm_factor "
      "<double>[ --use_adjusted_data ]\n",
      prg_name);
  printf(" --start_date start_date of kalman test peroid\n");
  printf(" --end_date end_date of kalman test period\n");
  printf(" --logfile complete path of filefor logging\n");
  printf(" --instrument_1 ticker(eg TATAMOTORS)\n");
  printf(" --instrument_2 ticker(eg TATAMTRDVR)\n");
  printf(" --sampling_time time in seconds between successive samples\n");
  printf(" --mode ( 0 for price/1 for log price\n");
  printf(" --norm_factor <normalization factor - for multiplication with regression error\n");
  printf(" --rolling_reg_period <value in data samples/minutes> \n");
  printf(" --use_adjusted_data \n");
}

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"logfile", required_argument, 0, 'b'},
                                       {"start_date", required_argument, 0, 'c'},
                                       {"end_date", required_argument, 0, 'd'},
                                       {"instrument_1", required_argument, 0, 'f'},
                                       {"instrument_2", required_argument, 0, 'g'},
                                       {"sampling_time", required_argument, 0, 'o'},
                                       {"mode", required_argument, 0, 'p'},
                                       {"rolling_reg_period", required_argument, 0, 'q'},
                                       {"norm_factor", required_argument, 0, 'r'},
                                       {"use_adjusted_data", no_argument, 0, 's'},
                                       {0, 0, 0, 0}};

typedef struct {
  uint32_t secs_;
  double px_;
  double px2_;
} stdata;

std::vector<stdata*> inst1_data_;
std::vector<stdata*> inst2_data_;
std::vector<stdata*> both_inst_data_;

void populateData(std::string ticker_, std::vector<stdata*>& inst_data_, time_t start_time_, time_t end_time_,
                  bool adjusted_data_) {
  std::string inst_filename_ = "";
  if (adjusted_data_) {
    inst_filename_ = FILE_PATH_PREFIX_ADJUSTED + ticker_;
  } else {
    inst_filename_ = FILE_PATH_PREFIX + ticker_;
  }

  std::ifstream file_;
  file_.open(inst_filename_.c_str(), std::ifstream::in);
  if (false == file_.is_open()) {
    std::cerr << "Unable To Open The DataFile For Reading Data : " << inst_filename_ << std::endl;
    exit(-1);
  }

#define MAX_LINE_SIZE 1024
  char line_buffer[MAX_LINE_SIZE];
  while (file_.good()) {
    memset((void*)line_buffer, 0, MAX_LINE_SIZE);
    file_.getline(line_buffer, MAX_LINE_SIZE);

    // Skip comments in case we are adding in datafile
    if (std::string::npos != std::string(line_buffer).find("#")) continue;

    HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line_buffer, MAX_LINE_SIZE);
    std::vector<char const*> const& tokens = pst.GetTokens();

    // if it does not match front month future then continue
    if (std::string::npos == std::string(tokens[1]).find("_FF_0_0")) continue;

    time_t data_time = (time_t)(atoi(tokens[0]));

    if (data_time < start_time_) continue;  // Skip data until given date
    if (data_time > end_time_) {
      return;
    }  // Break out as we have loaded all events
    stdata* new_sample = new stdata();
    new_sample->secs_ = atoi(tokens[0]);
    new_sample->px_ = atof(tokens[6]);
    inst_data_.push_back(new_sample);
  }
}

int main(int argc, char** argv) {
  int c;  // getopt argument
  int hflag = 0;
  std::string logfilename_ = "";
  int start_date_ = 0;
  int end_date_ = 0;
  std::string ticker1_ = "";
  std::string ticker2_ = "";
  int sampling_time_ = 0;
  int mode_ = 0;
  int rolling_reg_period_ = 2500;
  double norm_factor_ = 0;
  bool adjusted_data_ = false;

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'b':
        logfilename_ = optarg;
        break;

      case 'c':
        start_date_ = atoi(optarg);
        break;

      case 'd':
        end_date_ = atoi(optarg);
        break;

      case 'f':
        ticker1_ = optarg;
        break;

      case 'g':
        ticker2_ = optarg;
        break;

      case 'o':
        sampling_time_ = atoi(optarg);
        break;

      case 'p':
        mode_ = atoi(optarg);
        break;

      case 'q':
        rolling_reg_period_ = atoi(optarg);
        break;

      case 'r':
        norm_factor_ = atof(optarg);
        break;

      case 's':
        adjusted_data_ = true;
        break;

      case '?':
        if (optopt == 'b' || optopt == 'c' || optopt == 'd' || optopt == 'f' || optopt == 'g' || optopt == 'o' ||
            optopt == 'p' || optopt == 'q' || optopt == 'r') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    print_usage(argv[0]);
    exit(-1);
  }

  if (start_date_ == 0 || end_date_ == 0 || ticker1_.empty() || ticker2_.empty() || logfilename_.empty() ||
      sampling_time_ == 0 || (mode_ > 1 || mode_ < 0) || norm_factor_ <= 1e-15) {
    print_usage(argv[0]);
    exit(-1);
  }

  // create debuglogger
  HFSAT::DebugLogger dbglogger_(256 * 1024, 1);
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios_base::app);

  // grab data
  int train_start_date = start_date_ - 10000;
  int train_end_date = start_date_;  // exclusive of the date

  struct tm start_timeinfo = {0};
  struct tm end_timeinfo = {0};
  struct tm end_timeinfo_test = {0};

  start_timeinfo.tm_year = (train_start_date / 10000) - 1900;
  start_timeinfo.tm_mon = ((train_start_date / 100) % 100) - 1;
  start_timeinfo.tm_mday = train_start_date % 100;

  // Start Time
  time_t start_time_of_start_train_day = mktime(&start_timeinfo);

  end_timeinfo.tm_year = (train_end_date / 10000) - 1900;
  end_timeinfo.tm_mon = ((train_end_date / 100) % 100) - 1;
  end_timeinfo.tm_mday = train_end_date % 100;

  // End Time
  time_t end_time_of_end_train_day = mktime(&end_timeinfo);

  end_timeinfo_test.tm_year = (end_date_ / 10000) - 1900;
  end_timeinfo_test.tm_mon = ((end_date_ / 100) % 100) - 1;
  end_timeinfo_test.tm_mday = end_date_ % 100;

  // End time of test period
  time_t end_time_of_test_day_ = mktime(&end_timeinfo_test);

  populateData(ticker1_, inst1_data_, start_time_of_start_train_day, end_time_of_test_day_, adjusted_data_);
  populateData(ticker2_, inst2_data_, start_time_of_start_train_day, end_time_of_test_day_, adjusted_data_);

  // get contemporaneous data points  in another vector
  std::vector<stdata*>::iterator iter_1 = inst1_data_.begin();
  std::vector<stdata*>::iterator iter_2 = inst2_data_.begin();
  while (iter_1 != inst1_data_.end() && iter_2 != inst2_data_.end()) {
    stdata* inst1_sample = *iter_1;
    stdata* inst2_sample = *iter_2;

    if (inst1_sample->secs_ < inst2_sample->secs_) {
      iter_1++;
      continue;
    } else if (inst2_sample->secs_ < inst1_sample->secs_) {
      iter_2++;
      continue;
    } else  // same time stamp
    {
      stdata* both_inst_sample = new stdata();
      both_inst_sample->secs_ = inst1_sample->secs_;
      if (mode_ == 0) {
        both_inst_sample->px_ = inst1_sample->px_;
        both_inst_sample->px2_ = inst2_sample->px_;
      } else {
        both_inst_sample->px_ = log(inst1_sample->px_);
        both_inst_sample->px2_ = log(inst2_sample->px_);
      }
      both_inst_data_.push_back(both_inst_sample);
      iter_1++;
      iter_2++;
    }
  }

  // at this point data upto training period is loaded
  // do rolling regression to estimate kalman initialization values
  double sum_px1 = 0;
  double sum_px1_sqr = 0;
  double sum_px2 = 0;
  double sum_px2_sqr = 0;
  double sum_px1_px2 = 0;
  double num_points = 0;

  double num_samples_ = 0;
  double sample_px1_sum = 0;
  double sample_px2_sum = 0;

  uint32_t last_recorded_data_secs_ = 0;
  size_t train_data_end_offset_ = both_inst_data_.size() - 1;

  for (size_t i = 0; i < both_inst_data_.size(); i++) {
    stdata* both_inst_sample = both_inst_data_[i];
    // for getting kalman params only train data is used
    if ((time_t)both_inst_sample->secs_ > end_time_of_end_train_day) {
      train_data_end_offset_ = i - 1;
      break;
    }

    uint32_t c_time_ = both_inst_sample->secs_;
    num_samples_ += 1;
    sample_px1_sum += both_inst_sample->px_;
    sample_px2_sum += both_inst_sample->px2_;

    if ((int32_t)(c_time_ - last_recorded_data_secs_) >= sampling_time_) {
      sample_px1_sum = sample_px1_sum / (num_samples_);
      sample_px2_sum = sample_px2_sum / (num_samples_);
      sum_px1 += sample_px1_sum;
      sum_px2 += sample_px2_sum;
      sum_px1_sqr += sample_px1_sum * sample_px1_sum;
      sum_px2_sqr += sample_px2_sum * sample_px2_sum;
      sum_px1_px2 += sample_px1_sum * sample_px2_sum;
      num_points += 1;

      sample_px1_sum = 0.0;
      sample_px2_sum = 0.0;
      num_samples_ = 0;

      last_recorded_data_secs_ = c_time_;
    }
  }

  // compute regression statistics
  double mean_2 = sum_px2 / num_points;
  // set intercept as 0 to start with - TODO revert
  //  double beta_ = ( sum_px1_px2 - num_points*mean_1*mean_2)/(sum_px2_sqr - num_points*mean_2*mean_2);
  //  double intercept_ = mean_1 - beta_*mean_2;
  double beta_ = (sum_px1_px2 / sum_px2_sqr);
  double intercept_ = 0;
  // compute stdev of regression error
  double variance_;
  double sum_error_sqr_ = 0.0;
  for (size_t i = 0; i < train_data_end_offset_; i++) {
    sum_error_sqr_ = (both_inst_data_[i]->px_ - beta_ * both_inst_data_[i]->px2_ - intercept_) *
                     (both_inst_data_[i]->px_ - beta_ * both_inst_data_[i]->px2_ - intercept_);
  }
  variance_ = sum_error_sqr_ / train_data_end_offset_;

  // Kalman class setup
  // setup vector Q for Kalman - vector is {intercept, beta} - state noise covariance
  std::vector<double> Q;

  /* The challenge is that using actual error covariance makes the intercept very spiky -
     Consequently beta becomes very spiky and can no longer be used as hedge ration.
     The compromise is a setup which allows intercept but all future variation is
     focussed in beta itself */
  //  Q.push_back(variance_*sum_px2_sqr/(num_points*(sum_px2_sqr - num_points*mean_2*mean_2)));
  //  Q.push_back(1e-08);
  Q.push_back(variance_ / (sum_px2_sqr - num_points * mean_2 * mean_2));
  Q.push_back(0);
  Q.push_back(0);
  //  Q.push_back(1e-08);
  //  Q.push_back(-1.0*mean_2*variance_/(sum_px2_sqr - num_points*mean_2*mean_2));
  //  Q.push_back(-1.0*mean_2*variance_/(sum_px2_sqr - num_points*mean_2*mean_2));
  Q.push_back(variance_ / (sum_px2_sqr - num_points * mean_2 * mean_2));

  // setup vector R for Kalman - observation noise covariance
  std::vector<double> R;
  R.push_back(variance_ * norm_factor_);

  // setup vector P for Kalman - estimate of errors in state observations
  std::vector<double> P;
  P = Q;
  //  std::transform( Q.begin(), Q.end(), Q.begin(), std::bind1st(std::multiplies<double>(), LEARNING_SPEED) );

  // setup vector A for kalman - I
  std::vector<double> A;
  A.push_back(1);
  A.push_back(0);
  A.push_back(0);
  A.push_back(1);

  // setup vector U for kalman - 0
  std::vector<double> U;
  U.push_back(0);

  // setup vector W for kalman - 2*2 I matrix
  std::vector<double> W;
  W.push_back(1);
  W.push_back(0);
  W.push_back(0);
  W.push_back(1);

  // setup vector V for kalman - 1*1 I matrix
  std::vector<double> V;
  V.push_back(1);

  // setup vector H for kalman
  std::vector<double> H;
  H.push_back(1);
  H.push_back(both_inst_data_[0]->px2_);

  // initial state estimate
  std::vector<double> X;
  X.push_back(intercept_);
  X.push_back(beta_);

  // observation
  std::vector<double> Y;
  Y.push_back(both_inst_data_[0]->px_);

  // create Kalman instance
  Kalman::KalmanReg* kalman_ = new Kalman::KalmanReg(2, 1, 2, 1, 1, dbglogger_);
  kalman_->setA(A);
  kalman_->setU(U);
  kalman_->setR(R);
  kalman_->setW(W);
  kalman_->setV(V);
  kalman_->setH(H);
  kalman_->setQ(Q);
  kalman_->init_Kalman(X, P);

  std::vector<double> kalman_intercept_;
  std::vector<double> kalman_beta_;
  std::vector<double> rolling_intercept_vec_;
  std::vector<double> rolling_beta_vec_;
  std::vector<double> kalman_error_;
  std::vector<double> static_error_;
  std::vector<double> rolling_error_;

  // same vars being resused for rolling regression
  sum_px1 = 0;
  sum_px1_sqr = 0;
  sum_px2 = 0;
  sum_px2_sqr = 0;
  sum_px1_px2 = 0;
  num_points = 0;

  double rolling_beta_;
  double rolling_intercept_;
  //#define PRINT_FREQ 2500

  double kalman_error_sum_ = 0.0;
  double rolling_error_sum_ = 0.0;
  double kalman_error_sum_sqr_ = 0.0;
  double rolling_error_sum_sqr_ = 0.0;

  //  for( size_t i = 0; i < both_inst_data_.size() - 1; i++ )
  for (size_t i = 0; i < train_data_end_offset_ - 1; i++) {
    stdata* this_sample_data = both_inst_data_[i];
    // rolling regression computations
    sum_px1 += this_sample_data->px_;
    sum_px2 += this_sample_data->px2_;
    sum_px1_sqr += this_sample_data->px_ * this_sample_data->px_;
    sum_px2_sqr += this_sample_data->px2_ * this_sample_data->px2_;
    sum_px1_px2 += this_sample_data->px_ * this_sample_data->px2_;
    num_points += 1;

    // kalman update
    Y[0] = this_sample_data->px_;
    kalman_->setObservation(Y);
    H[1] = this_sample_data->px2_;
    kalman_->setH(H);
    kalman_->updateWeights();

    if (num_points == rolling_reg_period_ + 1) {
      mean_2 = sum_px2 / num_points;
      //      rolling_beta_ = ( sum_px1_px2 - num_points*mean_1*mean_2)/(sum_px2_sqr - num_points*mean_2*mean_2);
      //      rolling_intercept_ = mean_1 - rolling_beta_*mean_2;
      rolling_beta_ = sum_px1_px2 / sum_px2_sqr;
      rolling_intercept_ = 0.0;

      num_points -= 1;
      sum_px1 -= both_inst_data_[i - rolling_reg_period_]->px_;
      sum_px2 -= both_inst_data_[i - rolling_reg_period_]->px2_;
      sum_px1_sqr -= both_inst_data_[i - rolling_reg_period_]->px_ * both_inst_data_[i - rolling_reg_period_]->px_;
      sum_px2_sqr -= both_inst_data_[i - rolling_reg_period_]->px2_ * both_inst_data_[i - rolling_reg_period_]->px2_;
      sum_px1_px2 -= both_inst_data_[i - rolling_reg_period_]->px_ * both_inst_data_[i - rolling_reg_period_]->px2_;

      kalman_intercept_.push_back(kalman_->getKalmanParam(0));
      kalman_beta_.push_back(kalman_->getKalmanParam(1));
      rolling_beta_vec_.push_back(rolling_beta_);

      double t_kalman_error_ = both_inst_data_[i + 1]->px_ - both_inst_data_[i + 1]->px2_ * kalman_->getKalmanParam(1) -
                               kalman_->getKalmanParam(0);
      double t_rolling_error_ =
          both_inst_data_[i + 1]->px_ - rolling_beta_ * both_inst_data_[i + 1]->px2_ - rolling_intercept_;
      kalman_error_sum_ += t_kalman_error_;
      kalman_error_sum_sqr_ += t_kalman_error_ * t_kalman_error_;
      rolling_error_sum_ += t_rolling_error_;
      rolling_error_sum_sqr_ += t_rolling_error_ * t_rolling_error_;

      kalman_error_.push_back(t_kalman_error_);
      static_error_.push_back(both_inst_data_[i + 1]->px_ - beta_ * both_inst_data_[i + 1]->px2_ - intercept_);
      rolling_error_.push_back(t_rolling_error_);
      //      std::cout << kalman_->getKalmanParam(0) << ' ' << kalman_->getKalmanParam(1) << ' '
      //      		<< rolling_intercept_ << ' ' << rolling_beta_ << ' ' << static_error_[static_error_.size()-1] <<
      //      '
      //      '
      //      		<< rolling_error_[rolling_error_.size()-1] << ' ' << kalman_error_[kalman_error_.size()-1] <<
      //      '\n';
    }

    // if( i % PRINT_FREQ == 0 )
    // {
    //    std::cerr << "KalmanP " << kalman_->getKalmanP(0,0) << ' ' << kalman_->getKalmanP(0,1) << ' ' <<
    //    kalman_->getKalmanP(1,0) << ' ' << kalman_->getKalmanP(1,1) << '\n';
    // }
  }

  std::cout << "NF " << norm_factor_
            << " KError: " << sqrt(kalman_error_sum_sqr_ / kalman_error_.size() -
                                   kalman_error_sum_ / kalman_error_.size() * kalman_error_sum_ / kalman_error_.size())
            << " RError: "
            << sqrt(rolling_error_sum_sqr_ / rolling_error_.size() -
                    rolling_error_sum_ / rolling_error_.size() * rolling_error_sum_ / rolling_error_.size())
            << " KParams: " << kalman_->getKalmanParam(0) << ' ' << kalman_->getKalmanParam(1)
            << " PMatrix: " << kalman_->getKalmanP(0, 0) << ' ' << kalman_->getKalmanP(0, 1) << ' '
            << kalman_->getKalmanP(1, 0) << ' ' << kalman_->getKalmanP(1, 1)
            << " QMatrix: " << kalman_->getKalmanQ(0, 0) << ' ' << kalman_->getKalmanQ(0, 1) << ' '
            << kalman_->getKalmanQ(1, 0) << ' ' << kalman_->getKalmanQ(1, 1)
            << " RMatrix: " << kalman_->getKalmanR(0, 0) << '\n';
}
