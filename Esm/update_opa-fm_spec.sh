#!/bin/bash

id=$(./get_id_and_versionid.sh | cut -f1 -d' ')
versionid=$(./get_id_and_versionid.sh | cut -f2 -d' ')

if [ "$id" = "" -o "$versionid" = "" ]
then
	exit 1
fi

if [ "$id" = "rhel" ]
then
	GE_7_0=$(echo "$versionid >= 7.0" | bc)
	GE_7_4=$(echo "$versionid >= 7.4" | bc)
	if [ $GE_7_4 = 1 ]
	then
		sed -i "s/__RPM_BLDRQ1/expat-devel, rdma-core-devel, libibmad-devel, openssl-devel/g" opa-fm.spec
	else
		sed -i "s/__RPM_BLDRQ1/expat-devel, libibumad-devel, libibverbs-devel, libibmad-devel, openssl-devel/g" opa-fm.spec
	fi
	if [ $GE_7_0 = 1 ]
	then
		sed -i "s/__RPM_BLDRQ2/BuildRequires: systemd %{?systemd_requires} %{?BuildRequires}/g" opa-fm.spec
		sed -i "s/__RPM_RQ1/Requires: systemd %{?systemd_requires}/g" opa-fm.spec
		sed -i "/__RPM_SYSCONF/,+1d" opa-fm.spec
	else
		sed -i "s/__RPM_BLDRQ2/Requires(post): \/sbin\/chkconfig/g" opa-fm.spec
		sed -i "s/__RPM_RQ1/Requires(preun): \/sbin\/chkconfig/g" opa-fm.spec
		sed -i 's/RPM_INS=n/RPM_INS=y/g' opa-fm.spec
		sed -i "s/__RPM_SYSCONF/%{_sysconfdir}\/init.d\/opafm/g" opa-fm.spec
	fi
	sed -i "s/__RPM_RQ2/Requires: libibumad%{?_isa}, libibmad%{?_isa}, libibverbs%{?_isa}, rdma, expat%{?_isa}, libhfi1, openssl%{?_isa}/g" opa-fm.spec
	sed -i "/__RPM_DEBUG/,+1d" opa-fm.spec
elif [ "$id" = "sles" ]
then
	GE_11_1=$(echo "$versionid >= 11.1" | bc)
	GE_12_1=$(echo "$versionid >= 12.1" | bc)
	GE_12_3=$(echo "$versionid >= 12.3" | bc)
	if [ $GE_12_3 = 1 ]
	then
		sed -i "s/__RPM_BLDRQ1/libexpat-devel, rdma-core-devel, libibmad-devel, openssl-devel/g" opa-fm.spec
	else
		sed -i "s/__RPM_BLDRQ1/libexpat-devel, libibumad-devel, libibverbs-devel, libibmad-devel, openssl-devel/g" opa-fm.spec
	fi
	if [ $GE_12_1 = 1 ]
	then
		sed -i "s/__RPM_BLDRQ2/BuildRequires: systemd %{?systemd_requires} %{?BuildRequires}/g" opa-fm.spec
		sed -i "s/__RPM_RQ1/Requires: systemd %{?systemd_requires}/g" opa-fm.spec
	else
		sed -i "s/__RPM_BLDRQ2/Requires(post): /sbin/chkconfig/g" opa-fm.spec
		sed -i "s/__RPM_RQ1/Requires(preun): /sbin/chkconfig/g" opa-fm.spec
	fi
	if [ $GE_11_1 = 1 ]
	then
		sed -i "s/__RPM_DEBUG/%debug_package/g" opa-fm.spec
	else
		sed -i "/__RPM_DEBUG/,+1d" opa-fm.spec
	fi
	sed -i "/__RPM_SYSCONF/,+1d" opa-fm.spec
	sed -i "s/__RPM_RQ2/Requires: libibumad3, libibmad5, libibverbs1, rdma, libexpat1, openssl/g" opa-fm.spec
else
	echo ERROR: Unsupported distribution: $id $versionid
	exit 1
fi

exit 0
