#ifndef OC_SORT_CPP_OCSORT_HPP
#define OC_SORT_CPP_OCSORT_HPP
#include "../../common/include/Tracker.hpp"
#include "Association.hpp"
#include "KalmanBoxTracker.hpp"
#include "lapjv.hpp"
#include <functional>
#include <unordered_map>

namespace ocsort {

class OCSort : public Tracker {
  public:
    void init(const std::map<std::string, std::string> &params) override;
    std::vector<Eigen::RowVectorXf> update(const Eigen::MatrixXf dets) override;
    ~OCSort() override {}

    float det_thresh;
    int max_age;
    int min_hits;
    float iou_threshold;
    int delta_t;
    std::function<Eigen::MatrixXf(const Eigen::MatrixXf &,
                                  const Eigen::MatrixXf &)>
        asso_func;
    float inertia;
    bool use_byte;
    std::vector<KalmanBoxTracker> trackers;
    int frame_count;
};

} // namespace ocsort
#endif // OC_SORT_CPP_OCSORT_H