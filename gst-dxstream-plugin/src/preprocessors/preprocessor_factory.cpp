#include "preprocessor_factory.h"

#ifdef HAVE_LIBRGA
#include "rga_preprocessor.h"
#elif DEEPX_V3
#include "v3_dsp_preprocessor.h"
#else
#include "libyuv_preprocessor.h"
#endif

std::shared_ptr<Preprocessor> PreprocessorFactory::create_preprocessor(GstDxPreprocess *element) {
    // Future: could add logic here to select based on element properties
    // or runtime conditions
    
#ifdef HAVE_LIBRGA
    return std::make_shared<RgaPreprocessor>(element);
#elif DEEPX_V3
    return std::make_shared<V3DspPreprocessor>(element);
#else
    return std::make_shared<LibyuvPreprocessor>(element);
#endif
}