#!/bin/sh
#===============================================================================
# Copyright 2016-2018 Intel Corporation
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
#===============================================================================

dnnl_root="$1"
output="$2"
shift 2

echo -e '#include "dnnl.h"' > "$output"
echo -e "const void *c_functions[] = {" >> "$output"
cpp -w "${@/#/-I}" "${dnnl_root}/include/dnnl.h" \
    | grep -o 'dnnl_\w\+(' \
    | sed 's/\(.*\)(/(void*)\1,/g' \
    | sort -u >> "$output"
echo -e "NULL};\nint main() { return 0; }" >> "$output"
