
#pragma once
#include <cstdint>

namespace HFSAT {
namespace MD5 {
#define ROUND0(a, b, c, d, k, s, t) ROUND_TAIL(a, b, d ^ (b & (c ^ d)), k, s, t)
#define ROUND1(a, b, c, d, k, s, t) ROUND_TAIL(a, b, c ^ (d & (b ^ c)), k, s, t)
#define ROUND2(a, b, c, d, k, s, t) ROUND_TAIL(a, b, b ^ c ^ d, k, s, t)
#define ROUND3(a, b, c, d, k, s, t) ROUND_TAIL(a, b, c ^ (b | ~d), k, s, t)

#define ROUND_TAIL(a, b, expr, k, s, t) \
  a += (expr) + UINT32_C(t) + block[k]; \
  a = b + (a << s | a >> (32 - s));

void md5_compress(uint32_t state[4], uint32_t block[16]);
void MD5(const uint8_t *message, size_t len, uint32_t *hash);

}  // namespace MD5
}  // namespace HFSAT
