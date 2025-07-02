#include "../include/OCSort.hpp"
#include "../../common/include/TrackerFactory.hpp"
#include "iomanip"
#include <numeric>
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
    id_count = 0;
}
std::ostream &precision(std::ostream &os) {
    os << std::fixed << std::setprecision(2);
    return os;
}

std::vector<Eigen::RowVectorXf> OCSort::update(Eigen::MatrixXf dets) {
    this->frame_count += 1;

    Eigen::MatrixXf high_conf_dets, low_conf_dets;
    SplitDetections(dets, high_conf_dets, low_conf_dets);

    Eigen::MatrixXf predicted_bboxes, velocities, last_observation_bboxes,
        k_obs_data;
    if (!this->trackers.empty()) {
        PrepareTrackDataForAssociation(predicted_bboxes, velocities,
                                       last_observation_bboxes, k_obs_data);
    } else {
        predicted_bboxes.resize(0, 5);
        velocities.resize(0, 2);
        last_observation_bboxes.resize(0, 5);
        k_obs_data.resize(0, 5);
    }

    std::vector<Eigen::Matrix<int, 1, 2>> matched_pairs_pass1;
    std::vector<int> unmatched_dets_pass1;
    std::vector<int> unmatched_trks_pass1;

    if (high_conf_dets.rows() > 0 && !this->trackers.empty()) {
        PerformFirstPassAssociation(high_conf_dets, predicted_bboxes,
                                    velocities, k_obs_data, matched_pairs_pass1,
                                    unmatched_dets_pass1, unmatched_trks_pass1);
        UpdateTrackersFromMatches(matched_pairs_pass1, high_conf_dets);
    } else {
        if (high_conf_dets.rows() > 0) {
            unmatched_dets_pass1.resize(high_conf_dets.rows());
            std::iota(unmatched_dets_pass1.begin(), unmatched_dets_pass1.end(),
                      0);
        }
        if (!this->trackers.empty()) {
            unmatched_trks_pass1.resize(this->trackers.size());
            std::iota(unmatched_trks_pass1.begin(), unmatched_trks_pass1.end(),
                      0);
        }
    }

    if (this->use_byte && low_conf_dets.rows() > 0 &&
        !unmatched_trks_pass1.empty()) {
        PerformByteAssociation(low_conf_dets, predicted_bboxes,
                               unmatched_trks_pass1);
    }

    if (!unmatched_dets_pass1.empty() && !unmatched_trks_pass1.empty()) {
        PerformIOUReAssociation(high_conf_dets, last_observation_bboxes,
                                unmatched_dets_pass1, unmatched_trks_pass1);
    }

    ManageUnmatchedAndCreateNewTrackers(
        dets, high_conf_dets, unmatched_dets_pass1, unmatched_trks_pass1);

    return GenerateOutputAndCleanup();
}

void OCSort::SplitDetections(const Eigen::MatrixXf &input_dets_raw,
                             Eigen::MatrixXf &high_conf_dets,
                             Eigen::MatrixXf &low_conf_dets) {
    Eigen::VectorXf confs = input_dets_raw.col(4);
    long num_dets = input_dets_raw.rows();
    std::vector<int> high_conf_indices, low_conf_indices;

    for (long i = 0; i < num_dets; ++i) {
        if (confs(i) > this->det_thresh) {
            high_conf_indices.push_back(i);
        } else if (confs(i) > 0.1 && confs(i) < this->det_thresh) {
            low_conf_indices.push_back(i);
        }
    }

    high_conf_dets.resize(high_conf_indices.size(), input_dets_raw.cols());
    for (size_t i = 0; i < high_conf_indices.size(); ++i) {
        high_conf_dets.row(i) = input_dets_raw.row(high_conf_indices[i]);
    }

    low_conf_dets.resize(low_conf_indices.size(), input_dets_raw.cols());
    for (size_t i = 0; i < low_conf_indices.size(); ++i) {
        low_conf_dets.row(i) = input_dets_raw.row(low_conf_indices[i]);
    }
}

