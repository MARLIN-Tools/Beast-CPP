#include "beast/heuristics/neural_network.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace beast::heuristics {

NeuralNetwork::NeuralNetwork(
    const std::filesystem::path& model_file,
    const bool fifty_moves_rule,
    std::filesystem::path syzygy_path,
    const int syzygy_probe_limit,
    const int threads
)
    : Heuristic(fifty_moves_rule, std::move(syzygy_path), syzygy_probe_limit)
#ifdef BEAST_WITH_ONNX
      ,
      env_(ORT_LOGGING_LEVEL_WARNING, "beast"),
      session_(
          env_,
          model_file.c_str(),
          [&threads] {
              Ort::SessionOptions options;
              options.SetIntraOpNumThreads(threads);
              return options;
          }()
      )
#endif
{
#ifdef BEAST_WITH_ONNX
    Ort::AllocatorWithDefaultOptions allocator;
    input_name_ = session_.GetInputNameAllocated(0, allocator).get();
    output_name_ = session_.GetOutputNameAllocated(0, allocator).get();

    auto model_version_alloc = session_.GetModelMetadata().LookupCustomMetadataMapAllocated(
        "model_version",
        allocator
    );
    const std::string model_version = model_version_alloc ? model_version_alloc.get() : "v1";
    nn_input_ = beast::neural_networks::netInputFromString(model_version);
#else
    (void)model_file;
    (void)threads;
    nn_input_ = beast::neural_networks::netInputFromVersion(beast::neural_networks::NetInputVersion::V1);
    throw std::runtime_error("ONNX runtime support is not compiled in. Build with BEAST_WITH_ONNX=ON.");
#endif
}

bool NeuralNetwork::useQuiescence() const {
    return true;
}

double NeuralNetwork::evaluateInternal(const chess::Board& board) const {
#ifdef BEAST_WITH_ONNX
    nn_input_(board, input_buffer_);
    const std::array<int64_t, 4> input_shape = {1, static_cast<int64_t>(input_buffer_.size() / 64), 8, 8};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_buffer_.data(),
        input_buffer_.size(),
        input_shape.data(),
        input_shape.size()
    );

    const char* input_names[] = {input_name_.c_str()};
    const char* output_names[] = {output_name_.c_str()};
    auto outputs = session_.Run(Ort::RunOptions{nullptr}, input_names, &tensor, 1, output_names, 1);
    float* output_data = outputs[0].GetTensorMutableData<float>();
    return std::round(static_cast<double>(output_data[0]) * 2000.0);
#else
    (void)board;
    throw std::runtime_error("ONNX runtime support is not compiled in.");
#endif
}

}  // namespace beast::heuristics
