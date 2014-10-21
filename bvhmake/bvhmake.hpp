#pragma once

#include <unordered_map>
#include <functional>
#include <assimp/importer.hpp>

typedef std::function<void()> BuildMethod;
typedef std::function<void()> FitMethod;

class BVHBuilder
{
public:
    /// \brief Registers all the different build implementations and sets the
    ///    default methods.
    BVHBuilder();

    /// \brief Get a comma separated list of all registered build types.
    std::string GetBuildMethods();

    /// \brief Get a comma separated list of all registered geometry types.
    std::string GetFitMethods();

    /// \brief Search for a named build method and set for the imports to come.
    /// \returns false if the build method is unknown
    bool SetBuildMethod( const char* _methodName );

    /// \brief Search for a named bounding geometry method and set for the
    ///    imports to come.
    /// \returns false if the type method is unknown
    bool SetGeometryType( const char* _geometryName );

    /// \brief Returns success or not.
    bool LoadSceneWithAssimp( const char* _file );

    /// \brief Write the vertex and triangle arrays to file
    void ExportGeometry( std::ofstream& _file );

private:
    BuildMethod m_buildMethod;
    FitMethod m_fitMethod;
    std::unordered_map<std::string, BuildMethod> m_buildMethods;
    std::unordered_map<std::string, FitMethod> m_fitMethods;
    Assimp::Importer m_importer;	///< Importer which is cleaning up everything automatically
    const struct aiScene* m_scene;	///< The Assimp scene from the loaded file.
};