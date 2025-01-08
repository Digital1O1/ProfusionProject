#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Create a gradient from 0 to 255
    cv::Mat grayscale(1, 256, CV_8UC1);
    for (int i = 0; i < 256; ++i) {
        grayscale.at<uchar>(0, i) = i;
    }

    // Apply the JET colormap
    cv::Mat jetColormap;
    cv::applyColorMap(grayscale, jetColormap, cv::COLORMAP_JET);

    // Extract blue pixel range
    for (int i = 0; i < 256; ++i) {
        cv::Vec3b color = jetColormap.at<cv::Vec3b>(0, i);
        if (color[0] > 0 && color[1] == 0 && color[2] == 0) {
            std::cout << "Blue starts at intensity: " << i << std::endl;
        }
        if (color[0] > 0 && color[1] > 0) {
            std::cout << "Blue transitions to cyan at intensity: " << i << std::endl;
            break;
        }
    }

    cv::imshow("Colormap", jetColormap);
    cv::waitKey(0);
    return 0;
}
