#sudo apt-get update -y
#sudo apt-get install -y qemu-system-x86-64

cd ci/libc_tests
git clone https://github.com/unikraft/app-helloworld
git clone https://github.com/unikraft/lib-libc-test
cp config app-helloworld/.config
cp Makefile app-helloworld/Makefile
cd app-helloworld && make -j$(nproc)
qemu-system-x86_64 -kernel "build/app-helloworld_qemu-x86_64" -enable-kvm -nographic -cpu Skylake-Server > ../test_results.txt