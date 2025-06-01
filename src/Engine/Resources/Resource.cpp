//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#include "Resource.h"

#include <utility>
#include "Engine.h"
#include "Environment.h"

Resource::Resource(UString filepath)
{
	m_sFilePath = std::move(filepath);
	m_bFileFound = true;
	if (!env->fileExists(m_sFilePath)) // modifies the input string if found
	{
		debugLog("Resource Warning: File {:s} does not exist!\n", m_sFilePath.toUtf8());
		m_bFileFound = false;
	}

	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
	// give it a dummy name for unnamed resources, mainly for debugging purposes
	m_sName = UString::fmt("{:p}:postinit=n:found={}:{:s}", static_cast<const void*>(this), m_bFileFound, m_sFilePath);
}

Resource::Resource()
{
	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
	m_bFileFound = true;
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
	loadAsync();
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
