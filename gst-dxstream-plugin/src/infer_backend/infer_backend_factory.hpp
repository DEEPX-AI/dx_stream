#pragma once

#include "infer_backend.hpp"
#include <memory>

enum class BackendType { AUTO = 0, DXRT = 1, DXVNPU = 2 };

class InferBackendFactory {
public:
    static std::unique_ptr<IInferBackend> Create(BackendType type);
};
