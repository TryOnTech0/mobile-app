#include "ClothScanner.h"

ClothScanner::ClothScanner(QObject* parent) : QObject(parent) {}

bool ClothScanner::captureFromCamera(int cameraID) {
    m_scanningActive = true;
    return true;
}

// void ClothScanner::processFrame(const cv::Mat& frame) {
//     capturedFrames.push_back(frame);
//     emit progressUpdated(static_cast<int>((capturedFrames.size()/10.0)*100));
// }

bool ClothScanner::isScanningActive() const {
    return m_scanningActive;
}
