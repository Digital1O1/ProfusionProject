#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include "yen_threshold.h"

#define THRESHOLD_WEIGHT 0.4
#define WARPEDFRAME_WEIGHT 0.6

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

    int high = 0, low = 0;
    for (int i = 0; i < 256; i++)
    {
        if (hist.at<int>(i) != 0)
        {
            high = i;
            if (low == 0)
                low = i;
        }
    }

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

    cv::LUT(src, lut, dst);
}

cv::Mat ImgProc_YenThreshold(cv::Mat src, bool compressed, double &foundThresh)
{
    cv::Mat grey;

    if (src.channels() != 1)
        cv::cvtColor(src, grey, cv::COLOR_BGR2GRAY);
    else
        grey = src;

    int bins = 256;
    int histSize[] = {bins};
    const float range[] = {0, 256};
    const float *ranges[] = {range};
    int channels[] = {0};

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(2.7);
    cv::Mat cl;
    clahe->apply(grey, cl);

    cv::Mat hist;
    cv::calcHist(&cl, 1, channels, cv::Mat(), hist, 1, histSize, ranges, true, false);

    int yen_threshold = Yen(hist);
    foundThresh = yen_threshold;

    cv::Mat thresholded;
    if (compressed)
        cv::threshold(cl, thresholded, double(yen_threshold), 255, cv::THRESH_BINARY);
    else
        cv::threshold(cl, thresholded, double(yen_threshold), 255, cv::THRESH_TOZERO);

    return thresholded;
}

int main()
{
    std::string filename = "/home/pi/Onboard_VS_Streaming_RPI/GStreamer Folder/Homography_GStreamer_BW/homography_matrix.yaml";
    std::cout << "\nOpening file: " << filename << std::endl;

    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cerr << "Failed to open " << filename << std::endl;
        return -1;
    }

    cv::Mat infraredToVisibleHomography, visibleToInfraredHomography;
    fs["irToVisibleHomography"] >> infraredToVisibleHomography;
    fs["visibleToIRHomography"] >> visibleToInfraredHomography;
    fs.release();

    if (infraredToVisibleHomography.empty() || visibleToInfraredHomography.empty())
    {
        std::cerr << "Failed to read matrices from file" << std::endl;
        return -1;
    }

    cv::Mat irImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/irCamera_0-2-mod.tif", cv::IMREAD_UNCHANGED);
    cv::Mat visibleImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/visibleCamera_0-1mod.tif", cv::IMREAD_UNCHANGED);

    if (irImage.empty() || visibleImage.empty())
    {
        std::cerr << "Could not open or find the images!" << std::endl;
        return -1;
    }

    if (irImage.depth() != CV_8U)
    {
        double minVal, maxVal;
        cv::minMaxLoc(irImage, &minVal, &maxVal);
        irImage.convertTo(irImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * (255.0 / (maxVal - minVal)));
    }

    if (visibleImage.depth() != CV_8U)
    {
        double minVal, maxVal;
        cv::minMaxLoc(visibleImage, &minVal, &maxVal);
        visibleImage.convertTo(visibleImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * (255.0 / (maxVal - minVal)));
    }

    double foundThresh;
    cv::Mat processedFrame;
    int remapMin = 0;

    cv::Mat yenThresholdedFrame = ImgProc_YenThreshold(irImage, false, foundThresh);
    remapMin = (int)foundThresh;
    int topkvalue = 0;
    remap_lut_threshold(yenThresholdedFrame, processedFrame, 0.1, remapMin, topkvalue);

    cv::Ptr<cv::Mat> ColoredFrame = cv::Ptr<cv::Mat>(new cv::Mat());
    cv::applyColorMap(processedFrame, *ColoredFrame, cv::COLORMAP_JET);

    if (ColoredFrame->size() != visibleImage.size())
        cv::resize(*ColoredFrame, *ColoredFrame, visibleImage.size());

    if (ColoredFrame->channels() == 4)
        cv::cvtColor(*ColoredFrame, *ColoredFrame, cv::COLOR_BGRA2BGR);

    if (visibleImage.channels() == 1)
        cv::cvtColor(visibleImage, visibleImage, cv::COLOR_GRAY2BGR);

    if (ColoredFrame->type() != visibleImage.type())
        ColoredFrame->convertTo(*ColoredFrame, visibleImage.type());

    cv::Mat blendedFrame;
    cv::addWeighted(*ColoredFrame, THRESHOLD_WEIGHT, visibleImage, WARPEDFRAME_WEIGHT, 0, blendedFrame);

    cv::imshow("Blended Frame", blendedFrame);
    cv::waitKey(0);

    return 0;
}
