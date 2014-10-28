#pragma once

#include <string>

/// Scene manager - loads data from a file and pushes it to the GPU.
class Scene
{
public:
    /// Load a scene.
    Scene( const std::string& _file );

    /// Unload all the scene data and GPU resources
    ~Scene();
};