#pragma once

#include "preprocessor.h"

// Forward declaration already done in preprocessor.h

class PreprocessorFactory {
public:
    static std::shared_ptr<Preprocessor> create_preprocessor(GstDxPreprocess *element);
};