# MinBucket-MPSI
An efficient C++ repo implementing the uMPSU protocol presented in our paper: **MinBucket MPSI: Breaking the Max Size Bottleneck in Multi-Party Private Set Intersection**

Two testing methods are available. For quick verification of our code, you can refer to the **Docker Quick Start** section.

## Docker Quick Start

Docker simplifies the setup and execution of our protocols by 
providing a pre-configured environment. Follow these steps to get 
started:

### Prerequisites

- Ensure the Docker is installed on your machine. You can download Docker from the [official website](https://www.docker.com/products/docker-desktop).

### Pulling the Docker image

```
docker pull kafei2cy/minbucket-mpsi:v2.0
```

### Running a Docker Container

To run a Docker container from the image you pulled and access the Container Shell, use the following command:

```shell
docker run -it --name your-container-name kafei2cy/minbucket-mpsi:v2.0 /bin/bash

#if you want use tc to change network setting, use the following command
docker run -it --cap-add=NET_ADMIN --name your-container-name kafei2cy/minbucket-mpsi:v2.0 /bin/bash
```

- `--name your-container-name` gives your container a name for easier reference.

### How to Modify Test Parameters

### Change The Network Setting

```
#change the network setting，for example:
tc qdisc add dev lo root netem delay 80ms rate 400Mbit

#delete the setting
tc qdisc del dev lo root
```

### Test for MinBucket-MPSI

```
cd /home/MPSI/MinBucket-MPSI/

#for MPSI test
#set big set size = 2^14, Party number of big set = 1, Party number of small set = 2
#if you want change, just change the arguments value
python3 MPSI_auto_script.py -nn 14 -big 1 -small 2


#for MPSI-CARD test
#set big set size = 2^14, Party number of big set = 1, Party number of small set = 2
#if you want change, just change the arguments value
python3 MPSICA_auto_script.py -nn 14 -big 1 -small 2
```

### Stopping and Removing a Docker Container

To stop a running container, use the following command:

```sh
docker stop your-container-name
```

To remove a stopped container, use the following command:

```sh
docker rm your-container-name
```


## Compiling the MinBucket-MPSI Locally

### Environment

This code and following instructions is tested on Ubuntu 22.04, with g++ 11.4.0 and CMake 3.22.1

### Dependencies

```
sudo apt-get update
sudo apt-get install build-essential tar curl zip unzip pkg-config libssl-dev libomp-dev libtool
sudo apt install gcc g++ gdb git make cmake
```

### Notes for Errors on Boost

When building libOTe using the command `python3 build.py ...`, the following error may occur:

```
CMake Error at /home/cy/Desktop/tbb/eAPSU/pnECRG_OTP/volepsi/out/coproto/thirdparty/getBoost.cmake:67 (file):
  file DOWNLOAD HASH mismatch

    for file: [your_path_to_libOTe/out/boost_1_84_0.tar.bz2]
      expected hash: [1bed88e40401b2cb7a1f76d4bab499e352fa4d0c5f31c0dbae64e24d34d7513b]
        actual hash: [79e6d3f986444e5a80afbeccdaf2d1c1cf964baa8d766d20859d653a16c39848]
             status: [0;"No error"]
```

for the version of libOTe we are using, adjust line 8 in the file `libOTe/out/coproto/thirdparty/getBoost.cmake` to:

```
set(URL "https://archives.boost.io/release/1.84.0/source/boost_1_84_0.tar.bz2")
```

### Installation

```
#For convenience, we will create a folder for installing and compiling .
mkdir MPSI
cd MPSI/

#First, install the libraries required by our repository.
#openssl
git clone https://github.com/openssl/openssl.git
cd openssl/ 
#download the latest OpenSSL from the website, to support curve25519, modify crypto/ec/curve25519.c line 211: remove "static", then compile it:
./Configure no-shared enable-ec_nistp_64_gcc_128 no-ssl2 no-ssl3 no-comp --prefix=/usr/local/openssl
make depend
sudo make install

#vole-psi
git clone https://github.com/Visa-Research/volepsi.git
cd volepsi
# compile and install volepsi
python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON -DVOLE_PSI_ENABLE_GMW=ON -DVOLE_PSI_ENABLE_CPSI=OFF -DVOLE_PSI_ENABLE_OPPRF=OFF
python3 build.py --install=../libvolepsi
cp out/build/linux/volePSI/config.h ../libvolepsi/include/volePSI/


#libOTe
#When you encounter a hash mismatch error with Boost, you can refer to the "Notes for Errors on Boost" section.
git clone --recursive https://github.com/osu-crypto/libOTe.git
cd libOTe
git checkout b216559
git submodule update --recursive --init 
python3 build.py --all --boost --relic
git submodule update --recursive --init 
sudo python3 build.py -DENABLE_SODIUM=OFF -DENABLE_MRR_TWIST=OFF -DENABLE_RELIC=ON --install=/usr/local/libOTe

#vcpkg
git clone https://github.com/microsoft/vcpkg
cd vcpkg/
./bootstrap-vcpkg.sh
./vcpkg install seal[no-throw-tran]
./vcpkg install kuku
./vcpkg install openssl
./vcpkg install log4cplus
./vcpkg install cppzmq
./vcpkg install flatbuffers
./vcpkg install jsoncpp
./vcpkg install tclap

#Next, get the MinBucket-MPSI repository: clone it with Git, or download the ZIP and extract it.
#For example, we use git clone the repo
git clone https://github.com/Azzzting/MinBucket-MPSI.git

#Then clone the Kunlun repository into the MinBucket-MPSI directory.
cd MinBucket-MPSI
git clone https://github.com/yuchen1024/Kunlun.git
```

### Linking Vole-PSI

Vole-PSI can be linked via cmake in our CMakeLists.txt, just change the path to libvolepsi.

```
find_package(volePSI REQUIRED HINTS "/path/to/libvolepsi")
```

You will need to modify the following files:

- MPSI/MinBucket-MPSI/MCRG/CMakeLists.txt
  
- MPSI/MinBucket-MPSI/uMCRG/MCRG_diff_large/CMakeLists.txt

- MPSI/MinBucket-MPSI/JPEQT/CMakeLists.txt
  

### Compiling

```
#in MPSI/MinBucket-MPSI/MCRG
mkdir build
cd build
cmake ..
make

#in MPSI/MinBucket-MPSI/uMCRG/MCRG
mkdir build
cd build
cmake .. -DLIBOTE_PATH=/usr/local/ -DCMAKE_TOOLCHAIN_FILE=/path_to_MPSI/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
cd ..
cp 16M-1024.json build/

#in MPSI/MinBucket-MPSI/uMCRG/MCRG_diff_large
mkdir mcrg_result
mkdir build
cd build
cmake ..
make

#in MPSI/MinBucket-MPSI/JPEQT
mkdir build
cd build
cmake ..
make

```

### How to Modify Test Parameters

- **Change the size of the big set**:

Modify line 120 in `MPSI/MinBucket-MPSI/uMCRG/MCRG/auto_prepare.py`.

```
#indicates the big set size is 2^14 
prepare_receiver_data(pow(2,14), 256, 16, i, '0')
```

- **Change the number of party**:
  

Modify line 6,8 in `MPSI/MinBucket-MPSI/auto_test.sh`.

```
# set parites
#big_set_total is the number of participants who holds big sets(>2^{10})
big_set_total=1
#small_set_total is the number of participants who holds small sets(=2^{10})
small_set_total=2
```

### Change The Network Setting 

```
#change the network setting, for example:
sudo tc qdisc add dev lo root netem delay 80ms rate 400Mbit

#delete the setting
sudo tc qdisc del dev lo root
```

### Test for MinBucket-MPSI

After modifying the parameters each time, run the `auto_test.sh` script to apply the changes，then you can start the testing process.

```
#in MPSI/MinBucket-MPSI/
#if you change the test setting, run auto_test.sh to apply it.
./auto_test.sh

#for MPSI test
./run_uMCRG.sh
./run_MCRG.sh
./run_J-PEQT.sh

#If you have already run the above commands, then you can run this command directly.
./run_JP-PEQT

#for MPSI-CARD test
./run_uMCRG.sh
./run_MCRG.sh
./run_JP-PEQT.sh
```

## Code Structure
Our code consists of the following three parts：
- uMCRG: The unbalanced MCRG protocol in the paper.
- MCRG: The balanced MCRG protocol in the paper.
- JPEQT: The Joint Private Equality Test(J-PEQT) and Joint Permuted Private Equality Test(JP-PEQT) protocol in the paper.
- auto_test.sh: Automated test script.
