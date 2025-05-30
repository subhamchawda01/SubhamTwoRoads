/**
        \file ModelMath/signal_algo.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_SIGNAL_ALGO_H
#define BASE_MODELMATH_SIGNAL_ALGO_H

#include <iostream>
#include <string>

namespace HFSAT {
typedef enum {
  kSignalAlgoLinear,
  kSignalAlgoNoDepLinear,
  kSignalAlgoClassifier,
  kSignalAlgoCART,
  kSignalAlgoNeuralNetwork,
  kSignalAlgoLogistic,
  kSignalAlgoNonLinear,
  kSignalAlgoSIGLR,
  kSignalAlgoRandomForest,
  kSignalAlgoSelective,
  kSignalAlgoSelectiveNew,
  kSignalAlgoSelectiveSiglr,
  kSignalAlgoBoosting,
  kSignalAlgoTreeBoosting,
  kSignalAlgoOnlineLinear,
  kSignalAlgoOnlineSIGLR,
  kSignalAlgoOnlineSelectiveNew,
  kSignalAlgoOnlineSelectiveSiglr,
  kSignalAlgoSelectiveContinuous,
  kSignalAlgoMDLinear,
  kSignalAlgoMAX,
  kSignalAlgoGradBoostingClassifier
} SignalAlgo_t;

SignalAlgo_t GetSignalAlgo(const std::string& _in_str_);
}
#endif  // BASE_MODELMATH_SIGNAL_ALGO_H
