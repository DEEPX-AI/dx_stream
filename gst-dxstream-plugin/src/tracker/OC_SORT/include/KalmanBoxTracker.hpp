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
    Eigen::RowVectorXf last_observation = Eigen::RowVectorXf::Zero(5);
    std::unordered_map<int, Eigen::VectorXf> observations;
    std::vector<Eigen::VectorXf> history_observations;
    Eigen::RowVectorXf velocity = Eigen::RowVectorXf::Zero(2); // [2,1]
    int delta_t;
};
} // namespace ocsort

#endif // OC_SORT_CPP_KALMANBOXTRACKER_HPP