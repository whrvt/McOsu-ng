//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#include "OsuUISongBrowserSongButton.h"

#include <algorithm>
#include <utility>

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"
#include "Mouse.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"
#include "OsuBackgroundImageHandler.h"

#include "OsuUISongBrowserCollectionButton.h"
#include "OsuUISongBrowserSongDifficultyButton.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUIContextMenu.h"
namespace cv::osu {
ConVar draw_songbrowser_thumbnails("osu_draw_songbrowser_thumbnails", true, FCVAR_NONE);
ConVar songbrowser_thumbnail_delay("osu_songbrowser_thumbnail_delay", 0.1f, FCVAR_NONE);
ConVar songbrowser_thumbnail_fade_in_duration("osu_songbrowser_thumbnail_fade_in_duration", 0.1f, FCVAR_NONE);
}

float OsuUISongBrowserSongButton::thumbnailYRatio = 1.333333f;

OsuUISongBrowserSongButton::OsuUISongBrowserSongButton(OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, OsuDatabaseBeatmap *databaseBeatmap) : OsuUISongBrowserButton(songBrowser, view, contextMenu, xPos, yPos, xSize, ySize, std::move(name))
{
	m_databaseBeatmap = databaseBeatmap;
	m_representativeDatabaseBeatmap = NULL;

	m_grade = OsuScore::GRADE::GRADE_D;
	m_bHasGrade = false;

	// settings
	setHideIfSelected(true);

	// labels
	/*
	m_sTitle = "Title";
	m_sArtist = "Artist";
	m_sMapper = "Mapper";
	*/

	m_fThumbnailFadeInTime = 0.0f;
	m_fTextOffset = 0.0f;
	m_fGradeOffset = 0.0f;
	m_fTextSpacingScale = 0.075f;
	m_fTextMarginScale = 0.075f;
	m_fTitleScale = 0.22f;
	m_fSubTitleScale = 0.14f;
	m_fGradeScale = 0.45f;

	// build children
	if (m_databaseBeatmap != NULL)
	{
		const std::vector<OsuDatabaseBeatmap*> &difficulties = m_databaseBeatmap->getDifficulties();

		// and add them
		for (int i=0; i<difficulties.size(); i++)
		{
			OsuUISongBrowserSongButton *songButton = new OsuUISongBrowserSongDifficultyButton(m_songBrowser, m_view, m_contextMenu, 0, 0, 0, 0, "", difficulties[i], this);

			m_children.push_back(songButton);
		}
	}

	updateRepresentativeDatabaseBeatmap();
	updateLayoutEx();
}

OsuUISongBrowserSongButton::~OsuUISongBrowserSongButton()
{
	for (int i=0; i<m_children.size(); i++)
	{
		delete m_children[i];
	}
}

void OsuUISongBrowserSongButton::draw()
{
	if (!m_bVisible) return;
	OsuUISongBrowserButton::draw();

	// draw background image
	if (m_representativeDatabaseBeatmap != NULL)
		drawBeatmapBackgroundThumbnail(osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_representativeDatabaseBeatmap));

	drawTitle();
	drawSubTitle();
}

void OsuUISongBrowserSongButton::update()
{
	if (!m_bVisible) return;
	OsuUISongBrowserButton::update();

	// HACKHACK: calling these two every frame is a bit insane, but too lazy to write delta detection logic atm. (UI desync is not a problem since parent buttons are invisible while selected, so no resorting happens in that state)
	sortChildren();
	updateRepresentativeDatabaseBeatmap();
}

void OsuUISongBrowserSongButton::drawBeatmapBackgroundThumbnail(Image *image)
{
	if (!cv::osu::draw_songbrowser_thumbnails.getBool() || osu->getSkin()->getVersion() < 2.2f) return;

	float alpha = 1.0f;
	if (cv::osu::songbrowser_thumbnail_fade_in_duration.getFloat() > 0.0f)
	{
		if (image == NULL || !image->isReady())
			m_fThumbnailFadeInTime = engine->getTime();
		else if (m_fThumbnailFadeInTime > 0.0f && engine->getTime() > m_fThumbnailFadeInTime)
		{
			alpha = std::clamp<float>((engine->getTime() - m_fThumbnailFadeInTime)/cv::osu::songbrowser_thumbnail_fade_in_duration.getFloat(), 0.0f, 1.0f);
			alpha = 1.0f - (1.0f - alpha)*(1.0f - alpha);
		}
	}

	if (image == NULL || !image->isReady()) return;

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float beatmapBackgroundScale = Osu::getImageScaleToFillResolution(image, Vector2(size.y*thumbnailYRatio, size.y))*1.05f;

	Vector2 centerOffset = Vector2(std::round((size.y*thumbnailYRatio)/2.0f), std::round(size.y/2.0f));
	McRect clipRect = McRect(pos.x-2, pos.y+1, (int)(size.y*thumbnailYRatio)+5, size.y+2);

	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	g->pushTransform();
	{
		g->scale(beatmapBackgroundScale, beatmapBackgroundScale);
		g->translate(pos.x + (int)centerOffset.x, pos.y + (int)centerOffset.y);
		g->pushClipRect(clipRect);
		{
			g->drawImage(image);
		}
		g->popClipRect();
	}
	g->popTransform();

	// debug cliprect bounding box
	if (cv::osu::debug.getBool())
	{
		Vector2 clipRectPos = Vector2(clipRect.getX(), clipRect.getY()-1);
		Vector2 clipRectSize = Vector2(clipRect.getWidth(), clipRect.getHeight());

		g->setColor(0xffffff00);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x, clipRectPos.y+clipRectSize.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x+clipRectSize.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
	}
}

