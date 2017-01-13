#!/bin/bash
#
# Script to build Fedora and Red Hat fwts rpms
#
# NOTE: release changes should be made directly to the fwts.spec file
# and not this script.  This script reads from fwts.spec to determine the
# values of major, minor, and submajor values.

#setup RPM env
RPMBUILD=$(if [ -x "/usr/bin/rpmbuild" ]; then echo rpmbuild; \
	   else echo rpm; fi)
RPM="$(pwd)/rpm"
mkdir -p $RPM/SOURCES $RPM/BUILD $RPM/SRPMS $RPM/SPECS

# get the tarball
major=$(cat fwts.spec | grep -m 1 major | awk -F " " ' { print $3 }')
minor=$(cat fwts.spec | grep -m 1 minor | awk -F " " ' { print $3 }')
subminor=$(cat fwts.spec | grep -m 1 subminor | awk -F " " ' { print $3 }')
tarversion="V$major.$minor.$subminor"

(cd $RPM/SOURCES && curl -O http://fwts.ubuntu.com/release/fwts-${tarversion}.tar.gz)

# copy the specfile over
rm -f $RPM/SPECS/fwts.spec
cp fwts.spec $RPM/SPECS

# build the rpm
$RPMBUILD --define "_sourcedir $RPM/SOURCES" --define "_builddir $RPM/BUILD" --define "_srcrpmdir $RPM/SRPMS" --define "_rpmdir $RPM/RPMS" --define "_specdir $RPM/SPECS" -ba $RPM/SPECS/fwts.spec
