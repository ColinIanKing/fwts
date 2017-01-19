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

# find the version in src/lib/include/fwts_version.h
_fwts_version=$(git grep FWTS_VERSION ../src/lib/include/fwts_version.h | awk -F "\"" ' { print $2 } ')
# strip off leading "V" from FWTS_VERSION
fwts_version=${_fwts_version#"V"}

# change __VERSION, __MAJOR, and __MINOR_SUBMINOR in fwts.spec.in
major=$(echo $fwts_version | awk -F "." ' { print $1 }')
minor_subminor=$(echo $fwts_version | awk -F "." ' { print $2"."$3 }')
sed -i 's/{__VERSION}/'${fwts_version}'/g' fwts.spec.in
sed -i 's/{__MAJOR}/'${major}'/g' fwts.spec.in
sed -i 's/{__MINOR_SUBMINOR}/'${minor_subminor}'/g' fwts.spec.in

# create fwts.spec from fwts.spec.in
cp -f fwts.spec.in fwts.spec

# get the tarball
(cd $RPM/SOURCES && curl -O http://fwts.ubuntu.com/release/fwts-V${fwts_version}.tar.gz)

# copy the specfile over
rm -f $RPM/SPECS/fwts.spec
cp fwts.spec $RPM/SPECS

# build the rpm
$RPMBUILD --define "_sourcedir $RPM/SOURCES" --define "_builddir $RPM/BUILD" --define "_srcrpmdir $RPM/SRPMS" --define "_rpmdir $RPM/RPMS" --define "_specdir $RPM/SPECS" -ba $RPM/SPECS/fwts.spec
