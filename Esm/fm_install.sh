#!/bin/bash
# BEGIN_ICS_COPYRIGHT8 ****************************************
# 
# Copyright (c) 2015-2017, Intel Corporation
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

[ -z "${BUILDDIR}" ] && BUILDDIR="."
[ -z "${DESTDIR}" ] && DESTDIR="/"
[ -z "${LIBDIR}" ] && LIBDIR="/usr/lib"

#if [ ! -f "${BUILDDIR}/RELEASE_PATH" ]; then
#	echo "Wrong BUILDDIR? No such file ${BUILDDIR}/RELEASE_PATH"
#	exit 1
#fi

fm_mans="opafmcmd.8 opafmcmdall.8"

mkdir -p ${DESTDIR}/usr/sbin
mkdir -p ${DESTDIR}/usr/lib/systemd/system
mkdir -p ${DESTDIR}/usr/lib/opa-fm/{bin,runtime}
mkdir -p ${DESTDIR}/usr/share/opa-fm/samples
mkdir -p ${DESTDIR}/usr/share/man/man8
mkdir -p ${DESTDIR}/etc/opa-fm
mkdir -p ${DESTDIR}/usr/lib/opa

cd stage.rpm

cp -t ${DESTDIR}/usr/lib/systemd/system opafm.service
cp -t ${DESTDIR}/usr/lib/opa-fm/bin opafmctrl
cp -t ${DESTDIR}/usr/lib/opa-fm/bin opafmd

if [ "$RPM_INS" = "y" ]
then
	mkdir -p ${DESTDIR}/etc/init.d
	cp -t ${DESTDIR}/etc/init.d opafm
fi

cp -t ${DESTDIR}/etc/opa-fm opafm.xml

cp -t ${DESTDIR}/usr/lib/opa-fm/bin fm_capture
cp -t ${DESTDIR}/usr/lib/opa-fm/bin fm_cmd
cp -t ${DESTDIR}/usr/lib/opa-fm/bin fm_cmdall
cp -t ${DESTDIR}/usr/lib/opa-fm/bin smpoolsize

cp -t ${DESTDIR}/usr/lib/opa-fm/runtime sm
cp -t ${DESTDIR}/usr/lib/opa-fm/runtime fe

cp -t ${DESTDIR}/usr/lib/opa-fm/bin config_check
cp -t ${DESTDIR}/usr/lib/opa-fm/bin config_convert
cp -t ${DESTDIR}/usr/lib/opa-fm/bin config_diff
cp -t ${DESTDIR}/usr/lib/opa-fm/bin config_generate
cp -t ${DESTDIR}/usr/lib/opa-fm/bin opafm
cp -t ${DESTDIR}/usr/lib/opa-fm/bin opaxmlextract
cp -t ${DESTDIR}/usr/lib/opa-fm/bin opaxmlfilter

cp -t ${DESTDIR}/usr/lib/opa .comp_opafm.pl

cp -t ${DESTDIR}/usr/share/opa-fm opafm_src.xml

cp -t ${DESTDIR}/usr/share/opa-fm opafm.xml

cp -t ${DESTDIR}/usr/share/opa-fm/samples opa_ca_openssl.cnf-sample
cp -t ${DESTDIR}/usr/share/opa-fm/samples opa_comp_openssl.cnf-sample

ln -s /usr/lib/opa-fm/bin/fm_cmd ${DESTDIR}/usr/sbin/opafmcmd
ln -s /usr/lib/opa-fm/bin/fm_cmdall ${DESTDIR}/usr/sbin/opafmcmdall

cp -t ${DESTDIR}/usr/share/man/man8 $fm_mans