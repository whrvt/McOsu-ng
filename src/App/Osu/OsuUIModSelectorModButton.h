//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		mod image buttons (EZ, HD, HR, HT, DT, etc.)
//
// $NoKeywords: $osumsmb
//===============================================================================//

#pragma once
#ifndef OSUUIMODSELECTORMODBUTTON_H
#define OSUUIMODSELECTORMODBUTTON_H

#include "CBaseUIImageButton.h"

class Osu;
class OsuSkinImage;
class OsuModSelector;

class OsuUIModSelectorModButton : public CBaseUIButton
{
public:
	OsuUIModSelectorModButton(OsuModSelector* osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw() override;
	void update() override;

	void click() {onMouseDownInside();}

	void resetState();

	void setState(unsigned int state, bool initialState, UString modName, UString tooltipText, std::function<OsuSkinImage*()> getImageFunc);
	void setBaseScale(float xScale, float yScale);
	void setAvailable(bool available) {m_bAvailable = available;}

	UString getActiveModName();
	[[nodiscard]] inline int getState() const {return m_iState;}
	[[nodiscard]] inline bool isOn() const {return m_bOn;}

private:
	void onMouseDownInside() override;
	void onFocusStolen() override;

	void setOn(bool on);
	void setState(int state, bool updateModConVar = true);

	OsuModSelector *m_osuModSelector;

	bool m_bAvailable;
	bool m_bOn;
	int m_iState;
	float m_fEnabledScaleMultiplier;
	float m_fEnabledRotationDeg;
	Vector2 m_vBaseScale;

	struct STATE
	{
		UString modName;
		std::vector<UString> tooltipTextLines;
		std::function<OsuSkinImage*()> getImageFunc;
	};
	std::vector<STATE> m_states;

	Vector2 m_vScale;
	float m_fRot;
	std::function<OsuSkinImage*()> getActiveImageFunc;

	bool m_bFocusStolenDelay;
};

#endif
