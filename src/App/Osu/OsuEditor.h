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
	~OsuEditor() override;

	void draw() override;
	void update() override;

	void onResolutionChange(Vector2 newResolution) override;

private:
	void onBack() override;
};

#endif
