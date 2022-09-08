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

#ifndef EPID_COMMON_TESTHELPER_TESTAPP_TESTHELPER_H_
#define EPID_COMMON_TESTHELPER_TESTAPP_TESTHELPER_H_

#include <string>
#include <vector>

void split_filter(std::vector<std::string>* positive,
                  std::vector<std::string>* negative, std::string filter_expr);

std::string join_filter(std::vector<std::string> const& positive,
                        std::vector<std::string> const& negative);

#endif  // EPID_COMMON_TESTHELPER_TESTAPP_TESTHELPER_H_
