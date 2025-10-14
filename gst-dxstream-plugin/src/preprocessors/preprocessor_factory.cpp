#include "preprocessor_factory.h"
#include "libyuv_preprocessor.h"

#ifdef HAVE_LIBRGA
#include "rga_preprocessor.h"
#endif

Preprocessor* PreprocessorFactory::create_preprocessor(GstDxPreprocess *element) {
    // Future: could add logic here to select based on element properties
    // or runtime conditions
    
#ifdef HAVE_LIBRGA
    return new RgaPreprocessor(element);
#else
    return new LibyuvPreprocessor(element);
#endif
}