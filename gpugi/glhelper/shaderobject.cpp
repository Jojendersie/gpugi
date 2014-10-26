#include "shaderobject.hpp"
#include "uniformbuffer.hpp"

#include "../utilities/logger.hpp"
#include "../utilities/pathutils.hpp"
#include "../utilities/assert.hpp"

#include <iostream>
#include <fstream>
#include <memory>

namespace gl
{
	/// Global event for changed shader files.
	/// All Shader Objects will register upon this event. If any shader file is changed, just brodcast here!
	//ezEvent<const std::string&> ShaderObject::s_shaderFileChangedEvent;

	const ShaderObject* ShaderObject::s_currentlyActiveShaderObject = NULL;

	ShaderObject::ShaderObject(const std::string& name) :
		m_name(name),
		m_program(0),
		m_containsAssembledProgram(false)
	{
		for (Shader& shader : m_aShader)
		{
			shader.shaderObject = 0;
			shader.sOrigin = "";
			shader.loaded = false;
		}

		//  s_shaderFileChangedEvent.AddEventHandler(ezEvent<const std::string&>::Handler(&ShaderObject::FileEventHandler, this));
	}

	ShaderObject::~ShaderObject()
	{
		if (s_currentlyActiveShaderObject == this)
		{
			GL_CALL(glUseProgram, 0);
			s_currentlyActiveShaderObject = NULL;
		}

		for (Shader& shader : m_aShader)
		{
			if (shader.loaded)
				GL_CALL(glDeleteShader, shader.shaderObject);
		}

		if (m_containsAssembledProgram)
			GL_CALL(glDeleteProgram, m_program);

		//    s_shaderFileChangedEvent.RemoveEventHandler(ezEvent<const std::string&>::Handler(&ShaderObject::FileEventHandler, this));
	}

	Result ShaderObject::AddShaderFromFile(ShaderType type, const std::string& filename, const std::string& prefixCode)
	{
		// load new code
		std::unordered_set<std::string> includingFiles;
		std::string sourceCode = ReadShaderFromFile(filename, prefixCode, includingFiles);
		if (sourceCode == "")
			return FAILURE;

		Result result = AddShaderFromSource(type, sourceCode, filename);

		if (result != FAILURE)
		{
			// memorize files
			for (auto it = includingFiles.begin(); it != includingFiles.end(); ++it)
				m_filesPerShaderType.emplace(*it, type);
		}

		return result;
	}

