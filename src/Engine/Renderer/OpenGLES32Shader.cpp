//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGLES32 GLSL implementation of Shader
//
// $NoKeywords: $gles32shader
//===============================================================================//

#include "OpenGLES32Shader.h"

#ifdef MCENGINE_FEATURE_GLES32

#include "Engine.h"
#include "ConVar.h"

#include "OpenGLHeaders.h"
#include "OpenGLES32Interface.h"
#include "OpenGLStateCache.h"

OpenGLES32Shader::OpenGLES32Shader(UString shader, bool source)
{
	SHADER_PARSE_RESULT parsedVertexShader = parseShaderFromFileOrString("OpenGLES32Interface::VertexShader", shader, source);
	SHADER_PARSE_RESULT parsedFragmentShader = parseShaderFromFileOrString("OpenGLES32Interface::FragmentShader", shader, source);

	m_sVsh = parsedVertexShader.source;
	m_sFsh = parsedFragmentShader.source;
	m_bSource = true;

	m_iProgram = 0;
	m_iVertexShader = 0;
	m_iFragmentShader = 0;

	m_iProgramBackup = 0;
}

OpenGLES32Shader::OpenGLES32Shader(UString vertexShader, UString fragmentShader, bool source) : Shader()
{
	m_sVsh = vertexShader;
	m_sFsh = fragmentShader;
	m_bSource = source;

	m_iProgram = 0;
	m_iVertexShader = 0;
	m_iFragmentShader = 0;

	m_iProgramBackup = 0;
}

void OpenGLES32Shader::init()
{
	m_bReady = compile(m_sVsh, m_sFsh, m_bSource);
}

void OpenGLES32Shader::initAsync()
{
	m_bAsyncReady = true;
}

void OpenGLES32Shader::destroy()
{
	auto *gles32 = static_cast<OpenGLES32Interface *>(graphics);
	if (gles32 != NULL)
		gles32->unregisterShader(this);

	if (m_iProgram != 0)
		glDeleteProgram(m_iProgram);
	if (m_iFragmentShader != 0)
		glDeleteShader(m_iFragmentShader);
	if (m_iVertexShader != 0)
		glDeleteShader(m_iVertexShader);

	m_iProgram = 0;
	m_iFragmentShader = 0;
	m_iVertexShader = 0;

	m_iProgramBackup = 0;
	m_uniformLocationCache.clear();
}

void OpenGLES32Shader::enable()
{
	if (!m_bReady) return;

	// use the state cache instead of querying gl directly
	m_iProgramBackup = OpenGLStateCache::getInstance().getCurrentProgram();
	glUseProgram(m_iProgram);

	// update cache
	OpenGLStateCache::getInstance().setCurrentProgram(m_iProgram);
}

void OpenGLES32Shader::disable()
{
	if (!m_bReady) return;

	glUseProgram(m_iProgramBackup); // restore

	// update cache
	OpenGLStateCache::getInstance().setCurrentProgram(m_iProgramBackup);
}

int OpenGLES32Shader::getAndCacheUniformLocation(const UString &name)
{
	if (!m_bReady) return -1;

	m_sTempStringBuffer.reserve(name.lengthUtf8());
	m_sTempStringBuffer.assign(name.toUtf8(), name.lengthUtf8());

	const auto cachedValue = m_uniformLocationCache.find(m_sTempStringBuffer);
	const bool cached = (cachedValue != m_uniformLocationCache.end());

	const int id = (cached ? cachedValue->second : glGetUniformLocation(m_iProgram, name.toUtf8()));
	if (!cached && id != -1)
		m_uniformLocationCache[m_sTempStringBuffer] = id;

	return id;
}

void OpenGLES32Shader::setUniform1f(UString name, float value)
{
	if (!m_bReady) return;

	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform1f(id, value);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n", name.toUtf8());
}

void OpenGLES32Shader::setUniform1fv(UString name, int count, float *values)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform1fv(id, count, values);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform1i(UString name, int value)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform1i(id, value);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform2f(UString name, float value1, float value2)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform2f(id, value1, value2);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform2fv(UString name, int count, float *vectors)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform2fv(id, count, (float*)&vectors[0]);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform3f(UString name, float x, float y, float z)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform3f(id, x, y, z);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform3fv(UString name, int count, float *vectors)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform3fv(id, count, (float*)&vectors[0]);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniform4f(UString name, float x, float y, float z, float w)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniform4f(id, x, y, z, w);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniformMatrix4fv(UString name, Matrix4 &matrix)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniformMatrix4fv(id, 1, GL_FALSE, matrix.get());
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