void OsuUISongBrowserSongButton::drawGrade()
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_grade);
	g->pushTransform();
	{
		const float scale = calculateGradeScale();

		g->setColor(0xffffffff);
		grade->drawRaw(Vector2(pos.x + m_fGradeOffset + grade->getSizeBaseRaw().x*scale/2, pos.y + size.y/2), scale);
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::drawTitle(float deselectedAlpha, bool forceSelectedStyle)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	g->setColor((m_bSelected || forceSelectedStyle) ? osu->getSkin()->getSongSelectActiveText() : osu->getSkin()->getSongSelectInactiveText());
	if (!(m_bSelected || forceSelectedStyle))
		g->setAlpha(deselectedAlpha);

	g->pushTransform();
	{
		g->scale(titleScale, titleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale);
		g->drawString(m_font, buildTitleString());

		// debugging
		//g->drawString(m_font, UString::format("%i, %i", m_diff->setID, m_diff->ID));
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::drawSubTitle(float deselectedAlpha, bool forceSelectedStyle)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	const float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	g->setColor((m_bSelected || forceSelectedStyle) ? osu->getSkin()->getSongSelectActiveText() : osu->getSkin()->getSongSelectInactiveText());
	if (!(m_bSelected || forceSelectedStyle))
		g->setAlpha(deselectedAlpha);

	g->pushTransform();
	{
		g->scale(subTitleScale, subTitleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale*0.85f);
		g->drawString(m_font, buildSubTitleString());

		// debug stuff
		/*
		g->translate(-300, 0);
		long long oldestTime = std::numeric_limits<long long>::min();
		for (int i=0; i<m_beatmap->getNumDifficulties(); i++)
		{
			if ((*m_beatmap->getDifficultiesPointer())[i]->lastModificationTime > oldestTime)
				oldestTime = (*m_beatmap->getDifficultiesPointer())[i]->lastModificationTime;
		}
		g->drawString(m_font, UString::format("t = %I64d", oldestTime));
		*/
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::sortChildren()
{
	std::ranges::sort(m_children, OsuSongBrowser2::sortByDifficulty);
}

void OsuUISongBrowserSongButton::updateLayoutEx()
{
	OsuUISongBrowserButton::updateLayoutEx();

	// scaling
	const Vector2 size = getActualSize();

	m_fTextOffset = 0.0f;
	m_fGradeOffset = 0.0f;

	if (m_bHasGrade)
		m_fTextOffset += calculateGradeWidth();

	if (osu->getSkin()->getVersion() < 2.2f)
	{
		m_fTextOffset += size.x*0.02f*2.0f;
		if (m_bHasGrade)
			m_fGradeOffset += calculateGradeWidth()/2;
	}
	else
	{
		m_fTextOffset += size.y*thumbnailYRatio + size.x*0.02f;
		m_fGradeOffset += size.y*thumbnailYRatio + size.x*0.0125f;
	}
}

void OsuUISongBrowserSongButton::onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected)
{
	OsuUISongBrowserButton::onSelected(wasSelected, autoSelectBottomMostChild, wasParentSelected);

	// resort children (since they might have been updated in the meantime)
	sortChildren();

	// update grade on child
	for (int c=0; c<m_children.size(); c++)
	{
		OsuUISongBrowserSongDifficultyButton *child = (OsuUISongBrowserSongDifficultyButton*)m_children[c];
		child->updateGrade();
	}

	m_songBrowser->onSelectionChange(this, false);

	// now, automatically select the bottom child (hardest diff, assuming default sorting, and respecting the current search matches)
	if (autoSelectBottomMostChild)
	{
		for (int i=m_children.size()-1; i>=0; i--)
		{
			if (m_children[i]->isSearchMatch()) // NOTE: if no search is active, then all search matches return true by default
			{
				m_children[i]->select(true, false, wasSelected);
				break;
			}
		}
	}
}

void OsuUISongBrowserSongButton::onRightMouseUpInside()
{
	triggerContextMenu(mouse->getPos());
}

void OsuUISongBrowserSongButton::onOpenBeatmapFolderClicked()
{
	const auto toOpen = m_representativeDatabaseBeatmap ? m_representativeDatabaseBeatmap : m_databaseBeatmap ? m_databaseBeatmap : nullptr;
	if (!toOpen)
		return;
	env->openFileBrowser("beatmap folder", toOpen->getFolder());
}

void OsuUISongBrowserSongButton::triggerContextMenu(Vector2 pos)
{
	if (m_contextMenu != NULL)
	{
		m_contextMenu->setPos(pos);
		m_contextMenu->setRelPos(pos);
		m_contextMenu->begin(0, true);
		{
			if (m_representativeDatabaseBeatmap || m_databaseBeatmap)
				m_contextMenu->addButtonJustified("[...] Open Beatmap Folder", true, 0)->setClickCallback(fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onOpenBeatmapFolderClicked));

			if (m_databaseBeatmap != NULL && m_databaseBeatmap->getDifficulties().size() < 1)
				m_contextMenu->addButtonJustified("[+] Add to Collection", true, 1);
			m_contextMenu->addButtonJustified("[+Set] Add to Collection", true, 2);

			if (osu->getSongBrowser()->getGroupingMode() == OsuSongBrowser2::GROUP::GROUP_COLLECTIONS)
			{
				// get the collection name for this diff/set
				UString collectionName;
				{
					const std::vector<OsuUISongBrowserCollectionButton*> &collectionButtons = osu->getSongBrowser()->getCollectionButtons();
					for (size_t i=0; i<collectionButtons.size(); i++)
					{
						if (collectionButtons[i]->isSelected())
						{
							collectionName = collectionButtons[i]->getCollectionName();
							break;
						}
					}
				}

				// check if this entry in the collection is coming from osu! or not
				// the entry could be either a set button, or an independent diff button
				bool isLegacyEntry = false;
				{
					const std::vector<OsuDatabase::Collection> &collections = osu->getSongBrowser()->getDatabase()->getCollections();
					for (size_t i=0; i<collections.size(); i++)
					{
						if (collections[i].name == collectionName)
						{
							for (size_t e=0; e<collections[i].hashes.size(); e++)
							{
								if (m_databaseBeatmap->getDifficulties().size() < 1)
								{
									// independent diff

									if (collections[i].hashes[e].hash == m_databaseBeatmap->getMD5Hash())
									{
										isLegacyEntry = collections[i].hashes[e].isLegacyEntry;
										break;
									}
								}
								else
								{
									// set

									const std::vector<OsuDatabaseBeatmap*> &diffs = m_databaseBeatmap->getDifficulties();

									for (size_t d=0; d<diffs.size(); d++)
									{
										if (collections[i].hashes[e].hash == diffs[d]->getMD5Hash())
										{
											// one single entry of the set coming from osu! is enough to deny removing the set (as a whole)
											if (collections[i].hashes[e].isLegacyEntry)
											{
												isLegacyEntry = true;
												break;
											}
										}
									}

									if (isLegacyEntry)
										break;
								}
							}

							break;
						}
					}
				}

				CBaseUIButton *spacer = m_contextMenu->addButtonJustified("---", false);
				spacer->setEnabled(false);
				spacer->setTextColor(0xff888888);
				spacer->setTextDarkColor(0xff000000);

				CBaseUIButton *removeDiffButton = NULL;
				if (m_databaseBeatmap == NULL || m_databaseBeatmap->getDifficulties().size() < 1)
					removeDiffButton = m_contextMenu->addButtonJustified("[-] Remove from Collection", true, 3);

				CBaseUIButton *removeSetButton = m_contextMenu->addButtonJustified("[-Set] Remove from Collection", true, 4);

				if (isLegacyEntry)
				{
					if (removeDiffButton != NULL)
					{
						removeDiffButton->setTextColor(0xff888888);
						removeDiffButton->setTextDarkColor(0xff000000);
					}

					removeSetButton->setTextColor(0xff888888);
					removeSetButton->setTextDarkColor(0xff000000);
				}
			}
		}
		m_contextMenu->end(false, false);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onContextMenu) );
		OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
		OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
	}
}

