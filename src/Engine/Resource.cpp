//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#include "Resource.h"
#include "Engine.h"
#include "Environment.h"

Resource::Resource(UString filepath)
{
	m_sFilePath = filepath;
	if (!env->fileExists(m_sFilePath)) // modifies the input string if found
	{
		UString errorMessage = "File does not exist: ";
		errorMessage.append(m_sFilePath);
		debugLog("Resource Warning: File {:s} does not exist!\n", m_sFilePath.toUtf8());
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
