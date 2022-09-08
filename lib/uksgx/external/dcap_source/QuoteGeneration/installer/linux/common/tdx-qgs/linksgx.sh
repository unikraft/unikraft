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

if test $(id -u) -ne 0; then
    echo "Root privilege is required."
    exit 1
fi

if [ -c /dev/sgx_provision -o -c /dev/sgx/provision ]; then
    getent group sgx_prv &> /dev/null
    if [ "$?" != "0" ]; then
        # Add sgx_prv for dcap driver, which ensures that no matter what
        # the order of package installation, aesmd can have access to
        # the sgx_provision device file.
        groupadd sgx_prv

        if ! which udevadm &> /dev/null; then
            exit 0
        fi
        udevadm control --reload || :
        udevadm trigger || :
    fi
    usermod -aG sgx_prv qgsd &> /dev/null

    # For systemd which supports https://github.com/systemd/systemd/pull/18944/files
    if [ "sgx" = "$(stat -c '%G' /dev/sgx_enclave 2>/dev/null)" ]; then
        usermod -aG sgx qgsd &> /dev/null
    fi
fi

/usr/sbin/modprobe vhost-vsock &> /dev/null
/usr/sbin/modprobe vsock_loopback &> /dev/null

echo
