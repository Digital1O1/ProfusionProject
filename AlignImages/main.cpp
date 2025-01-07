#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include "yen_threshold.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <string>

#define NOIR_CAMERA 0
#define VISIBLE_CAMERA 1
#define CHESSBOARD_COLUMNS 9
#define CHESSBOARD_ROWS 6
#define LINE_THICKNESS 1
#define NUMBER_OF_CALIBRATION_IMAGES 1
#define CALIBRATION_DELAY 1000 // In milliseconds
#define THRESHOLD_WEIGHT 0.4   // Increase to see more of the Yen Threshold Image
#define WARPEDFRAME_WEIGHT 0.6
#define ESC_KEY 27
#define ESC_KEY 27
#define HORIZONTAL_RESOLUTION 640
#define VERTICAL_RESOLUTION 480

std::mutex irMutex, visibleMutex;
std::atomic<bool> captureFrames(true);

using namespace cv;

void remap_lut_threshold(cv::Mat &src, cv::Mat &dst, float k, int threshold, int &topkvalue)
{
    int histSize = 256;
    float range[] = {0, 256};
    const float *histRange = {range};
    cv::Mat hist;
    cv::calcHist(&src, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

    int high = 0;
    int low = 0;
    for (int i = 0; i < 256; i++)
    {
        if (hist.at<int>(i) != 0)
        {
            high = i;
            if (low == 0)
                low = i;
        }
    }
    // cout<<"high_low:"<<high<<"\t"<<low<<endl;
    cv::Mat histCumulative = hist.clone();
    for (int i = 1; i < histSize; i++)
    {
        histCumulative.at<float>(i) += histCumulative.at<float>(i - 1);
    }
    float abovePixels = histCumulative.at<float>(255) - histCumulative.at<float>(threshold);
    float totalPixels = src.rows * src.cols;
    topkvalue = 0;
    for (int i = 0; i < histSize; i++)
    {
        if (histCumulative.at<float>(i) >= totalPixels - k * abovePixels)
        {
            topkvalue = i;
            break;
        }
    }
    // cout<<"abovePixels\t"<<abovePixels<<"\ttopkvalue"<<topkvalue<<endl;
    cv::Mat lut(1, 256, CV_8U);
    for (int i = 0; i <= 255; i++)
    {
        if (i > topkvalue)
            lut.at<uchar>(0, i) = 255;
        else if (i > threshold)
            lut.at<uchar>(0, i) = 255.0 * (i - threshold) / (high - threshold);
        else
            lut.at<uchar>(0, i) = 0;
    }
    // cout<<lut<<endl;
    cv::LUT(src, lut, dst);
}

cv::Mat ImgProc_YenThreshold(cv::Mat src, bool compressed, double &foundThresh)
{
    // Convert frame to grayscale
    cv::Mat grey;
    cv::cvtColor(src, grey, cv::COLOR_BGR2GRAY);

    // Histogram constants
    int bins = 256;
    int histSize[] = {bins};
    const float range[] = {0, 256};
    const float *ranges[] = {range};
    int channels[] = {0};

    // equalize
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(2.7);
    cv::Mat cl;
    clahe->apply(grey, cl);

    // make histogram
    cv::Mat hist;
    cv::calcHist(&cl, 1, channels, cv::Mat(), hist, 1, histSize, ranges, true, false);

    // yen_thresholding
    int yen_threshold = Yen(hist);
    foundThresh = yen_threshold;

    // Apply binary thresholding
    cv::Mat thresholded;
    if (compressed)
    {
        cv::threshold(cl, thresholded, double(yen_threshold), 255, cv::THRESH_BINARY);
    }
    else
    {
        cv::threshold(cl, thresholded, double(yen_threshold), 255, cv::THRESH_TOZERO);
    }

    return thresholded;
}

void captureVisibleFrames(VideoCapture &cap, Mat &frame)
{
    while (captureFrames)
    {
        Mat tempFrame; // See commented section above for the logic behind using a temporary Mat object
        cap >> tempFrame;

        if (tempFrame.empty())
        {
            std::cerr << "Error: Couldn't read frame from visible camera" << std::endl;
            captureFrames = false;
            break;
        }

        // Write frame directly with lock
        std::lock_guard<std::mutex> lock(visibleMutex);
        frame = tempFrame; // Assign directly without cloning
    }
}

void captureIRFrames(VideoCapture &cap, Mat &frame)
{
    while (captureFrames)
    {
        Mat tempFrame;
        cap >> tempFrame;

        if (tempFrame.empty())
        {
            std::cerr << "Error: Couldn't read frame from IR camera" << std::endl;
            captureFrames = false;
            break;
        }

        cv::flip(tempFrame, tempFrame, 1);

        // Write frame directly with lock
        std::lock_guard<std::mutex> lock(irMutex);
        frame = tempFrame; // Assign directly without cloning
    }
}

int main()
{
    
    // Path to the YAML file
    std::string filename = "/home/pi/Onboard_VS_Streaming_RPI/GStreamer Folder/ApplyHomographyGStreamer/homography_matrix.yaml";
    std::cout << "\nOpening file: " << filename << std::endl;

    // Open the file using FileStorage
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cerr << "Failed to open " << filename << std::endl;
        return -1;
    }

    // Variables to store the matrices
    cv::Mat infraredToVisibleHomography, visibleToInfraredHomography;
    cv::Ptr<cv::Mat> ColoredFrame;
    // Read the matrices from the file
    fs["irToVisibleHomography"] >> infraredToVisibleHomography;
    fs["visibleToIRHomography"] >> visibleToInfraredHomography;

    // Close the file
    fs.release();

    // Check if matrices are empty after reading from file
    if (infraredToVisibleHomography.empty() || visibleToInfraredHomography.empty())
    {
        std::cerr << "Failed to read matrices from file" << std::endl;
        return -1;
    }

    // Output the read values as a sanity check
    // std::cout << "infraredToVisibleHomography:\n"
    //           << infraredToVisibleHomography << std::endl;
    // std::cout << "visibleToInfraredHomography:\n"
    //           << visibleToInfraredHomography << std::endl;

    cv::Mat irImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/irCamera_0-2-mod.tif", cv::IMREAD_UNCHANGED);
    cv::Mat visibleImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/visibleCamera_0-1mod.tif", cv::IMREAD_UNCHANGED);

    if (irImage.empty() || visibleImage.empty())
    {
        std::cerr << "Could not open or find the images!" << std::endl;
        return -1;
    }

    // Convert to grayscale if the images are not already grayscale
    if (irImage.channels() != 1)
    {
        cv::cvtColor(irImage, irImage, cv::COLOR_BGR2GRAY); // Convert to grayscale if not already
    }
    if (visibleImage.channels() != 1)
    {
        cv::cvtColor(visibleImage, visibleImage, cv::COLOR_BGR2GRAY); // Convert to grayscale if not already
    }

    // Just used for display purposes
    cv::resize(irImage, irImage, Size(640, 480));
    cv::resize(visibleImage, visibleImage, Size(640, 480));

    // cv::imshow("irImage", irImage);
    // cv::imshow("visibleImage", visibleImage);
    cv::Mat warpedFrame, visibleToIRProjectedFrame;


    cv::warpPerspective(visibleImage, warpedFrame, visibleToInfraredHomography, irImage.size());

    cv::addWeighted(irImage, THRESHOLD_WEIGHT, visibleImage, WARPEDFRAME_WEIGHT, 0, visibleToIRProjectedFrame);
    cv::imshow("FinalImage", visibleToIRProjectedFrame);
    cv::imwrite("FinalImage.PNG", visibleToIRProjectedFrame);

    cv::waitKey(0);

    cv::destroyAllWindows();

    return 0;
}