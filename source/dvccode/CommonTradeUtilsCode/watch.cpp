#include <sstream>

#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFSAT {

const std::string Watch::UTCTimeString() const {
  std::stringstream ss;
  boost::posix_time::ptime pt_ = boost::posix_time::from_time_t(tv_.tv_sec);
  ss << pt_;
  return (ss.str());
}

const std::string Watch::NYTimeString() const {
  std::stringstream ss;
  ss << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(tv_.tv_sec);
  return (ss.str());
}

const std::string Watch::IndTimeString() const {
  std::stringstream ss;
  ss << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(tv_.tv_sec);
  return (ss.str());
}

std::string Watch::tv_ToString() const {
  std::ostringstream temp_oss;
  temp_oss << tv_.tv_sec << "." << std::setw(6) << std::setfill('0') << tv_.tv_usec;
  return temp_oss.str();
}
}
