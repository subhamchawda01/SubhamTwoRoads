#ifndef _SURVILANCE_HANDLING_HPP_
#define _SURVILANCE_HANDLING_HPP_

#include <iostream>
#include <map>
#include <sys/time.h>
#include <unordered_map>
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"

namespace HFSAT {

class SurveillanceHandling {
 public:
  static SurveillanceHandling& GetUniqueInstance();
  static void CheckProductUnderSurvilance(std::string shortcode_);
  static void GetSurveillanceLogging(std::string shortcode_);
  static std::unordered_map<int, std::string> surveillance_mapping;

 private:
  SurveillanceHandling();
  static SurveillanceHandling* unique_instance_;
};
}

#endif
