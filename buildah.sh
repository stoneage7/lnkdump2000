#! /bin/bash

set -e

echo "run with buildah unshare $0"

I=lnkdump2kbuild

if ! buildah images | grep -q $I; then
  C=$(buildah bud .)
  buildah tag $C localhost/$I
fi

C=$(buildah from localhost/$I)
echo $C
buildah copy $C . /b
buildah --workingdir /b run $C cmake . -DCMAKE_BUILD_TYPE=Release
buildah --workingdir /b run $C make
M=$(buildah mount $C)
cp -v $M/b/lnkdump2k .

buildah images
buildah containers