void OsuUISongBrowserSongButton::onContextMenu(const UString& text, int id)
{
	if (id == 1 || id == 2)
	{
		m_contextMenu->begin(0, true);
		{
			const std::vector<OsuDatabase::Collection> &collections = osu->getSongBrowser()->getDatabase()->getCollections();

			m_contextMenu->addButtonJustified("[+] Create new Collection?", true, -id*2);

			if (collections.size() > 0)
			{
				CBaseUIButton *spacer = m_contextMenu->addButtonJustified("---", false);
				spacer->setEnabled(false);
				spacer->setTextColor(0xff888888);
				spacer->setTextDarkColor(0xff000000);

				for (size_t i=0; i<collections.size(); i++)
				{
					bool isDiffAndAlreadyContained = false;
					if (m_databaseBeatmap != NULL && m_databaseBeatmap->getDifficulties().size() < 1)
					{
						for (size_t h=0; h<collections[i].hashes.size(); h++)
						{
							if (collections[i].hashes[h].hash == m_databaseBeatmap->getMD5Hash())
							{
								isDiffAndAlreadyContained = true;

								// edge case: allow adding the set of this diff if it does not represent the entire set (and the set is not yet added completely)
								if (id == 2)
								{
									const auto *diffButtonPointer = this->as<const OsuUISongBrowserSongDifficultyButton>();
									if (diffButtonPointer != NULL && diffButtonPointer->getParentSongButton() != NULL && diffButtonPointer->getParentSongButton()->getDatabaseBeatmap() != NULL)
									{
										const OsuDatabaseBeatmap *setContainer = diffButtonPointer->getParentSongButton()->getDatabaseBeatmap();

										if (setContainer->getDifficulties().size() > 1)
										{
											const std::vector<OsuDatabaseBeatmap*> &diffs = setContainer->getDifficulties();

											bool isSetAlreadyContained = true;
											for (size_t s=0; s<diffs.size(); s++)
											{
												bool isDiffAlreadyContained = false;
												for (size_t h2=0; h2<collections[i].hashes.size(); h2++)
												{
													if (diffs[s]->getMD5Hash() == collections[i].hashes[h2].hash)
													{
														isDiffAlreadyContained = true;
														break;
													}
												}

												if (!isDiffAlreadyContained)
												{
													isSetAlreadyContained = false;
													break;
												}
											}

											if (!isSetAlreadyContained)
												isDiffAndAlreadyContained = false;
										}
									}
								}

								break;
							}
						}
					}

					bool isContainerAndSetAlreadyContained = false;
					if (m_databaseBeatmap != NULL && m_databaseBeatmap->getDifficulties().size() > 0)
					{
						const std::vector<OsuDatabaseBeatmap*> &diffs = m_databaseBeatmap->getDifficulties();

						bool foundAllDiffs = true;
						for (size_t d=0; d<diffs.size(); d++)
						{
							const std::vector<OsuDatabaseBeatmap*> &diffs = m_databaseBeatmap->getDifficulties();

							bool foundDiff = false;
							for (size_t h=0; h<collections[i].hashes.size(); h++)
							{
								if (collections[i].hashes[h].hash == diffs[d]->getMD5Hash())
								{
									foundDiff = true;

									break;
								}
							}

							foundAllDiffs = foundDiff;
							if (!foundAllDiffs)
								break;
						}

						isContainerAndSetAlreadyContained = foundAllDiffs;
					}

					CBaseUIButton *collectionButton = m_contextMenu->addButtonJustified(collections[i].name, false, id);

					if (isDiffAndAlreadyContained || isContainerAndSetAlreadyContained)
					{
						collectionButton->setEnabled(false);
						collectionButton->setTextColor(0xff555555);
						collectionButton->setTextDarkColor(0xff000000);
					}
				}
			}
		}
		m_contextMenu->end(false, true);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onAddToCollectionConfirmed) );
		OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
		OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
	}
	else if (id == 3 || id == 4)
	{
		// get the collection name for this diff/set
		UString collectionName;
		{
			const std::vector<OsuUISongBrowserCollectionButton*> &collectionButtons = osu->getSongBrowser()->getCollectionButtons();
			for (size_t i=0; i<collectionButtons.size(); i++)
			{
				if (collectionButtons[i]->isSelected())
				{
					collectionName = collectionButtons[i]->getCollectionName();
					break;
				}
			}
		}

		// check if this entry in the collection is coming from osu! or not
		// the entry could be either a set button, or an independent diff button
		bool isLegacyEntry = false;
		{
			const std::vector<OsuDatabase::Collection> &collections = osu->getSongBrowser()->getDatabase()->getCollections();
			for (size_t i=0; i<collections.size(); i++)
			{
				if (collections[i].name == collectionName)
				{
					for (size_t e=0; e<collections[i].hashes.size(); e++)
					{
						if (m_databaseBeatmap->getDifficulties().size() < 1)
						{
							// independent diff

							if (collections[i].hashes[e].hash == m_databaseBeatmap->getMD5Hash())
							{
								isLegacyEntry = collections[i].hashes[e].isLegacyEntry;
								break;
							}
						}
						else
						{
							// set

							const std::vector<OsuDatabaseBeatmap*> &diffs = m_databaseBeatmap->getDifficulties();

							for (size_t d=0; d<diffs.size(); d++)
							{
								if (collections[i].hashes[e].hash == diffs[d]->getMD5Hash())
								{
									// one single entry of the set coming from osu! is enough to deny removing the set (as a whole)
									if (collections[i].hashes[e].isLegacyEntry)
									{
										isLegacyEntry = true;
										break;
									}
								}
							}

							if (isLegacyEntry)
								break;
						}
					}

					break;
				}
			}
		}

		if (isLegacyEntry)
		{
			if (id == 3)
				osu->getNotificationOverlay()->addNotification("Can't remove collection entry loaded from osu!", 0xffffff00);
			else if (id == 4)
				osu->getNotificationOverlay()->addNotification("Can't remove collection set loaded from osu!", 0xffffff00);
		}
		else
			osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
	}
}

