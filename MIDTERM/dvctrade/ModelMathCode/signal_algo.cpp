/**
    \file ModelMathCode/signal_algo.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvctrade/ModelMath/signal_algo.hpp"

namespace HFSAT {

SignalAlgo_t GetSignalAlgo(const std::string& _in_str_) {
  if (_in_str_.compare("LINEAR") == 0) {
    return kSignalAlgoLinear;
  } else if (_in_str_.compare("NONLINEAR") == 0) {
    return kSignalAlgoNonLinear;
  } else if (_in_str_.compare("NODEPLINEAR") == 0) {
    return kSignalAlgoNoDepLinear;
  } else if (_in_str_.compare("CLASSIFIER") == 0) {
    return kSignalAlgoClassifier;
  } else if (_in_str_.compare("CART") == 0) {
    return kSignalAlgoCART;
  } else if (_in_str_.compare("NEURALNETWORK") == 0) {
    return kSignalAlgoNeuralNetwork;
  } else if (_in_str_.compare("LOGISTIC") == 0) {
    return kSignalAlgoLogistic;
  } else if (_in_str_.compare("SIGLR") == 0) {
    return kSignalAlgoSIGLR;
  } else if (_in_str_.compare("RANDOMFOREST") == 0) {
    return kSignalAlgoRandomForest;
  } else if (_in_str_.compare("SELECTIVE") == 0) {
    return kSignalAlgoSelective;
  } else if (_in_str_.compare("SELECTIVENEW") == 0) {
    return kSignalAlgoSelectiveNew;
  } else if (_in_str_.compare("SELECTIVESIGLR") == 0) {
    return kSignalAlgoSelectiveSiglr;
  } else if (_in_str_.compare("BOOSTING") == 0) {
    return kSignalAlgoBoosting;
  } else if (_in_str_.compare("TREEBOOSTING") == 0) {
    return kSignalAlgoTreeBoosting;
  } else if (_in_str_.compare("ONLINELINEAR") == 0) {
    return kSignalAlgoOnlineLinear;
  } else if (_in_str_.compare("ONLINESIGLR") == 0) {
    return kSignalAlgoOnlineSIGLR;
  } else if (_in_str_.compare("ONLINESELECTIVENEW") == 0) {
    return kSignalAlgoOnlineSelectiveNew;
  } else if (_in_str_.compare("ONLINESELECTIVESIGLR") == 0) {
    return kSignalAlgoOnlineSelectiveSiglr;
  } else if (_in_str_.compare("SELECTIVECONTINUOUS") == 0) {
    return kSignalAlgoSelectiveContinuous;
  } else if (_in_str_.compare("MDLINEAR") == 0) {
    return kSignalAlgoMDLinear;
  } else if (_in_str_.compare("GRADBOOSTINGCLASSIFIER") == 0) {
    return kSignalAlgoGradBoostingClassifier;
  } else {
    return kSignalAlgoMAX;
  }
  return kSignalAlgoMAX;
}
}
