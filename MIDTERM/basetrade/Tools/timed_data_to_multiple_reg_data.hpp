/**
 \file Tools/timed_data_to_reg_data.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/Tools/simple_line_processor.hpp"
#include "basetrade/Tools/td2rd_utils.hpp"

namespace HFSAT {

void PrepareCounterGaps(const HFSAT::NormalizingAlgo alg, const std::vector<int> intialCounters,
                        std::vector<int>& modifiedCounters) {
  switch (alg) {
    case na_t1:
    case na_e1: {
      for (auto i = 0u; i < intialCounters.size(); ++i) {
        modifiedCounters.push_back(intialCounters[i]);
      }
    } break;
    case na_t3:
    case na_e3:
    case ac_e3: {
      for (auto i = 0u; i < intialCounters.size(); ++i) {
        modifiedCounters.push_back(intialCounters[i] * 1 / 3);
        modifiedCounters.push_back(intialCounters[i] * 2 / 3);
        modifiedCounters.push_back(intialCounters[i]);
      }
    } break;
    case na_e5:
    case na_t5:
    case ac_e5: {
      for (auto i = 0u; i < intialCounters.size(); ++i) {
        modifiedCounters.push_back(intialCounters[i] * 1 / 5);
        modifiedCounters.push_back(intialCounters[i] * 2 / 5);
        modifiedCounters.push_back(intialCounters[i] * 3 / 5);
        modifiedCounters.push_back(intialCounters[i] * 4 / 5);
        modifiedCounters.push_back(intialCounters[i]);
      }
    } break;
    default: {  // for na_m4 na_s4
      for (auto i = 0u; i < intialCounters.size(); ++i) {
        modifiedCounters.push_back(intialCounters[i]);
      }
    }
  }
}

double GetValue2Model(const HFSAT::NormalizingAlgo alg, const bool is_returns_based_, const double baseVal,
                      const std::vector<double>& predictedValues, int index, bool& isValidRetVal) {
  switch (alg) {
    case na_t1:
    case na_e1: {
      if (predictedValues[index] == DATA_NOT_PRESENT) {
        isValidRetVal = false;
        return -1;
      }
      register double valToModel = (predictedValues[index] - baseVal);
      if (is_returns_based_) {
        valToModel /= baseVal;
      }
      return valToModel;
    } break;
    case na_t3:
    case na_e3:
    case ac_e3: {
      double nfac1_ = 4.0 / 9.0;
      double nfac2_ = 3.0 / 9.0;
      double nfac3_ = 2.0 / 9.0;

      if (predictedValues[index * 3] == DATA_NOT_PRESENT || predictedValues[index * 3 + 1] == DATA_NOT_PRESENT ||
          predictedValues[index * 3 + 2] == DATA_NOT_PRESENT) {
        isValidRetVal = false;
        return -1;
      }

      double pred_value_ = (nfac1_ * (predictedValues[index * 3] - baseVal)) +
                           (nfac2_ * (predictedValues[index * 3 + 1] - baseVal)) +
                           (nfac3_ * (predictedValues[index * 3 + 2] - baseVal));
      double valToModel = pred_value_;
      if (is_returns_based_) {
        valToModel = (pred_value_ / baseVal);
      }
      return valToModel;
    } break;
    case na_e5:
    case ac_e5: {
      if (predictedValues[index * 5] == DATA_NOT_PRESENT || predictedValues[index * 5 + 1] == DATA_NOT_PRESENT ||
          predictedValues[index * 5 + 2] == DATA_NOT_PRESENT || predictedValues[index * 5 + 3] == DATA_NOT_PRESENT ||
          predictedValues[index * 5 + 4] == DATA_NOT_PRESENT) {
        isValidRetVal = false;
        return -1;
      }

      // assume value at c/5*k is proportional to sqrt(k), hence nfac is proportional to 1/ sqrt(k)
      double nfac1_ = (1.00 / sqrt(1.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                             (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
      double nfac2_ = (1.00 / sqrt(2.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                             (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
      double nfac3_ = (1.00 / sqrt(3.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                             (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
      double nfac4_ = (1.00 / sqrt(4.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                             (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
      double nfac5_ = (1.00 / sqrt(5.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                             (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));

      double pred_value_ =
          (nfac1_ * (predictedValues[index * 5] - baseVal)) + (nfac2_ * (predictedValues[index * 5 + 1] - baseVal)) +
          (nfac3_ * (predictedValues[index * 5 + 2] - baseVal)) +
          (nfac4_ * (predictedValues[index * 5 + 3] - baseVal)) + (nfac5_ * (predictedValues[index * 5 + 4] - baseVal));

      double valToModel = pred_value_;
      if (is_returns_based_) {
        valToModel = (pred_value_ / baseVal);
      }
      return valToModel;
    } break;
    default:
      return -1;
  }
}

void MultipleTd2RdProcessing(BufferedVec<TD2RD_UTILS::TimedData>& td_vec,
                             std::vector<SimpleLineProcessor*>& line_processor_vectors, const bool is_returns_based_,
                             std::vector<int> counters_to_predict_, HFSAT::NormalizingAlgo alg,
                             bool isFVRequired,  // if true and if trade_vol_Reader is not null, then we will have 2 *
                                                 // counters_to_predict_.size() number of outputs
                             DailyTradeVolumeReader* trade_vol_reader, bool fsudm_output_) {
  // assert(line_processor_vectors.size() == counters_to_predict_.size());

  std::vector<int> pred_gaps;
  PrepareCounterGaps(alg, counters_to_predict_, pred_gaps);

  HFSAT::TD2RD_UTILS::TD2PredictionData td2pd =
      (alg == HFSAT::na_e1 || alg == HFSAT::na_e3 || alg == HFSAT::na_e5 || alg == HFSAT::ac_e3)
          ? HFSAT::TD2RD_UTILS::TD2PredictionData(&td_vec, pred_gaps, true)
          : HFSAT::TD2RD_UTILS::TD2PredictionData(&td_vec, pred_gaps, false);

  // ac_e3 specific processing
  std::vector<std::vector<double> > value2ModelVec;
  std::vector<std::vector<double> > basePriceVec;
  std::vector<std::vector<double> > indicators_;
  if (alg == HFSAT::ac_e3) {
    value2ModelVec.resize(
        counters_to_predict_.size());  // resize it to number of different time periods for which this algo is running
    basePriceVec.resize(counters_to_predict_.size());
  }

  std::vector<double> factor_for_ac_e3;

  while (td2pd.predictForNextEvent()) {
    TD2RD_UTILS::TimedData* base_data = td2pd.getBaseData();
    if (base_data == NULL) break;
    isFVRequired = isFVRequired && (trade_vol_reader != NULL);
    double fv_factor =
        trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_data->msec / 1000);

    std::vector<double>* pred_data = td2pd.getPredictionData();

    if (alg == HFSAT::ac_e3) {
      // store it in memory and process later
      if (indicators_.empty()) indicators_.resize(base_data->num_indicators);

      for (auto i = 0u; i < indicators_.size(); ++i) indicators_[i].push_back(base_data->indicator_values[i]);
    }

    for (auto i = 0u; i < counters_to_predict_.size(); ++i) {
      bool isValidRetVal = true;  // passed by reference. we ensure that if future data for some interval is not
                                  // present, we don't log anything for that event
      double valToModel = GetValue2Model(alg, is_returns_based_, base_data->prices[1], *pred_data, i, isValidRetVal);
      if (!isValidRetVal) continue;
      if (alg == HFSAT::ac_e3) {
        value2ModelVec[i].push_back(valToModel);
        basePriceVec[i].push_back(base_data->prices[1]);  // base price has index 1 now
        factor_for_ac_e3.push_back(fv_factor);
      } else {
        if (isFVRequired && fsudm_output_) {
          // without factor---------
          line_processor_vectors[i * 3]->AddWord(valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 3]->AddWord(base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 3]->FinishLine();

          // with fv factor---------
          line_processor_vectors[i * 3 + 1]->AddWord(fv_factor * valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 3 + 1]->AddWord(fv_factor * base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 3 + 1]->FinishLine();

          // with fsudm factor---------
          double fsudm_factor_ = std::fabs(valToModel);
          line_processor_vectors[i * 3 + 2]->AddWord(fsudm_factor_ * valToModel);

          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 3 + 2]->AddWord(fsudm_factor_ * base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 3 + 2]->FinishLine();
        } else if (isFVRequired) {
          // without factor---------
          line_processor_vectors[i * 2]->AddWord(valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 2]->AddWord(base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 2]->FinishLine();

          // with fv factor---------
          line_processor_vectors[i * 2 + 1]->AddWord(fv_factor * valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 2 + 1]->AddWord(fv_factor * base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 2 + 1]->FinishLine();
        } else if (fsudm_output_) {
          // without factor---------
          line_processor_vectors[i * 2]->AddWord(valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 2]->AddWord(base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 2]->FinishLine();

          // with fsudm factor---------
          double fsudm_factor_ = std::fabs(valToModel);
          line_processor_vectors[i * 2 + 1]->AddWord(fsudm_factor_ * valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i * 2 + 1]->AddWord(fsudm_factor_ * base_data->indicator_values[j]);
          }
          line_processor_vectors[i * 2 + 1]->FinishLine();
        } else {
          line_processor_vectors[i]->AddWord(valToModel);

          // now print the variables
          for (unsigned int j = 0; j < base_data->num_indicators; ++j) {
            line_processor_vectors[i]->AddWord(base_data->indicator_values[j]);
          }
          line_processor_vectors[i]->FinishLine();
        }
      }
    }
  }

  if (alg == HFSAT::ac_e3) {
    VectorUtils::CalcAndRemoveMeanFromSeriesVec(basePriceVec);
    VectorUtils::CalcAndRemoveMeanFromSeriesVec(value2ModelVec);

    for (unsigned int k = 0; k < counters_to_predict_.size(); ++k) {
      double mean_rev_beta_ = GetSLRCoeffNoMean(value2ModelVec[k], basePriceVec[k]);
      VectorUtils::ScaledVectorAddition(value2ModelVec[k], basePriceVec[k], -mean_rev_beta_);

      for (auto i = 0u; i < value2ModelVec[k].size(); i++) {
        if (isFVRequired && fsudm_output_) {
          {
            // print value_to_model
            line_processor_vectors[k * 3]->AddWord(value2ModelVec[k][i]);
            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 3]->AddWord(indicators_[j][i]);
            line_processor_vectors[k * 3]->FinishLine();
          }

          // With fv factor
          {
            // print value_to_model
            line_processor_vectors[k * 3 + 1]->AddWord(factor_for_ac_e3[i] * value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 3 + 1]->AddWord(factor_for_ac_e3[i] * indicators_[j][i]);
            line_processor_vectors[k * 3 + 1]->FinishLine();
          }

          // With fsudm factor
          {
            // print value_to_model
            double fsudm_factor_ = std::fabs(value2ModelVec[k][i]);
            line_processor_vectors[k * 3 + 2]->AddWord(fsudm_factor_ * value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 3 + 2]->AddWord(fsudm_factor_ * indicators_[j][i]);
            line_processor_vectors[k * 3 + 2]->FinishLine();
          }
        }
        if (isFVRequired) {
          // without factor
          {
            // print value_to_model
            line_processor_vectors[k * 2]->AddWord(value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 2]->AddWord(indicators_[j][i]);
            line_processor_vectors[k * 2]->FinishLine();
          }

          // With factor
          {
            // print value_to_model
            line_processor_vectors[k * 2 + 1]->AddWord(factor_for_ac_e3[i] * value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 2 + 1]->AddWord(factor_for_ac_e3[i] * indicators_[j][i]);
            line_processor_vectors[k * 2 + 1]->FinishLine();
          }
        }
        if (fsudm_output_) {
          // without factor
          {
            // print value_to_model
            line_processor_vectors[k * 2]->AddWord(value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 2]->AddWord(indicators_[j][i]);
            line_processor_vectors[k * 2]->FinishLine();
          }
          // With fsudm factor
          {
            // print value_to_model
            double fsudm_factor_ = std::fabs(value2ModelVec[k][i]);
            line_processor_vectors[k * 2 + 1]->AddWord(fsudm_factor_ * value2ModelVec[k][i]);

            // now print the variables
            for (unsigned int j = 0u; j < indicators_.size(); j++)
              line_processor_vectors[k * 2 + 1]->AddWord(fsudm_factor_ * indicators_[j][i]);
            line_processor_vectors[k * 2 + 1]->FinishLine();
          }
        } else {
          // print value_to_model
          line_processor_vectors[k]->AddWord(factor_for_ac_e3[i] * value2ModelVec[k][i]);

          // now print the variables
          for (unsigned int j = 0u; j < indicators_.size(); j++)
            line_processor_vectors[k]->AddWord(factor_for_ac_e3[i] * indicators_[j][i]);
          line_processor_vectors[k]->FinishLine();
        }
      }
    }
  }

  for (auto i = 0u; i < line_processor_vectors.size(); ++i) {
    line_processor_vectors[i]->Close();
  }
}
}
