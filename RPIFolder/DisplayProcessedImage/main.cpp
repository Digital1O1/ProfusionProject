#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

int main()
{
    std::string watch_dir = "/home/pi/ProfusionProject/DisplayProcessedImage/IncomingData"; // Directory to monitor
    std::string file_extension = ".png";                                                    // File extension to watch for

    std::cout << "Monitoring directory: " << watch_dir << " for PNG files...\n";

    while (true)
    {
        bool file_found = false;

        // Iterate over files in the directory
        for (const auto &entry : fs::directory_iterator(watch_dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == file_extension)
            {
                std::string file_path = entry.path().string();
                std::cout << "New file detected: " << file_path << std::endl;

                // Load the image using OpenCV
                cv::Mat image = cv::imread(file_path, cv::IMREAD_COLOR);

                // Check if the image was loaded successfully
                if (image.empty())
                {
                    std::cerr << "Error: Could not load image: " << file_path << std::endl;
                }
                else
                {
                    // Display the image
                    cv::imshow("Received Image", image);
                    std::cout << "Displaying image. Press any key to close." << std::endl;

                    // Wait for a key press
                    cv::waitKey(0);

                    // Optionally, delete the file after displaying
                    fs::remove(file_path);
                    std::cout << "File " << file_path << " deleted.\n";
                }

                file_found = true;
                break; // Stop monitoring temporarily after detecting one file
            }
        }

        // If no file is found, wait for a short period before checking again
        if (!file_found)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
