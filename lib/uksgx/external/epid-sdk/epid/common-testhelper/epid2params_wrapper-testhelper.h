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
/// Epid2Params C++ wrapper interface.
/*! \file */

#ifndef EPID_COMMON_TESTHELPER_EPID2PARAMS_WRAPPER_TESTHELPER_H_
#define EPID_COMMON_TESTHELPER_EPID2PARAMS_WRAPPER_TESTHELPER_H_

typedef struct Epid2Params_ Epid2Params_;
typedef struct FiniteField FiniteField;
typedef struct EcGroup EcGroup;

/// C++ Wrapper to manage memory for Epid2Params via RAII
class Epid2ParamsObj {
 public:
  /// Create a Epid2Params
  Epid2ParamsObj();

  // This class instances are not meant to be copied.
  // Explicitly delete copy constructor and assignment operator.
  Epid2ParamsObj(const Epid2ParamsObj&) = delete;
  Epid2ParamsObj& operator=(const Epid2ParamsObj&) = delete;

  /// Destroy the Epid2Params
  ~Epid2ParamsObj();
  /// get a pointer to the stored Epid2Params
  Epid2Params_* ctx() const;
  /// cast operator to get the pointer to the stored Epid2Params
  operator Epid2Params_*() const;
  /// const cast operator to get the pointer to the stored Epid2Params
  operator const Epid2Params_*() const;
  /// get a pointer to the prime field Fp
  FiniteField* Fp() const;
  /// get a pointer to elliptic curve group G1
  EcGroup* G1() const;

 private:
  /// The stored parameters
  Epid2Params_* params_;
};

#endif  // EPID_COMMON_TESTHELPER_EPID2PARAMS_WRAPPER_TESTHELPER_H_
