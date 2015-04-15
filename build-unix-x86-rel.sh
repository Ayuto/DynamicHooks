mkdir Build
mkdir Build/unix-x86
cd Build/unix-x86
cmake ../../src -G"Unix Makefiles"
make
cd ../..

mkdir Build/unix-x86-tests
cd Build/unix-x86-tests
cmake ../../tests -G"Unix Makefiles"
make
cd ../..

python run_tests.py Build/unix-x86-tests