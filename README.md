# Vita Final Renderer
![Capture](https://github.com/user-attachments/assets/d398914a-6cc6-4a57-a51d-7bd762968198)

This project is an attempt at real-time physically based lighting on the PlayStation Vita and uses the open-source VitaSDK toolchain.

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
   To link against existing precompiled shader binaries instead of rebuilding them, add `-DBUILD_SHADERS=OFF`:
   ```sh
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=psvita-toolchain.cmake -DBUILD_SHADERS=OFF
   ```
2. Compile and package the project:
   ```sh
   cmake --build build
   ```

