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

%define _license_file COPYING

Name:           libsgx-dcap-default-qpl
Version:        @version@
Release:        1%{?dist}
Summary:        Intel(R) Software Guard Extensions Default Quote Provider Library
Group:          Development/Libraries

License:        BSD License
URL:            https://github.com/intel/SGXDataCenterAttestationPrimitives
Source0:        %{name}-%{version}.tar.gz

%description
Intel(R) Software Guard Extensions Default Quote Provider Library

%package devel
Summary:        Intel(R) Software Guard Extensions Default Quote Provider Library for Developers
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel
Intel(R) Software Guard Extensions Default Quote Provider Library for Developers

%prep
%setup -qc

%install
make DESTDIR=%{?buildroot} install
install -d %{?buildroot}/%{name}%{_docdir}/%{name}
find %{?_sourcedir}/package/licenses/ -type f -print0 | xargs -0 -n1 cat >> %{?buildroot}/%{name}%{_docdir}/%{name}/%{_license_file}
rm -f %{_specdir}/list-%{name}
for f in $(find %{?buildroot}/%{name} -type f -o -type l); do
    echo $f | sed -e "s#%{?buildroot}/%{name}##" >> %{_specdir}/list-%{name}
done
cp -r %{?buildroot}/%{name}/* %{?buildroot}/
rm -fr %{?buildroot}/%{name}
rm -f %{_specdir}/list-%{name}-devel
for f in $(find %{?buildroot}/%{name}-dev -type f -o -type l); do
    echo $f | sed -e "s#%{?buildroot}/%{name}-dev##" >> %{_specdir}/list-%{name}-devel
done
cp -r %{?buildroot}/%{name}-dev/* %{?buildroot}/
rm -fr %{?buildroot}/%{name}-dev
sed -i 's#^/etc/sgx_default_qcnl.conf#%config &#' %{_specdir}/list-%{name}

%files -f %{_specdir}/list-%{name}

%files devel -f %{_specdir}/list-%{name}-devel

%debug_package

%changelog
* Mon Mar 09 2020 SGX Team
- Initial Release
