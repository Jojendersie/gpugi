#include "scene.hpp"
#include "../glhelper/gl.hpp"

Scene::Scene( const std::string& _file )
{
    GLint x;
    glGetIntegerv( GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &x );
    glGetIntegerv( GL_MAX_TEXTURE_BUFFER_SIZE, &x );
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &x );
    return;
}

Scene::~Scene()
{
}