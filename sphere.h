#pragma once
#include <vector>

class Sphere
{
public:
    Sphere(float radius = 1.0f, int sectorCount = 36, int stackCount = 18, int up = 3);
    ~Sphere() {}
    void build(float radius, int sectorCount, int stackCount, int up = 3);

   
    unsigned int getIndexCount() const { return (unsigned int)indices.size(); }
    unsigned int getIndexSize() const { return (unsigned int)indices.size() * sizeof(unsigned int); }
    const unsigned int* getIndices() const { return indices.data(); }

    unsigned int getInterleavedVertexSize() const { return (unsigned int)interleavedVertices.size() * sizeof(float); }
    int getInterleavedStride() const { return interleavedStride; }   
    const float* getInterleavedVertices() const { return interleavedVertices.data(); }

    void draw() const;                           
private:
    void buildVertices();
    void buildConnectedVertices();
    void changeUpAxis(int from, int to);
    void deleteArrays();
    void addVertexCoord(float x, float y, float z);
    void addNormalCoord(float x, float y, float z);
    void addTexCoord(float s, float t);
    void addIndices(unsigned int i1, unsigned int i2, unsigned int i3);

    float radius;
    int sectorCount;              
    int stackCount;   
    int upAxis;                           
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> lineIndices;
    std::vector<float> interleavedVertices;
    int interleavedStride;                  
};
