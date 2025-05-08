#pragma once
#include <QObject>
// #include <opencv2/core.hpp>
#include <vector>  

class ClothScanner : public QObject {
    Q_OBJECT
public:
    ClothScanner(QObject* parent = nullptr);
    bool captureFromCamera(int cameraID);
    // void processFrame(const cv::Mat &frame);
    bool isScanningActive() const;

signals:
    void progressUpdated(int percent);

private:
    bool m_scanningActive = false;
    // std::vector<cv::Mat> capturedFrames;
};
