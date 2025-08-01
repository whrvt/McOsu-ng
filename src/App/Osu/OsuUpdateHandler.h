//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#pragma once
#ifndef OSUUPDATECHECKER_H
#define OSUUPDATECHECKER_H

#include "cbase.h"

#include "Thread.h"

class OsuUpdateHandler
{
public:
	enum class STATUS : uint8_t
	{
		STATUS_UP_TO_DATE,
		STATUS_CHECKING_FOR_UPDATE,
		STATUS_DOWNLOADING_UPDATE,
		STATUS_INSTALLING_UPDATE,
		STATUS_SUCCESS_INSTALLATION,
		STATUS_ERROR
	};

public:
	static void *run(void *data);

public:
	OsuUpdateHandler();
	virtual ~OsuUpdateHandler();

	void stop(); // tells the update thread to stop at the next cancellation point
	void wait(); // blocks until the update thread is finished

	void checkForUpdates();

	[[nodiscard]] inline STATUS getStatus() const {return m_status;}
	bool isUpdateAvailable();

private:
	static constexpr auto GITHUB_API_RELEASE_URL = "https://api.github.com/repos/whrvt/" PACKAGE_NAME "/releases";
	static constexpr auto GITHUB_RELEASE_DOWNLOAD_URL = "https://github.com/whrvt/" PACKAGE_NAME "/releases";
	static constexpr auto TEMP_UPDATE_DOWNLOAD_FILEPATH = "update.zip";

	

	// async
	void _requestUpdate();
	bool _downloadUpdate(const UString& url);
	void _installUpdate(const UString& zipFilePath);


	McThread *m_updateThread;
	bool m_bThreadDone;

	bool _m_bKYS;

	// releases
	enum class STREAM : uint8_t
	{
		STREAM_NULL,
		STREAM_DESKTOP
	};

	STREAM stringToStream(const UString& streamString);
	OS stringToOS(const UString& osString);
	STREAM getReleaseStream();

	struct GITHUB_RELEASE_BUILD
	{
		OS os;
		STREAM stream;
		float version;
		UString downloadURL;
	};
	std::vector<GITHUB_RELEASE_BUILD> m_releases;

	// status
	STATUS m_status;
	int m_iNumRetries;
};

#endif
