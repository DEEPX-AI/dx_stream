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
  private:
    uint64_t id_count;
    void SplitDetections(const Eigen::MatrixXf &input_dets_raw,
                         Eigen::MatrixXf &high_conf_dets,
                         Eigen::MatrixXf &low_conf_dets);

    void PrepareTrackDataForAssociation(
        Eigen::MatrixXf &out_predicted_bbox_states,
        Eigen::MatrixXf &out_velocities,
        Eigen::MatrixXf &out_last_observed_bboxes,
        Eigen::MatrixXf &out_k_previous_observations_matrix);

    void PerformFirstPassAssociation(
        const Eigen::MatrixXf &high_conf_dets,
        const Eigen::MatrixXf &predicted_track_bboxes,
        const Eigen::MatrixXf &velocities,
        const Eigen::MatrixXf &k_observations_data,
        std::vector<Eigen::Matrix<int, 1, 2>> &out_matched_pairs,
        std::vector<int> &out_unmatched_det_indices,
        std::vector<int> &out_unmatched_trk_indices);

    void UpdateTrackersFromMatches(
        const std::vector<Eigen::Matrix<int, 1, 2>> &matched_pairs,
        const Eigen::MatrixXf &source_dets_matrix,
        const std::vector<int> *det_indices_map = nullptr);

    std::vector<Eigen::Matrix<int, 1, 2>>
    SolveHungarianAssignment(const Eigen::MatrixXf &iou_matrix,
                             float cost_threshold_for_lapjv);

    Eigen::MatrixXf BuildSubMatrix(const Eigen::MatrixXf &source_matrix,
                                   const std::vector<int> &row_indices);

    void
    PerformByteAssociation(const Eigen::MatrixXf &low_conf_dets,
                           const Eigen::MatrixXf &all_predicted_track_bboxes,
                           std::vector<int> &unmatched_trk_indices);

    void
    PerformIOUReAssociation(const Eigen::MatrixXf &high_conf_dets,
                            const Eigen::MatrixXf &all_last_observed_bboxes,
                            std::vector<int> &unmatched_det_indices,
                            std::vector<int> &unmatched_trk_indices);

    void ManageUnmatchedAndCreateNewTrackers(
        const Eigen::MatrixXf &original_input_detections,
        const Eigen::MatrixXf &high_conf_dets,
        const std::vector<int> &final_unmatched_det_indices,
        const std::vector<int> &final_unmatched_trk_indices);

    std::vector<Eigen::RowVectorXf> GenerateOutputAndCleanup();

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