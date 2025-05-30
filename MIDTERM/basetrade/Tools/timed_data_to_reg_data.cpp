/**
   \file Tools/timed_data_to_reg_data.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "basetrade/Tools/timed_data_to_reg_data.hpp"
#include "basetrade/Tools/simple_line_bwriter.hpp"
#include "basetrade/Tools/daily_volume_reader.hpp"
#include "basetrade/Tools/multprocessing.hpp"

HFSAT::NormalizingAlgo global_normalizing_algo_ = HFSAT::StringToNormalizingAlgo("na_s4");

void ParseCommandLineParams(const int argc, const char **argv, std::string &model_filename_,
                            std::string &input_data_filename_, std::vector<int> &counters_to_predict_,
                            std::vector<HFSAT::NormalizingAlgo> &normalizing_algo_, std::string &output_data_filename_,
                            std::string &trade_volume_file_, bool &print_time_, unsigned int &fsudm_filter_level_) {
  // expect :
  // 1. $dat_to_reg MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0] << " MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO "
                                         "OUTPUTDATAFILENAME <FILTER>? [PRINT_TIME = 0] <fsudm_level>"
              << std::endl;
    HFSAT::ExitVerbose(HFSAT::kDatToRegCommandLineLessArgs);
  } else {
    model_filename_ = argv[1];
    input_data_filename_ = argv[2];
    // counters_to_predict_ = std::max ( 1, (int)round ( atof ( argv[3] ) ) );
    global_normalizing_algo_ = HFSAT::StringToNormalizingAlgo(argv[4]);
    if (global_normalizing_algo_ == HFSAT::na_mult) {
      std::string config_file_ = argv[3];
      std::ifstream config_file_base_;
      config_file_base_.open(config_file_.c_str(), std::ifstream::in);
      char base_line_buffer_[1000];
      bzero(base_line_buffer_, 1000);
      while (config_file_base_.good()) {
        bzero(base_line_buffer_, 1000);
        config_file_base_.getline(base_line_buffer_, 1000);
        HFSAT::PerishableStringTokenizer base_st_(base_line_buffer_, 1000);
        const std::vector<const char *> &base_tokens_ = base_st_.GetTokens();
        if (base_tokens_.size() != 2 && base_tokens_.size() != 0) {
          std::cerr << "Invalid config file " << std::endl;
          exit(1);
        } else if (base_tokens_.size() == 0)
          continue;
        else {
          counters_to_predict_.push_back(std::max(1, (int)round(atof(base_tokens_[0]))));
          normalizing_algo_.push_back(HFSAT::StringToNormalizingAlgo(base_tokens_[1]));
          std::cout << "Algo : " << normalizing_algo_[normalizing_algo_.size() - 1]
                    << " Duration : " << counters_to_predict_[normalizing_algo_.size() - 1] << std::endl;
        }
      }

    } else {
      counters_to_predict_.push_back(std::max(1, (int)round(atof(argv[3]))));
      normalizing_algo_.push_back(global_normalizing_algo_);
    }
    output_data_filename_ = argv[5];
    if (argc >= 7) {
      trade_volume_file_ = argv[6];
    }
    if (argc >= 8) {
      print_time_ = (atoi(argv[7]) != 0);
    }
    if (argc >= 9) {
      fsudm_filter_level_ = std::max(0, std::min(3, atoi(argv[8])));
    }
  }
}

/// input arguments : model_filename input_data_filename counters_to_predict_ normalizing_algo_ outputdatafile
/// model_filename is used to find out the parameters of computation of returns / change
/// inputdatafile format :
/// [ msecs_from_midnight event_count_from_start predprice baseprice var1 var2 ... varn ]
/// outputdatafile format :
/// [ ( predprice - baseprice ) var1 ... varn ]
int main(int argc, char **argv) {
  std::string model_filename_;
  std::string input_data_filename_;
  std::string output_data_filename_;
  std::string trade_volume_file_ = "";
  bool print_time_ = false;
  bool is_returns_based_ = true;
  std::vector<int> counters_to_predict_;
  // int counters_to_predict_ = 5000; ///< could be msecs_to_predict or events_to_predict
  std::vector<HFSAT::NormalizingAlgo> normalizing_algo_;
  unsigned int fsudm_filter_level_ = 0;  // ( 0 -> 1 ) ( 1-> abs ( y ) ) ( 2-> y^2 ) ( 3-> abs ( y ) * y^2 )

  ParseCommandLineParams(argc, (const char **)argv, model_filename_, input_data_filename_, counters_to_predict_,
                         normalizing_algo_, output_data_filename_, trade_volume_file_, print_time_,
                         fsudm_filter_level_);

  if (!HFSAT::GetDepCalcParams(model_filename_, is_returns_based_)) {
    std::cerr << "GetDepCalcParams did not find a line like MODELMATH CART CHANGE 0.5 na_t1" << std::endl;
    exit(0);
  }
  DailyTradeVolumeReader *tvr = NULL;
  int trade_file_size_ = 0;
  if (trade_volume_file_ != "") {
    if (HFSAT::FileUtils::ExistsWithSize(trade_volume_file_, trade_file_size_)) {
      tvr = new DailyTradeVolumeReader(trade_volume_file_);
    }
  }

  switch (global_normalizing_algo_) {
    case HFSAT::na_t1: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T1Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_t3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_t5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_e1: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E1Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_e3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_e5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::ac_e3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::ACE3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                            print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::ac_e5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::ACE5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                            print_time_, fsudm_filter_level_);
    } break;
    case HFSAT::na_s4: {  // 4 columns of dependant created based on time
      if (fsudm_filter_level_ > 0) {
        std::cerr << "fsudm is not implemented fpr na_s4\n";
      }
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::S4Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_);
    } break;
    case HFSAT::na_sall: {  // all columns of dependant created based on time
      if (fsudm_filter_level_ > 0) {
        std::cerr << "fsudm is not implemented for na_sall\n";
      }
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::SAllProcessing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0],
                            tvr);
    } break;
    case HFSAT::na_mult: {  // all columns of dependant created based on time
      if (fsudm_filter_level_ > 0) {
        std::cerr << "fsudm is not implemented for na_mult\n";
      }
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      // std::cerr<<"Not implemented for this predalgo\n";
      // exit(1);
      HFSAT::MultProcessing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_,
                            normalizing_algo_, tvr);
    } break;
    default:
    case HFSAT::na_m4: {  // 4 columns of dependant created based on events
      if (fsudm_filter_level_ > 0) {
        std::cerr << "fsudm is not implemented for na_m4\n";
      }
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::M4Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_[0], tvr,
                          print_time_);
    } break;
  }
}
