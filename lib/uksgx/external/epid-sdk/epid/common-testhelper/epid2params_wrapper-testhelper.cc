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
/// Epid2Params C++ wrapper implementation.
/*! \file */

#include "epid/common-testhelper/epid2params_wrapper-testhelper.h"

#include <cstdio>
#include <stdexcept>
#include <string>

extern "C" {
#include "epid/common/src/epid2params.h"
}

Epid2ParamsObj::Epid2ParamsObj() : params_(nullptr) {
  EpidStatus sts = kEpidNoErr;
  sts = CreateEpid2Params(&params_);
  if (kEpidNoErr != sts) {
    printf("%s(%d): %s\n", __FILE__, __LINE__, "test defect:");
    throw std::logic_error(std::string("Failed to call: ") +
                           "CreateEpid2Params()");
  }
}

Epid2ParamsObj::~Epid2ParamsObj() { DeleteEpid2Params(&params_); }

Epid2Params_* Epid2ParamsObj::ctx() const { return params_; }

Epid2ParamsObj::operator Epid2Params_*() const { return params_; }

Epid2ParamsObj::operator const Epid2Params_*() const { return params_; }

FiniteField* Epid2ParamsObj::Fp() const { return params_->Fp; }

EcGroup* Epid2ParamsObj::G1() const { return params_->G1; }
