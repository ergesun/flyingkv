//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source
// code.

// url: https://github.com/PeterScott/murmur3/blob/master/murmur3.h

#ifndef FLYINGKV_COMMON_HASH_ALGORITHMS_H
#define FLYINGKV_COMMON_HASH_ALGORITHMS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32 (const void *key, int len, uint32_t seed, void *out);

void MurmurHash3_x86_128(const void *key, int len, uint32_t seed, void *out);

void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, void *out);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif //FLYINGKV_COMMON_HASH_ALGORITHMS_H
