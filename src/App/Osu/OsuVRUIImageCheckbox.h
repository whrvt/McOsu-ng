//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple VR checkbox with an image/icon on top
//
// $NoKeywords: $osuvricb
//===============================================================================//

#pragma once
#ifndef OSUVRUIIMAGECHECKBOX_H
#define OSUVRUIIMAGECHECKBOX_H

#include "OsuVRUIButton.h"

class OsuVRUIImageCheckbox : public OsuVRUIButton
{
public:
	OsuVRUIImageCheckbox(OsuVR *vr, float x, float y, float width, float height, UString imageResourceNameChecked, UString imageResourceNameUnchecked);

	void drawVR(Graphics *g, Matrix4 &mvp) override;
	void update(Vector2 cursorPos) override;

	void setChecked(bool checked) {m_bChecked = checked;}

	[[nodiscard]] inline bool isChecked() const {return m_bChecked;}

private:
	void onClicked() override;

	void onCursorInside() override;
	void onCursorOutside() override;

	void updateImageResource();

	bool m_bChecked;

	UString m_sImageResourceNameChecked;
	UString m_sImageResourceNameUnchecked;
	Image *m_imageChecked;
	Image *m_imageUnchecked;

	float m_fAnimation;
};

#endif
