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

top_dir=`dirname $0`
out_dir=$top_dir
ae_file_name=prebuilt_dcap_1.14.tar.gz
checksum_file=SHA256SUM_prebuilt_dcap_1.14.cfg
server_url_path=https://download.01.org/intel-sgx/sgx-dcap/1.14/linux
server_ae_url=$server_url_path/$ae_file_name
server_checksum_url=$server_url_path/$checksum_file

rm -rf $out_dir/$ae_file_name
wget $server_ae_url -P $out_dir
if [ $? -ne 0 ]; then
    echo "Fail to download file $server_ae_url"
    exit -1
fi

rm -f $out_dir/$checksum_file
wget $server_checksum_url -P $out_dir
if [ $? -ne 0 ]; then
    echo "Fail to download file $server_checksum_url"
    exit -1
fi

pushd $out_dir

sha256sum -c $checksum_file
if [ $? -ne 0 ]; then
    echo "Checksum verification failure"
    exit -1
fi

tar -zxf $ae_file_name
rm -f $ae_file_name
rm -f $checksum_file

popd
