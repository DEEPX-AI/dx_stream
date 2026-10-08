#pragma once

#include "dxcommon.hpp"
#include "Tracker.hpp"
#include <memory>

// TrackerFactory::createTracker is called directly (not via GObject/GStreamer
// registry lookup) by white-box tracker contract tests, so its symbol must be
// exported from the plugin DLL on Windows — otherwise it is invisible in the
// import lib and tests fail with LNK2019.
class DX_API TrackerFactory {
  public:
    static std::unique_ptr<Tracker>
    createTracker(const std::string &trackerType);
};