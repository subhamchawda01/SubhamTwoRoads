#ifndef MULTISESSOPM_ENGINE_HPP
#define MULTISESSOPM_ENGINE_HPP

#include "dvccode/CDef/fwd_decl.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {
namespace ORS {
BaseEngine *makeMultiSessionEngine(const std::string &exchange, Settings &settings, HFSAT::DebugLogger &logger,
                                   std::string output_log_dir);
}
}

#endif  // !MULTISESSOPM_ENGINE_HPP
