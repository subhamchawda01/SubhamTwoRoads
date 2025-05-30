#include <string>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

#include "dvccode/CDef/error_utils.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "dvctrade/ModelMath/signal_algo.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

/// to compile g++ -o exec_test25 cpptest25.cpp -I$HOME/basetrade_install -I/apps/boost/include -L/apps/boost/lib
/// -L$HOME/basetrade_install/libdebug

using namespace HFSAT;

typedef enum {
  kModelCreationPhaseInit,
  kModelCreationPhasePreModel,
  kModelCreationPhaseCreModel,
  kModelCreationPhasePostModel,
  kModelCreationPhaseIndicatorStarted,
  kModelCreationPhaseIndicatorEnded,
  kModelCreationPhaseMAX
} ModelCreationPhases_t;  ///< internal enum for Finite State Machine during creation

int main(int argc, char** argv) {
  std::string _model_filename_ = "/home/gchak/basetrade/testbed/testmodelfile";
  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
    SignalAlgo_t signal_algo_ = kSignalAlgoLinear;

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_ ..
                                                                               // in tokenizing the string contents are
                                                                               // changed
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }

        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit: {
            if (strcmp(tokens_[0], "MODELINIT") == 0) {
              // MODELINIT
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              if (strcmp(tokens_[1], "DEPBASE") == 0) {
                // MODELINIT DEPBASE ZN0 Midprice MktSizeWPrice
                if (tokens_.size() < 4) {
                  ExitVerbose(kModelCreationDepBaseLineLessArgs);
                }
                /// dep_market_view__ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView ( tokens_[2] ) ;
                std::cout << " dep_base_pricetype_ = " << (int)StringToPriceType_t((std::string)tokens_[3])
                          << std::endl;
                std::cout << " dep_pred_pricetype_ = " << (int)StringToPriceType_t((std::string)tokens_[4])
                          << std::endl;
                /// p_indicator_logger_ = new IndicatorLogger ( _dbglogger_, _watch_, _model_filename_,
                /// *dep_market_view__, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_ ) ;
              }
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
          } break;
          case kModelCreationPhasePreModel: {
            // MODELMATH LINEAR CHANGE 30 FUT3
            // MODELMATH LINEAR CHANGE 5 FUT1
            // MODELMATH NODEPLINEAR CHANGE 30 FUT3
            // MODELMATH CLASSIFIER CHANGE 30 FUT3
            // MODELMATH CART CHANGE 30 FUT3
            // MODELMATH NEURALNETWORK CHANGE 30 FUT3

            if (strcmp(tokens_[0], "MODELMATH") == 0) {
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelMathLineLessArgs);
              }
              signal_algo_ = GetSignalAlgo(tokens_[1]);
              current_model_creation_phase_ = kModelCreationPhasePostModel;
            }
          } break;
          case kModelCreationPhasePostModel: {
            if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
            } else {
              if (strcmp(tokens_[0], "MODELARGS") == 0) {
                // common modelmath arguments like :
                // MODELARGS number_of_layers_in_neural_network
              }
            }
          } break;
          case kModelCreationPhaseIndicatorStarted: {
            if (strcmp(tokens_[0], "INDICATOREND") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
            } else {
              if (strcmp(tokens_[0], "INDICATOR") == 0) {
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationIndicatorLineLessArgs);
                }
                // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
                std::cout << tokens_[0] << " " << tokens_[1] << " " << tokens_[2] << " " << tokens_[3] << " "
                          << tokens_[4] << std::endl;
              }
            }
          } break;
          case kModelCreationPhaseIndicatorEnded: {
            // called for modelmath objects that might need a signal that all inputs have been registered and linkages
            // can be finalized now.
          } break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }
}
