#include "../include/KalmanFilter.hpp"
#include <iostream>
namespace ocsort {
KalmanFilterNew::KalmanFilterNew() : I(Eigen::MatrixXf::Identity(7, 7)) {};
KalmanFilterNew::KalmanFilterNew(int dim_x_, int dim_z_) : I(Eigen::MatrixXf::Identity(dim_x_, dim_x_)) {
    x = Eigen::VectorXf::Zero(dim_x_, 1);
    P = Eigen::MatrixXf::Identity(dim_x_, dim_x_);
    Q = Eigen::MatrixXf::Identity(dim_x_, dim_x_);
    B = Eigen::MatrixXf::Identity(dim_x_, dim_x_);
    F = Eigen::MatrixXf::Identity(dim_x_, dim_x_);
    H = Eigen::MatrixXf::Zero(dim_z_, dim_x_);
    R = Eigen::MatrixXf::Identity(dim_z_, dim_z_);
    M = Eigen::MatrixXf::Zero(dim_x_, dim_z_);
    z = Eigen::VectorXf::Zero(dim_z_, 1);
    K = Eigen::MatrixXf::Zero(dim_x_, dim_z_);
    y = Eigen::VectorXf::Zero(dim_x_, 1);
    S = Eigen::MatrixXf::Zero(dim_z_, dim_z_);
    SI = Eigen::MatrixXf::Zero(dim_z_, dim_z_);

    x_prior = x;
    P_prior = P;
    x_post = x;
    P_post = P;
};
void KalmanFilterNew::predict() {
    x = F * x;
    P = _alpha_sq * (F * P * F.transpose()) + Q;
    x_prior = x;
    P_prior = P;
}
// Minimum number of history entries always kept. unfreeze() only needs the last
// two non-null observations, and one fewer remains right after it runs, so 2
// would do; this leaves room.
static constexpr std::size_t kHistoryKeep = 8;

void KalmanFilterNew::update(const Eigen::VectorXf &z_) {
    history_obs.push_back(z_);
    ++total_pushes;

    // Only trim while no freeze is in effect.
    //
    // Between freeze and unfreeze the entries are exactly what unfreeze() will
    // read, so they stay. That window is bounded: OCSort drops a tracker once
    // `time_since_update > max_age`, so a gap cannot exceed max_age. The vector
    // is therefore at most `kHistoryKeep + max_age + 2` entries — a constant,
    // independent of how long the track lives.
    if (!attr_saved.IsInitialized && history_obs.size() > kHistoryKeep) {
        history_obs.erase(history_obs.begin(),
                          history_obs.end() - static_cast<std::ptrdiff_t>(kHistoryKeep));
    }
    if (z_.size() == 0) {
        if (true == observed)
            freeze();
        observed = false;
        z = Eigen::VectorXf::Zero(H.rows(), 1);
        x_post = x;
        P_post = P;
        y = Eigen::VectorXf::Zero(H.rows(), 1);
        return;
    }
    if (false == observed)
        unfreeze();
    observed = true;
    y = z_ - H * x;
    auto PHT = P * H.transpose();
    S = H * PHT + R;
    K = PHT * SI;
    x = x + K * y;
    auto I_KH = I - K * H;
    P = ((I_KH * P) * I_KH.transpose()) + ((K * R) * K.transpose());
    z = z_;
    x_post = x;
    P_post = P;
}
void KalmanFilterNew::freeze() {
    attr_saved.IsInitialized = true;
    attr_saved.x = x;
    attr_saved.P = P;
    attr_saved.Q = Q;
    attr_saved.B = B;
    attr_saved.F = F;
    attr_saved.H = H;
    attr_saved.R = R;
    attr_saved._alpha_sq = _alpha_sq;
    attr_saved.M = M;
    attr_saved.z = z;
    attr_saved.K = K;
    attr_saved.y = y;
    attr_saved.S = S;
    attr_saved.SI = SI;
    attr_saved.x_prior = x_prior;
    attr_saved.P_prior = P_prior;
    attr_saved.x_post = x_post;
    attr_saved.P_post = P_post;
    attr_saved.pushes_at_freeze = total_pushes;
}
void KalmanFilterNew::unfreeze() {
    if (!attr_saved.IsInitialized) {
        return;
    }

    // Building the virtual trajectory needs only the two most recent real
    // observations and the number of frames between them. Upstream reads them
    // through a list alias (new_history = self.history_obs,
    // kalmanfilter.py:421) that disappears when the call returns — no copy. The
    // port made it a full deep copy into a member, so every track carried a
    // second history for its whole life. Reading in place before the rewind
    // gives the same values with no copy.
    int lastNotNullIndex = -1;
    int secondLastNotNullIndex = -1;
    Eigen::VectorXf box1;
    Eigen::VectorXf box2;

    for (int i = static_cast<int>(history_obs.size()) - 1; i >= 0; --i) {
        if (history_obs[i].size() == 0) {
            continue;
        }
        if (lastNotNullIndex == -1) {
            lastNotNullIndex = i;
            box2 = history_obs[i];
        } else if (secondLastNotNullIndex == -1) {
            secondLastNotNullIndex = i;
            box1 = history_obs[i];
            break;
        }
    }

    x = attr_saved.x;
    P = attr_saved.P;
    Q = attr_saved.Q;
    B = attr_saved.B;
    F = attr_saved.F;
    H = attr_saved.H;
    R = attr_saved.R;
    _alpha_sq = attr_saved._alpha_sq;
    M = attr_saved.M;
    z = attr_saved.z;
    K = attr_saved.K;
    y = attr_saved.y;
    S = attr_saved.S;
    SI = attr_saved.SI;
    x_prior = attr_saved.x_prior;
    P_prior = attr_saved.P_prior;
    x_post = attr_saved.x_post;

    // Rewind the history to the freeze-time snapshot minus its last entry
    // (upstream :424 and :426). The non-observations collected during the gap are
    // dropped here, so the history does not grow with every gap. The snapshot is
    // a prefix (see the pushes_at_freeze comment in the header), so the rewind
    // copies nothing and stores nothing. The port instead removed one entry from
    // the current history and kept the snapshot forever, leaving a track holding
    // three full histories: current, new_history, and snapshot.
    // pushes_at_freeze = 0 matches upstream's attr_saved = None (:425).
    //
    // The rewind is computed as a relative count: drop everything appended since
    // the freeze (the gap's non-observations plus the observation just added),
    // then one more for upstream's `[:-1]`. An absolute length would point
    // somewhere else once the front has been trimmed.
    const std::size_t appended = total_pushes - attr_saved.pushes_at_freeze;
    if (history_obs.size() > appended + 1) {
        history_obs.resize(history_obs.size() - appended - 1);
    } else {
        history_obs.clear();
    }
    attr_saved.pushes_at_freeze = 0;
    attr_saved.IsInitialized = false;

    if (lastNotNullIndex == -1 || secondLastNotNullIndex == -1) {
        return;
    }

    int time_gap = lastNotNullIndex - secondLastNotNullIndex;
    if (time_gap <= 0) {
        return;
    }

    double x1 = box1[0];
    double y1 = box1[1];
    double x2 = box2[0];
    double y2 = box2[1];

    double w1 = std::sqrt(box1[2] * box1[3]);
    double h1 = std::sqrt(box1[2] / box1[3]);
    double w2 = std::sqrt(box2[2] * box2[3]);
    double h2 = std::sqrt(box2[2] / box2[3]);

    double dx = (x2 - x1) / time_gap;
    double dy = (y2 - y1) / time_gap;
    double dw = (w2 - w1) / time_gap;
    double dh = (h2 - h1) / time_gap;

    for (int i = 0; i < time_gap; ++i) {
        double x_interp = x1 + (i + 1) * dx;
        double y_interp = y1 + (i + 1) * dy;
        double w_interp = w1 + (i + 1) * dw;
        double h_interp = h1 + (i + 1) * dh;

        double s = w_interp * h_interp;
        double r = w_interp / h_interp;

        Eigen::VectorXf new_box(4);
        new_box << static_cast<float>(x_interp), static_cast<float>(y_interp), static_cast<float>(s), static_cast<float>(r);

        y = new_box - H * x;
        Eigen::MatrixXf PHT = P * H.transpose();
        S = H * PHT + R;
        SI = S.inverse();
        K = PHT * SI;
        x = x + K * y;

        Eigen::MatrixXf I_KH = I - K * H;
        P = (I_KH * P) * I_KH.transpose() + (K * R) * K.transpose();

        z = new_box;
        x_post = x;
        P_post = P;

        if (i != time_gap - 1) {
            predict();
        }
    }
}

} // namespace ocsort