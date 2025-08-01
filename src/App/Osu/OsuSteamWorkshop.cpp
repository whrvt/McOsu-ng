//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		workshop support (subscribed items, upload/download/status)
//
// $NoKeywords: $osusteamws
//===============================================================================//

#include "OsuSteamWorkshop.h"

#ifdef MCENGINE_FEATURE_STEAMWORKS

#include "Engine.h"
#include "ConVar.h"
#include "File.h"
#include "ResourceManager.h"
#include "SteamworksInterface.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuNotificationOverlay.h"



class OsuSteamWorkshopLoader final : public Resource
{
public:
	OsuSteamWorkshopLoader() : Resource()
	{
		

		m_bLoadDetails = true;
	}

	void setLoadDetails(bool loadDetails) {m_bLoadDetails = loadDetails;}

	[[nodiscard]] bool getLoadDetails() const {return m_bLoadDetails;}
	[[nodiscard]] Type getResType() const override { return APPDEFINED; } // TODO: handle this better?
protected:
	void init() override
	{
		if (m_bReady) return;

		osu->getSteamWorkshop()->m_subscribedItems = m_subscribedItems;

		m_bReady = true;
	}

	void initAsync() override
	{
		if (m_bAsyncReady) return;

		const std::vector<uint64_t> subscribedItems = steam->getWorkshopSubscribedItems();

		debugLog("OsuSteamWorkshop: Subscribed to {} item(s)\n", subscribedItems.size());

		const std::vector<SteamworksInterface::WorkshopItemDetails> details = (m_bLoadDetails ? steam->getWorkshopItemDetails(subscribedItems) : std::vector<SteamworksInterface::WorkshopItemDetails>());

		for (int i=0; i<subscribedItems.size(); i++)
		{
			const bool installed = steam->isWorkshopSubscribedItemInstalled(subscribedItems[i]);
			const bool downloading = steam->isWorkshopSubscribedItemDownloading(subscribedItems[i]);

			if (!installed && !downloading)
			{
				debugLog("OsuSteamWorkshop: Item {} not yet ready, skipping\n", subscribedItems[i]);
				continue;
			}

			SteamworksInterface::WorkshopItemDetails detail;
			detail.publishedFileId = 0;

			for (int d=0; d<details.size(); d++)
			{
				if (details[d].publishedFileId == subscribedItems[i])
				{
					detail = details[d];
					break;
				}
			}

			//if (details.title.length() > 0)
			{
				OsuSteamWorkshop::SUBSCRIBED_ITEM item;
				item.type = OsuSteamWorkshop::SUBSCRIBED_ITEM_TYPE::SKIN;
				item.status = (installed ? OsuSteamWorkshop::SUBSCRIBED_ITEM_STATUS::INSTALLED : OsuSteamWorkshop::SUBSCRIBED_ITEM_STATUS::DOWNLOADING);
				item.id = subscribedItems[i];
				item.title = (detail.title.length() > 0 ? detail.title : UString::format("%llu", subscribedItems[i]));

				if (installed)
				{
					item.installInfo = steam->getWorkshopItemInstallInfo(subscribedItems[i]);

					if (item.installInfo.length() < 1)
					{
						debugLog("OsuSteamWorkshop: Invalid item {} (installInfo = {:s})\n", subscribedItems[i], item.installInfo.toUtf8());
						continue;
					}
				}

				m_subscribedItems.push_back(item);
			}
			//else
			//	debugLog("OsuSteamWorkshop: Invalid item {} (title = {:s})\n", subscribedItems[i], details.title.toUtf8());
		}

		debugLog("OsuSteamWorkshop: Done\n");

		m_bAsyncReady = true;
	}

	void destroy() override
	{
		m_subscribedItems.clear();
	}

private:
	std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> m_subscribedItems;
	bool m_bLoadDetails;
};



class OsuSteamWorkshopUploader final : public Resource
{
public:
	OsuSteamWorkshopUploader()
	{
		

		m_bPrepared = false;

		m_iPublishedFileId = 0;
	}

	void set(UString skinName, UString skinPath, UString thumbnailFilePath, UString workshopItemIdFilePath, uint64_t publishedFileId)
	{
		m_sSkinName = skinName;
		m_sSkinPath = skinPath;
		m_sThumbnailFilePath = thumbnailFilePath;
		m_sWorkshopItemIdFilePath = workshopItemIdFilePath;
		m_iPublishedFileId = publishedFileId;

		m_bPrepared = true;
	}
	[[nodiscard]] Type getResType() const override { return APPDEFINED; } // TODO: handle this better?
protected:
	void init() override
	{
		if (!m_bPrepared || m_bReady) return;

		if (m_sErrorMessage.length() < 1)
			osu->getNotificationOverlay()->addNotification("Done.", 0xff00ff00);
		else
			osu->getSteamWorkshop()->handleUploadError(m_sErrorMessage);

		m_bReady = true;
	}

