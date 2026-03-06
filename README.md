# Beast-CPP
UCI-compatible C++ chess engine with:
- classical heuristic (HCE)
- ONNX-based neural-network evaluation
- alpha-beta + quiescence + TT + move ordering improvements
- time management and iterative deepening

## Attribution
Beast-CPP is based on the original Beast 3.2.4 project by maelic13:
https://github.com/maelic13/beast

## Repository Layout
- `cpp/`: C++ engine source and CMake project
- `external/chess-library/include/chess.hpp`: bundled chess movegen/header dependency
- `models/`: ONNX model files

## Neural Model Note
- Beast-CPP uses ONNX Runtime for neural evaluation.
- ONNX model files (`.onnx`) are not the same format as Stockfish-style `.nnue` files.

## Build (Windows, ONNX enabled)
```powershell
Invoke-WebRequest https://github.com/microsoft/onnxruntime/releases/download/v1.20.1/onnxruntime-win-x64-1.20.1.zip -OutFile onnxruntime.zip
Expand-Archive onnxruntime.zip -DestinationPath .
$env:ONNXRUNTIME_ROOT="$PWD\\onnxruntime-win-x64-1.20.1"

cd cpp
cmake -S . -B build -DBEAST_WITH_ONNX=ON -DONNXRUNTIME_ROOT="$env:ONNXRUNTIME_ROOT"
cmake --build build --config Release -j
```

For CPU-specific tuning:
```powershell
cmake -S . -B build -DBEAST_WITH_ONNX=ON -DONNXRUNTIME_ROOT="$env:ONNXRUNTIME_ROOT" -DBEAST_ARCH_FLAGS="/arch:AVX2"
```

You can also set ONNX runtime root globally:
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
- GitHub Actions downloads ONNX Runtime during the workflow (not vendored in this repo).
- Workflow file: `.github/workflows/release-windows.yml`
