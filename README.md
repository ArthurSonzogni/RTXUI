### Install clang 18
```bash
# Download Clang 18 repository
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
ln -s /usr/bin/clang-18 /usr/bin/clang # Set clang as default
```

### Install Ninja
```bash
sudo apt install ninja-build
```

