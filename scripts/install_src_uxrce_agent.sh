cd ~

git clone -b v2.4.3 https://github.com/ANCL/Micro-XRCE-DDS-Agent.git
cd Micro-XRCE-DDS-Agent
mkdir -p build
cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig /usr/local/lib/