#include "dx_stream/gst-dxmeta.hpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <limits>
#include <dirent.h>

const int FEATURE_DIM = 512; 

const char* env_path = std::getenv("FEATURES_DIR_PATH");
const std::string FEATURES_DIR_PATH = env_path ? env_path : "/workspace/dxstream/text_features/";

struct FeatureData {
    std::vector<float> feature_vector;
    std::string label;
};

std::string last_label = "";
float last_similarity = 0.0f;

static std::vector<FeatureData> preloaded_features;
static bool features_loaded = false;

bool load_features_from_directory(const std::string& dir_path) {
    DIR *dir = opendir(dir_path.c_str());
    if (dir == nullptr) {
        std::cerr << "Error: Cannot open directory: " << dir_path << std::endl;
        return false;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string filename = ent->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".bin") {
            std::string file_path = dir_path + filename;
            std::ifstream input(file_path, std::ios::binary);

            if (!input) {
                std::cerr << "Warning: Cannot open feature file: " << file_path << std::endl;
                continue;
            }

            std::vector<float> feature_vector(FEATURE_DIM);
            input.read(reinterpret_cast<char*>(feature_vector.data()), FEATURE_DIM * sizeof(float));

            if (input.gcount() != FEATURE_DIM * sizeof(float)) {
                std::cerr << "Warning: Incomplete feature vector in file: " << file_path << ". Ignored." << std::endl;
                continue;
            }

            float norm = 0.0f;
            for (float v : feature_vector) norm += v * v;
            norm = std::sqrt(norm);
            if (norm > 0.0f) {
                for (float& v : feature_vector) v /= norm;
            }

            FeatureData data;
            data.feature_vector = std::move(feature_vector);
            data.label = filename.substr(0, filename.length() - 4);
            preloaded_features.push_back(std::move(data));
        }
    }
    closedir(dir);

    std::cout << "Successfully loaded " << preloaded_features.size() << " features from " << dir_path << std::endl;
    return !preloaded_features.empty();
}

float calculate_cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) {
    float dot_product = 0.0f;
    for (size_t i = 0; i < vec1.size(); ++i) {
        dot_product += vec1[i] * vec2[i];
    }
    return dot_product;
}

extern "C" void PostProcess(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {
    if (!features_loaded) {
        features_loaded = true;
        if (!load_features_from_directory(FEATURES_DIR_PATH)) {
            std::cerr << "Feature comparison disabled due to loading failure." << std::endl;
        }
    }

    std::vector<float> feature_vector;
    float norm = 0.0f;
    int feature_length = network_output[0]._shape.size() - 1;
    float *vec = (float *)network_output[0]._data;
    for (int i = 0; i < network_output[0]._shape[feature_length]; i++) {
        float v = *(vec + i);
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm == 0.0f) {
        std::cerr << "Warning: Norm of the vector is zero. Normalization skipped." << std::endl;
        return;
    }
    for (int i = 0; i < network_output[0]._shape[feature_length]; i++) {
        feature_vector.push_back(*(vec + i) / norm);
    }

    if (!preloaded_features.empty()) {
        float best_similarity = -std::numeric_limits<float>::infinity();
        const FeatureData* best_match = nullptr;

        for (const auto& stored_feature : preloaded_features) {
            float similarity = calculate_cosine_similarity(feature_vector, stored_feature.feature_vector);
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_match = &stored_feature;
            }
        }

        if (best_match != nullptr) {
            if (best_similarity > 0.24) {
                last_label = best_match->label;
                last_similarity = best_similarity;
            }
        }
        DXObjectMeta *new_object_meta = dx_create_object_meta(buf);
        new_object_meta->_label_name = g_string_new(last_label.c_str());
        new_object_meta->_confidence = last_similarity;
        new_object_meta->_label = -1;
        new_object_meta->_box[0] = -1;
        new_object_meta->_box[1] = -1;
        new_object_meta->_box[2] = -1;
        new_object_meta->_box[3] = -1;
        dx_add_object_meta_to_frame_meta(new_object_meta, frame_meta);
    }
}