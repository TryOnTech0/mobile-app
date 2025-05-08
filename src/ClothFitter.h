#pragma once
#include "CommonTypes.h"
#include <vector>
#include <string>

class ClothFitter {
public:
    ClothFitter();
    bool loadClothModel(const std::string &filePath);
    void updateTransformation(const std::vector<BodyKeypoint> &keypoints);
    
private:
    Mesh originalMesh;
};