	std::string ShaderObject::ReadShaderFromFile(const std::string& shaderFilename, const std::string& prefixCode,
												std::unordered_set<std::string>& includedFiles, unsigned int fileIndex)
	{
		// open file
		std::ifstream file(shaderFilename.c_str());
		if (file.bad() || file.fail())
		{
			LOG_ERROR("Unable to open shader file " + shaderFilename);
			return "";
		}

		// Reserve
		std::string sourceCode;
		file.seekg(0, std::ios::end);
		sourceCode.reserve(static_cast<size_t>(file.tellg()));
		file.seekg(0, std::ios::beg);

		// Read
		sourceCode.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		std::string insertionBuffer;
		size_t parseCursorPos = 0;
		size_t parseCursorOriginalFileNumber = 1;
		size_t versionPos = sourceCode.find("#version");

		// Add #line macro for proper error output (see http://stackoverflow.com/questions/18176321/is-line-0-valid-in-glsl)
		// The big problem: Officially you can only give a number as second argument, no shader filename 
		// Don't insert one if this is the main file, recognizable by a #version tag!
		if (versionPos == std::string::npos)
		{
			insertionBuffer = "#line 1 " + std::to_string(fileIndex) + "\n";
			sourceCode.insert(0, insertionBuffer);
			parseCursorPos = insertionBuffer.size();
			parseCursorOriginalFileNumber = 1;
		}

		unsigned int lastFileIndex = fileIndex;

		// Prefix code (optional)
		if (!prefixCode.empty())
		{
			if (versionPos != std::string::npos)
			{
				// Insert after version and surround by #line macro for proper error output.
				size_t nextLineIdx = sourceCode.find_first_of("\n", versionPos);
				size_t numLinesOfPrefixCode = std::count(prefixCode.begin(), prefixCode.end(), '\n') + 1;
				size_t numLinesBeforeVersion = std::count(sourceCode.begin(), sourceCode.begin() + versionPos, '\n');

				insertionBuffer = "\n#line 1 " + std::to_string(++lastFileIndex) + "\n";
				insertionBuffer += prefixCode;
				insertionBuffer += "\n#line " + std::to_string(numLinesBeforeVersion + 1) + " " + std::to_string(fileIndex) + "\n";

				sourceCode.insert(nextLineIdx, insertionBuffer);

				// parse cursor moves on
				parseCursorPos = nextLineIdx + insertionBuffer.size(); // This is the main reason why currently no #include files in prefix code is supported. Changing these has a lot of side effects in line numbering!
				parseCursorOriginalFileNumber = numLinesBeforeVersion + 1; // jumped over #version!
			}
		}

		// push into file list to prevent circular includes
		includedFiles.insert(shaderFilename);

		// parse all include tags
		size_t includePos = sourceCode.find("#include", parseCursorPos);
		std::string relativePath = PathUtils::GetDirectory(shaderFilename);
		while (includePos != std::string::npos)
		{
			parseCursorOriginalFileNumber += std::count(sourceCode.begin() + parseCursorPos, sourceCode.begin() + includePos, '\n');
			parseCursorPos = includePos;

			// parse filepath
			size_t quotMarksFirst = sourceCode.find("\"", includePos);
			if (quotMarksFirst == std::string::npos)
			{
				LOG_ERROR("Invalid #include directive in shader file " + shaderFilename + ". Expected \"");
				break;
			}
			size_t quotMarksLast = sourceCode.find("\"", quotMarksFirst + 1);
			if (quotMarksFirst == std::string::npos)
			{
				LOG_ERROR("Invalid #include directive in shader file " + shaderFilename + ". Expected \"");
				break;
			}

			size_t stringLength = quotMarksLast - quotMarksFirst - 1;
			if (stringLength == 0)
			{
				LOG_ERROR("Invalid #include directive in shader file " + shaderFilename + ". Quotation marks empty!");
				break;
			}


			std::string includeCommand = sourceCode.substr(quotMarksFirst + 1, stringLength);
			std::string includeFile = PathUtils::AppendPath(relativePath, includeCommand);

			// check if already included
			if (shaderFilename.find(includeFile) != std::string::npos)
			{
				sourceCode.replace(includePos, includePos - quotMarksLast + 1, "\n");
				// just do nothing...
			}
			else
			{
				insertionBuffer = ReadShaderFromFile(includeFile, "", includedFiles, ++lastFileIndex);
				insertionBuffer += "\n#line " + std::to_string(parseCursorOriginalFileNumber + 1) + " " + std::to_string(fileIndex); // whitespace replaces #include!
				sourceCode.replace(includePos, quotMarksLast - includePos + 1, insertionBuffer);

				parseCursorPos += insertionBuffer.size();
			}

			// find next include
			includePos = sourceCode.find("#include", parseCursorPos);
		}

		return sourceCode;
	}