void OpenGLES32Shader::setUniformMatrix4fv(UString name, float *v)
{
	if (!m_bReady) return;
	const int id = getAndCacheUniformLocation(name);
	if (id != -1)
		glUniformMatrix4fv(id, 1, GL_FALSE, v);
	else if (debug_shaders->getBool())
		debugLog("OpenGLES32Shader Warning: Can't find uniform {:s}\n",name.toUtf8());
}

int OpenGLES32Shader::getAttribLocation(UString name)
{
	if (!m_bReady) return -1;
	return glGetAttribLocation(m_iProgram, name.toUtf8());
}

bool OpenGLES32Shader::isActive()
{
	int currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	return (m_bReady && currentProgram == m_iProgram);
}

bool OpenGLES32Shader::compile(UString vertexShader, UString fragmentShader, bool source)
{
	// load & compile shaders
	debugLog("Compiling {:s} ...\n", (source ? "vertex source" : vertexShader.toUtf8()));
	m_iVertexShader = source ? createShaderFromString(vertexShader, GL_VERTEX_SHADER) : createShaderFromFile(vertexShader, GL_VERTEX_SHADER);
	debugLog("Compiling {:s} ...\n", (source ? "fragment source" : fragmentShader.toUtf8()));
	m_iFragmentShader = source ? createShaderFromString(fragmentShader, GL_FRAGMENT_SHADER) : createShaderFromFile(fragmentShader, GL_FRAGMENT_SHADER);

	if (m_iVertexShader == 0 || m_iFragmentShader == 0)
	{
		engine->showMessageError("OpenGLES32Shader Error", "Couldn't createShader()");
		return false;
	}

	// create program
	m_iProgram = (int)glCreateProgram();
	if (m_iProgram == 0)
	{
		engine->showMessageError("OpenGLES32Shader Error", "Couldn't glCreateProgram()");
		return false;
	}

	// attach
	glAttachShader(m_iProgram, m_iVertexShader);
	glAttachShader(m_iProgram, m_iFragmentShader);

	// link
	glLinkProgram(m_iProgram);

	GLint ret = GL_FALSE;
	glGetProgramiv(m_iProgram, GL_LINK_STATUS, &ret);
	if (ret == GL_FALSE)
	{
		engine->showMessageError("OpenGLES32Shader Error", "Couldn't glLinkProgram()");
		return false;
	}

	// validate
	ret = GL_FALSE;
	glValidateProgram(m_iProgram);
	glGetProgramiv(m_iProgram, GL_VALIDATE_STATUS, &ret);
	if (ret == GL_FALSE)
	{
		engine->showMessageError("OpenGLES32Shader Error", "Couldn't glValidateProgram()");
		return false;
	}

	return true;
}

int OpenGLES32Shader::createShaderFromString(UString shaderSource, int shaderType)
{
	const GLint shader = glCreateShader(shaderType);

	if (shader == 0)
	{
		engine->showMessageError("OpenGLES32Shader Error", "Couldn't glCreateShader()");
		return 0;
	}

	// compile shader
	const char *shaderSourceChar = shaderSource.toUtf8();
	glShaderSource(shader, 1, &shaderSourceChar, NULL);
	glCompileShader(shader);

	GLint ret = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
	if (ret == GL_FALSE)
	{
		debugLog("------------------OpenGLES32Shader Compile Error------------------\n");

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 0)
		{
			char *errorLog = new char[ret];
			{
				glGetShaderInfoLog(shader, ret, &ret, errorLog);
				debugLog(fmt::runtime(errorLog));
			}
			delete[] errorLog;
		}

		debugLog("-----------------------------------------------------------------\n");

		engine->showMessageError("OpenGLES32Shader Error", "Couldn't glShaderSource() or glCompileShader()");
		return 0;
	}

	return shader;
}

int OpenGLES32Shader::createShaderFromFile(UString fileName, int shaderType)
{
	// load file
	std::ifstream inFile(fileName.toUtf8());
	if (!inFile)
	{
		engine->showMessageError("OpenGLES32Shader Error", fileName);
		return 0;
	}
	std::string line;
	std::string shaderSource;
	while (inFile.good())
	{
		std::getline(inFile, line);
		shaderSource += line + "\n\0";
	}
	shaderSource += "\n\0";
	inFile.close();

	UString shaderSourcePtr = UString(shaderSource.c_str());

	return createShaderFromString(shaderSourcePtr, shaderType);
}

#endif