	void initAsync() override
	{
		if (!m_bPrepared || m_bAsyncReady) return;

		// (assume all values have been validated)

		// 1) create new item if necessary
		bool isNewItem = false;
		if (m_iPublishedFileId < 1)
		{
			isNewItem = true;
			m_iPublishedFileId = steam->createWorkshopItem();
		}

		if (m_iPublishedFileId > 0)
		{
			// 1.1) create steamworkshopitemid.txt if necessary
			if (isNewItem)
			{
				const UString publishedFileIdString = UString::format("%llu", m_iPublishedFileId);

				McFile steamworkshopitemidtxt(m_sWorkshopItemIdFilePath, McFile::TYPE::WRITE);

				if (!steamworkshopitemidtxt.canWrite())
					debugLog("OsuSteamWorkshop Error: Can't write \"{:s}\"!!!\n", m_sWorkshopItemIdFilePath.toUtf8());

				steamworkshopitemidtxt.write(publishedFileIdString.toUtf8(), publishedFileIdString.lengthUtf8());
			}

			// 2) prepare item update
			if (steam->pushWorkshopItemUpdate(m_iPublishedFileId))
			{
				if (isNewItem)
				{
					steam->setWorkshopItemTitle(m_sSkinName);
					steam->setWorkshopItemDescription("TBD");
					steam->setWorkshopItemVisibility(false);

					std::vector<UString> tags;
					tags.emplace_back("Skin");
					/*
					tags.push_back("Standard");
					tags.push_back("Mania");
					tags.push_back("Taiko");
					tags.push_back("Catch");
					tags.push_back("VR");
					tags.push_back("FPoSu");
					*/
					steam->setWorkshopItemTags(tags);

					steam->setWorkshopItemMetadata("skin");
				}

				steam->setWorkshopItemContent(m_sSkinPath);
				steam->setWorkshopItemPreview(m_sThumbnailFilePath);

				// 3) start item update
				if (steam->popWorkshopItemUpdate("")) // TODO: changelog support?
				{
					// 3.1) the first thing anyone does after updating an existing item is checking if it works (e.g. new content shows up)
					// force steam to immediately download the changes
					if (!isNewItem)
						steam->forceWorkshopItemUpdateDownload(m_iPublishedFileId);

					// 4) open item page in overlay if successful
					steam->openWorkshopItemURLInGameOverlay(m_iPublishedFileId);
				}
				else
					m_sErrorMessage = "popWorkshopItemUpdate() failed!";
			}
			else
				m_sErrorMessage = "pushWorkshopItemUpdate() failed!";
		}
		else
			m_sErrorMessage = UString::format("createWorkshopItem() failed (%llu)!", m_iPublishedFileId);

		m_bAsyncReady = true;
	}

	void destroy() override
	{
		m_bPrepared = false;
		m_sErrorMessage.clear();
	}

private:
	bool m_bPrepared;
	UString m_sErrorMessage;

	UString m_sSkinName;
	UString m_sSkinPath;
	UString m_sThumbnailFilePath;
	UString m_sWorkshopItemIdFilePath;
	uint64_t m_iPublishedFileId;
};


namespace cv::osu {
ConVar workshop_upload_skin("osu_workshop_upload_skin");
}

OsuSteamWorkshop::OsuSteamWorkshop()
{
	

	m_loader = new OsuSteamWorkshopLoader();
	m_uploader = new OsuSteamWorkshopUploader();

	// convar refs



	// convar callbacks
	cv::osu::workshop_upload_skin.setCallback( fastdelegate::MakeDelegate(this, &OsuSteamWorkshop::onUpload) );
}

OsuSteamWorkshop::~OsuSteamWorkshop()
{
	if (resourceManager->isLoadingResource(m_loader))
		while (!m_loader->isAsyncReady()) {;}

	resourceManager->destroyResource(m_loader);

	if (resourceManager->isLoadingResource(m_uploader))
		while (!m_uploader->isAsyncReady()) {;}

	resourceManager->destroyResource(m_uploader);
}

void OsuSteamWorkshop::refresh(bool async, bool alsoLoadDetailsWhichTakeVeryLongToLoad)
{
	if (!steam->isReady()) return;

	if (resourceManager->isLoadingResource(m_loader))
	{
		if (!m_loader->isAsyncReady())
			return;
	}

	// reset
	m_loader->release();
	m_loader->setLoadDetails(alsoLoadDetailsWhichTakeVeryLongToLoad);

	// schedule
	if (async)
		resourceManager->requestNextLoadAsync();

	resourceManager->loadResource(m_loader);
}

