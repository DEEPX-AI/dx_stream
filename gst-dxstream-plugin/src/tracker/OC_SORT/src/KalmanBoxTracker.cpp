#include "../include/KalmanBoxTracker.hpp"
#include <utility>
namespace ocsort {
KalmanBoxTracker::KalmanBoxTracker(Eigen::VectorXf bbox_, int cls_, int idx_,
                                   uint64_t id_count_, int delta_t_) 
    : bbox(std::move(bbox_)),
      id(static_cast<int>(id_count_)),
      conf(bbox(4)),
      cls(cls_),
      idx(idx_),
      delta_t(delta_t_) {
    Eigen::Matrix<float, 7, 7> F_temp;
    F_temp << 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 1;
    kf->set_F(F_temp);

    Eigen::Matrix<float, 4, 7> H_temp;
    H_temp << 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 0;
    kf->set_H(H_temp);
    Eigen::Matrix<float, 4, 4> R_temp = kf->get_R();
    R_temp.block(2, 2, 2, 2) *= 10.0;
    kf->set_R(R_temp);

    Eigen::MatrixXf P_temp = kf->get_P();
    P_temp.block(4, 4, 3, 3) *= 1000.0;
    P_temp *= 10.0;
    kf->set_P(P_temp);

    Eigen::MatrixXf Q_temp = kf->get_Q();
    Q_temp.bottomRightCorner(1, 1)(0, 0) *= 0.01f;
    Q_temp.block(4, 4, 3, 3) *= 0.01f;
    kf->set_Q(Q_temp);

    Eigen::VectorXf x_temp = kf->get_x();
    x_temp.head<4>() = convert_bbox_to_z(bbox);
    kf->set_x(x_temp);
}

void KalmanBoxTracker::update(Eigen::VectorXf *bbox_, int cls_, int idx_) {
    if (bbox_ == nullptr) {
        Eigen::VectorXf tmp;
        kf->update(tmp);
        return;
    }

    conf = (*bbox_)[4];
    cls = cls_;
    idx = idx_;

    if (int(last_observation.sum()) >= 0) {
        Eigen::VectorXf previous_box_tmp;

        for (int i = 0; i < delta_t; ++i) {
            int dt = delta_t - i;
            if (observations.count(age - dt) > 0) {
                previous_box_tmp = observations[age - dt];
                break;
            }
        }

        if (previous_box_tmp.size() == 0) {
            previous_box_tmp = last_observation;
        }

        velocity = speed_direction(previous_box_tmp, *bbox_);
    }

    last_observation = *bbox_;
    observations[age] = *bbox_;
    history_observations.push_back(*bbox_);
    time_since_update = 0;
    history.clear();
    hits += 1;
    hit_streak += 1;

    Eigen::VectorXf tmp = convert_bbox_to_z(*bbox_);
    kf->update(tmp);
}

Eigen::RowVectorXf KalmanBoxTracker::predict() {
    Eigen::VectorXf x_temp = kf->get_x();
    if (x_temp(6) + x_temp(2) <= 0)
        x_temp(6) *= 0.0f;
    kf->set_x(x_temp);
    kf->predict();
    age += 1;
    if (time_since_update > 0)
        hit_streak = 0;
    time_since_update += 1;
    history.push_back(convert_x_to_bbox(kf->get_x()));
    return convert_x_to_bbox(kf->get_x());
}
Eigen::VectorXf KalmanBoxTracker::get_state() const {
    return convert_x_to_bbox(kf->get_x());
}
} // namespace ocsort