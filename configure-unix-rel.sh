mkdir Build
mkdir Build/unix-rel
cd Build/unix-rel
cmake ../.. -G"Unix Makefiles" -DBUILD_TYPE=test -DCMAKE_BUILD_TYPE=Release

