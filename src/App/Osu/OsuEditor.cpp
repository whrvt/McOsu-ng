/*
 * OsuEditor.h
 *
 *  Created on: 30. Mai 2017
 *      Author: Psy
 */

#include "OsuEditor.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuEditor::OsuEditor() : OsuScreenBackable()
{

}

OsuEditor::~OsuEditor()
{
}

void OsuEditor::draw()
{
	if (!m_bVisible) return;

	// draw back button on top of (after) everything else
	OsuScreenBackable::draw();
}

void OsuEditor::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// update stuff if visible
}

void OsuEditor::onBack()
{
	soundEngine->play(osu->getSkin()->getMenuClick());

	osu->toggleEditor();
}

void OsuEditor::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	debugLog("OsuEditor::onResolutionChange({}, {})\n", (int)newResolution.x, (int)newResolution.y);
}