void OCSort::PrepareTrackDataForAssociation(
    Eigen::MatrixXf &out_predicted_bbox_states, Eigen::MatrixXf &out_velocities,
    Eigen::MatrixXf &out_last_observed_bboxes,
    Eigen::MatrixXf &out_k_previous_observations_matrix) {
    size_t num_trackers = this->trackers.size();
    out_predicted_bbox_states.resize(num_trackers, 5);
    out_velocities.resize(num_trackers, 2);
    out_last_observed_bboxes.resize(num_trackers, 5);
    out_k_previous_observations_matrix.resize(num_trackers, 5);

    for (size_t i = 0; i < num_trackers; ++i) {
        Eigen::RowVectorXf pos = this->trackers[i].predict();
        out_predicted_bbox_states.row(i) << pos(0), pos(1), pos(2), pos(3), 0;
        out_velocities.row(i) = this->trackers[i].velocity;
        out_last_observed_bboxes.row(i) = this->trackers[i].last_observation;
        out_k_previous_observations_matrix.row(i) =
            k_previous_obs(this->trackers[i].observations,
                           this->trackers[i].age, this->delta_t);
    }
}

void OCSort::PerformFirstPassAssociation(
    const Eigen::MatrixXf &high_conf_dets,
    const Eigen::MatrixXf &predicted_track_bboxes,
    const Eigen::MatrixXf &velocities,
    const Eigen::MatrixXf &k_observations_data,
    std::vector<Eigen::Matrix<int, 1, 2>> &out_matched_pairs,
    std::vector<int> &out_unmatched_det_indices,
    std::vector<int> &out_unmatched_trk_indices) {
    auto association_result =
        associate(high_conf_dets, predicted_track_bboxes, this->iou_threshold,
                  velocities, k_observations_data, this->inertia);
    out_matched_pairs = std::get<0>(association_result);
    out_unmatched_det_indices = std::get<1>(association_result);
    out_unmatched_trk_indices = std::get<2>(association_result);
}

void OCSort::UpdateTrackersFromMatches(
    const std::vector<Eigen::Matrix<int, 1, 2>> &matched_pairs,
    const Eigen::MatrixXf &source_dets_matrix,
    const std::vector<int> *det_indices_map) {
    for (const auto &match : matched_pairs) {
        int det_idx_in_source = match(0, 0);
        int trk_idx_global = match(0, 1);

        int original_det_idx = det_idx_in_source;
        if (det_indices_map) {
            original_det_idx = (*det_indices_map)[det_idx_in_source];
        }
        Eigen::VectorXf bbox_for_update =
            source_dets_matrix.block<1, 5>(original_det_idx, 0).transpose();

        float cls = source_dets_matrix(original_det_idx, 5);
        float det_specific_idx = source_dets_matrix(original_det_idx, 6);

        this->trackers[trk_idx_global].update(&bbox_for_update, cls,
                                              det_specific_idx);
    }
}

int execLapjv(const std::vector<std::vector<float>> &cost,
              std::vector<int> &rowsol, std::vector<int> &colsol,
              bool extend_cost, float cost_limit, bool use_cost_limit) {
    if (!cost.empty() && !cost[0].empty()) {
        rowsol.assign(cost.size(), -1);
        colsol.assign(cost[0].size(), -1);
    }
    return 0;
}

std::vector<Eigen::Matrix<int, 1, 2>>
OCSort::SolveHungarianAssignment(const Eigen::MatrixXf &iou_matrix,
                                 float lapjv_cost_threshold) {
    std::vector<std::vector<float>> cost_matrix_for_lapjv(
        iou_matrix.rows(), std::vector<float>(iou_matrix.cols()));
    for (int i = 0; i < iou_matrix.rows(); ++i) {
        for (int j = 0; j < iou_matrix.cols(); ++j) {
            cost_matrix_for_lapjv[i][j] = -iou_matrix(i, j);
        }
    }
    if (cost_matrix_for_lapjv.empty() ||
        (!cost_matrix_for_lapjv.empty() && cost_matrix_for_lapjv[0].empty())) {
        return {};
    }

    std::vector<int> rowsol_lapjv, colsol_lapjv;
    execLapjv(cost_matrix_for_lapjv, rowsol_lapjv, colsol_lapjv, true,
              lapjv_cost_threshold, true);

    std::vector<Eigen::Matrix<int, 1, 2>> matched_pairs;

    if (!rowsol_lapjv.empty()) {
        for (size_t r = 0; r < rowsol_lapjv.size(); ++r) {
            if (rowsol_lapjv[r] >= 0 &&
                iou_matrix(r, rowsol_lapjv[r]) >= this->iou_threshold) {
                Eigen::Matrix<int, 1, 2> pair;
                pair << static_cast<int>(r), rowsol_lapjv[r];
                matched_pairs.push_back(pair);
            }
        }
    }

    return matched_pairs;
}

