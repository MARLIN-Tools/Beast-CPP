# Beast-CPP
UCI-compatible C++ chess engine with:
- classical heuristic (HCE)
- ONNX neural-network heuristic
- alpha-beta + quiescence + TT + move ordering improvements
- time management and iterative deepening

## Attribution
Beast-CPP is based on the original Beast 3.2.4 project by maelic13:
https://github.com/maelic13/beast

## Repository Layout
- `cpp/`: C++ engine source and CMake project
- `external/chess-library/include/chess.hpp`: bundled chess movegen/header dependency
- `external/onnxruntime/onnxruntime-win-x64-1.20.1`: bundled ONNX Runtime (Windows x64)
- `models/`: ONNX model files

## Build (Windows, ONNX enabled)
```powershell
cd cpp
cmake -S . -B build -DBEAST_WITH_ONNX=ON
cmake --build build --config Release -j
```

For CPU-specific tuning:
```powershell
cmake -S . -B build -DBEAST_WITH_ONNX=ON -DBEAST_ARCH_FLAGS="/arch:AVX2"
```

`cpp/CMakeLists.txt` will use bundled ONNX Runtime automatically if present at:
`external/onnxruntime/onnxruntime-win-x64-1.20.1`

You can also override with:
```powershell
$env:ONNXRUNTIME_ROOT="C:\\path\\to\\onnxruntime"
```

## Build (Classical only)
```powershell
cd cpp
cmake -S . -B build
cmake --build build --config Release -j
```

## Run
```powershell
cpp\\build\\Release\\beast_cpp.exe
```

For NN mode through UCI:
```text
setoption name Heuristic value neural_network
setoption name ModelFile value <absolute-path-to-onnx-model>
```

## Releases
- Tag push `v*` builds Windows ONNX binaries in 3 variants:
  - `x64-baseline`
  - `x64-avx2`
  - `x64-avx2-bmi2`
- Workflow file: `.github/workflows/release-windows.yml`
