#pragma once
#include "CommonTypes.h"
#include <QObject>
#include <vector>
// #include <opencv2/core.hpp>

class BodyTracker : public QObject {
    Q_OBJECT
public:
    explicit BodyTracker(QObject* parent = nullptr);
    bool initCamera(int cameraID = 0);
    void update();
    // void processFrame(const cv::Mat& frame);
    std::vector<BodyKeypoint> getKeypoints() const;

signals:
    void keypointsUpdated();

private:
    std::vector<BodyKeypoint> keypoints;
};
