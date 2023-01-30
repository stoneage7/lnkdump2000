#! /bin/bash

set -e

echo "run with buildah unshare $0"

I=lnkdump2kbuild
LINUXDEPLOY="$HOME/bin/linuxdeploy-x86_64.AppImage"
APPIMAGETOOL="$HOME/bin/appimagetool-x86_64.AppImage"
IMAGENAME="LnkDump2000-x86_64.AppImage"

if ! buildah images | grep -q $I; then
  C=$(buildah bud -q .)
  buildah tag $C localhost/$I
fi

echo "Running build."

C=$(buildah from localhost/$I)
echo $C
buildah copy $C . /b
buildah copy $C "$LINUXDEPLOY" /b
buildah copy $C "$APPIMAGETOOL" /b
buildah --workingdir /b run $C cmake . -DCMAKE_BUILD_TYPE=Release
buildah --workingdir /b run $C make
buildah --workingdir /b run $C "./$(basename $LINUXDEPLOY)" --appimage-extract-and-run --appdir appdir --executable=lnkdump2k --desktop-file=lnkdump2k.desktop --icon-file=lnkdump2k.png
buildah --workingdir /b run $C "./$(basename $APPIMAGETOOL)" --appimage-extract-and-run --comp xz appdir $IMAGENAME
M=$(buildah mount $C)
cp -v $M/b/lnkdump2k .
cp -v $M/b/$IMAGENAME .

buildah images
buildah containers

