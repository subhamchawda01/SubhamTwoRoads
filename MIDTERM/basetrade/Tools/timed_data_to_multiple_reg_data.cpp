/**
    \file Tools/timed_data_to_multiple_reg_data.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/buffered_vec.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "basetrade/Tools/td2rd_utils.hpp"
#include "basetrade/Tools/timed_data_to_reg_data.hpp"
#include "basetrade/Tools/timed_data_to_multiple_reg_data.hpp"
#include "basetrade/Tools/simple_line_bwriter.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& model_filename_,
                            std::string& input_data_filename_, std::vector<int>& counters_to_predict_,
                            HFSAT::NormalizingAlgo& normalizing_algo_,
                            std::vector<std::string>& output_data_filename_) {
  // expect :
  // 1. $dat_to_reg MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0] << " MODELFILENAME INPUTDATAFILENAME NORMALIZING_ALGO PREDICTION_COUNTER_1 "
                                         "OUTPUT_FILE_1 [ PREDICTION_COUNTER_2 OUTPUT_FILE_2 ...] " << std::endl;
    HFSAT::ExitVerbose(HFSAT::kDatToRegCommandLineLessArgs);
  } else {
    model_filename_ = argv[1];
    input_data_filename_ = argv[2];
    normalizing_algo_ = HFSAT::StringToNormalizingAlgo(argv[3]);

    for (int i = 4; i + 1 < argc; i += 2) {
      counters_to_predict_.push_back(std::max(1, (int)round(atof(argv[i]))));
      output_data_filename_.push_back(argv[i + 1]);
    }
  }
}

/// input arguments : model_filename input_data_filename counters_to_predict_ normalizing_algo_ outputdatafile
/// model_filename is used to find out the parameters of computation of returns / change
/// inputdatafile format :
/// [ msecs_from_midnight event_count_from_start predprice baseprice var1 var2 ... varn ]
/// outputdatafile format :
/// [ ( predprice - baseprice ) var1 ... varn ]
int main(int argc, char** argv) {
  std::string model_filename_;
  std::string input_data_filename_;
  std::vector<std::string> output_data_filename_;

  bool is_returns_based_ = true;
  std::vector<int> counters_to_predict_;  // = 5000; ///< could be msecs_to_predict or events_to_predict
  HFSAT::NormalizingAlgo normalizing_algo_ = HFSAT::na_t1;

  ParseCommandLineParams(argc, (const char**)argv, model_filename_, input_data_filename_, counters_to_predict_,
                         normalizing_algo_, output_data_filename_);

  if (!HFSAT::GetDepCalcParams(model_filename_, is_returns_based_)) {
    std::cerr << "GetDepCalcParams did not find a line like MODELMATH CART CHANGE 0.5 na_t1" << std::endl;
    exit(0);
  }

  std::vector<HFSAT::SimpleLineProcessor*> lineProcessors;
  for (auto i = 0u; i < output_data_filename_.size(); ++i) {
    lineProcessors.push_back(new HFSAT::SimpleLineBWriter(output_data_filename_[i]));
  }

  HFSAT::BufferedDataSource<HFSAT::TD2RD_UTILS::TimedData>* src =
      new HFSAT::TD2RD_UTILS::TimedDataFileSource<HFSAT::TD2RD_UTILS::TimedData>(input_data_filename_);
  HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData> td_vec = HFSAT::BufferedVec<HFSAT::TD2RD_UTILS::TimedData>(src);
  HFSAT::MultipleTd2RdProcessing(td_vec, lineProcessors, is_returns_based_, counters_to_predict_, normalizing_algo_,
                                 false, NULL, false);
}
