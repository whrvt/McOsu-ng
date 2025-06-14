/*
 * OsuEditor.h
 *
 *  Created on: 30. Mai 2017
 *      Author: Psy
 */

#pragma once
#ifndef OSUEDITOR_H
#define OSUEDITOR_H

#include "OsuScreenBackable.h"

class OsuEditor : public OsuScreenBackable
{
public:
	OsuEditor();
	virtual ~OsuEditor();

	virtual void draw();
	virtual void update();

	virtual void onResolutionChange(Vector2 newResolution);

private:
	virtual void onBack();
};

#endif
