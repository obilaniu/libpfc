mkdir -p "$2/kmod/kmod.dir"
cp -Rf   "$1/kmod/"* "$2/kmod/kmod.dir"
rm -f    "$2/kmod/kmod.dir/meson.build" "$2/kmod/kmod.dir/mesonbuild.sh"
cd       "$2/kmod/kmod.dir"
sed -i -e "s|\$(src)|$1/kmod|" "$2/kmod/kmod.dir/Kbuild"
make
mv       "pfc.ko" "$2/kmod"
