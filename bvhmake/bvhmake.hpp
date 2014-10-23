#pragma once

#include <unordered_map>
#include <functional>
#include <assimp/importer.hpp>
#include <ei/matrix.hpp>
#include <memory>

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

    /// \brief Write the vertex, triangle and material arrays to file
    void ExportGeometry( std::ofstream& _file );

private:
    BuildMethod m_buildMethod;
    FitMethod m_fitMethod;
    std::unordered_map<std::string, BuildMethod> m_buildMethods;
    std::unordered_map<std::string, FitMethod> m_fitMethods;
    Assimp::Importer m_importer;	///< Importer which is cleaning up everything automatically
    const struct aiScene* m_scene;	///< The Assimp scene from the loaded file.
    std::unique_ptr<ε::Vec3[]> m_positions;
    std::unique_ptr<uint[]> m_triangles;
    uint m_triangleCursor;          ///< Point to the end of m_triangles during filling
    uint m_vertexCursor;            ///< Point to the end of m_positions during filling

    /// \brief Prepare headers for geometry export by counting elements
    /// \param [out] _numVertices Counter for the vertices must be 0 before call.
    /// \param [out] _numFaces Counter for the vertices must be 0 before call.
    void CountGeometry( const struct aiNode* _node, uint& _numVertices, uint& _numFaces );
    /// \brief Recursive export function
    void ExportVertices( std::ofstream& _file,
        const struct aiNode* _node,
        const ε::Mat4x4& _transformation );
};