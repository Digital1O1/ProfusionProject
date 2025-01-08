#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#define THRESHOLD_WEIGHT 0.4
#define WARPEDFRAME_WEIGHT 0.6

using namespace cv;

int main()
{
    // ------------------ [ YAML STUFF ] ------------------ //
    std::string filename = "/home/pi/Onboard_VS_Streaming_RPI/GStreamer Folder/Homography_GStreamer_BW/homography_matrix.yaml";
    std::cout << "\nOpening file: " << filename << std::endl;

    FileStorage fs(filename, FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cerr << "Failed to open " << filename << std::endl;
        return -1;
    }

    Mat infraredToVisibleHomography, visibleToInfraredHomography;
    fs["irToVisibleHomography"] >> infraredToVisibleHomography;
    fs["visibleToIRHomography"] >> visibleToInfraredHomography;
    fs.release();

    if (infraredToVisibleHomography.empty() || visibleToInfraredHomography.empty())
    {
        std::cerr << "Failed to read matrices from file" << std::endl;
        return -1;
    }

    // ------------------ [ READ TIFF IMAGES ] ------------------ //

    Mat irImage = imread("/home/pi/ProfusionProject/AlignImages/images/irCamera_0-2-mod.tif", IMREAD_UNCHANGED);
    Mat visibleImage = imread("/home/pi/ProfusionProject/AlignImages/images/visibleCamera_0-1mod.tif", IMREAD_UNCHANGED);

    if (irImage.empty() || visibleImage.empty())
    {
        std::cerr << "Could not open or find the images!" << std::endl;
        return -1;
    }

    if (irImage.channels() != 1)
        cvtColor(irImage, irImage, COLOR_BGR2GRAY);
    if (visibleImage.channels() != 1)
        cvtColor(visibleImage, visibleImage, COLOR_BGR2GRAY);

    // ------------------ [ PROCESS IR FRAME ] ------------------ //


    resize(irImage, irImage, Size(640, 480));
    resize(visibleImage, visibleImage, Size(640, 480));

    Mat warpedFrame, visibleToIRProjectedFrame;
    int offsetX = 0; // Initial offset values
    int offsetY = 0;

    while (true)
    {
        // Create a translation matrix based on the offsets
        Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, offsetX, 0, 1, offsetY);

        // Apply the homography and translation
        warpPerspective(visibleImage, warpedFrame, visibleToInfraredHomography, irImage.size());
        warpAffine(warpedFrame, warpedFrame, translationMatrix, irImage.size());

        // Blend the images
        addWeighted(irImage, THRESHOLD_WEIGHT, warpedFrame, WARPEDFRAME_WEIGHT, 0, visibleToIRProjectedFrame);

        // Display the blended image
        imshow("Aligned Image", visibleToIRProjectedFrame);

        // Get user input for dynamic adjustments
        char key = waitKey(0); // Wait for key press
        if (key == 13)         // Enter key to save and exit
        {
            imwrite("FinalImage.PNG", visibleToIRProjectedFrame);
            std::cout << "Final image saved as 'FinalImage.PNG'" << std::endl;
            break;
        }
        else if (key == 'w') // Move up
            offsetY -= 5;
        else if (key == 's') // Move down
            offsetY += 5;
        else if (key == 'a') // Move left
            offsetX -= 5;
        else if (key == 'd') // Move right
            offsetX += 5;
        else if (key == 27) // Escape key to exit without saving
        {
            std::cout << "Exiting without saving." << std::endl;
            break;
        }

        // Display the current offsets
        // OffsetX: -245, OffsetY: -20
        std::cout << "OffsetX: " << offsetX << ", OffsetY: " << offsetY << std::endl;
    }

    destroyAllWindows();
    return 0;
}
