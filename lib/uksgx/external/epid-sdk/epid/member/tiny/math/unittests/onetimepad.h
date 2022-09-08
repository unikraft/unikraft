/*############################################################################
# Copyright 2017 Intel Corporation
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
/// One time pad class
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_UNITTESTS_ONETIMEPAD_H_
#define EPID_MEMBER_TINY_MATH_UNITTESTS_ONETIMEPAD_H_

#include <climits>  // for CHAR_BIT
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define __STDCALL __stdcall
#else
#define __STDCALL
#endif

/// One time pad with Bitsupplier interface
class OneTimePad {
 public:
  /// Default constructor
  OneTimePad() : bytes_consumed_(0), reported_having_no_data_(false) {}
  /// Construct using mersenne twister
  explicit OneTimePad(size_t num_bytes)
      : bytes_consumed_(0), reported_having_no_data_(false) {
    data_.resize(num_bytes);
    std::mt19937 generator;
    generator.seed(1);
    for (size_t i = 0; i < num_bytes; i++)
      data_[i] = static_cast<uint8_t>(generator() & 0x000000ff);
  }
  /// Construct with data
  explicit OneTimePad(std::vector<uint8_t> const& uint8_data)
      : reported_having_no_data_(false) {
    InitUint8(uint8_data);
  }
  /// Re-initialize with unit8 data
  void InitUint8(std::vector<uint8_t> const& uint8_data) {
    if (uint8_data.size() > SIZE_MAX / CHAR_BIT)
      throw std::invalid_argument("input exceeded SIZE_MAX bits");
    bytes_consumed_ = 0;
    data_.clear();
    data_ = uint8_data;
    reported_having_no_data_ = false;
  }
  /// Re-initialize with unit32 data
  void InitUint32(std::vector<uint32_t> const& uint32_data) {
    if (uint32_data.size() * sizeof(uint32_t) > SIZE_MAX / CHAR_BIT)
      throw std::invalid_argument("input exceeded SIZE_MAX bits");
    bytes_consumed_ = 0;
    data_.clear();
    for (auto u32 : uint32_data) {
      data_.push_back((uint8_t)(u32 & 0xFF));
      data_.push_back((uint8_t)((u32 & 0xFF00) >> 8));
      data_.push_back((uint8_t)((u32 & 0xFF0000) >> 16));
      data_.push_back((uint8_t)((u32 & 0xFF000000) >> 24));
      reported_having_no_data_ = false;
    }
  }
  /// Destructor
  ~OneTimePad() {}
  /// returns bits consumed
  size_t BitsConsumed() const { return bytes_consumed_ * CHAR_BIT; }
  /// Generates random number
  static int __STDCALL Generate(unsigned int* random_data, int num_bits,
                                void* user_data) {
    size_t num_bytes = num_bits / CHAR_BIT;
    size_t extra_bits = num_bits % CHAR_BIT;
    uint8_t* random_bytes = reinterpret_cast<uint8_t*>(random_data);
    OneTimePad* myprng = (OneTimePad*)user_data;
    if ((!random_data) || (num_bits <= 0)) {
      return -5;  // bad arg
    }
    if (myprng->reported_having_no_data_) {
      throw std::runtime_error(
          "request for random data after being informed random data was "
          "exhausted");
    }
    if ((size_t)num_bits > myprng->BitsAvailable()) {
      // cause an exception to be thrown on next invocation
      myprng->reported_having_no_data_ = true;
      return -1;  // out of random data
    }
    if (0 != extra_bits) {
      // can only handle even number of byte requests
      return -5;
    }

    for (unsigned int n = 0; n < num_bytes; n++) {
      random_bytes[n] = myprng->data_[myprng->bytes_consumed_++];
    }
    return 0;
  }

 private:
  /// returns bits available
  size_t BitsAvailable() const {
    return (data_.size() - bytes_consumed_) * CHAR_BIT;
  }

  size_t bytes_consumed_ = 0;
  std::vector<uint8_t> data_;
  bool reported_having_no_data_ = false;
};

#endif  // EPID_MEMBER_TINY_MATH_UNITTESTS_ONETIMEPAD_H_
