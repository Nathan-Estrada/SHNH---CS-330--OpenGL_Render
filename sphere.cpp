#ifdef _WIN32
#include <windows.h> 
#endif

#include <GL/gl.h>

#include <iostream>
#include <iomanip>
#include <cmath>
#include "Sphere.h"

Sphere::Sphere(float radius, int sectors, int stacks,int up) : interleavedStride(32)
{
    build(radius, sectors, stacks,up);
}

void Sphere::build(float radius, int numSectors, int numStacks,int up)
{
    
    this->radius = radius;
    this->sectorCount = numSectors;
    this->stackCount = numStacks;
    this->upAxis = up;

    buildVertices();
}


void Sphere::draw() const
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, interleavedStride, &interleavedVertices[0]);
    glNormalPointer(GL_FLOAT, interleavedStride, &interleavedVertices[3]);
    glTexCoordPointer(2, GL_FLOAT, interleavedStride, &interleavedVertices[6]);

    glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, indices.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Sphere::deleteArrays()
{
    std::vector<float>().swap(vertices);
    std::vector<float>().swap(normals);
    std::vector<float>().swap(texCoords);
    std::vector<unsigned int>().swap(indices);
    std::vector<unsigned int>().swap(lineIndices);
}

//Vertices calculations
void Sphere::buildVertices()
{
    float pi = 3.14159;

    // clear memory of prev arrays
    deleteArrays();

    float x, y, z, xy;                              //Position coordinates
    float nx, ny, nz, lengthInv = 1.0f / radius;    //Normal coordinates
    float u, v;                                     //Texture coordinates
    float sectorStep = 2 * pi / sectorCount;
    float stackStep = pi / stackCount;
    float sectorAngle;
    float stackAngle;

    for (int i = 0; i <= stackCount; i++)
    {
        stackAngle = pi / 2 - i * stackStep;
        xy = radius * cosf(stackAngle); 
        z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep; 

            //vertex coordinate calculations
            x = xy * cosf(sectorAngle); 
            y = xy * sinf(sectorAngle);
            addVertexCoord(x, y, z);

            //normal coordinate calculations
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            addNormalCoord(nx, ny, nz);

            //texture cooridnate calculations
            u = (float)j / sectorCount;
            v = (float)i / stackCount;
            addTexCoord(u, v);
        }
    }

    //Building squares with triangles
    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; j++, k1++, k2++)
        {
            if (i != 0)
            {
                addIndices(k1, k2, k1 + 1);
            }

            if (i != (stackCount - 1))
            {
                addIndices(k1 + 1, k2, k2 + 1);
            }

            lineIndices.push_back(k1);
            lineIndices.push_back(k2);
            if (i != 0)
            {
                lineIndices.push_back(k1);
                lineIndices.push_back(k1 + 1);
            }
        }
    }

    buildConnectedVertices();
}

void Sphere::buildConnectedVertices()
{
    std::vector<float>().swap(interleavedVertices);

    std::size_t i, j;
    std::size_t count = vertices.size();
    for (i = 0, j = 0; i < count; i += 3, j += 2)
    {
        interleavedVertices.push_back(vertices[i]);
        interleavedVertices.push_back(vertices[i + 1]);
        interleavedVertices.push_back(vertices[i + 2]);

        interleavedVertices.push_back(normals[i]);
        interleavedVertices.push_back(normals[i + 1]);
        interleavedVertices.push_back(normals[i + 2]);

        interleavedVertices.push_back(texCoords[j]);
        interleavedVertices.push_back(texCoords[j + 1]);
    }
}

void Sphere::changeUpAxis(int from, int to)
{
    
    float tx[] = { 1.0f, 0.0f, 0.0f }; 
    float ty[] = { 0.0f, 1.0f, 0.0f }; 
    float tz[] = { 0.0f, 0.0f, 1.0f }; 

    if (from == 1 && to == 2)
    {
        tx[0] = 0.0f; tx[1] = 1.0f;
        ty[0] = -1.0f; ty[1] = 0.0f;
    }
    else if (from == 1 && to == 3)
    {
        tx[0] = 0.0f; tx[2] = 1.0f;
        tz[0] = -1.0f; tz[2] = 0.0f;
    }
    else if (from == 2 && to == 1)
    {
        tx[0] = 0.0f; tx[1] = -1.0f;
        ty[0] = 1.0f; ty[1] = 0.0f;
    }
    else if (from == 2 && to == 3)
    {
        ty[1] = 0.0f; ty[2] = 1.0f;
        tz[1] = -1.0f; tz[2] = 0.0f;
    }
    else if (from == 3 && to == 1)
    {
        tx[0] = 0.0f; tx[2] = -1.0f;
        tz[0] = 1.0f; tz[2] = 0.0f;
    }
    else
    {
        ty[1] = 0.0f; ty[2] = -1.0f;
        tz[1] = 1.0f; tz[2] = 0.0f;
    }

    std::size_t i;
    std::size_t j;
    std::size_t count = vertices.size();
    float vx, vy, vz;
    float nx, ny, nz;
    for (i = 0, j = 0; i < count; i += 3, j += 8)
    {
        vx = vertices[i];
        vy = vertices[i + 1];
        vz = vertices[i + 2];
        vertices[i] = tx[0] * vx + ty[0] * vy + tz[0] * vz;
        vertices[i + 1] = tx[1] * vx + ty[1] * vy + tz[1] * vz;
        vertices[i + 2] = tx[2] * vx + ty[2] * vy + tz[2] * vz;

        nx = normals[i];
        ny = normals[i + 1];
        nz = normals[i + 2];
        normals[i] = tx[0] * nx + ty[0] * ny + tz[0] * nz;
        normals[i + 1] = tx[1] * nx + ty[1] * ny + tz[1] * nz;
        normals[i + 2] = tx[2] * nx + ty[2] * ny + tz[2] * nz;

        interleavedVertices[j] = vertices[i];
        interleavedVertices[j + 1] = vertices[i + 1];
        interleavedVertices[j + 2] = vertices[i + 2];
        interleavedVertices[j + 3] = normals[i];
        interleavedVertices[j + 4] = normals[i + 1];
        interleavedVertices[j + 5] = normals[i + 2];
    }
}

void Sphere::addVertexCoord(float x, float y, float z)
{
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
}

void Sphere::addNormalCoord(float nx, float ny, float nz)
{
    normals.push_back(nx);
    normals.push_back(ny);
    normals.push_back(nz);
}

void Sphere::addTexCoord(float s, float t)
{
    texCoords.push_back(s);
    texCoords.push_back(t);
}

void Sphere::addIndices(unsigned int i1, unsigned int i2, unsigned int i3)
{
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
}