void OsuUISongBrowserSongButton::onAddToCollectionConfirmed(const UString& text, int id)
{
	if (id == -2 || id == -4)
	{
		m_contextMenu->begin(0, true);
		{
			CBaseUIButton *label = m_contextMenu->addButtonJustified("Enter Collection Name:", false);
			label->setEnabled(false);

			CBaseUIButton *spacer = m_contextMenu->addButtonJustified("---", false);
			spacer->setEnabled(false);
			spacer->setTextColor(0xff888888);
			spacer->setTextDarkColor(0xff000000);

			m_contextMenu->addTextbox("", id);

			spacer = m_contextMenu->addButtonJustified("---", false);
			spacer->setEnabled(false);
			spacer->setTextColor(0xff888888);
			spacer->setTextDarkColor(0xff000000);

			label = m_contextMenu->addButtonJustified("(Press ENTER to confirm.)", false, id);
			label->setTextColor(0xff555555);
			label->setTextDarkColor(0xff000000);
		}
		m_contextMenu->end(false, false);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onCreateNewCollectionConfirmed) );
		OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
		OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
	}
	else
	{
		// just forward it
		osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
	}
}

void OsuUISongBrowserSongButton::onCreateNewCollectionConfirmed(const UString& text, int id)
{
	if (id == -2 || id == -4)
	{
		// just forward it
		osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
	}
}

