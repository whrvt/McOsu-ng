//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		workshop support (subscribed items, upload/download/status)
//
// $NoKeywords: $osusteamws
//===============================================================================//

#pragma once
#ifndef OSUSTEAMWORKSHOP_H
#define OSUSTEAMWORKSHOP_H

#include "EngineFeatures.h"
#include "UString.h"

#ifdef MCENGINE_FEATURE_STEAMWORKS

#include "cbase.h"

class Osu;

class OsuSteamWorkshopLoader;
class OsuSteamWorkshopUploader;

class OsuSteamWorkshop
{
public:
	enum class SUBSCRIBED_ITEM_TYPE : uint8_t
	{
		SKIN
	};

	enum class SUBSCRIBED_ITEM_STATUS : uint8_t
	{
		DOWNLOADING,
		INSTALLED
	};

	struct SUBSCRIBED_ITEM
	{
		SUBSCRIBED_ITEM_TYPE type;
		SUBSCRIBED_ITEM_STATUS status;
		uint64_t id;
		UString title;
		UString installInfo;
	};

public:
	OsuSteamWorkshop();
	~OsuSteamWorkshop();

	void refresh(bool async, bool alsoLoadDetailsWhichTakeVeryLongToLoad = true);

	bool isReady() const;
	bool areDetailsLoaded() const;
	bool isUploading() const;
	const std::vector<SUBSCRIBED_ITEM> &getSubscribedItems() const {return m_subscribedItems;}

private:
	friend class OsuSteamWorkshopLoader;
	friend class OsuSteamWorkshopUploader;

	void onUpload();
	void handleUploadError(UString errorMessage);

	OsuSteamWorkshopLoader *m_loader;
	OsuSteamWorkshopUploader *m_uploader;

	std::vector<SUBSCRIBED_ITEM> m_subscribedItems;
};

#else
class OsuSteamWorkshop
{
public:
	enum class SUBSCRIBED_ITEM_TYPE{SKIN};
	enum class SUBSCRIBED_ITEM_STATUS{DOWNLOADING,INSTALLED};
	struct SUBSCRIBED_ITEM
	{
		SUBSCRIBED_ITEM_TYPE type;
		SUBSCRIBED_ITEM_STATUS status;
		long long unsigned int id;
		UString title;
		UString installInfo;
	};

	[[maybe_unused]] constexpr OsuSteamWorkshop() = default;
	[[maybe_unused]] constexpr OsuSteamWorkshop(auto *){OsuSteamWorkshop();};
	[[maybe_unused]] constexpr ~OsuSteamWorkshop() = default;

	[[maybe_unused]] constexpr void refresh(bool,bool _ = true){}

	[[maybe_unused]] constexpr bool isReady(){return false;}
	[[maybe_unused]] constexpr bool areDetailsLoaded(){return false;}
	[[maybe_unused]] constexpr bool isUploading(){return false;}
	[[maybe_unused]] constexpr std::vector<SUBSCRIBED_ITEM> getSubscribedItems() {return std::vector<SUBSCRIBED_ITEM>{};}
};
#endif
#endif