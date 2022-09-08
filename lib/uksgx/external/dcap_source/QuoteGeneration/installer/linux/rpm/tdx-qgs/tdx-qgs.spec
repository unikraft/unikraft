#
# Copyright (C) 2011-2019 Intel Corporation. All rights reserved.
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

Name:           tdx-qgs
Version:        @version@
Release:        1%{?dist}
Summary:        Intel(R) TD Quoting Generation Service
Group:          Development/System

License:        BSD License
URL:            https://github.com/intel/linux-sgx
Source0:        %{name}-%{version}.tar.gz

%description
Intel(R) TD Quoting Generation Service

%prep
%setup -qc

%build
make %{?_smp_mflags}

%install
make DESTDIR=%{?buildroot} install
echo "%{_install_path}" > %{_specdir}/list-%{name}
find %{?buildroot} | sort | \
awk '$0 !~ last "/" {print last} {last=$0} END {print last}' | \
sed -e "s#^%{?buildroot}##" | \
grep -v "^%{_install_path}" >> %{_specdir}/list-%{name} || :
sed -i 's#^/etc/qgsd.conf#%config &#' %{_specdir}/list-%{name}

%files -f %{_specdir}/list-%{name}

%posttrans
if [ -x %{_install_path}/startup.sh ]; then %{_install_path}/startup.sh; fi

%preun
if [ -x %{_install_path}/cleanup.sh ]; then %{_install_path}/cleanup.sh; fi

%changelog
* Thu Feb 18 2021 SGX Team
- Initial Release
