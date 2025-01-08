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
void frameInformation(cv::Mat src, std::string name)
{
    // Extract depth and channels from the type
    int depth = src.depth();
    int channels = src.channels();

    // Convert depth to a human-readable string
    std::string depthStr;
    switch (depth)
    {
    case CV_8U:
        depthStr = "CV_8U (8-bit unsigned)";
        break;
    case CV_8S:
        depthStr = "CV_8S (8-bit signed)";
        break;
    case CV_16U:
        depthStr = "CV_16U (16-bit unsigned)";
        break;
    case CV_16S:
        depthStr = "CV_16S (16-bit signed)";
        break;
    case CV_32S:
        depthStr = "CV_32S (32-bit signed)";
        break;
    case CV_32F:
        depthStr = "CV_32F (32-bit float)";
        break;
    case CV_64F:
        depthStr = "CV_64F (64-bit float)";
        break;
    default:
        depthStr = "Unknown";
        break;
    }

    // Output the information
    std::cout << "\n\n"
              << name << " Information:" << std::endl
              << "Type (encoded): " << src.type() << std::endl
              << "Depth: " << depthStr << std::endl
              << "Number of colored channels: " << channels << std::endl
              << "Size: " << src.size() << std::endl
              << std::endl;
}

cv::Mat ImgProc_YenThreshold(cv::Mat src, bool compressed, double &foundThresh)
{
    cv::Mat grey;

    if (src.channels() != 1)
    {
        cv::cvtColor(src, grey, cv::COLOR_BGR2GRAY);
    }
    else
        grey = src;
    // Convert frame to grayscale
    // cv::imshow("src", src);
    // cv::waitKey(0);
    // cv::imshow("grey", grey);

    // Histogram constants
    int bins = 256;
    int histSize[] = {bins};
    const float range[] = {0, 256};
    const float *ranges[] = {range};
    int channels[] = {0};

    // Equalize
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(2.7);
    cv::Mat cl;
    clahe->apply(grey, cl);

    // Create histogram
    cv::Mat hist;
    cv::calcHist(&cl, 1, channels, cv::Mat(), hist, 1, histSize, ranges, true, false);

    // Yen thresholding
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
        Mat tempFrame;
        cap >> tempFrame;

        if (tempFrame.empty())
        {
            std::cerr << "Error: Couldn't read frame from visible camera" << std::endl;
            captureFrames = false;
            break;
        }

        std::lock_guard<std::mutex> lock(visibleMutex);
        frame = tempFrame;
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

        std::lock_guard<std::mutex> lock(irMutex);
        frame = tempFrame;
    }
}

