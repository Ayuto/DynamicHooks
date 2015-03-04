mkdir Build
mkdir Build/unix-dbg
cd Build/unix-dbg
cmake ../.. -G"Unix Makefiles" -DBUILD_TYPE=static -DCMAKE_BUILD_TYPE=Debug
