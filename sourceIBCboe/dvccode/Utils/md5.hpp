#pragma once
#include <cstdint>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <cstdint>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <memory>
#include <fstream>
#include <algorithm>
#include <immintrin.h>
#include <boost/format.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include "dvccode/Utils/md5-x86-asm.hpp"

using boost::uuids::detail::md5;

// Declaration and initialization of AVX512_SUPPORTED
extern bool AVX512_SUPPORTED ;

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
// Wrapper function for Boost MD5 calculation
void boostMD5(unsigned char* data, size_t length);

// MD5 AVX512VL 
/*
struct HASH_uint32_t {
  uint32_t A, B, C, D;
};
*/

inline void md5_init(MD5_STATE* state) {
  state->A = _mm_cvtsi32_si128(0x67452301);
  state->B = _mm_cvtsi32_si128(0xefcdab89);
  state->C = _mm_cvtsi32_si128(0x98badcfe);
  state->D = _mm_cvtsi32_si128(0x10325476);
}

inline void MD5_AVX512VL(const void* __restrict__ src, size_t len, uint32_t *hash) {
  MD5_STATE state;
  md5_init(&state);
  char* __restrict__ _src = (char* __restrict__)src;
  uint64_t totalLen = len << 3; // length in bits

  for(; len >= 64; len -= 64) {
          md5_block_avx512(&state, _src);
          _src += 64;
  }
  len &= 63;


  // finalize
  char block[64];
  memcpy(block, _src, len);
  block[len++] = 0x80;

  // write this in a loop to avoid duplicating the force-inlined process_block function twice
  for(int iter = (len <= 64-8); iter < 2; iter++) {
    if(iter == 0) {
      memset(block + len, 0, 64-len);
      len = 0;
    } else {
       memset(block + len, 0, 64-8 - len);
       memcpy(block + 64-8, &totalLen, 8);
    }

    md5_block_avx512(&state, block);
  }
  hash[0] = _mm_cvtsi128_si32(state.A);
  hash[1] = _mm_cvtsi128_si32(state.B);
  hash[2] = _mm_cvtsi128_si32(state.C);
  hash[3] = _mm_cvtsi128_si32(state.D);
/*
  HASH_uint32_t hash_opt;
  hash_opt.A = _mm_cvtsi128_si32(state.A);
  hash_opt.B = _mm_cvtsi128_si32(state.B);
  hash_opt.C = _mm_cvtsi128_si32(state.C);
  hash_opt.D = _mm_cvtsi128_si32(state.D);
  std::memcpy(hash, &hash_opt, 16);
*/
}

}  // namespace MD5
}  // namespace HFSAT