int main()
{
    // ------------------ [ YAML STUFF ] ------------------ //
    std::string filename = "/home/pi/Onboard_VS_Streaming_RPI/GStreamer Folder/Homography_GStreamer_BW/homography_matrix.yaml";
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

    // ------------------ [ READ TIFF IMAGES ] ------------------ //

    cv::Mat irImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/irCamera_0-2-mod.tif", cv::IMREAD_UNCHANGED);
    cv::Mat visibleImage = cv::imread("/home/pi/ProfusionProject/AlignImages/images/visibleCamera_0-1mod.tif", cv::IMREAD_UNCHANGED);

    if (irImage.empty() || visibleImage.empty())
    {
        std::cerr << "Could not open or find the images!" << std::endl;
        return -1;
    }

    // HAVE TO NORMALIZE THE VISIBLE DATA AS WELL
    if (irImage.depth() != CV_8U)
    {
        std::cout << "Normalizing IR frame to 8-bits" << std::endl;
        double minVal, maxVal;
        cv::minMaxLoc(irImage, &minVal, &maxVal); // Get min and max pixel values
        irImage.convertTo(irImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * (255.0 / (maxVal - minVal)));

        if (irImage.empty())
        {
            std::cerr << "Error: IR Image is empty after conversion!" << std::endl;
            return -1;
        }
    }

    if (visibleImage.depth() != CV_8U)
    {
        std::cout << "Normalizing Visible frame to 8-bits" << std::endl;
        double minVal, maxVal;
        cv::minMaxLoc(visibleImage, &minVal, &maxVal); // Get min and max pixel values
        visibleImage.convertTo(visibleImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * (255.0 / (maxVal - minVal)));

        if (visibleImage.empty())
        {
            std::cerr << "Error: Visible Image is empty after conversion!" << std::endl;
            return -1;
        }
    }

    // ------------------ [ APPLY THRESHOLDING ALGORITHM TO IR FRAME ] ------------------ //
    double foundThresh;
    cv::Mat processedFrame;
    int remapMin = 0;

    cv::Mat yenThresholdedFrame = ImgProc_YenThreshold(irImage, false, foundThresh);

    remapMin = (int)foundThresh;
    int topkvalue = 0;
    remap_lut_threshold(yenThresholdedFrame, processedFrame, 0.1, remapMin, topkvalue);

    cv::Ptr<cv::Mat> ColoredFrame;
    ColoredFrame = cv::Ptr<cv::Mat>(new cv::Mat());
    cv::applyColorMap(processedFrame, *ColoredFrame, cv::COLORMAP_JET);
    // cv::imshow("ColoredFrame", *ColoredFrame);
    std::vector<cv::Mat> channels;
    cv::split(*ColoredFrame, channels);
    std::vector<cv::Mat> chansToMerge = {channels[0], channels[1], channels[2], yenThresholdedFrame};
    cv::merge(&chansToMerge[0], chansToMerge.size(), *ColoredFrame);

    //  ------------------ [ SET PHYSICAL OFFSET FOR IR FRAME ] ------------------ //

    cv::Mat warpedVisibleFrame, visibleToIRProjectedFrame;
    // Set translation values
    int offsetX = 115; // Negative value moves left | Positive values to the right
    int offsetY = 65;  // Negative values moves up | Positive values move down
    cv::Mat translationMatrix = (cv::Mat_<double>(2, 3) << 1, 0, offsetX, 0, 1, offsetY);

    cv::Mat translatedIRFrame;
    // void cv::warpAffine(cv::InputArray src, cv::OutputArray dst, cv::InputArray M, cv::Size dsize, int flags = 1, int borderMode = 0, const cv::Scalar &borderValue = cv::Scalar())

    // Move translationMatrix to align with visible frame
    cv::warpAffine(*ColoredFrame, translatedIRFrame, translationMatrix, irImage.size());

    // Apply homography to visible frame
    cv::warpPerspective(visibleImage, warpedVisibleFrame, visibleToInfraredHomography, visibleImage.size());

    // Needed since ColoredMap is 4 channels while warpedVisibleFrame before cvtColor() is grayscale (One channel) and addWeighted() needs both frames to have the same number of channels
    std::cout << "Chagning warpedVisibleFrame channels  " << std::endl;
    cv::cvtColor(warpedVisibleFrame, warpedVisibleFrame, cv::COLOR_GRAY2BGRA);

    // Create a mask where all pixels with zero intensity are marked
    cv::Mat mask;
    // void cv::inRange(cv::InputArray src, cv::InputArray lowerb, cv::InputArray upperb, cv::OutputArray dst)
    // cv::inRange(translatedIRFrame, cv::Scalar(0, 0, 0, 0), cv::Scalar(0, 0, 0, 255), mask);
    cv::inRange(translatedIRFrame, cv::Scalar(50, 0, 0, 0), cv::Scalar(255, 50, 50, 255), mask);

    // Set alpha channel to 0 for pixels matching the mask
    std::vector<cv::Mat> IRchannels;
    cv::split(translatedIRFrame, IRchannels);
    IRchannels[3].setTo(0, mask);
    cv::merge(IRchannels, translatedIRFrame);
    cv::imshow("mask", mask);

    cv::imshow("translatedIRFrame", translatedIRFrame);
    // Blend the translated IR frame with the visible frame
    // void cv::addWeighted(cv::InputArray src1, double alpha, cv::InputArray src2, double beta, double gamma, cv::OutputArray dst, int dtype = -1)
    cv::addWeighted(translatedIRFrame, THRESHOLD_WEIGHT, warpedVisibleFrame, WARPEDFRAME_WEIGHT, 0, visibleToIRProjectedFrame);

    //  ------------------ [ DISPLAY/WRITE  ] ------------------ //

    // cv::Mat displayWarpedImage;

    // cv::imshow("visibleToIRProjectedFrame", visibleToIRProjectedFrame);
    //   Save the final blended image
    //  cv::imwrite("FinalImage.PNG", visibleToIRProjectedFrame);

    // cv::destroyAllWindows();
    cv::waitKey(0);

    return 0;
}
