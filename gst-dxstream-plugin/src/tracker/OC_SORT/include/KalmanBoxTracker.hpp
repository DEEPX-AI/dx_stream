#ifndef OC_SORT_CPP_KALMANBOXTRACKER_HPP
#define OC_SORT_CPP_KALMANBOXTRACKER_HPP
////////////// KalmanBoxTracker /////////////
#include "KalmanFilter.hpp"
#include "Utilities.hpp"
#include "iostream"
#include <memory>
/*
This class represents the internal state of individual
tracked objects observed as bbox.
*/
namespace ocsort {

class KalmanBoxTracker {
  public:
    /*method*/
    KalmanBoxTracker() = default;
    KalmanBoxTracker(Eigen::VectorXf bbox_, int cls_, int idx_,
                     uint64_t id_count_, int delta_t_ = 3);
    void update(Eigen::VectorXf *bbox_, int cls_, int idx_);
    Eigen::RowVectorXf predict();
    Eigen::VectorXf get_state() const;
    ~KalmanBoxTracker() = default;

    // Getters
    Eigen::RowVectorXf get_velocity() const { return velocity; }
    Eigen::RowVectorXf get_last_observation() const { return last_observation; }
    const std::unordered_map<int, Eigen::VectorXf>& get_observations() const { return observations; }
    int get_age() const { return age; }
    int get_time_since_update() const { return time_since_update; }
    int get_hit_streak() const { return hit_streak; }
    int get_id() const { return id; }
    int get_cls() const { return cls; }
    float get_conf() const { return conf; }
    int get_idx() const { return idx; }

  private:
    /*variable*/
    Eigen::VectorXf bbox; // [5,1]
    std::unique_ptr<KalmanFilterNew> kf = std::make_unique<KalmanFilterNew>(7, 4);
    int time_since_update = 0;
    int id;
    std::vector<Eigen::VectorXf> history;
    int hits = 0;
    int hit_streak = 0;
    int age = 0;
    float conf;
    int cls;
    int idx;
    // Upstream: np.array([-1,-1,-1,-1,-1]) (ocsort.py KalmanBoxTracker.__init__).
    // Must be -1, not 0. Two places read this as "no observation yet":
    //   1. update(): `if (last_observation.sum() >= 0)`. A zero vector passes on
    //      the first match and builds a fake velocity pointing from the image
    //      origin to the detection. Upstream leaves velocity unset instead.
    //   2. OCSort's output step: a zero vector reads as a valid observation, so a
    //      track that was never matched is emitted as a (0,0,0,0) box. Upstream
    //      falls back to get_state() there.
    Eigen::RowVectorXf last_observation = Eigen::RowVectorXf::Constant(5, -1.0f);
    // Oldest observation key not yet erased. Keeps trimming O(number erased).
    int oldest_obs_age = 0;
    std::unordered_map<int, Eigen::VectorXf> observations;
    // The reference implementation keeps a `history_observations` list alongside
    // `observations`; it is deliberately not ported. Its only consumer there is
    // Head Padding (noahcao/OC_SORT ocsort.py:416), which cannot be expressed in
    // this output contract: rows have no frame-offset column and their last field
    // indexes the CURRENT frame's detection list, while dxtracker edits buffers in
    // place and has already forwarded the earlier ones. Storing it here cost 20 MB
    // per 12-hour track at 10 fps and was never read. If Head Padding is ever
    // implemented, note that it reads only indices -2 and -3, so `min_hits`
    // entries suffice -- it does not need to be unbounded.
    Eigen::RowVectorXf velocity = Eigen::RowVectorXf::Zero(2); // [2,1]
    int delta_t;
};
} // namespace ocsort

#endif // OC_SORT_CPP_KALMANBOXTRACKER_HPP