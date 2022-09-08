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
%define _license_file COPYING

Name:           sgx-dcap-pccs
Version:        @version@
Release:        1%{?dist}
Summary:        Intel(R) Software Guard Extensions PCK Caching Service
Group:          Applications/Internet
Requires:       gcc gcc-c++ make

License:        BSD License
URL:            https://github.com/intel/SGXDataCenterAttestationPrimitives
Source0:        %{name}-%{version}.tar.gz

%description
Intel(R) Software Guard Extensions PCK Caching Service

%prep
%setup -qc

%install
make DESTDIR=%{?buildroot} install
install -d %{?buildroot}%{_docdir}/%{name}
find linux/installer/common/sdk/output/package/licenses/ -type f -print0 | \
xargs -0 -n1 cat >> %{?buildroot}%{_docdir}/%{name}/%{_license_file}
find %{?buildroot} -type d -links 2 | \
sed -e "s#^%{?buildroot}##" | \
grep -v "^%{_libdir}" | \
grep -v "^%{_bindir}" | \
grep -v "^%{_sysconfdir}" | \
grep -v "^%{_install_path}" | \
sed -e "s#^#%dir #" > %{_specdir}/listfiles
for f in $(find %{?buildroot}); do
    if [ -d ${f} ]; then
        echo ${f} | \
        sed -e "s#^%{?buildroot}##" | \
        grep "^%{_install_path}" | \
        sed -e "s#^#%dir #" >> %{_specdir}/listfiles
    else
        echo ${f} | \
        sed -e "s#^%{?buildroot}##" >> %{_specdir}/listfiles
    fi
done
sed -i 's#^%{_install_path}/config/default.json#%config &#' %{_specdir}/listfiles

%files -f %{_specdir}/listfiles

%post
if [ -x %{_install_path}/startup.sh ]; then %{_install_path}/startup.sh; fi

%preun
if [ $1 == 0 -a -x %{_install_path}/cleanup.sh ]; then %{_install_path}/cleanup.sh; fi

%changelog
* Mon Mar 10 2020 SGX Team
- Initial Release
