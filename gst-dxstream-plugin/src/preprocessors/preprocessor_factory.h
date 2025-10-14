#pragma once

#include "preprocessor.h"

// Forward declaration already done in preprocessor.h

class PreprocessorFactory {
public:
    static Preprocessor* create_preprocessor(GstDxPreprocess *element);
};