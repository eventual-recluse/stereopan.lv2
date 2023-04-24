# stereopan.lv2
A simple lv2 plugin including amp, stereo width and balance adjustment with parameter smoothing.

# Build

Building this plugin needs the lv2 development libraries as well as the usual C++ build tools.

To build on linux from the command line:
```
g++ -fvisibility=hidden -O3 -ffast-math -fPIC -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed -shared -pthread `pkg-config --cflags lv2` -lm `pkg-config --libs lv2` stereopan.cpp -o stereopan.so
```
# Installation:

After building, create a folder named "stereopan.lv2" in an lv2 plugin folder and copy manifest.ttl, stereopan.ttl and stereopan.so to that folder. The local folder for lv2 plugins is usually  ~/.lv2/  on linux.
