# Demo for CMake package usage

This folder demonstrates how to use eventpp as CMake package.  
To build the demo program,  

1, Install eventpp as CMake package

1.1, Install eventpp locally.  

In eventpp root folder, run the commands,  
```
mkdir build
cd build
cmake ..
sudo make install
```

2, Then  

```
mkdir build
cd build
cmake .. -G"MinGW Makefiles"
mingw32-make.exe
```

If you use other making system rather than MingW, replace the -G generator and mingw32-make.

