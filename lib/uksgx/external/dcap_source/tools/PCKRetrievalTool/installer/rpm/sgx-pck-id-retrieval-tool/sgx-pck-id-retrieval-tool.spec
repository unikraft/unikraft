#
# Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
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


%define _install_path @install_path@

Name:           sgx-pck-id-retrieval-tool
Version:        @version@
Release:        1%{?dist}
Summary:        Intel(R) Software Guard Extensions:this tool is used to collect the platform information to retrieve the PCK certs from PCS(Provisioning Certification Server)
Group:          Development/System
Recommends:     libsgx-urts >= 2.17, libsgx-ae-pce >= %{version}-%{release}, libsgx-ae-id-enclave >= %{version}-%{release},libsgx-ra-uefi >= %{version}-%{release}

License:        BSD License
URL:            https://github.com/intel/SGXDataCenterAttestationPrimitives
Source0:        %{name}-%{version}.tar.gz

%description
Intel(R) Software Guard Extensions:this tool is used to collect the platform information to retrieve the PCK certs from PCS(Provisioning Certification Server)

%prep
%setup -qc

%install
make DESTDIR=%{?buildroot} install
echo "%{_install_path}" > %{_specdir}/list-%{name}
find %{?buildroot} | sort | \
awk '$0 !~ last "/" {print last} {last=$0} END {print last}' | \
sed -e "s#^%{?buildroot}##" | \
grep -v "^%{_install_path}" >> %{_specdir}/list-%{name} || :
sed -i 's#^/etc/rad.conf#%config &#' %{_specdir}/list-%{name}

%files -f %{_specdir}/list-%{name}

%debug_package

%posttrans
################################################################################
# Set up SGX pck cert id retrieve tool                                         #
################################################################################

# Install the SGX_PCK_ID_RETRIEVE_TOOL 
ln -s -f /opt/intel/sgx-pck-id-retrieval-tool/PCKIDRetrievalTool /usr/local/bin/PCKIDRetrievalTool
retval=$?

if test $retval -ne 0; then
    echo "failed to install $SGX_PCK_ID_RETRIEVE_TOOL_NAME."
    exit 6
fi

echo -e "Installation succeed!"

%postun

# Removing SGX_PCK_ID_RETRIEVE_TOOL soft link file
rm -f /usr/local/bin/PCKIDRetrievalTool

echo -e "Uninstallation succeed!"

%changelog
* Mon Apr 28 2020 SGX Team
- Initial Release