Eigen::MatrixXf OCSort::BuildSubMatrix(const Eigen::MatrixXf &source_matrix,
                                       const std::vector<int> &row_indices) {
    if (row_indices.empty()) {
        return Eigen::MatrixXf(0, source_matrix.cols());
    }
    Eigen::MatrixXf sub_matrix(row_indices.size(), source_matrix.cols());
    for (size_t i = 0; i < row_indices.size(); ++i) {
        sub_matrix.row(i) = source_matrix.row(row_indices[i]);
    }
    return sub_matrix;
}

void OCSort::PerformByteAssociation(
    const Eigen::MatrixXf &low_conf_dets,
    const Eigen::MatrixXf &all_predicted_track_bboxes,
    std::vector<int> &unmatched_trk_indices) {
    if (low_conf_dets.rows() == 0 || unmatched_trk_indices.empty())
        return;

    Eigen::MatrixXf u_trks_predictions =
        BuildSubMatrix(all_predicted_track_bboxes, unmatched_trk_indices);
    if (u_trks_predictions.rows() == 0)
        return;

    Eigen::MatrixXf iou_values =
        giou_batch(low_conf_dets.leftCols(4), u_trks_predictions.leftCols(4));
    if (iou_values.rows() == 0 || iou_values.cols() == 0 ||
        iou_values.maxCoeff() <= this->iou_threshold)
        return;
    std::vector<Eigen::Matrix<int, 1, 2>> matched_pairs_byte =
        SolveHungarianAssignment(iou_values, 0.01f);

    std::vector<int> tracks_matched_in_this_step_local_indices;
    for (const auto &match : matched_pairs_byte) {
        int det_idx_local = match(0, 0);
        int trk_idx_local = match(0, 1);

        int global_trk_idx = unmatched_trk_indices[trk_idx_local];
        Eigen::VectorXf bbox_for_update =
            low_conf_dets.block<1, 5>(det_idx_local, 0).transpose();
        float cls = low_conf_dets(det_idx_local, 5);
        float det_specific_idx = low_conf_dets(det_idx_local, 6);
        this->trackers[global_trk_idx].update(&bbox_for_update, cls,
                                              det_specific_idx);

        tracks_matched_in_this_step_local_indices.push_back(trk_idx_local);
    }

    if (!tracks_matched_in_this_step_local_indices.empty()) {
        std::sort(tracks_matched_in_this_step_local_indices.rbegin(),
                  tracks_matched_in_this_step_local_indices.rend());
        for (int local_idx_to_remove :
             tracks_matched_in_this_step_local_indices) {
            unmatched_trk_indices.erase(unmatched_trk_indices.begin() +
                                        local_idx_to_remove);
        }
    }
}

