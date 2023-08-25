#!/bin/bash

#
# Copyright (C) 2010-2023 Canonical
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


#
# Get fwts sources, strip out .git directory, add in necessary debian packaging
# files, build source package ready for upload.
#
RELEASES="bionic focal jammy lunar mantic"
REPO=git://kernel.ubuntu.com/hwe/fwts.git
RELEASE_TAR_URL=http://fwts.ubuntu.com/release
FWTS=fwts

#
#  Clone the repo
#
get_source()
{
	echo Getting source
	git clone $REPO
}

#
#  Figure out latest tagged version
#
get_version()
{
	pushd $FWTS >& /dev/null
	git tag | tail -1
	popd >& /dev/null
}

#
#  Checkout version
#
checkout_version()
{
	echo "Checking out version $1"
	pushd $FWTS >& /dev/null
	git checkout -b latest $1
	popd >& /dev/null
}

#
#  Remove .git repo as we don't want this in the final package
#
rm_git()
{
	rm -rf $FWTS/.git
}

#
#  Remove the source
#
rm_source()
{
	rm -rf $FWTS
}

#
#  Prepare the orig tarball
#

prepare_tarball()
{
	mkdir $version
	pushd $version >& /dev/null
	wget -N $RELEASE_TAR_URL/fwts-$version.tar.gz
	wget -N $RELEASE_TAR_URL/SHA256SUMS
	grep "fwts-$version.tar.gz" SHA256SUMS > SHA256SUMS.local
	sha256sum -c SHA256SUMS.local

	if [ $? -ne 0 ]; then
		echo "Checksum unmatched. Abort"
		exit
	fi

	mv fwts-$version.tar.gz fwts_`echo $version|cut -b 2-`.orig.tar.gz
	popd >& /dev/null
}

#
#  Create source package ready for upload and build
#
mk_package()
{
	rel=$1

	rm -rf $version/$rel
  	mkdir -p $version/$rel
	cp -r $FWTS $version/$rel
	cp $version/fwts_`echo $version|cut -b 2-`.orig.tar.gz $version/$rel

	pushd $version/$rel/$FWTS >& /dev/null

	deb_topline=`head -1 debian/changelog`
	deb_release=`echo $deb_topline | cut -f3 -d' '`
	if [ "x$rel;" = "x$deb_release" ]; then
		suffix=''
	else
		suffix="~`echo $rel | cut -c1`"
	fi
	
	#	
	# Mungify changelog hack
	#
	sed "s/) $deb_release/$suffix) $rel;/" debian/changelog > debian/changelog.new
	mv debian/changelog.new debian/changelog

  	echo 'y' | debuild -S
	rm -rf $FWTS
	popd >& /dev/null
}


#
#  Here we go..
#
rm_source
get_source

if [ $# -eq 1 ]; then
	version=$1
else
	version=`get_version`
fi

checkout_version $version
rm_git

prepare_tarball

for I in $RELEASES 
do
	echo Building package for release $I with version $version
	mk_package $I
done

rm_source
