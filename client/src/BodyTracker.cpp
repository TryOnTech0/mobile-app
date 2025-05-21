#include "BodyTracker.h"
// #include <opencv2/opencv.hpp>

BodyTracker::BodyTracker(QObject* parent) : QObject(parent) {}

bool BodyTracker::initCamera(int cameraID) { 
    // Add temporary implementation
    return true; 
}

void BodyTracker::update() {
    // Temporary implementation
}

// void BodyTracker::processFrame(const cv::Mat& frame) {
//     // Process frame logic
//     emit keypointsUpdated();
// }

std::vector<BodyKeypoint> BodyTracker::getKeypoints() const {
    return keypoints;
}
