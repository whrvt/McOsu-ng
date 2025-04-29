//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		steamworks api wrapper
//
// $NoKeywords: $steam
//===============================================================================//

#pragma once
#ifndef STEAMWORKSINTERFACE_H
#define STEAMWORKSINTERFACE_H

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_STEAMWORKS

#include "cbase.h"

class SteamworksInterface
{
public:
	struct WorkshopItemDetails
	{
		uint64_t publishedFileId;

		UString title;
		UString description;
	};

public:
	SteamworksInterface();
	~SteamworksInterface();

	void update();

	uint64_t createWorkshopItem();

	bool pushWorkshopItemUpdate(uint64_t publishedFileId);
	bool setWorkshopItemTitle(UString title);
	bool setWorkshopItemDescription(UString description);
	bool setWorkshopItemLanguage(UString language);
	bool setWorkshopItemMetadata(UString metadata);
	bool setWorkshopItemVisibility(bool visible, bool friendsOnly = false);
	bool setWorkshopItemTags(std::vector<UString> tags);
	bool setWorkshopItemContent(UString contentFolderAbsolutePath);
	bool setWorkshopItemPreview(UString previewFileAbsolutePath);
	bool popWorkshopItemUpdate(UString changeNote);

	std::vector<uint64_t> getWorkshopSubscribedItems();
	bool isWorkshopSubscribedItemInstalled(uint64_t publishedFileId);
	bool isWorkshopSubscribedItemDownloading(uint64_t publishedFileId);
	UString getWorkshopItemInstallInfo(uint64_t publishedFileId);
	std::vector<WorkshopItemDetails> getWorkshopItemDetails(const std::vector<uint64_t> &publishedFileIds);
	void forceWorkshopItemUpdateDownload(uint64_t publishedFileId);

	void startWorkshopItemPlaytimeTracking(uint64_t publishedFileId);
	void stopWorkshopItemPlaytimeTracking(uint64_t publishedFileId);
	void stopWorkshopPlaytimeTrackingForAllItems();

	void openURLInGameOverlay(UString url);
	void openWorkshopItemURLInGameOverlay(uint64_t publishedFileId);

	void setRichPresence(UString key, UString value);

	inline bool isReady() const {return m_bReady.load();}
	inline const UString getLastError() const {return m_sLastError;}
	bool isGameOverlayEnabled();
	UString getUsername();

private:
	void handleWorkshopLegalAgreementNotAccepted(uint64_t publishedFileId);
	void handleLastError(int res);

	std::atomic<bool> m_bReady;

	uint64_t m_pendingItemUpdateHandle;
	UString m_sLastError;
};

extern SteamworksInterface *steam;

#else
class SteamworksInterface
{
public:
	[[maybe_unused]] constexpr SteamworksInterface() = default;
	[[maybe_unused]] constexpr ~SteamworksInterface() = default;

	[[maybe_unused]] constexpr void update(){}

	[[maybe_unused]] constexpr auto createWorkshopItem(){}

	[[maybe_unused]] constexpr bool pushWorkshopItemUpdate(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemTitle(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemDescription(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemLanguage(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemMetadata(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemVisibility(auto, auto = false){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemTags(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemContent(auto){return false;}
	[[maybe_unused]] constexpr bool setWorkshopItemPreview(auto){return false;}
	[[maybe_unused]] constexpr bool popWorkshopItemUpdate(auto){return false;}

	[[maybe_unused]] constexpr auto getWorkshopSubscribedItems(){}
	[[maybe_unused]] constexpr bool isWorkshopSubscribedItemInstalled(auto){return false;}
	[[maybe_unused]] constexpr bool isWorkshopSubscribedItemDownloading(auto){return false;}
	[[maybe_unused]] constexpr auto getWorkshopItemInstallInfo(auto){}
	[[maybe_unused]] constexpr auto getWorkshopItemDetails(auto&){}
	[[maybe_unused]] constexpr void forceWorkshopItemUpdateDownload(auto){}

	[[maybe_unused]] constexpr void startWorkshopItemPlaytimeTracking(auto){}
	[[maybe_unused]] constexpr void stopWorkshopItemPlaytimeTracking(auto){}
	[[maybe_unused]] constexpr void stopWorkshopPlaytimeTrackingForAllItems(){}

	[[maybe_unused]] constexpr void openURLInGameOverlay(auto){}
	[[maybe_unused]] constexpr void openWorkshopItemURLInGameOverlay(auto){}

	[[maybe_unused]] constexpr void setRichPresence(auto, auto){}

	[[maybe_unused]] constexpr bool isReady(){return false;}
	[[maybe_unused]] constexpr auto getLastError(){}
	[[maybe_unused]] constexpr bool isGameOverlayEnabled(){return false;}
	[[maybe_unused]] constexpr auto getUsername(){}
};

constexpr SteamworksInterface *steam{};

#endif

#endif
