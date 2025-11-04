#include "httplib.h"
#include "nlohmann/json.hpp"
#include "base64.h"
#include "nanodet.h"
#include "yolov8_pose.h"

#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

// Use nlohmann::json for convenience
using json = nlohmann::json;

// Global detector instances
std::unique_ptr<NanoDet> nanoDet;
std::unique_ptr<Yolov8Pose> yolov8Pose;

void setup_detectors(const std::string& root_path) {
    std::string det_param = root_path + "/weights/nanodet.param";
    std::string det_bin = root_path + "/weights/nanodet.bin";
    std::string yolov8_param = root_path + "/weights/yolov8s-pose-opt.param";
    std::string yolov8_bin = root_path + "/weights/yolov8s-pose-opt.bin";

    nanoDet = std::make_unique<NanoDet>(det_param.c_str(), det_bin.c_str(), false);
    yolov8Pose = std::make_unique<Yolov8Pose>(yolov8_param.c_str(), yolov8_bin.c_str(), false);
    
    std::cout << "Detectors loaded successfully." << std::endl;
}

int main(int argc, char* argv[]) {
    // Determine the path to the executable to locate the weights directory
    std::string executable_path = argv[0];
    std::string root_path = executable_path.substr(0, executable_path.find_last_of("/"));

    // A small hack for development environment if the executable is in a subdirectory like 'build'
    if (root_path.find("build") != std::string::npos || root_path.find("bin") != std::string::npos) {
        root_path = root_path.substr(0, root_path.find_last_of("/"));
    }

    try {
        setup_detectors(root_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load models: " << e.what() << std::endl;
        return -1;
    }

    httplib::Server svr;

    svr.Post("/v1/det/single", [](const httplib::Request& req, httplib::Response& res) {
        json response_json;
        try {
            // 1. Parse request
            auto body = json::parse(req.body);
            if (!body.contains("data")) {
                res.status = 400;
                response_json["error"] = "Missing 'data' field in request body.";
                res.set_content(response_json.dump(), "application/json");
                return;
            }
            std::string b64_image_string = body["data"];

            // 2. Base64 Decode
            std::string decoded_image_string = base64_decode(b64_image_string);
            std::vector<uchar> image_data(decoded_image_string.begin(), decoded_image_string.end());
            cv::Mat image = cv::imdecode(image_data, cv::IMREAD_COLOR);

            if (image.empty()) {
                res.status = 400;
                response_json["error"] = "Failed to decode image. Invalid image data.";
                res.set_content(response_json.dump(), "application/json");
                return;
            }

            // 3. Run Detection (logic from main.cpp)
            std::vector<ObjectDetect> objects;
            object_rect effect_roi;
            cv::Mat resized_img;
            nanoDet->resize_uniform(image, resized_img, cv::Size(320, 320), effect_roi);
            auto results = nanoDet->detect(resized_img, 0.3, 0.3);
            nanoDet->convert_boxes(results, effect_roi, image.cols, image.rows, objects);

            json result_array = json::array();
            cv::Mat vis_image = image.clone();

            if (!objects.empty() && yolov8Pose->isValidROI(objects)) {
                std::vector<cv::Mat> meters_image = yolov8Pose->cut_roi_img(image, objects);
                std::vector<float> scale_values;
                bool isGet = yolov8Pose->get_results(meters_image, scale_values);

                if (isGet) {
                    // Apply correction and build JSON
                    for (size_t i = 0; i < objects.size(); ++i) {
                        json meter_result;
                        float final_scale = scale_values[i];
                        if (final_scale <= 0.50f) {
                            final_scale += 0.012f;
                        } else {
                            final_scale += 0.008f;
                        }
                        
                        meter_result["type"] = "meter";
                        meter_result["box"] = {objects[i].rect.x, objects[i].rect.y, objects[i].rect.width, objects[i].rect.height};
                        // Note: The original logic doesn't seem to expose angle directly, but scale_value is the key output.
                        // If angle is needed, it would require modification in yolov8_pose.cpp
                        meter_result["angle"] = -1; // Placeholder for angle
                        meter_result["scale_value"] = final_scale;
                        result_array.push_back(meter_result);
                    }
                    vis_image = yolov8Pose->result_visualizer(image, objects, scale_values);
                }
            }

            // 4. Encode output image and create final JSON
            std::vector<uchar> buf;
            cv::imencode(".jpg", vis_image, buf);
            auto* enc_msg = reinterpret_cast<unsigned char*>(buf.data());
            std::string encoded_output_image = base64_encode(enc_msg, buf.size());

            response_json["results"] = result_array;
            response_json["output_image"] = encoded_output_image;

            res.status = 200;
            res.set_content(response_json.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            response_json["error"] = "Internal Server Error: " + std::string(e.what());
            res.set_content(response_json.dump(), "application/json");
        }
    });

    std::cout << "Server starting on http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
