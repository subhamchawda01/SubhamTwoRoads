#ifndef _COMMON_MODULES_HPP_
#define _COMMON_MODULES_HPP_

#include "dvccode/CDef/assumptions.hpp"

namespace HFSAT {
namespace ORS {

/// Used to get server_assigned_client_id_mantissa_ ( which is a small number and hence an array index ) by
/// subtracting from server_assigned_client_id_.
/// Equivalent to just ( & 0x0000ffff )
inline int SACItoKey(int _server_assigned_client_id_) { return (_server_assigned_client_id_ & SACI_MANTISSA_HEX); }
}
}

#endif
