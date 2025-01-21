#pragma once

#include <eigen3/Eigen/Dense>
#include <map>
#include <vector>

class Tracker {
  public:
    virtual void init(const std::map<std::string, std::string> &params) = 0;
    virtual std::vector<Eigen::RowVectorXf>
    update(const Eigen::MatrixXf dets) = 0;
    virtual ~Tracker() {}
};