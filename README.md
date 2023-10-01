# Chibi Tech

This is just a test ground for my projects. It might work. It might not. I make no promises.

# Build
NOTE: only supports Windows.

```
mkdir out && cd out

// Generate ninja files and Build (ninja should be installed with Visual Studio, might require vcvarsall.bat to be called)
cmake -S ..\ --preset=x64-debug
ninja -fbuild.ninja

// Generate for release and Build
cmake -S ..\ --preset=x64-release
ninja -fbuild.ninja
