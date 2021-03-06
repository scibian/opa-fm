# BEGIN_ICS_COPYRIGHT8 ****************************************
# 
# Copyright (c) 2015, Intel Corporation
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Intel Corporation nor the names of its contributors
#       may be used to endorse or promote products derived from this software
#       without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# END_ICS_COPYRIGHT8   ****************************************

#[ICS VERSION STRING: unknown]
Name: opa-fm
Version: 10.6.0.0
Release: 132%{?dist}
Summary: Intel Omni-Path Fabric Management Software

License: GPLv2 or BSD
Url: https://github.com/01org/opa-fm
# tarball created by:
# git clone https://github.com/01org/opa-fm.git
# cd opa-fm
# tar czf opa-fm.tar.gz --exclude-cvs .
Source0: %{name}.tar.gz
ExclusiveArch: x86_64
# The Intel OPA product line is only available on x86_64 platforms at this time.

Requires: rdma

#BuildRequires: libibverbs-devel >= 1.1-1, libibumad-devel, libibmad-devel
BuildRequires: expat-devel, libibumad-devel, libibverbs-devel, libibmad-devel, openssl-devel

BuildRequires: systemd %{?systemd_requires} %{?BuildRequires}
Requires: systemd %{?systemd_requires}
Requires: libibumad%{?_isa}, libibmad%{?_isa}, libibverbs%{?_isa}, rdma, expat%{?_isa}, libhfi1, openssl%{?_isa}

%description
The %{name} contains Intel Omni-Path fabric management applications. This 
includes: the Subnet Manager, Baseboard Manager, Performance Manager, 
Fabric Executive, and some fabric management tools.

%prep
%setup -q -c

%build
cd Esm
./fmbuild $BUILD_ARGS

%install
BUILDDIR=%{_builddir} DESTDIR=%{buildroot} LIBDIR=%{_libdir} RPM_INS=n ./Esm/fm_install.sh
mkdir -p %{buildroot}/%{_localstatedir}/usr/lib/opa-fm/

%post
if [ $1 = 1 ]; then
	if [ $(command -v systemctl) ]; then
		/bin/systemctl daemon-reload >/dev/null 2>&1 || :
	else
		/sbin/chkconfig --add opafm
	fi
fi
%preun
if [ $1 = 1 ] || [ $1 = 0 ]; then
	if [ $(command -v systemctl) ]; then
		systemctl stop opafm.service >/dev/null 2>&1 || :
	else
		/sbin/chkconfig --del opafm
	fi
fi

%files
%doc Esm/README 

/usr/lib/systemd/system/opafm.service
%config(noreplace) %{_sysconfdir}/opa-fm/opafm.xml
/usr/lib/opa-fm/bin/*
/usr/lib/opa-fm/runtime/*
/usr/share/opa-fm/*
/usr/lib/opa/.comp_opafm.pl
%{_sbindir}/opafmcmd
%{_sbindir}/opafmcmdall
%{_mandir}/man8/*

%changelog
* Thu Oct 09 2014 Kaike Wan <kaike.wan@intel.com> - 10.0.0.0-177
- Initial version 