float OsuUISongBrowserSongButton::calculateGradeScale()
{
	const Vector2 size = getActualSize();

	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_grade);

	return Osu::getImageScaleToFitResolution(grade->getSizeBaseRaw(), Vector2(size.x, size.y*m_fGradeScale));
}

float OsuUISongBrowserSongButton::calculateGradeWidth()
{
	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_grade);

	return grade->getSizeBaseRaw().x * calculateGradeScale();
}

void OsuUISongBrowserSongButton::updateRepresentativeDatabaseBeatmap()
{
	if (m_databaseBeatmap != NULL && m_children.size() > 0)
	{
		const OsuDatabaseBeatmap *previousRepresentativeDatabaseBeatmap = m_representativeDatabaseBeatmap;

		// use the bottom child (hardest diff, assuming default sorting, and respecting the current search matches)
		for (int i=m_children.size()-1; i>=0; i--)
		{
			if (m_children[i]->isSearchMatch() && m_children[i]->as<OsuUISongBrowserSongButton>()) // NOTE: if no search is active, then all search matches return true by default
			{
				m_representativeDatabaseBeatmap = static_cast<const OsuUISongBrowserSongButton*>(m_children[i])->getDatabaseBeatmap();
				break;
			}
		}

		if (m_representativeDatabaseBeatmap != NULL && m_representativeDatabaseBeatmap != previousRepresentativeDatabaseBeatmap)
		{
			m_sTitle = m_representativeDatabaseBeatmap->getTitle();
			m_sArtist = m_representativeDatabaseBeatmap->getArtist();
			m_sMapper = m_representativeDatabaseBeatmap->getCreator();
		}
	}
}
