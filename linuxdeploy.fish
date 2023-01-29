#! /usr/bin/fish

rm -r appdir/
~/bin/linuxdeploy-x86_64.AppImage --appdir appdir --executable=build_release/lnkdump2k --desktop-file=lnkdump2k.desktop --icon-file=lnkdump2k.png
~/bin/appimagetool-x86_64.AppImage --comp xz appdir