void OCSort::PerformIOUReAssociation(
    const Eigen::MatrixXf &high_conf_dets,
    const Eigen::MatrixXf &all_last_observed_bboxes,
    std::vector<int> &unmatched_det_indices,
    std::vector<int> &unmatched_trk_indices) {
    if (unmatched_det_indices.empty() || unmatched_trk_indices.empty())
        return;

    Eigen::MatrixXf current_unmatched_dets_subset =
        BuildSubMatrix(high_conf_dets, unmatched_det_indices);
    Eigen::MatrixXf current_unmatched_trks_last_boxes_subset =
        BuildSubMatrix(all_last_observed_bboxes, unmatched_trk_indices);

    if (current_unmatched_dets_subset.rows() == 0 ||
        current_unmatched_trks_last_boxes_subset.rows() == 0)
        return;

    Eigen::MatrixXf iou_values =
        giou_batch(current_unmatched_dets_subset.leftCols(4),
                   current_unmatched_trks_last_boxes_subset.leftCols(4));
    if (iou_values.rows() == 0 || iou_values.cols() == 0 ||
        iou_values.maxCoeff() <= this->iou_threshold)
        return;
    std::vector<Eigen::Matrix<int, 1, 2>> matched_pairs =
        SolveHungarianAssignment(iou_values, 0.01f);

    std::vector<int> dets_rematched_local_indices;
    std::vector<int> trks_rematched_local_indices;

    for (const auto &match : matched_pairs) {
        int det_idx_local_subset = match(0, 0);
        int trk_idx_local_subset = match(0, 1);

        int original_high_conf_det_idx =
            unmatched_det_indices[det_idx_local_subset];
        int global_trk_idx = unmatched_trk_indices[trk_idx_local_subset];

        Eigen::VectorXf bbox_for_update =
            high_conf_dets.block<1, 5>(original_high_conf_det_idx, 0)
                .transpose();
        float cls = high_conf_dets(original_high_conf_det_idx, 5);
        float det_specific_idx = high_conf_dets(original_high_conf_det_idx, 6);
        this->trackers[global_trk_idx].update(&bbox_for_update, cls,
                                              det_specific_idx);

        dets_rematched_local_indices.push_back(det_idx_local_subset);
        trks_rematched_local_indices.push_back(trk_idx_local_subset);
    }
    if (!dets_rematched_local_indices.empty()) {
        std::sort(dets_rematched_local_indices.rbegin(),
                  dets_rematched_local_indices.rend());
        for (int local_idx : dets_rematched_local_indices)
            unmatched_det_indices.erase(unmatched_det_indices.begin() +
                                        local_idx);
    }
    if (!trks_rematched_local_indices.empty()) {
        std::sort(trks_rematched_local_indices.rbegin(),
                  trks_rematched_local_indices.rend());
        for (int local_idx : trks_rematched_local_indices)
            unmatched_trk_indices.erase(unmatched_trk_indices.begin() +
                                        local_idx);
    }
}

void OCSort::ManageUnmatchedAndCreateNewTrackers(
    const Eigen::MatrixXf &original_input_detections,
    const Eigen::MatrixXf &high_conf_dets,
    const std::vector<int> &final_unmatched_det_indices,
    const std::vector<int> &final_unmatched_trk_indices) {
    for (int trk_idx : final_unmatched_trk_indices) {
        this->trackers[trk_idx].update(nullptr, 0, -1);
    }

    for (int det_idx_in_high_conf : final_unmatched_det_indices) {
        this->id_count++;
        Eigen::RowVectorXf new_trk_bbox_data =
            high_conf_dets.block<1, 5>(det_idx_in_high_conf, 0);
        int cls_ = static_cast<int>(high_conf_dets(det_idx_in_high_conf, 5));
        int original_frame_idx_ =
            static_cast<int>(high_conf_dets(det_idx_in_high_conf, 6));

        KalmanBoxTracker new_trk =
            KalmanBoxTracker(new_trk_bbox_data, cls_, original_frame_idx_,
                             this->id_count, this->delta_t);
        this->trackers.push_back(new_trk);
    }
}

std::vector<Eigen::RowVectorXf> OCSort::GenerateOutputAndCleanup() {
    std::vector<Eigen::RowVectorXf> ret_results;
    std::vector<KalmanBoxTracker> next_trackers_list;

    for (size_t i = 0; i < this->trackers.size(); ++i) {
        if (this->trackers[i].time_since_update > this->max_age) {
            continue;
        }

        if (this->trackers[i].time_since_update < 1 &&
            ((this->trackers[i].hit_streak >= this->min_hits) ||
             (this->frame_count <= this->min_hits))) {

            Eigen::Matrix<float, 1, 4> d_bbox_coords;
            bool has_valid_last_obs =
                (this->trackers[i].last_observation.size() >= 4 &&
                 this->trackers[i].last_observation(0) >= 0.0f);

            if (!has_valid_last_obs) {
                d_bbox_coords = this->trackers[i].get_state();
            } else {
                d_bbox_coords = this->trackers[i].last_observation.head<4>();
            }

            Eigen::RowVectorXf tracking_res(8);
            tracking_res << d_bbox_coords(0), d_bbox_coords(1),
                d_bbox_coords(2), d_bbox_coords(3),
                static_cast<float>(this->trackers[i].id + 1),
                static_cast<float>(this->trackers[i].cls),
                this->trackers[i].conf,
                static_cast<float>(this->trackers[i].idx);
            ret_results.push_back(tracking_res);
        }
        next_trackers_list.push_back(this->trackers[i]);
    }
    this->trackers = next_trackers_list;
    return ret_results;
}
} // namespace ocsort