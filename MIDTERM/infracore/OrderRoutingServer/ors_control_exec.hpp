#ifndef BASE_CMEORDERROUTINGSERVER_ORS_CONTROL_EXEC_HPP
#define BASE_CMEORDERROUTINGSERVER_ORS_CONTROL_EXEC_HPP

#include <vector>
#include <string>

namespace HFSAT {
namespace ORS {

std::string JoinText(const std::string& separator_text_, const std::vector<std::string>& r_control_command_text_) {
  std::string retval = "";
  if (r_control_command_text_.size() > 0) {
    retval = r_control_command_text_[0];
  }
  for (unsigned int i = 1; i < r_control_command_text_.size(); i++) {
    retval += separator_text_ + r_control_command_text_[i];
  }
  return retval;
}
}
}

#endif  // BASE_CMEORDERROUTINGSERVER_ORS_CONTROL_EXEC_HPP
