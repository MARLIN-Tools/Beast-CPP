#pragma once

#include <string>
#include <vector>

#ifdef BEAST_WITH_ONNX
#include <onnxruntime_cxx_api.h>
#endif

#include "beast/heuristics/infra/heuristic.hpp"
#include "beast/neural_networks/net_input_factory.hpp"

namespace beast::heuristics {

class NeuralNetwork final : public Heuristic {
   public:
    NeuralNetwork(
        const std::filesystem::path& model_file,
        bool fifty_moves_rule,
        std::filesystem::path syzygy_path,
        int syzygy_probe_limit,
        int threads
    );

    [[nodiscard]] bool useQuiescence() const override;

   protected:
    [[nodiscard]] double evaluateInternal(const chess::Board& board) const override;

   private:
#ifdef BEAST_WITH_ONNX
    Ort::Env env_;
    mutable Ort::Session session_;
    std::string input_name_;
    std::string output_name_;
#endif
    beast::neural_networks::NetInputFunction nn_input_;
    mutable std::vector<float> input_buffer_;
};

}  // namespace beast::heuristics
