/**
    \file Tools/timed_data_to_reg_data_bd.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "basetrade/Tools/timed_data_to_reg_data_bd.hpp"
#include "basetrade/Tools/simple_line_bwriter.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &input_data_filename_,
                            std::string &output_data_filename_, int &msecs_to_predict_, int &msecs_collar_,
                            double &min_px_incr_) {
  // expect :
  // 1. $dat_to_reg INPUTDATAFILENAME OUTPUTDATAFILENAME MSECS_TO_PREDICT MSECS_COLLAR MIN_PX_INCR
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " INPUTDATAFILENAME OUTPUTDATAFILENAME MSECS_TO_PREDICT MSECS_COLLAR MIN_PX_INCR " << std::endl;
    HFSAT::ExitVerbose(HFSAT::kDatToRegCommandLineLessArgs);
  } else {
    input_data_filename_ = argv[1];
    output_data_filename_ = argv[2];
    msecs_to_predict_ = atoi(argv[3]);
    msecs_collar_ = atoi(argv[4]);
    min_px_incr_ = atof(argv[5]);
  }
}

int main(int argc, char **argv) {
  std::string input_data_filename_;
  std::string output_data_filename_;
  int msecs_to_predict_;
  int msecs_collar_;
  double min_px_incr_;

  ParseCommandLineParams(argc, (const char **)argv, input_data_filename_, output_data_filename_, msecs_to_predict_,
                         msecs_collar_, min_px_incr_);

  HFSAT::SimpleLineBWriter simple_line_bwriter_(output_data_filename_);
  HFSAT::T3BDProcessing(input_data_filename_, simple_line_bwriter_, msecs_to_predict_, msecs_collar_, min_px_incr_);
}
