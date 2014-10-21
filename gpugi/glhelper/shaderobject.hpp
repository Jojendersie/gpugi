#pragma once

#include <unordered_set>
#include <unordered_map>

#include "../utilities/utils.hpp"

#include "gl.hpp"
#include "shaderdatametainfo.hpp"

// Activates dumps of compile logs.
#define SHADER_COMPILE_LOGS

namespace gl
{
	/// Easy to use wrapper for OpenGL shader.
	class ShaderObject
	{
	public:
		typedef std::unordered_map<std::string, UniformVariableInfo> GlobalUniformInfos;
		typedef std::unordered_map<std::string, ShaderStorageBufferMetaInfo> ShaderStorageInfos;
		typedef std::unordered_map<std::string, UniformBufferMetaInfo> UniformBlockInfos;

		enum class ShaderType
		{
			VERTEX,
			FRAGMENT,
			EVALUATION,
			CONTROL,
			GEOMETRY,
			COMPUTE,

			NUM_SHADER_TYPES
		};

		/// Constructs ShaderObject
		/// \param shaderName   Name mainly used for debugging and identification.
		ShaderObject(const std::string& shaderName);
		~ShaderObject();

		const std::string& GetName() const { return m_name; }

		Result AddShaderFromFile(ShaderType type, const std::string& sFilename, const std::string& prefixCode = "");
		Result AddShaderFromSource(ShaderType type, const std::string& pSourceCode, const std::string& sOriginName);
		Result CreateProgram();

		/// Returns raw gl program identifier (you know what you're doing, right?)
		GLuint GetProgram() const;

		/// Makes program active
		/// You can only activate one program at a time
		void Activate() const;


		// Manipulation of global uniforms
		// Make sure that the ShaderObject is already activated
		// (Setting of ordinary global uniform variables is explicitly not supported! You can still handle this yourself using the available Meta-Info)

		/// Binds an ubo by name
		Result BindUBO(class UniformBuffer& ubo, const std::string& sUBOName);
		/// Binds an ubo by its intern buffer name
		Result BindUBO(UniformBuffer& ubo);


		/// The set of active user-defined inputs to the first shader stage in this program. 
		/// 
		/// If the first stage is a Vertex Shader, then this is the list of active attributes.
		/// If the program only contains a Compute Shader, then there are no inputs.
		GLint GetTotalProgramInputCount() const   { return m_iTotalProgramInputCount; }

		/// The set of active user-defined outputs from the final shader stage in this program.
		/// 
		/// If the final stage is a Fragment Shader, then this represents the fragment outputs that get written to individual color buffers.
		/// If the program only contains a Compute Shader, then there are no outputs.
		GLint GetTotalProgramOutputCount() const  { return m_iTotalProgramOutputCount; }



		/// Returns infos about global uniforms
		/// \remarks Deliberately not const so user can use operator[] on the map
		GlobalUniformInfos& GetGlobalUniformInfo()    { return m_GlobalUniformInfo; }

		const GlobalUniformInfos& GetGlobalUniformInfo() const { return m_GlobalUniformInfo; }

		/// Returns infos about used uniform buffer definitions
		/// \remarks Deliberately not const so user can use operator[] on the map
		UniformBlockInfos& GetUniformBufferInfo()    { return m_UniformBlockInfos; }
		const UniformBlockInfos& GetUniformBufferInfo() const { return m_UniformBlockInfos; }

		/// Returns infos about used shader storage buffer definitions
		const ShaderStorageInfos& GetShaderStorageBufferInfo() const    { return m_ShaderStorageInfos; }


		/// Global event for changed shader files.
		/// All Shader Objects will register upon this event. If any shader file is changed, just broadcast here!
		// TODO - not yet reimplemented
		// todo Automatic Shader reload on file change -> state restore needed
		//static ezEvent<const std::string&> s_shaderFileChangedEvent;

	private:
		/// Print information about the compiling step
		void PrintShaderInfoLog(ShaderId shader, const std::string& sShaderName);
		/// Print information about the linking step
		void PrintProgramInfoLog(ProgramId program);

		/// file handler event for hot reloading
		void FileEventHandler(const std::string& changedShaderFile);

		/// Reads shader source code from file and performs parsing of #include directives
		/// \param fileIndex	This will used as second parameter for each #line macro. It is a kind of file identifier.
		/// \remarks Uses a lot of potentially slow string operations.
		static std::string ReadShaderFromFile(const std::string& shaderFilename, const std::string& prefixCode = "",
										std::unordered_set<std::string>& includedFiles = std::unordered_set<std::string>(), unsigned int fileIndex = 0);



		/// queries uniform informations from the program
		void QueryProgramInformations();

		/// Intern helper function for gather general BufferInformations
		template<typename BufferVariableType>
		void QueryBlockInformations(std::unordered_map<std::string, BufferInfo<BufferVariableType>>& BufferToFill, GLenum InterfaceName);


		/// Name for identifying at runtime
		const std::string m_name;

		// the program itself
		ProgramId m_program;
		bool m_containsAssembledProgram;

		/// currently active shader object
		/// As long as no user bypasses the Activate mechanism by calling glUseProgram, this pointer will always point the currently bound program.
		static const ShaderObject* s_currentlyActiveShaderObject;

		/// list of relevant files - if any of these changes a reload will be triggered
		std::unordered_map<std::string, ShaderType> m_filesPerShaderType;

		// underlying shaders
		struct Shader
		{
			ShaderId  shaderObject;
			std::string  sOrigin;
			bool      loaded;
		};
		Shader m_aShader[(unsigned int)ShaderType::NUM_SHADER_TYPES];

		// meta information
		GlobalUniformInfos m_GlobalUniformInfo;
		UniformBlockInfos  m_UniformBlockInfos;
		ShaderStorageInfos m_ShaderStorageInfos;

		// misc
		GLint m_iTotalProgramInputCount;  ///< \see GetTotalProgramInputCount
		GLint m_iTotalProgramOutputCount; ///< \see GetTotalProgramOutputCount

		// currently missing meta information
		// - transform feedback buffer
		// - transform feedback varying
		// - subroutines
		// - atomic counter buffer
	};
}