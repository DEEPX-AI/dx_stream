#include "../include/TrackerFactory.hpp"
#include "../../OC_SORT/include/OCSort.hpp"
#include <memory>
#include <stdexcept>

std::unique_ptr<Tracker>
TrackerFactory::createTracker(const std::string &trackerType) {
    if (trackerType == "OC_SORT") {
        return std::make_unique<ocsort::OCSort>();
    } else {
        throw std::invalid_argument("Unknown tracker type: " + trackerType);
    }
}