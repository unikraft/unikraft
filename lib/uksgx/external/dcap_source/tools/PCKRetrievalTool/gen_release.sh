#!/usr/bin/env bash
#
# Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/../../"

SGX_VERSION=$(awk '/STRFILEVER/ {print $3}' ${ROOT_DIR}/QuoteGeneration/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')


rel_dir_base=PCKIDRetrievalTool_v$SGX_VERSION
rel_dir_name=$rel_dir_base$1

rm -rf $rel_dir_base*
make clean
make STANDALONE=1

mkdir $rel_dir_name
cp PCKIDRetrievalTool $rel_dir_name
cp network_setting.conf $rel_dir_name
cp ../../QuoteGeneration/psw/ae/data/prebuilt/libsgx_pce.signed.so $rel_dir_name/libsgx_pce.signed.so.1
cp ../../QuoteGeneration/psw/ae/data/prebuilt/libsgx_id_enclave.signed.so $rel_dir_name/libsgx_id_enclave.signed.so.1
cp ../SGXPlatformRegistration/build/lib64/libmpa_uefi.so $rel_dir_name/libmpa_uefi.so.1
cp ../../../../build/linux/libsgx_enclave_common.so $rel_dir_name/libsgx_enclave_common.so.1
cp ../../../../build/linux/libsgx_urts.so $rel_dir_name/libsgx_urts.so
cp README_standalone.txt $rel_dir_name/README.txt
cp License.txt $rel_dir_name
cd $rel_dir_name
ln -s libsgx_urts.so libsgx_urts.so.1
cd ..

tar cvpzf $rel_dir_name.tar.gz $rel_dir_name

exit 0

