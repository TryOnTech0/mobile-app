// CommonTypes.h
#pragma once
#include <vector>  // Add vector include

struct BodyKeypoint {
    float x;
    float y;
    float confidence;
};

struct Vertex {
    float x;
    float y;
    float z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};