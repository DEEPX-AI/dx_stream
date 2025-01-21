#pragma once

#include "Tracker.hpp"
#include <memory>

class TrackerFactory {
  public:
    static std::unique_ptr<Tracker>
    createTracker(const std::string &trackerType);
};