#include "../include/OCSort.hpp"
#include "../../common/include/TrackerFactory.hpp"
#include "iomanip"
#include <utility>

namespace ocsort {
template <typename Matrix>
std::ostream &operator<<(std::ostream &os, const std::vector<Matrix> &v) {
    os << "{";
    for (auto it = v.begin(); it != v.end(); ++it) {
        os << "(" << *it << ")\n";
        if (it != v.end() - 1)
            os << ",";
    }
    os << "}\n";
    return os;
}

void OCSort::init(const std::map<std::string, std::string> &params) {

    // Helper lambda function to retrieve value or use default
    auto get_param = [&](const std::string &key,
                         const std::string &default_value) {
        auto it = params.find(key);
        return (it != params.end()) ? it->second : default_value;
    };

    max_age = std::stoi(get_param("max_age", "30"));
    min_hits = std::stoi(get_param("min_hits", "3"));
    iou_threshold = std::stof(get_param("iou_threshold", "0.3"));
    trackers.clear();
    frame_count = 0;
    det_thresh = std::stof(get_param("det_thresh", "0.5"));
    delta_t = std::stoi(get_param("delta_t", "3"));

    std::unordered_map<std::string,
                       std::function<Eigen::MatrixXf(const Eigen::MatrixXf &,
                                                     const Eigen::MatrixXf &)>>
        ASSO_FUNCS{{"iou", iou_batch}, {"giou", giou_batch}};

    std::string asso_func_key = get_param("asso_func", "iou");
    std::function<Eigen::MatrixXf(const Eigen::MatrixXf &,
                                  const Eigen::MatrixXf &)>
        asso_func = ASSO_FUNCS[asso_func_key];

    inertia = std::stof(get_param("inertia", "0.2"));
    use_byte = (get_param("use_byte", "false") == "true");
    KalmanBoxTracker::count = 0;
}
std::ostream &precision(std::ostream &os) {
    os << std::fixed << std::setprecision(2);
    return os;
}
std::vector<Eigen::RowVectorXf> OCSort::update(Eigen::MatrixXf dets) {
    frame_count += 1;
    Eigen::Matrix<float, Eigen::Dynamic, 4> xyxys = dets.leftCols(4);
    Eigen::Matrix<float, 1, Eigen::Dynamic> confs = dets.col(4);
    Eigen::Matrix<float, 1, Eigen::Dynamic> clss = dets.col(5);
    Eigen::MatrixXf output_results = dets;
    auto inds_low = confs.array() > 0.1;
    auto inds_high = confs.array() < det_thresh;
    auto inds_second = inds_low && inds_high;
    Eigen::Matrix<float, Eigen::Dynamic, 7> dets_second;
    Eigen::Matrix<bool, 1, Eigen::Dynamic> remain_inds =
        (confs.array() > det_thresh);
    Eigen::Matrix<float, Eigen::Dynamic, 7> dets_first;
    for (int i = 0; i < output_results.rows(); i++) {
        if (true == inds_second(i)) {
            dets_second.conservativeResize(dets_second.rows() + 1,
                                           Eigen::NoChange);
            dets_second.row(dets_second.rows() - 1) = output_results.row(i);
        }
        if (true == remain_inds(i)) {
            dets_first.conservativeResize(dets_first.rows() + 1,
                                          Eigen::NoChange);
            dets_first.row(dets_first.rows() - 1) = output_results.row(i);
        }
    }
    Eigen::MatrixXf trks = Eigen::MatrixXf::Zero(trackers.size(), 5);
    std::vector<int> to_del;
    std::vector<Eigen::RowVectorXf> ret;
    for (int i = 0; i < trks.rows(); i++) {
        Eigen::RowVectorXf pos = trackers[i].predict();
        trks.row(i) << pos(0), pos(1), pos(2), pos(3), 0;
    }
    Eigen::MatrixXf velocities = Eigen::MatrixXf::Zero(trackers.size(), 2);
    Eigen::MatrixXf last_boxes = Eigen::MatrixXf::Zero(trackers.size(), 5);
    Eigen::MatrixXf k_observations = Eigen::MatrixXf::Zero(trackers.size(), 5);
    for (int i = 0; i < trackers.size(); i++) {
        velocities.row(i) = trackers[i].velocity;
        last_boxes.row(i) = trackers[i].last_observation;
        k_observations.row(i) =
            k_previous_obs(trackers[i].observations, trackers[i].age, delta_t);
    }

    std::vector<Eigen::Matrix<int, 1, 2>> matched;
    std::vector<int> unmatched_dets;
    std::vector<int> unmatched_trks;
    auto result = associate(dets_first, trks, iou_threshold, velocities,
                            k_observations, inertia);
    matched = std::get<0>(result);
    unmatched_dets = std::get<1>(result);
    unmatched_trks = std::get<2>(result);
    for (auto m : matched) {
        Eigen::Matrix<float, 5, 1> tmp_bbox;
        tmp_bbox = dets_first.block<1, 5>(m(0), 0);
        trackers[m(1)].update(&(tmp_bbox), dets_first(m(0), 5),
                              dets_first(m(0), 6));
    }

    if (true == use_byte && dets_second.rows() > 0 &&
        unmatched_trks.size() > 0) {
        Eigen::MatrixXf u_trks(unmatched_trks.size(), trks.cols());
        int index_for_u_trks = 0;
        for (auto i : unmatched_trks) {
            u_trks.row(index_for_u_trks++) = trks.row(i);
        }
        Eigen::MatrixXf iou_left = giou_batch(dets_second, u_trks);
        if (iou_left.maxCoeff() > iou_threshold) {
            std::vector<std::vector<float>> iou_matrix(
                iou_left.rows(), std::vector<float>(iou_left.cols()));
            for (int i = 0; i < iou_left.rows(); i++) {
                for (int j = 0; j < iou_left.cols(); j++) {
                    iou_matrix[i][j] = -iou_left(i, j);
                }
            }
            std::vector<int> rowsol, colsol;
            float MIN_cost =
                execLapjv(iou_matrix, rowsol, colsol, true, 0.01, true);
            std::vector<std::vector<int>> matched_indices;
            for (int i = 0; i < rowsol.size(); i++) {
                if (rowsol.at(i) >= 0) {
                    matched_indices.push_back(
                        {colsol.at(rowsol.at(i)), rowsol.at(i)});
                }
            }

            std::vector<int> to_remove_trk_indices;
            for (auto m : matched_indices) {
                int det_ind = m[0];
                int trk_ind = unmatched_trks[m[1]];
                if (iou_left(m[0], m[1]) < iou_threshold)
                    continue;

                Eigen::Matrix<float, 5, 1> tmp_box;
                tmp_box = dets_second.block<1, 5>(det_ind, 0);
                trackers[trk_ind].update(&tmp_box, dets_second(det_ind, 5),
                                         dets_second(det_ind, 6));
                to_remove_trk_indices.push_back(trk_ind);
            }
            std::vector<int> tmp_res1(unmatched_trks.size());
            sort(unmatched_trks.begin(), unmatched_trks.end());
            sort(to_remove_trk_indices.begin(), to_remove_trk_indices.end());
            auto end1 =
                set_difference(unmatched_trks.begin(), unmatched_trks.end(),
                               to_remove_trk_indices.begin(),
                               to_remove_trk_indices.end(), tmp_res1.begin());
            tmp_res1.resize(end1 - tmp_res1.begin());
            unmatched_trks = tmp_res1;
        }
    }

    if (unmatched_dets.size() > 0 && unmatched_trks.size() > 0) {
        Eigen::MatrixXf left_dets(unmatched_dets.size(), 7);
        int inx_for_dets = 0;
        for (auto i : unmatched_dets) {
            left_dets.row(inx_for_dets++) = dets_first.row(i);
        }
        Eigen::MatrixXf left_trks(unmatched_trks.size(), last_boxes.cols());
        int indx_for_trk = 0;
        for (auto i : unmatched_trks) {
            left_trks.row(indx_for_trk++) = last_boxes.row(i);
        }
        Eigen::MatrixXf iou_left = giou_batch(left_dets, left_trks);
        if (iou_left.maxCoeff() > iou_threshold) {
            std::vector<std::vector<float>> iou_matrix(
                iou_left.rows(), std::vector<float>(iou_left.cols()));
            for (int i = 0; i < iou_left.rows(); i++) {
                for (int j = 0; j < iou_left.cols(); j++) {
                    iou_matrix[i][j] = -iou_left(i, j);
                }
            }
            std::vector<int> rowsol, colsol;
            float MIN_cost =
                execLapjv(iou_matrix, rowsol, colsol, true, 0.01, true);
            std::vector<std::vector<int>> rematched_indices;
            for (int i = 0; i < rowsol.size(); i++) {
                if (rowsol.at(i) >= 0) {
                    rematched_indices.push_back(
                        {colsol.at(rowsol.at(i)), rowsol.at(i)});
                }
            }
            std::vector<int> to_remove_det_indices;
            std::vector<int> to_remove_trk_indices;
            for (auto i : rematched_indices) {
                int det_ind = unmatched_dets[i.at(0)];
                int trk_ind = unmatched_trks[i.at(1)];
                if (iou_left(i.at(0), i.at(1)) < iou_threshold) {
                    continue;
                }
                Eigen::Matrix<float, 5, 1> tmp_bbox;
                tmp_bbox = dets_first.block<1, 5>(det_ind, 0);
                trackers.at(trk_ind).update(&tmp_bbox, dets_first(det_ind, 5),
                                            dets_first(det_ind, 6));
                to_remove_det_indices.push_back(det_ind);
                to_remove_trk_indices.push_back(trk_ind);
            }
            std::vector<int> tmp_res(unmatched_dets.size());
            sort(unmatched_dets.begin(), unmatched_dets.end());
            sort(to_remove_det_indices.begin(), to_remove_det_indices.end());
            auto end =
                set_difference(unmatched_dets.begin(), unmatched_dets.end(),
                               to_remove_det_indices.begin(),
                               to_remove_det_indices.end(), tmp_res.begin());
            tmp_res.resize(end - tmp_res.begin());
            unmatched_dets = tmp_res;
            std::vector<int> tmp_res1(unmatched_trks.size());
            sort(unmatched_trks.begin(), unmatched_trks.end());
            sort(to_remove_trk_indices.begin(), to_remove_trk_indices.end());
            auto end1 =
                set_difference(unmatched_trks.begin(), unmatched_trks.end(),
                               to_remove_trk_indices.begin(),
                               to_remove_trk_indices.end(), tmp_res1.begin());
            tmp_res1.resize(end1 - tmp_res1.begin());
            unmatched_trks = tmp_res1;
        }
    }

    for (auto m : unmatched_trks) {
        trackers.at(m).update(nullptr, 0, -1);
    }
    for (int i : unmatched_dets) {
        Eigen::RowVectorXf tmp_bbox = dets_first.block(i, 0, 1, 5);
        int cls_ = int(dets(i, 5));
        int idx_ = int(dets(i, 6));
        KalmanBoxTracker trk = KalmanBoxTracker(tmp_bbox, cls_, idx_, delta_t);
        trackers.push_back(trk);
    }
    int tmp_i = trackers.size();
    for (int i = trackers.size() - 1; i >= 0; i--) {
        Eigen::Matrix<float, 1, 4> d;
        int last_observation_sum = trackers.at(i).last_observation.sum();
        if (last_observation_sum < 0) {
            d = trackers.at(i).get_state();
        } else {
            d = trackers.at(i).last_observation.block(0, 0, 1, 4);
        }
        if (trackers.at(i).time_since_update < 1 &&
            ((trackers.at(i).hit_streak >= min_hits) |
             (frame_count <= min_hits))) {
            Eigen::RowVectorXf tracking_res(8);
            tracking_res << d(0), d(1), d(2), d(3), trackers.at(i).id + 1,
                trackers.at(i).cls, trackers.at(i).conf, trackers.at(i).idx;
            ret.push_back(tracking_res);
        }
        if (trackers.at(i).time_since_update > max_age) {
            trackers.erase(trackers.begin() + i);
        }
    }
    return ret;
}
} // namespace ocsort