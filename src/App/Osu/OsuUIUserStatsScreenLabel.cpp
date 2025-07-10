//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		currently only used for showing the pp/star algorithm versions
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUIUserStatsScreenLabel.h"

#include "Osu.h"
#include "OsuTooltipOverlay.h"

OsuUIUserStatsScreenLabel::OsuUIUserStatsScreenLabel(float xPos, float yPos, float xSize, float ySize, const UString& name, const UString& text) : CBaseUILabel()
{
	
}

void OsuUIUserStatsScreenLabel::update()
{
	CBaseUILabel::update();
	if (!m_bVisible) return;

	if (isMouseInside())
	{
		bool isEmpty = true;
		for (size_t i=0; i<m_tooltipTextLines.size(); i++)
		{
			if (m_tooltipTextLines[i].length() > 0)
			{
				isEmpty = false;
				break;
			}
		}

		if (!isEmpty)
		{
			osu->getTooltipOverlay()->begin();
			{
				for (size_t i=0; i<m_tooltipTextLines.size(); i++)
				{
					if (m_tooltipTextLines[i].length() > 0)
						osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
				}
			}
			osu->getTooltipOverlay()->end();
		}
	}
}
