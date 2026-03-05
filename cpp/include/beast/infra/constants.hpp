#pragma once

namespace beast::infra {

struct Constants {
    static constexpr const char* AUTHOR = "Miloslav Macurek";
    static constexpr const char* ENGINE_NAME = "Beast";
    static constexpr const char* ENGINE_VERSION = "3.2.4-cpp";

    static constexpr int DEFAULT_DEPTH = 2;
    static constexpr int INFINITE_DEPTH = 10000;
    static constexpr int TIME_FLEX = 100;  // [ms]
};

}  // namespace beast::infra

