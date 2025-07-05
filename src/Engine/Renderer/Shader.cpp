//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		shader wrapper
//
// $NoKeywords: $shader
//===============================================================================//

#include "Shader.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"

#include <sstream>
namespace cv
{
ConVar debug_shaders("debug_shaders", false, FCVAR_NONE);
}

Shader::SHADER_PARSE_RESULT Shader::parseShaderFromFileOrString(const UString &graphicsInterfaceAndShaderTypePrefix,
                                                                const UString &shaderSourceOrFilePath,
                                                                bool source)
{
	SHADER_PARSE_RESULT result;
	const UString shaderPrefix =
	    "###"; // e.g. ###OpenGLLegacyInterface::VertexShader##########################################################################################
	const UString descPrefix = "##"; // e.g. ##D3D11_BUFFER_DESC::D3D11_BIND_CONSTANT_BUFFER::ModelViewProjectionConstantBuffer::mvp::float4x4

	{
		if (!source)
			debugLog("Shader: Loading {:s} {:s} ...\n", graphicsInterfaceAndShaderTypePrefix.toUtf8(), shaderSourceOrFilePath.toUtf8());

		McFile file(!source ? shaderSourceOrFilePath : "");
		if (!source && !file.canRead())
		{
			engine->showMessageError("Shader Error", UString::format("Failed to load/read file %s", shaderSourceOrFilePath.toUtf8()));
			return result;
		}
		std::istringstream ss(source ? shaderSourceOrFilePath.toUtf8() : "");

		bool foundGraphicsInterfaceAndShaderTypePrefixAtLeastOnce = false;
		bool foundGraphicsInterfaceAndShaderTypePrefix = false;
		std::string curLine;

		while (!source ? file.canRead() : static_cast<bool>(std::getline(ss, curLine)))
		{
			UString uCurLine;
			if (!source)
				uCurLine = file.readLine();
			else
				uCurLine = UString(curLine.c_str(), curLine.length());

			const bool isShaderPrefixLine = (uCurLine.find(shaderPrefix) == 0);

			if (isShaderPrefixLine)
			{
				if (!foundGraphicsInterfaceAndShaderTypePrefix)
				{
					if (uCurLine.find(graphicsInterfaceAndShaderTypePrefix) == shaderPrefix.length())
					{
						foundGraphicsInterfaceAndShaderTypePrefix = true;
						foundGraphicsInterfaceAndShaderTypePrefixAtLeastOnce = true;
					}
				}
				else
					foundGraphicsInterfaceAndShaderTypePrefix = false;
			}
			else if (foundGraphicsInterfaceAndShaderTypePrefix)
			{
				const bool isDescPrefixLine = (uCurLine.find(descPrefix) == 0);

				if (!isDescPrefixLine)
				{
					if (!result.source.isEmpty())
						result.source.append('\n');
					result.source.append(uCurLine);
				}
				else
				{
					result.descs.push_back(uCurLine.substr(descPrefix.length()));
				}
			}
		}

		if (!foundGraphicsInterfaceAndShaderTypePrefixAtLeastOnce)
			engine->showMessageError("Shader Error",
			                         UString::format("Missing \"%s\" in file %s", graphicsInterfaceAndShaderTypePrefix.toUtf8(), shaderSourceOrFilePath.toUtf8()));
	}

	return result;
}
