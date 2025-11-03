#include "yolov8_pose.h"
#include "nanodet.h"

#include <chrono>
#include <iostream>
#include <vector>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define DET_PARAM  "../weights/nanodet.param"
#define DET_BIN "../weights/nanodet.bin"
#define YOLOV8_PARAM "../weights/yolov8s-pose-opt.param"
#define YOLOV8_BIN "../weights/yolov8s-pose-opt.bin"

std::unique_ptr<NanoDet> nanoDet(new NanoDet(DET_PARAM, DET_BIN, false));
std::unique_ptr<Yolov8Pose> yolov8Pose(new Yolov8Pose(YOLOV8_PARAM, YOLOV8_BIN, false));

// #define SAVE_IMG

void detectProcess(const cv::Mat& input_image, int index, const std::string& output_dir, const std::string& image_name) {
    if (input_image.empty()) {
        std::cerr << "cv::imread read image failed" << std::endl;
        return;
    }

    std::vector<ObjectDetect> objects;
    object_rect effect_roi;
    cv::Mat resized_img;
    nanoDet->resize_uniform(input_image, resized_img, cv::Size(320, 320), effect_roi);
    auto results = nanoDet->detect(resized_img, 0.3, 0.3);  // 0.4, 0.3
    nanoDet->convert_boxes(results, effect_roi, input_image.cols, input_image.rows, objects);

    std::cout << "Object size: " << objects.size() << std::endl;

    std::string output_filename = output_dir + "/" + image_name + ".txt";
    std::ofstream output_file(output_filename);

    if (!objects.empty() && yolov8Pose->isValidROI(objects)) {
        std::vector<cv::Mat> meters_image = yolov8Pose->cut_roi_img(input_image, objects);
        std::vector<float> scale_values;
        bool isGet = yolov8Pose->get_results(meters_image, scale_values);

        if (isGet)
        {
            for (const auto& scale_value:scale_values)
            {
                if (scale_value <= 0.50f)
                {
                    output_file << "scale_value: " << yolov8Pose->floatToString(scale_value + 0.012f) + " Mpa" << std::endl;
                }
                else
                {
                    output_file << "scale_value: " << yolov8Pose->floatToString(scale_value + 0.008f) + " Mpa" << std::endl;
                }
            }
        }else{
            output_file << "No objects detected." << std::endl;
        }

#ifdef SAVE_IMG
        cv::Mat save_frame = yolov8Pose->result_visualizer(input_image, objects, scale_values);
        
        std::string save_path = output_dir + "/" + image_name + "_processed.jpg";
        cv::imwrite(save_path, save_frame);
        std::cout << "Saved processed image to " << save_path << std::endl;
#else
#endif
    } else {
        output_file << "No objects detected." << std::endl;
    }
    output_file.close();
}

std::vector<cv::String> ReadImageNames(const std::string& pattern)
{
    std::vector<cv::String> fn;
    cv::glob(pattern, fn, false);
    return fn;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <mode> <path> <output_dir>" << std::endl;
        std::cerr << "Mode: single - Process a single image" << std::endl;
        std::cerr << "Folder - Process all images in a folder" << std::endl;
        return -1;
    }

    std::string mode = argv[1];
    std::string path = argv[2];
    std::string output_dir = argv[3];

    if (mode == "single") {
        cv::Mat image = cv::imread(path);

        if (image.empty()) {
            std::cerr << "Could not open or find the image at " << path << std::endl;
            return -1;
        }

        auto start = std::chrono::high_resolution_clock::now();
        
        std::string image_name = path.substr(path.find_last_of("/\") + 1);
        std::string::size_type const p(image_name.find_last_of('.'));
        std::string file_without_extension = image_name.substr(0, p);

        detectProcess(image, 0, output_dir, file_without_extension);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "Processing time: " << elapsed.count() << " ms" << std::endl;

    } else if (mode == "folder") {
        std::vector<cv::String> image_files = ReadImageNames(path + "/*.jpg");

        if (image_files.empty()) {
            std::cerr << "No images found in the folder " << path << std::endl;
            return -1;
        }

        int index = 0;
        for (const auto& file : image_files) {
            cv::Mat image = cv::imread(file);
            if(image.empty()) continue;

            auto start = std::chrono::high_resolution_clock::now();

            std::string image_name = file.substr(file.find_last_of("/\") + 1);
            std::string::size_type const p(image_name.find_last_of('.'));
            std::string file_without_extension = image_name.substr(0, p);

            detectProcess(image, index, output_dir, file_without_extension);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            std::cout << "Processing time: " << elapsed.count() << " ms" << std::endl;

            index++;
        }
    } else {
        std::cerr << "Invalid mode. Use 'single' or 'folder'." << std::endl;
        return -1;
    }

    std::cout << "Processing complete" << std::endl;
    return 0;
}

