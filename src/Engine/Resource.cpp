//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#include "Resource.h"
#include "Engine.h"
#include "Environment.h"
#include "File.h"

Resource::Resource(UString filepath)
{
	McFile checkFile(filepath, McFile::TYPE::CHECK);
	if (checkFile.exists()) // does a case-insensitive check on non-windows platforms, different from env->fileExists
	{
		m_sFilePath = checkFile.getPath();
	}
	else
	{
		m_sFilePath = filepath;
		UString errorMessage = "File does not exist: ";
		errorMessage.append(filepath);
		debugLog("Resource Warning: File %s does not exist!\n", filepath.toUtf8());
	}

	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
}

Resource::Resource()
{
	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
}

void Resource::load()
{
	init();
}

void Resource::loadAsync()
{
	initAsync();
}

void Resource::reload()
{
	release();
	loadAsync(); // TODO: this should also be reloaded asynchronously if it was initially loaded so, maybe
	load();
}

void Resource::release()
{
	destroy();

	// NOTE: these are set afterwards on purpose
	m_bReady = false;
	m_bAsyncReady = false;
}

void Resource::interruptLoad()
{
	m_bInterrupted = true;
}
