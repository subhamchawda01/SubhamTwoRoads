#include "baseinfra/FPGA/BMF_FPGA/mbochip/SumdfEventApi.hpp"
#include <fstream>
#include <sstream>
#include "dvccode/CDef/defines.hpp"

namespace BMFFPGAUtil {

bool ReadChannelConfig(std::ifstream& stream_config, SiliconUmdf::EventAPI::sumdfConfiguration_t& sumdf_config);
}
