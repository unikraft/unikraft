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
 * \brief Main entry point for unit tests.
 */

#include "epid/common-testhelper/testapp-testhelper.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {
  std::vector<std::string> positive;
  std::vector<std::string> negative;
  std::vector<char*> argv_new;
  argv_new.push_back(argv[0]);
  bool include_protected = false;
  bool print_help = false;
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == std::string("--also_run_protected_tests")) {
      include_protected = true;
    } else if (arg == std::string("--help")) {
      print_help = true;
      argv_new.push_back(argv[i]);
    } else if (arg.compare(0, 15, "--gtest_filter=") == 0) {
      split_filter(&positive, &negative, arg.substr(15));
    } else {
      argv_new.push_back(argv[i]);
    }
  }
  if (!include_protected) {
    negative.push_back("*.*_PROTECTED_*");
    negative.push_back("*.PROTECTED_*");
  }
  std::string filter = join_filter(positive, negative);
  if (filter != "") {
    argv_new.push_back(&filter[0]);
  }
  int argc_new = (int)argv_new.size();
  argv_new.push_back(nullptr);
  testing::InitGoogleTest(&argc_new, argv_new.data());
  if (print_help) {
    printf("\n");
    printf("Custom Options:\n");
    printf("  --also_run_protected_tests\n");
    printf("    similar to --gtest_also_run_disabled_tests, but for\n");
    printf("    protected tests (PROTECTED_ instead of DISABLED_)\n");
    printf("\n");
    printf("Protected tests are tests where some data is protected\n");
    printf("(i.e. hidden) from the code and can only be used indirectly.\n");
  }
  return RUN_ALL_TESTS();
}
