#!/bin/bash
if [ $# -ne 1 ];
then
	echo "You need to specify the version number"
	echo "The last version was" `git tag | tail -1`
	exit 1
fi
version=$1
echo '#define FWTS_VERSION "'$version'"' > src/lib/include/fwts_version.h
echo '#define FWTS_DATE    "'`date`'"' >> src/lib/include/fwts_version.h
git add src/lib/include/fwts_version.h
git commit -s -m"lib: fwts_version.h - update to $version"
git tag -m'"Version '$1'"' $1
