#include <iostream>
#include <assimp/DefaultLogger.hpp>
#include "bvhmake.hpp"
#include "../dependencies/glhelper/glhelper/utils/pathutils.hpp"
#include "..\gpugi\utilities\loggerinit.hpp"

// ************************************************************************* //
// Assimp logging interfacing
class LogDebugStream: public Assimp::LogStream {
	void write(const char* message) override { LOG_LVL0( message ); }
};

class LogInfoStream: public Assimp::LogStream {
	void write(const char* message) override { LOG_LVL1( message ); }
};

class LogWarnStream: public Assimp::LogStream {
	void write(const char* message) override { LOG_LVL2( message ); }
};

class LogErrStream: public Assimp::LogStream {
	void write(const char* message) override { LOG_ERROR( message ); }
};



int main( int _numArgs, const char** _args )
{
    // Logger init.
	Logger::g_logger.Initialize(new Logger::FilePolicy("log.txt"));
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
   // Assimp::DefaultLogger::get()->setLogSeverity( Assimp::Logger::LogSeverity::VERBOSE );
    Assimp::DefaultLogger::get()->attachStream( new LogDebugStream, Assimp::Logger::Debugging);
	Assimp::DefaultLogger::get()->attachStream( new LogInfoStream, Assimp::Logger::Info);
	Assimp::DefaultLogger::get()->attachStream( new LogWarnStream, Assimp::Logger::Warn);
	Assimp::DefaultLogger::get()->attachStream( new LogErrStream, Assimp::Logger::Err);

    // Builder init.
    BVHBuilder builder;

    // Show help text on invalid use
    if( _numArgs <= 1 )
    {
        std::cerr << "Expected at least one argument (the scene file to import)."
                     << std::endl << std::endl
                  << "bvhmake.exe <scene file> b=[build method] g=[bounding"\
                     "geometry type] o=[output directory]" << std::endl
                  << "  <scene file>: REQUIRED. Relative or absolute path and\n"\
                     "      file name. It is not possible to merge multiple files."
                     << std::endl
                  << "  b=[build method]: OPTIONAL. The valid arguments are:\n      "
                     << builder.GetBuildMethods() << std::endl
                     << "      The default is kdtree." << std::endl
                  << "  g=[bounding geometry type]: OPTIONAL. The valid arguments are:\n      "\
                     << builder.GetFitMethods() << std::endl
                     << "      The default is aabox." << std::endl
                  << "  o=[output directory]: OPTIONAL. A path where the scene\n"\
                     "      directory should be created. Otherwise the output is\n"\
                     "      located at the import file location." << std::endl
				  << "  t=[X]: OPTIONAL. Number of texture coordinates to export." << std::endl;
        return 1;
    }

    // Set defaults for optional arguments
    std::string outputPath = PathUtils::GetDirectory( std::string(_args[1]) );
	int numTextureCoordinates = 1;
    // Get the optional arguments
    for( int i = 2; i < _numArgs; ++i )
    {
        switch(_args[i][0])
        {
        case 'b':
            if( !builder.SetBuildMethod( _args[i] + 2 ) )
            {
                std::cerr << "Invalid build method: " << (_args[i] + 2) << std::endl;
                return 1;
            }
            break;
        case 'g':
            if( !builder.SetGeometryType( _args[i] + 2 ) )
            {
                std::cerr << "Invalid geometry type: " << (_args[i] + 2) << std::endl;
                return 1;
            }
            break;
        case 'o':
            outputPath = _args[i] + 2;
            break;
		case 't':
			numTextureCoordinates = atoi(_args[i] + 2);
			break;
        default:
            std::cerr << "Unknown optional argument!" << std::endl;
            return 1;
        }
    }
    
    // Try to create output files before spending time for Assimp
    std::string sceneName = PathUtils::GetFilename(std::string(_args[1]));
    sceneName.erase( sceneName.find_last_of( '.' ) );
	std::string materialFileName = outputPath + '/' + sceneName + ".json";
    sceneName = outputPath + '/' + sceneName + ".bim";
    std::ofstream sceneOut( sceneName, std::ofstream::binary );
    if( sceneOut.bad() )
    {
        std::cerr << "Cannot open file: " << sceneName << std::endl;
        return 2;
    } else
        std::cerr << "Opened scene file for output: " << sceneName << std::endl;

    // The material file is a json which already might contain stuff load that first.
    builder.LoadMaterials( materialFileName );

    std::cerr << "Loading with Assimp..." << std::endl;
    if( !builder.LoadSceneWithAssimp( _args[1] ) )
    {
        std::cerr << "Assimp could not load the scene: " << _args[1] << std::endl;
        return 3;
    }

	// Material before geom. because geom. deletes the assimp scene.
	std::cerr << "Exporting materials..." << std::endl;
	builder.ExportMaterials( sceneOut, materialFileName );

	// Export geometry must be first because it organizes the data for the
    // other calls
    std::cerr << "Exporting geometry..." << std::endl;
    builder.ExportGeometry( sceneOut, numTextureCoordinates );

    std::cerr << "Computing hierarchy..." << std::endl;
    builder.BuildBVH();

    std::cerr << "Exporting hierarchy..." << std::endl;
    builder.ExportBVH( sceneOut );
	builder.ExportTriangles( sceneOut );

    return 0;
}