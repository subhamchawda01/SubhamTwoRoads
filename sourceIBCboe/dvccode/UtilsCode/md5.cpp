
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dvccode/Utils/md5.hpp"

bool AVX512_SUPPORTED = __builtin_cpu_supports("avx512vl");

namespace HFSAT {
namespace MD5 {

// Wrapper function for Boost MD5 calculation
//const char* boostMD5(unsigned char* data, size_t length) {
void boostMD5(unsigned char* data, size_t length) {
    boost::uuids::detail::md5 boostHash;
    boost::uuids::detail::md5::digest_type boostDigest;

    boostHash.process_bytes(data + 16, length); // Skip the first 16 bytes (for the checksum)
    boostHash.get_digest(boostDigest);

    // Convert endianness of boostDigest to match the expected format
    for (int i = 0; i < 4; ++i) {
        uint32_t value = boostDigest[i];
        uint32_t swappedValue = (value << 24) | ((value << 8) & 0x00FF0000) |
                                ((value >> 8) & 0x0000FF00) | (value >> 24);
        std::memcpy(data + i * sizeof(uint32_t), &swappedValue, sizeof(uint32_t));
    }
/*
    // Reinterpret the MD5 digest as an array of uint32_t
    const auto charDigest = reinterpret_cast<const char*>(&boostDigest);

    // Convert the digest to a hex string
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(result));
    std::cout << "boostMD5: " << result << std::endl;

    // Copy the hex string to a dynamically allocated buffer
    char* md5sum = new char[result.size() + 1];
    std::copy(result.begin(), result.end(), md5sum);
    md5sum[result.size()] = '\0'; // Null-terminate the string

    return md5sum;
*/

}

void md5_compress(uint32_t state[4], uint32_t block[16]) {
  uint32_t a = state[0];
  uint32_t b = state[1];
  uint32_t c = state[2];
  uint32_t d = state[3];

  ROUND0(a, b, c, d, 0, 7, 0xD76AA478)
  ROUND0(d, a, b, c, 1, 12, 0xE8C7B756)
  ROUND0(c, d, a, b, 2, 17, 0x242070DB)
  ROUND0(b, c, d, a, 3, 22, 0xC1BDCEEE)
  ROUND0(a, b, c, d, 4, 7, 0xF57C0FAF)
  ROUND0(d, a, b, c, 5, 12, 0x4787C62A)
  ROUND0(c, d, a, b, 6, 17, 0xA8304613)
  ROUND0(b, c, d, a, 7, 22, 0xFD469501)
  ROUND0(a, b, c, d, 8, 7, 0x698098D8)
  ROUND0(d, a, b, c, 9, 12, 0x8B44F7AF)
  ROUND0(c, d, a, b, 10, 17, 0xFFFF5BB1)
  ROUND0(b, c, d, a, 11, 22, 0x895CD7BE)
  ROUND0(a, b, c, d, 12, 7, 0x6B901122)
  ROUND0(d, a, b, c, 13, 12, 0xFD987193)
  ROUND0(c, d, a, b, 14, 17, 0xA679438E)
  ROUND0(b, c, d, a, 15, 22, 0x49B40821)
  ROUND1(a, b, c, d, 1, 5, 0xF61E2562)
  ROUND1(d, a, b, c, 6, 9, 0xC040B340)
  ROUND1(c, d, a, b, 11, 14, 0x265E5A51)
  ROUND1(b, c, d, a, 0, 20, 0xE9B6C7AA)
  ROUND1(a, b, c, d, 5, 5, 0xD62F105D)
  ROUND1(d, a, b, c, 10, 9, 0x02441453)
  ROUND1(c, d, a, b, 15, 14, 0xD8A1E681)
  ROUND1(b, c, d, a, 4, 20, 0xE7D3FBC8)
  ROUND1(a, b, c, d, 9, 5, 0x21E1CDE6)
  ROUND1(d, a, b, c, 14, 9, 0xC33707D6)
  ROUND1(c, d, a, b, 3, 14, 0xF4D50D87)
  ROUND1(b, c, d, a, 8, 20, 0x455A14ED)
  ROUND1(a, b, c, d, 13, 5, 0xA9E3E905)
  ROUND1(d, a, b, c, 2, 9, 0xFCEFA3F8)
  ROUND1(c, d, a, b, 7, 14, 0x676F02D9)
  ROUND1(b, c, d, a, 12, 20, 0x8D2A4C8A)
  ROUND2(a, b, c, d, 5, 4, 0xFFFA3942)
  ROUND2(d, a, b, c, 8, 11, 0x8771F681)
  ROUND2(c, d, a, b, 11, 16, 0x6D9D6122)
  ROUND2(b, c, d, a, 14, 23, 0xFDE5380C)
  ROUND2(a, b, c, d, 1, 4, 0xA4BEEA44)
  ROUND2(d, a, b, c, 4, 11, 0x4BDECFA9)
  ROUND2(c, d, a, b, 7, 16, 0xF6BB4B60)
  ROUND2(b, c, d, a, 10, 23, 0xBEBFBC70)
  ROUND2(a, b, c, d, 13, 4, 0x289B7EC6)
  ROUND2(d, a, b, c, 0, 11, 0xEAA127FA)
  ROUND2(c, d, a, b, 3, 16, 0xD4EF3085)
  ROUND2(b, c, d, a, 6, 23, 0x04881D05)
  ROUND2(a, b, c, d, 9, 4, 0xD9D4D039)
  ROUND2(d, a, b, c, 12, 11, 0xE6DB99E5)
  ROUND2(c, d, a, b, 15, 16, 0x1FA27CF8)
  ROUND2(b, c, d, a, 2, 23, 0xC4AC5665)
  ROUND3(a, b, c, d, 0, 6, 0xF4292244)
  ROUND3(d, a, b, c, 7, 10, 0x432AFF97)
  ROUND3(c, d, a, b, 14, 15, 0xAB9423A7)
  ROUND3(b, c, d, a, 5, 21, 0xFC93A039)
  ROUND3(a, b, c, d, 12, 6, 0x655B59C3)
  ROUND3(d, a, b, c, 3, 10, 0x8F0CCC92)
  ROUND3(c, d, a, b, 10, 15, 0xFFEFF47D)
  ROUND3(b, c, d, a, 1, 21, 0x85845DD1)
  ROUND3(a, b, c, d, 8, 6, 0x6FA87E4F)
  ROUND3(d, a, b, c, 15, 10, 0xFE2CE6E0)
  ROUND3(c, d, a, b, 6, 15, 0xA3014314)
  ROUND3(b, c, d, a, 13, 21, 0x4E0811A1)
  ROUND3(a, b, c, d, 4, 6, 0xF7537E82)
  ROUND3(d, a, b, c, 11, 10, 0xBD3AF235)
  ROUND3(c, d, a, b, 2, 15, 0x2AD7D2BB)
  ROUND3(b, c, d, a, 9, 21, 0xEB86D391)

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

/* MD5 checksum calculation algo. */
void MD5(const uint8_t *message, size_t len, uint32_t *hash) {
  hash[0] = UINT32_C(0x67452301);
  hash[1] = UINT32_C(0xEFCDAB89);
  hash[2] = UINT32_C(0x98BADCFE);
  hash[3] = UINT32_C(0x10325476);

  uint32_t i;
  for (i = 0; len - i >= 64; i += 64) {
    md5_compress(hash, (uint32_t *)(message + i));
  }

  uint32_t block[16];
  uint8_t *byteBlock = (uint8_t *)block;

  uint32_t rem = len - i;
  memcpy(byteBlock, message + i, rem);

  byteBlock[rem] = 0x80;
  rem++;
  if (64 - rem >= 8) {
    memset(byteBlock + rem, 0, 56 - rem);
  } else {
    memset(byteBlock + rem, 0, 64 - rem);
    md5_compress(hash, block);
    memset(block, 0, 56);
  }
  block[14] = len << 3;
  block[15] = len >> 29;
  md5_compress(hash, block);
}

}  // namespace MD5
}  // namespace HFSAT
