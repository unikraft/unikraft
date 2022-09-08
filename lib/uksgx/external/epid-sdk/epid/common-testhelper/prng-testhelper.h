/*############################################################################
  # Copyright 2016-2017 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/

/*!
 * \file
 * \brief Pseudo random number generator interface.
 */
#ifndef EPID_COMMON_TESTHELPER_PRNG_TESTHELPER_H_
#define EPID_COMMON_TESTHELPER_PRNG_TESTHELPER_H_

#if defined(_WIN32) || defined(_WIN64)
#define __STDCALL __stdcall
#else
#define __STDCALL
#endif
#include <limits.h>  // for CHAR_BIT
#include <stdint.h>
#include <random>
#include <vector>

extern "C" {
#include "epid/common/types.h"
}

/// Return status for Prng Generate function
typedef enum {
  kPrngNoErr = 0,   //!< no error
  kPrngErr = -999,  //!< unspecified error
  kPrngNotImpl,     //!< not implemented error
  kPrngBadArgErr    //!< incorrect arg to function
} PrngStatus;

/// Pseudo random number generator (prng) class.
class Prng {
 public:
  Prng() : seed_(1) { set_seed(seed_); }
  ~Prng() {}
  /// Retrieve seed
  unsigned int get_seed() const { return seed_; }
  /// Set seed for random number generator
  void set_seed(unsigned int val) {
    seed_ = val;
    generator_.seed(seed_);
  }
  /// Generates random number
  static int __STDCALL Generate(unsigned int* random_data, int num_bits,
                                void* user_data) {
    unsigned int num_bytes = num_bits / CHAR_BIT;

    unsigned int extra_bits = num_bits % CHAR_BIT;
    unsigned char* random_bytes = reinterpret_cast<unsigned char*>(random_data);
    if (!random_data) {
      return kPrngBadArgErr;
    }
    if (num_bits <= 0) {
      return kPrngBadArgErr;
    }
    if (0 != extra_bits) {
      num_bytes += 1;
    }

    Prng* myprng = (Prng*)user_data;
    for (unsigned int n = 0; n < num_bytes; n++) {
      random_bytes[n] =
          static_cast<unsigned char>(myprng->generator_() & 0x000000ff);
    }

    return kPrngNoErr;
  }

 private:
  unsigned int seed_;
  std::mt19937 generator_;
};

// BitSupplier implementation returns pre-defined bytes.
class StaticPrng {
 public:
  StaticPrng(ConstOctStr bytes, size_t length)
      : bytes_((uint8_t const*)bytes, (uint8_t const*)bytes + length) {}
  ~StaticPrng() {}
  /// Generates random number
  static int __STDCALL Generate(unsigned int* random_data, int num_bits,
                                void* user_data) {
    unsigned int num_bytes = num_bits / CHAR_BIT;
    if (!random_data) {
      return kPrngBadArgErr;
    }
    if (num_bits <= 0) {
      return kPrngBadArgErr;
    }
    StaticPrng* myprng = (StaticPrng*)user_data;
    for (size_t i = 0; i < num_bytes; i++) {
      random_data[i] = myprng->bytes_[i % myprng->bytes_.size()];
    }
    return kPrngNoErr;
  }

 private:
  std::vector<uint8_t> bytes_;
};

#endif  // EPID_COMMON_TESTHELPER_PRNG_TESTHELPER_H_
