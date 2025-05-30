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

void ParseCommandLineParams(const int argc, const char **argv, std::string &model_filename_,
                            std::string &input_data_filename_, int &counters_to_predict_,
                            HFSAT::NormalizingAlgo &normalizing_algo_, std::string &output_data_filename_,
                            std::string &trade_volume_file_) {
  // expect :
  // 1. $dat_to_reg MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME
  if (argc < 6) {
    std::cerr
        << "Usage: " << argv[0]
        << " MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME <FILTER>?"
        << std::endl;
    HFSAT::ExitVerbose(HFSAT::kDatToRegCommandLineLessArgs);
  } else {
    model_filename_ = argv[1];
    input_data_filename_ = argv[2];
    counters_to_predict_ = std::max(1, (int)round(atof(argv[3])));
    normalizing_algo_ = HFSAT::StringToNormalizingAlgo(argv[4]);
    output_data_filename_ = argv[5];
    if (argc >= 7) {
      trade_volume_file_ = argv[6];
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

  bool is_returns_based_ = true;
  int counters_to_predict_ = 5000;  ///< could be msecs_to_predict or events_to_predict
  HFSAT::NormalizingAlgo normalizing_algo_ = HFSAT::na_t1;

  ParseCommandLineParams(argc, (const char **)argv, model_filename_, input_data_filename_, counters_to_predict_,
                         normalizing_algo_, output_data_filename_, trade_volume_file_);

  if (!HFSAT::GetDepCalcParams(model_filename_, is_returns_based_)) {
    std::cerr << "GetDepCalcParams did not find a line like MODELMATH CART CHANGE 0.5 na_t1" << std::endl;
    exit(0);
  }
  DailyTradeVolumeReader *tvr = NULL;
  if (trade_volume_file_ != "") {
    tvr = new DailyTradeVolumeReader(trade_volume_file_);
  }

  switch (normalizing_algo_) {
    case HFSAT::na_t1: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T1Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_t3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_t5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::T5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_e1: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E1Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_e3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_e5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::E5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::ac_e3: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::ACE3Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::ac_e5: {
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::ACE5Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    case HFSAT::na_s4: {  // 4 columns of dependant created based on time
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::S4Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
    default:
    case HFSAT::na_m4: {  // 4 columns of dependant created based on events
      HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
      HFSAT::M4Processing(input_data_filename_, simple_line_bwriter_, is_returns_based_, counters_to_predict_, tvr);
    } break;
  }
}
