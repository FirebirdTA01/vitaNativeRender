# Vita Final Renderer

This project targets the PlayStation Vita and uses the VitaSDK toolchain.

## Prerequisites
- [VitaSDK](https://vitasdk.org/) installed.
- The following environment variables set in your shell before running CMake:
  ```sh
  export VITASDK=/path/to/vitasdk
  export PATH="$VITASDK/bin:$PATH"
  ```

## Building
1. Generate the build files using the PS Vita toolchain:
   ```sh
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=psvita-toolchain.cmake
   ```
2. Compile and package the project:
   ```sh
   cmake --build build
   ```