bool OsuSteamWorkshop::isReady() const
{
	if (!steam->isReady()) return true; // empty

	return m_loader->isReady();
}

bool OsuSteamWorkshop::areDetailsLoaded() const
{
	if (!steam->isReady()) return true; // empty

	return m_loader->getLoadDetails();
}

bool OsuSteamWorkshop::isUploading() const
{
	if (!steam->isReady()) return false;

	return resourceManager->isLoadingResource(m_uploader);
}

void OsuSteamWorkshop::onUpload()
{
	if (osu->getSkin() == NULL) return;

	if (resourceManager->isLoadingResource(m_uploader))
	{
		if (!m_uploader->isAsyncReady())
			return;
	}

	if (!steam->isReady())
	{
		handleUploadError("Steam is not running.");
		return;
	}

	if (!osu->getSkin()->isReady())
	{
		handleUploadError("Skin is not fully loaded yet.");
		return;
	}

	if (cv::osu::skin_is_from_workshop.getBool() || osu->getSkin()->isWorkshopSkin())
	{
		handleUploadError("Skin must be a local skin.");
		return;
	}

	if (osu->getSkin()->isDefaultSkin())
	{
		handleUploadError("Skin must not be the default skin.");
		return;
	}

	// name and path
	const UString skinName = cv::osu::skin.getString();
	const UString skinPath = osu->getSkin()->getFilePath();

	debugLog("skinName = \"{:s}\", skinFolder = \"{:s}\"\n", cv::osu::skin.getString().toUtf8(), skinPath.toUtf8());

	if (skinName.length() < 1)
	{
		handleUploadError("Invalid skin name.");
		return;
	}

	if (skinPath.length() < 2)
	{
		handleUploadError("Invalid skin folder.");
		return;
	}

	// thumbnail
	UString thumbnailFilePath = skinPath;
	thumbnailFilePath.append("steamworkshopthumbnail.jpg");

	if (!env->fileExists(thumbnailFilePath))
	{
		handleUploadError("Missing steamworkshopthumbnail.jpg");
		return;
	}

	// thumbnail must be readable (filesystem)
	// thumbnail must be < 1 MB
	{
		McFile tempFileForSizeCheck(thumbnailFilePath);

		debugLog("filesize = {}\n", tempFileForSizeCheck.getFileSize());

		if (!tempFileForSizeCheck.canRead() || tempFileForSizeCheck.getFileSize() < 4)
		{
			handleUploadError("Thumbnail is unreadable.");
			return;
		}

		if (tempFileForSizeCheck.getFileSize() > (1000000 - 1))
		{
			handleUploadError("Thumbnail filesize is too big.");
			return;
		}
	}

	// thumbnail must be readable (engine)
	{
		Image *image = resourceManager->loadImageAbs(thumbnailFilePath, "");
		const bool isImageReady = image->isReady();
		resourceManager->destroyResource(image);

		if (!isImageReady)
		{
			handleUploadError("Thumbnail is broken.");
			return;
		}
	}

	// workshop item id
	UString workshopitemidFilePath = skinPath;
	workshopitemidFilePath.append("steamworkshopitemid.txt");
	uint64_t itemId = 0;
	{
		if (env->fileExists(workshopitemidFilePath))
		{
			McFile itemIdFile(workshopitemidFilePath);

			if (!itemIdFile.canRead() || itemIdFile.getFileSize() < 1)
			{
				handleUploadError("steamworkshopitemid.txt is broken.");
				return;
			}

			const UString existingItemIdString = itemIdFile.readLine();
			itemId = (uint64_t)existingItemIdString.toUnsignedLongLong();

			debugLog("itemId = {}\n", itemId);

			if (itemId < 1)
			{
				handleUploadError("Item ID is invalid.");
				return;
			}
		}
		else
			debugLog("steamworkshopitemid.txt not found, will create new item\n");
	}

	// reset
	m_uploader->release();

	// prepare
	m_uploader->set(skinName, skinPath, thumbnailFilePath, workshopitemidFilePath, itemId);

	// schedule
	resourceManager->requestNextLoadAsync();
	resourceManager->loadResource(m_uploader);

	osu->getNotificationOverlay()->addNotification((itemId == 0 ? "Uploading ...                                   " : "Updating ...                                 "), 0xffffffff, false, 60.0f);
}

void OsuSteamWorkshop::handleUploadError(UString errorMessage)
{
	debugLog("OsuSteamWorkshop Error: {:s}\n", errorMessage.toUtf8());
	osu->getNotificationOverlay()->addNotification(UString::format("Error: %s", errorMessage.toUtf8()), 0xffff0000, false, 3.0f);
}
#endif
