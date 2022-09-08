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
/*!
* \file
* \brief Main entry point helpers unit tests.
*/
#include "epid/common-testhelper/testapp-testhelper.h"
#include <sstream>
#include <string>
#include <vector>

void split_filter(std::vector<std::string>* positive,
                  std::vector<std::string>* negative, std::string filter_expr) {
  std::istringstream f(filter_expr);
  std::string s;
  bool is_neg = false;
  while (getline(f, s, ':')) {
    if (!is_neg) {
      if (s.compare(0, 1, "-") == 0) {
        is_neg = true;
        s = s.substr(1);
      } else {
        positive->push_back(s);
      }
    }
    if (is_neg) {
      negative->push_back(s);
    }
  }
}

std::string join_filter(std::vector<std::string> const& positive,
                        std::vector<std::string> const& negative) {
  std::ostringstream s;
  bool first = true;
  bool first_neg = true;
  if (!positive.empty() || !negative.empty()) {
    s << "--gtest_filter=";
  }
  for (const auto& i : positive) {
    if (!first) {
      s << ":";
    } else {
      first = false;
    }
    s << i;
  }
  for (const auto& i : negative) {
    if (!first) {
      s << ":";
    } else {
      first = false;
    }
    if (first_neg) {
      s << "-";
      first_neg = false;
    }
    s << i;
  }
  return s.str();
}
