/**
    \file Indicators/indicator_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

namespace HFSAT {

/// Common interface extended by all classes listening to Indicators
/// either as subcomputations like OfflineCorradjustedPairs
/// or in aggregation as OfflineCorradjustedPairsCombo
/// or BaseModelMath
class IndicatorListener {
 public:
  enum LISTENER_TYPE { kIndicator, kAggregator };
  virtual ~IndicatorListener(){};
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) = 0;
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                                 const double& _new_value_nochange_, const double& _new_value_increase_) = 0;
  int listener_type = kIndicator;
};

/// Simple class that encapsulates the indicator listener without a weight
/// Only used in CommonIndicator
class UnweightedIndicatorListenerPair {
 public:
  unsigned int indicator_index_;
  IndicatorListener* indicator_listener__;

 public:
  explicit UnweightedIndicatorListenerPair(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__)
      : indicator_index_(_indicator_index_), indicator_listener__(_indicator_listener__) {}

  /// Assignment operator needed by std::vector < Indicator_Listener > in CommonIndicator
  const UnweightedIndicatorListenerPair& operator=(const UnweightedIndicatorListenerPair& p) {
    indicator_index_ = p.indicator_index_;
    indicator_listener__ = p.indicator_listener__;
    return *this;  // This will allow assignments to be chained
  }

  virtual ~UnweightedIndicatorListenerPair() {}

  inline void OnIndicatorUpdate(double _indicator_value_) {
    indicator_listener__->OnIndicatorUpdate(indicator_index_, _indicator_value_);
  }
};

/// Simple class that encapsulates the indicator listener and
/// a multiplicative weight to be multiplied to the original indicator value
/// before sending to the listener
/// Only used in CommonIndicator
class IndicatorListenerPair {
 public:
  unsigned int indicator_index_;
  IndicatorListener* indicator_listener__;
  double node_value_;

 public:
  explicit IndicatorListenerPair(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                 double _node_value_)
      : indicator_index_(_indicator_index_), indicator_listener__(_indicator_listener__), node_value_(_node_value_) {}

  /// Assignment operator needed by std::vector < Indicator_Listener > in CommonIndicator
  const IndicatorListenerPair& operator=(const IndicatorListenerPair& p) {
    indicator_index_ = p.indicator_index_;
    indicator_listener__ = p.indicator_listener__;
    node_value_ = p.node_value_;
    return *this;  // This will allow assignments to be chained
  }

  virtual ~IndicatorListenerPair() {}

  inline void OnIndicatorUpdate(double _indicator_value_) {
    indicator_listener__->OnIndicatorUpdate(indicator_index_, _indicator_value_ * node_value_);
  }
};

class IndicatorListenerPairLogit {
 public:
  unsigned int indicator_index_;
  IndicatorListener* indicator_listener__;
  double node_value_decrease_;
  double node_value_nochange_;
  double node_value_increase_;
  double indicator_value_decrease_;
  double indicator_value_nochange_;
  double indicator_value_increase_;

 public:
  explicit IndicatorListenerPairLogit(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                      double _node_value_decrease_, double _node_value_nochange_,
                                      double _node_value_increase_)
      : indicator_index_(_indicator_index_),
        indicator_listener__(_indicator_listener__),
        node_value_decrease_(_node_value_decrease_),
        node_value_nochange_(_node_value_nochange_),
        node_value_increase_(_node_value_increase_),
        indicator_value_decrease_(0),
        indicator_value_nochange_(0),
        indicator_value_increase_(0) {}

  /// Assignment operator needed by std::vector < Indicator_Listener > in CommonIndicator
  const IndicatorListenerPairLogit& operator=(const IndicatorListenerPairLogit& p) {
    indicator_index_ = p.indicator_index_;
    indicator_listener__ = p.indicator_listener__;
    node_value_decrease_ = p.node_value_decrease_;
    node_value_nochange_ = p.node_value_nochange_;
    node_value_increase_ = p.node_value_increase_;
    return *this;  // This will allow assignments to be chained
  }

  virtual ~IndicatorListenerPairLogit() {}

  inline void OnIndicatorUpdate(double _indicator_value_) {
    indicator_value_decrease_ = _indicator_value_ * node_value_decrease_;
    indicator_value_nochange_ = _indicator_value_ * node_value_nochange_;
    indicator_value_increase_ = _indicator_value_ * node_value_increase_;
    indicator_listener__->OnIndicatorUpdate(indicator_index_, indicator_value_decrease_, indicator_value_nochange_,
                                            indicator_value_increase_);
  }
};

class IndicatorListenerPairSigmoid {
 public:
  unsigned int indicator_index_;
  IndicatorListener* indicator_listener__;
  double node_value_alpha_;
  double node_value_beta_;
  double indicator_value_;

 public:
  explicit IndicatorListenerPairSigmoid(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                        double _node_value_alpha_, double _node_value_beta_)
      : indicator_index_(_indicator_index_),
        indicator_listener__(_indicator_listener__),
        node_value_alpha_(_node_value_alpha_),
        node_value_beta_(_node_value_beta_),
        indicator_value_(0) {}

  /// Assignment operator needed by std::vector < Indicator_Listener > in CommonIndicator
  const IndicatorListenerPairSigmoid& operator=(const IndicatorListenerPairSigmoid& p) {
    indicator_index_ = p.indicator_index_;
    indicator_listener__ = p.indicator_listener__;
    node_value_alpha_ = p.node_value_alpha_;
    node_value_beta_ = p.node_value_beta_;
    return *this;  // This will allow assignments to be chained
  }

  virtual ~IndicatorListenerPairSigmoid() {}

  inline void OnIndicatorUpdate(double _indicator_value_) {
    indicator_value_ = node_value_beta_ * (1 / (1 + exp(-_indicator_value_ * node_value_alpha_)) - 0.5);
    indicator_listener__->OnIndicatorUpdate(indicator_index_, indicator_value_);
  }
};
}
