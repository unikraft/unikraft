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


set -e

if test $(id -u) -ne 0; then
    echo "Root privilege is required."
    exit 1
fi

PCCS_USER=pccs
PCCS_HOME=$(readlink -m $(dirname "$0"))
if [ "$1" == "debian" ]; then
    adduser --quiet --system $PCCS_USER --group --home $PCCS_HOME --no-create-home --shell /bin/bash
else
    if [ ! $(getent group $PCCS_USER) ]; then
        groupadd $PCCS_USER
    fi
    if ! id "$PCCS_USER" &>/dev/null; then
        adduser --system $PCCS_USER -g $PCCS_USER --home $PCCS_HOME --no-create-home --shell /bin/bash
    fi
fi
chown -R $PCCS_USER:$PCCS_USER $PCCS_HOME
chmod 640 $PCCS_HOME/config/default.json
if [ "$1" == "debian" -a "${DEBIAN_FRONTEND}" != "noninteractive" ]
then
    /bin/su -c "$PCCS_HOME/install.sh" $PCCS_USER
fi
#Install PCCS as system service
echo -n "Installing PCCS service ..."
if [ -d /run/systemd/system ]; then
    systemctl daemon-reload
    systemctl enable pccs
    if [ "$1" == "debian" ]; then 
        systemctl start pccs
    fi
elif [ -d /etc/init/ ]; then
    /sbin/initctl reload-configuration
else
    echo " failed."
    echo "Unsupported platform - neither systemctl nor initctl was found."
    exit 5
fi
echo "finished."
echo "Installation completed successfully."
