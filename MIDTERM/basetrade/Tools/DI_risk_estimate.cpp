/**
   \file Tools/DI_risk_estimate.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& position_filename_, int& yyyymmdd_) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " shc_code_ date " << std::endl;
    exit(0);
  } else {
    position_filename_ = argv[1];
    yyyymmdd_ = atoi(argv[2]);
  }
}

void LoadMarginScenario(int yyyymmdd_, std::vector<std::vector<double> >& margin_scenario_matrix_,
                        std::vector<std::vector<double> >& margin_scenario_pvalue_matrix_,
                        std::vector<std::vector<double> >& vertex_pvalue_vec_) {
  std::ifstream t_margin_scenario_infile_;
  std::string t_margin_scenario_infilename_;

  int this_YYYYMMDD_ = yyyymmdd_;  // Start yesterday.

  for (unsigned int ii = 0; ii < 40; ii++) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/tradeinfo/StructureInfo/scenarios/scenario." << this_YYYYMMDD_ << ".txt";
    t_margin_scenario_infilename_ = t_temp_oss_.str();

    if (FileUtils::exists(t_margin_scenario_infilename_)) {
      break;
    } else {
      // Try previous day
      this_YYYYMMDD_ = DateTime::CalcPrevDay(this_YYYYMMDD_);
    }
  }

  if (!FileUtils::exists(t_margin_scenario_infilename_)) {  // All attempts failed.
    // TODO ExitVerbose
  }

  t_margin_scenario_infile_.open(t_margin_scenario_infilename_);

  if (t_margin_scenario_infile_.is_open()) {
    const int kL1AvgBufferLen = 4096;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);

    while (t_margin_scenario_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_margin_scenario_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 10) {
        std::vector<double> t_vertex_row_;
        for (unsigned int i = 2; i < 10; i++) {
          t_vertex_row_.push_back((atof(tokens_[i])));
        }

        margin_scenario_matrix_.push_back(t_vertex_row_);
        margin_scenario_pvalue_matrix_.push_back(std::vector<double>(8, 0.0));
        vertex_pvalue_vec_.push_back(0.0);
      }
    }
  } else {
    // TODO ExitVerbose
  }
}

void ReadPositions(const std::string position_filename_, std::vector<int>& positions_,
                   std::vector<std::string> shortcodes_vec_)

{
  std::ifstream t_position_infile_;

  t_position_infile_.open(position_filename_.c_str());

  if (t_position_infile_.is_open()) {
    const int kL1AvgBufferLen = 4096;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);

    while (t_position_infile_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_position_infile_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() > 1) {
        shortcodes_vec_.push_back(tokens_[0]);
        positions_.push_back(atoi(tokens_[1]));
      }
    }
  } else {
    // TODO ExitVerbose
  }
}

void CalculateMargin() {
  double t_margin_ = 0.0;

  for (auto i = 0u; i < 8; i++) {
    double t_scenario_pnl_ = 0.0;
    for (unsigned int j = 0; j < positions_.size(); j++) {
      t_scenario_pnl_ +=
          positions_[j] * first_vertex_weight_vec_[j] * (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j]][i] -
                                                         vertex_pvalue_vec_[first_vertex_index_vec_[j]]);
      t_scenario_pnl_ += positions_[j] * (1 - first_vertex_weight_vec_[j]) *
                         (margin_scenario_pvalue_matrix_[first_vertex_index_vec_[j] + 1][i] -
                          vertex_pvalue_vec_[first_vertex_index_vec_[j] + 1]);
    }
    if (t_scenario_pnl_ < t_margin_) {
      t_margin_ = t_scenario_pnl_;
    }
  }

  margin_ = t_margin_;
}

inline void SetVertices(int yyyymmdd_, std::vector<std::string> shortcode_vec_,
                        std::vector<unsigned int> first_vertex_index_vec_,
                        std::vector<double> first_vertex_weight_vec_, ) {
  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    unsigned int term_ = CurveUtils::_get_term_(yyyymmdd_, shortcode_vec_[i]);
    double t_first_vertex_weight_ = (((double)term_) / 21.0) - (term_ / 21);
    first_vertex_index_vec_.push_back(term_ / 21);
    first_vertex_weight_vec_.push_back(t_first_vertex_weight_);
  }
}

int main(int argc, char** argv) {}
