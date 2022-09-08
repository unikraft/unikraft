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

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/../../../../"
LINUX_BUILD_DIR=$(readlink -m "${ROOT_DIR}/build/linux")
LINUX_INSTALLER_DIR="${ROOT_DIR}/installer/linux"
LINUX_INSTALLER_COMMON_DIR="${LINUX_INSTALLER_DIR}/common"
LINUX_INSTALLER_COMMON_DCAP_QL_DIR="${LINUX_INSTALLER_COMMON_DIR}/libsgx-dcap-ql"

source ${LINUX_INSTALLER_COMMON_DCAP_QL_DIR}/installConfig
DEB_FOLDER=${DCAP_QL_PACKAGE_NAME}-${DCAP_QL_VERSION}

SGX_VERSION=$(awk '/STRFILEVER/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
DEB_BUILD_FOLDER=${DCAP_QL_PACKAGE_NAME}-${SGX_VERSION}

main() {
    pre_build
    create_upstream_tarball
    unpack_upstream_tarball
    generate_install
    generate_copyright
    update_version
    rename_tarball
    build_deb_package
    post_build
}

pre_build() {
    rm -fR ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
    cp -fR ${SCRIPT_DIR}/${DEB_FOLDER} ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
}

post_build() {
    rm -fR ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
}

create_upstream_tarball() {
    ${LINUX_INSTALLER_COMMON_DCAP_QL_DIR}/createTarball.sh
    cp ${LINUX_INSTALLER_COMMON_DCAP_QL_DIR}/output/${TARBALL_NAME} ${SCRIPT_DIR}
}

unpack_upstream_tarball() {
    pushd ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
    cp ../${TARBALL_NAME} .
    tar xvf ${TARBALL_NAME}
    rm -f ${TARBALL_NAME}
    popd
}

generate_install() {
    echo "debian/tmp/${DCAP_QL_PACKAGE_NAME}/* ." > ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}/debian/${DCAP_QL_PACKAGE_NAME}.install
    echo "debian/tmp/${DCAP_QL_DEV_PACKAGE_NAME}/* ." > ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}/debian/${DCAP_QL_DEV_PACKAGE_NAME}.install
}

generate_copyright() {
    pushd ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
    rm -f debian/copyright
    find package/licenses/ -type f -print0 | xargs -0 -n1 cat >> debian/copyright
    popd
}

get_os_code() {
    OS_CODE=$(lsb_release -cs 2> /dev/null)
    if [ -z ${OS_CODE} ]; then
        OS_CODE=$(grep "VERSION_CODENAME" /etc/os-release 2> /dev/null | cut -d= -f2)
    fi
    echo ${OS_CODE}
}

update_version() {
    pushd ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
    INS_VERSION=$(echo $(dpkg-parsechangelog |grep "Version" | cut -d: -f2))
    DEB_VERSION=$(echo $INS_VERSION | cut -d- -f2)

    FULL_VERSION=${SGX_VERSION}-$(get_os_code)${DEB_VERSION}
    sed -i "s/${INS_VERSION}/${FULL_VERSION}/" debian/changelog
    sed -i "s/@dep_version@/${FULL_VERSION}/g" debian/control
    popd
}

rename_tarball() {
    TARBALL_NAME_NEW_VERSION=$(echo ${TARBALL_NAME} | sed "s/${DCAP_QL_VERSION}/${SGX_VERSION}/")
    mv ${SCRIPT_DIR}/${TARBALL_NAME} ${SCRIPT_DIR}/${TARBALL_NAME_NEW_VERSION}
}

build_deb_package() {
    pushd ${SCRIPT_DIR}/${DEB_BUILD_FOLDER}
    SOURCE_DATE_EPOCH="$(date +%s)" LINUX_BUILD_DIR="${LINUX_BUILD_DIR}" dpkg-buildpackage -us -uc
    popd
}
main $@