	Result ShaderObject::AddShaderFromSource(ShaderType type, const std::string& sourceCode, const std::string& sOriginName)
	{
		Shader& shader = m_aShader[static_cast<std::uint32_t>(type)];

		// create shader
		GLuint shaderObjectTemp = 0;
		switch (type)
		{
		case ShaderObject::ShaderType::VERTEX:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_VERTEX_SHADER);
			break;
		case ShaderObject::ShaderType::FRAGMENT:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_FRAGMENT_SHADER);
			break;
		case ShaderObject::ShaderType::EVALUATION:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_TESS_EVALUATION_SHADER);
			break;
		case ShaderObject::ShaderType::CONTROL:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_TESS_CONTROL_SHADER);
			break;
		case ShaderObject::ShaderType::GEOMETRY:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_GEOMETRY_SHADER);
			break;
		case ShaderObject::ShaderType::COMPUTE:
			shaderObjectTemp = GL_RET_CALL(glCreateShader, GL_COMPUTE_SHADER);
			break;

		default:
			Assert(false, "Unknown shader type");
			break;
		}

		// compile shader
		const char* sourceRaw = sourceCode.c_str();
		GL_CALL(glShaderSource, shaderObjectTemp, 1, &sourceRaw, nullptr);	// attach shader code

		Result result = gl::CheckGLError("glShaderSource");
		if (result == SUCCEEDED)
		{
			glCompileShader(shaderObjectTemp);								    // compile

			result = gl::CheckGLError("glCompileShader");
		}

		// gl get error seems to be unreliable - another check!
		if (result == SUCCEEDED)
		{
			GLint shaderCompiled;
			GL_CALL(glGetShaderiv, shaderObjectTemp, GL_COMPILE_STATUS, &shaderCompiled);

			if (shaderCompiled == GL_FALSE)
				result = FAILURE;
		}

		// log output
		PrintShaderInfoLog(shaderObjectTemp, sOriginName);

		// check result
		if (result == SUCCEEDED)
		{
			// destroy old shader
			if (shader.loaded)
			{
				glDeleteShader(shader.shaderObject);
				shader.sOrigin = "";
			}

			// memorize new data only if loading successful - this way a failed reload won't affect anything
			shader.shaderObject = shaderObjectTemp;
			shader.sOrigin = sOriginName;

			// remove old associated files
			for (auto it = m_filesPerShaderType.begin(); it != m_filesPerShaderType.end(); ++it)
			{
				if (it->second == type)
				{
					it = m_filesPerShaderType.erase(it);
					if (it != m_filesPerShaderType.end())
						break;
				}
			}

			shader.loaded = true;
		}
		else
			GL_CALL(glDeleteShader, shaderObjectTemp);

		return result;
	}


	Result ShaderObject::CreateProgram()
	{
		// Create shader program
		GLuint tempProgram = GL_RET_CALL(glCreateProgram);

		// attach programs
		int numAttachedShader = 0;
		for (Shader& shader : m_aShader)
		{
			if (shader.loaded)
			{
				GL_CALL(glAttachShader, tempProgram, shader.shaderObject);
				++numAttachedShader;
			}
		}
		Assert(numAttachedShader > 0, "Need at least one shader to link a gl program!");

		// Link program
		glLinkProgram(tempProgram);
		Result result = gl::CheckGLError("glLinkProgram");

		// gl get error seems to be unreliable - another check!
		if (result == SUCCEEDED)
		{
			GLint programLinked;
			GL_CALL(glGetProgramiv, tempProgram, GL_LINK_STATUS, &programLinked);

			if (programLinked == GL_FALSE)
				result = FAILURE;
		}

		// debug output
		PrintProgramInfoLog(tempProgram);

		// check
		if (result == SUCCEEDED)
		{
			// already a program there? destroy old one!
			if (m_containsAssembledProgram)
			{
				GL_CALL(glDeleteProgram, m_program);

				// clear meta information
				m_iTotalProgramInputCount = 0;
				m_iTotalProgramOutputCount = 0;
				m_GlobalUniformInfo.clear();
				m_UniformBlockInfos.clear();
				m_ShaderStorageInfos.clear();
			}

			// memorize new data only if loading successful - this way a failed reload won't affect anything
			m_program = tempProgram;
			m_containsAssembledProgram = true;

			// get informations about the program
			QueryProgramInformations();

			return SUCCEEDED;
		}
		else
			glDeleteProgram(tempProgram);


		return result;
	}

	void ShaderObject::QueryProgramInformations()
	{
		// query basic uniform & shader storage block infos
		QueryBlockInformations(m_UniformBlockInfos, GL_UNIFORM_BLOCK);
		QueryBlockInformations(m_ShaderStorageInfos, GL_SHADER_STORAGE_BLOCK);

		// informations about uniforms ...
		GLint iTotalNumUniforms = 0;
		GL_CALL(glGetProgramInterfaceiv, m_program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &iTotalNumUniforms);
		const GLuint iNumQueriedUniformProps = 10;
		const GLenum pQueriedUniformProps[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE, GL_IS_ROW_MAJOR, GL_ATOMIC_COUNTER_BUFFER_INDEX, GL_LOCATION };
		GLint pRawUniformBlockInfoData[iNumQueriedUniformProps];
		for (int blockIndex = 0; blockIndex < iTotalNumUniforms; ++blockIndex)
		{
			// general data
			GLsizei length = 0;
			GL_CALL(glGetProgramResourceiv, m_program, GL_UNIFORM, blockIndex, iNumQueriedUniformProps, pQueriedUniformProps, iNumQueriedUniformProps, &length, pRawUniformBlockInfoData);
			UniformVariableInfo uniformInfo;
			uniformInfo.Type = static_cast<gl::ShaderVariableType>(pRawUniformBlockInfoData[1]);
			uniformInfo.iArrayElementCount = static_cast<std::int32_t>(pRawUniformBlockInfoData[2]);
			uniformInfo.iBlockOffset = static_cast<std::int32_t>(pRawUniformBlockInfoData[3]);
			uniformInfo.iArrayStride = static_cast<std::int32_t>(pRawUniformBlockInfoData[5]) * 4;
			uniformInfo.iMatrixStride = static_cast<std::int32_t>(pRawUniformBlockInfoData[6]);
			uniformInfo.bRowMajor = pRawUniformBlockInfoData[7] > 0;
			uniformInfo.iAtomicCounterbufferIndex = pRawUniformBlockInfoData[8];
			uniformInfo.iLocation = pRawUniformBlockInfoData[9];

			// name
			GLint actualNameLength = 0;
			std::string name;
			name.resize(pRawUniformBlockInfoData[0] + 1);
			GL_CALL(glGetProgramResourceName, m_program, GL_UNIFORM, static_cast<GLuint>(blockIndex), static_cast<GLsizei>(name.size()), &actualNameLength, &name[0]);
			name.resize(actualNameLength);

			// where to store (to which ubo block does this variable belong)
			if (pRawUniformBlockInfoData[4] < 0)
				m_GlobalUniformInfo.emplace(name, uniformInfo);
			else
			{
				for (auto it = m_UniformBlockInfos.begin(); it != m_UniformBlockInfos.end(); ++it)
				{
					if (it->second.iInternalBufferIndex == pRawUniformBlockInfoData[4])
					{
						it->second.Variables.emplace(name, uniformInfo);
						break;
					}
				}
			}
		}

		// informations about shader storage variables 
		GLint iTotalNumStorages = 0;
		GL_CALL(glGetProgramInterfaceiv, m_program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &iTotalNumStorages);
		const GLuint numQueriedStorageProps = 10;
		const GLenum queriedStorageProps[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE, GL_IS_ROW_MAJOR, GL_TOP_LEVEL_ARRAY_SIZE, GL_TOP_LEVEL_ARRAY_STRIDE };
		GLint pRawStorageBlockInfoData[numQueriedStorageProps];
		for (GLint blockIndex = 0; blockIndex < iTotalNumStorages; ++blockIndex)
		{
			// general data
			GL_CALL(glGetProgramResourceiv, m_program, GL_BUFFER_VARIABLE, blockIndex, numQueriedStorageProps, queriedStorageProps, numQueriedStorageProps, nullptr, pRawStorageBlockInfoData);
			BufferVariableInfo storageInfo;
			storageInfo.Type = static_cast<gl::ShaderVariableType>(pRawStorageBlockInfoData[1]);
			storageInfo.iArrayElementCount = static_cast<std::int32_t>(pRawStorageBlockInfoData[2]);
			storageInfo.iBlockOffset = static_cast<std::int32_t>(pRawStorageBlockInfoData[3]);
			storageInfo.iArrayStride = static_cast<std::int32_t>(pRawStorageBlockInfoData[5]);
			storageInfo.iMatrixStride = static_cast<std::int32_t>(pRawStorageBlockInfoData[6]);
			storageInfo.bRowMajor = pRawStorageBlockInfoData[7] > 0;
			storageInfo.iTopLevelArraySize = pRawStorageBlockInfoData[8];
			storageInfo.iTopLevelArrayStride = pRawStorageBlockInfoData[9];

			// name
			GLint iActualNameLength = 0;
			std::string name;
			name.resize(pRawUniformBlockInfoData[0] + 1);
			GL_CALL(glGetProgramResourceName, m_program, GL_BUFFER_VARIABLE, blockIndex, static_cast<GLsizei>(name.size()), &iActualNameLength, &name[0]);
			name.resize(iActualNameLength);

			// where to store (to which shader storage block does this variable belong)
			for (auto it = m_ShaderStorageInfos.begin(); it != m_ShaderStorageInfos.end(); ++it)
			{
				if (it->second.iInternalBufferIndex == pRawStorageBlockInfoData[4])
				{
					it->second.Variables.emplace(name, storageInfo);
					break;
				}
			}
		}

		// other informations
		GL_CALL(glGetProgramInterfaceiv, m_program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &m_iTotalProgramInputCount);
		GL_CALL(glGetProgramInterfaceiv, m_program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &m_iTotalProgramOutputCount);
	}

	template<typename BufferVariableType>
	void ShaderObject::QueryBlockInformations(std::unordered_map<std::string, BufferInfo<BufferVariableType>>& BufferToFill, GLenum InterfaceName)
	{
		BufferToFill.clear();

		GLint iTotalNumBlocks = 0;
		GL_CALL(glGetProgramInterfaceiv, m_program, InterfaceName, GL_ACTIVE_RESOURCES, &iTotalNumBlocks);

		// gather infos about uniform blocks
		const GLuint iNumQueriedBlockProps = 4;
		const GLenum pQueriedBlockProps[] = { GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
		GLint pRawUniformBlockInfoData[iNumQueriedBlockProps];
		for (int blockIndex = 0; blockIndex < iTotalNumBlocks; ++blockIndex)
		{
			// general data
			GLsizei length = 0;
			GL_CALL(glGetProgramResourceiv, m_program, InterfaceName, blockIndex, iNumQueriedBlockProps, pQueriedBlockProps, iNumQueriedBlockProps, &length, pRawUniformBlockInfoData);
			BufferInfo<BufferVariableType> BlockInfo;
			BlockInfo.iInternalBufferIndex = blockIndex;
			BlockInfo.iBufferBinding = pRawUniformBlockInfoData[1];
			BlockInfo.iBufferDataSizeByte = pRawUniformBlockInfoData[2];// * sizeof(float);
			//BlockInfo.Variables.Reserve(pRawUniformBlockInfoData[3]);

			// name
			GLint actualNameLength = 0;
			std::string name;
			name.resize(pRawUniformBlockInfoData[0] + 1);
			GL_CALL(glGetProgramResourceName, m_program, InterfaceName, blockIndex, static_cast<GLsizei>(name.size()), &actualNameLength, &name[0]);
			name.resize(actualNameLength);

			BufferToFill.emplace(name, BlockInfo);
		}
	}

	GLuint ShaderObject::GetProgram() const
	{
		Assert(m_containsAssembledProgram, "No shader program ready yet for ShaderObject \"" + m_name+ " \". Call CreateProgram first!");
		return m_program;
	}

	void ShaderObject::Activate() const
	{
		Assert(m_containsAssembledProgram, "No shader program ready yet for ShaderObject \"" + m_name + " \". Call CreateProgram first!");
		GL_CALL(glUseProgram, m_program);
		s_currentlyActiveShaderObject = this;
	}

	Result ShaderObject::BindUBO(UniformBuffer& ubo)
	{
		return BindUBO(ubo, ubo.GetBufferName());
	}

	Result ShaderObject::BindUBO(UniformBuffer& ubo, const std::string& sUBOName)
	{
		auto it = m_UniformBlockInfos.find(sUBOName);
		if (it != m_UniformBlockInfos.end())
			return FAILURE;

		ubo.BindBuffer(it->second.iBufferBinding);

		return SUCCEEDED;
	}

	/*
	Result ShaderObject::BindImage(Texture& texture, Texture::ImageAccess accessMode, const std::string& sImageName)
	{
	auto it = m_GlobalUniformInfo.Find(sImageName);
	if(!it.IsValid())
	return FAILURE;

	// optional typechecking
	switch(it.Value().Type)
	{
	case ShaderVariableType::UNSIGNED_INT_IMAGE_3D:
	case ShaderVariableType::INT_IMAGE_3D:
	case ShaderVariableType::IMAGE_3D:
	ASSERT(dynamic_cast<Texture3D*>(&texture) != NULL, "3D Texture expected!");
	break;
	default:
	ASSERT(false, "Handling for this type of uniform not implemented!");
	break;
	}

	ASSERT(it.Value(). ??  >= 0, "Location of shader variable %s is invalid. You need to to specify the location with the layout qualifier!", it.Key());

	texture.BindImage(it.Value(). ??, accessMode);

	return EZ_SUCCESS;
	}

	Result ShaderObject::BindTexture(Texture& texture, const std::string& sTextureName)
	{
	auto it = m_GlobalUniformInfo.Find(sTextureName);
	if(!it.IsValid())
	return FAILURE;

	// optional typechecking
	switch(it.Value().Type)
	{
	case ShaderVariableType::UNSIGNED_INT_SAMPLER_3D:
	case ShaderVariableType::INT_SAMPLER_3D:
	case ShaderVariableType::SAMPLER_3D:
	ASSERT(dynamic_cast<Texture3D*>(&texture) != NULL, "3D Texture expected!");
	break;
	default:
	ASSERT(false, "Handling for this type of uniform not implemented!");
	break;
	}

	ASSERT(it.Value(). ?? >= 0, "Location of shader variable %s is invalid. You need to to specify the location with the layout qualifier!", it.Key());

	texture.Bind(it.Value(). ??);

	return EZ_SUCCESS;
	}
	*/

	void ShaderObject::PrintShaderInfoLog(ShaderId Shader, const std::string& sShaderName)
	{
#ifdef SHADER_COMPILE_LOGS
		GLint infologLength = 0;
		GLsizei charsWritten = 0;

		GL_CALL(glGetShaderiv, Shader, GL_INFO_LOG_LENGTH, &infologLength);
		std::string infoLog;
		infoLog.resize(infologLength);
		GL_CALL(glGetShaderInfoLog, Shader, infologLength, &charsWritten, &infoLog[0]);
		infoLog.resize(charsWritten);

		if (infoLog.size() > 0)
		{
			LOG_ERROR("ShaderObject \"" + m_name + " \": Shader " + sShaderName + " compiled.Output:"); // Not necessarily an error - depends on driver.
			LOG_ERROR(infoLog);
			__debugbreak();
		}
		else
			LOG_LVL0("ShaderObject \"" + m_name + " \": Shader " + sShaderName + " compiled successfully");
#endif
	}

	// Print information about the linking step
	void ShaderObject::PrintProgramInfoLog(ProgramId Program)
	{
#ifdef SHADER_COMPILE_LOGS
		GLint infologLength = 0;
		GLsizei charsWritten = 0;

		GL_CALL(glGetProgramiv, Program, GL_INFO_LOG_LENGTH, &infologLength);
		std::string infoLog;
		infoLog.resize(infologLength);
		GL_CALL(glGetProgramInfoLog, Program, infologLength, &charsWritten, &infoLog[0]);
		infoLog.resize(charsWritten);

		if (infoLog.size() > 0)
		{
			LOG_ERROR("Program \"" + m_name + " \" linked. Output:"); // Not necessarily an error - depends on driver.
			LOG_ERROR(infoLog);
			__debugbreak();
		}
		else
			LOG_LVL0("Program \"" + m_name + " \" linked successfully");
#endif
	}

	/// file handler event for hot reloading
	void ShaderObject::FileEventHandler(const std::string& changedShaderFile)
	{
		auto it = m_filesPerShaderType.find(changedShaderFile);
		if (it != m_filesPerShaderType.end())
		{
			if (m_aShader[static_cast<std::uint32_t>(it->second)].loaded)
			{
				std::string origin(m_aShader[static_cast<std::uint32_t>(it->second)].sOrigin);  // need to copy the string, since it could be deleted in the course of reloading..
				if (AddShaderFromFile(it->second, origin) != FAILURE && m_containsAssembledProgram)
					CreateProgram();
			}
		}
	}

	/* void ShaderObject::ResetShaderBinding()
	 {
	 glUseProgram(0);
	 g_pCurrentlyActiveShaderObject == NULL;
	 } */
}
