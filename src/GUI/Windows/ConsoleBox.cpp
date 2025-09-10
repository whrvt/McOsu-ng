//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		textbox + scrollview command suggestion list
//
// $NoKeywords: $
//===============================================================================//

#include "ConsoleBox.h"

#include <utility>

#include "AnimationHandler.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ResourceManager.h"

#include "Console.h"

#include "CBaseUIBoxShadow.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"
namespace cv
{
ConVar showconsolebox("showconsolebox");

ConVar consolebox_animspeed("consolebox_animspeed", 12.0f, FCVAR_NONE);
ConVar consolebox_draw_preview("consolebox_draw_preview", true, FCVAR_NONE, "whether the textbox shows the topmost suggestion while typing");
ConVar consolebox_draw_helptext("consolebox_draw_helptext", true, FCVAR_NONE, "whether convar suggestions also draw their helptext");

ConVar console_overlay("console_overlay", true, FCVAR_NONE, "should the log overlay always be visible (or only if the console is out)");
ConVar console_overlay_lines("console_overlay_lines", 6, FCVAR_NONE, "max number of lines of text");
ConVar console_overlay_scale("console_overlay_scale", 1.0f, FCVAR_NONE, "log text size multiplier");
} // namespace cv

class ConsoleBoxTextbox : public CBaseUITextbox
{
public:
	ConsoleBoxTextbox(float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUITextbox(xPos, yPos, xSize, ySize, std::move(name)) {}

	void setSuggestion(UString suggestion) { m_sSuggestion = std::move(suggestion); }

protected:
	void drawText() override
	{
		if (cv::consolebox_draw_preview.getBool())
		{
			if (m_sSuggestion.length() > 0 && m_sSuggestion.find(m_sText) == 0)
			{
				g->setColor(0xff444444);
				g->pushTransform();
				{
					g->translate((int)(m_vPos.x + m_iTextAddX + m_fTextScrollAddX), (int)(m_vPos.y + m_iTextAddY));
					g->drawString(m_font, m_sSuggestion);
				}
				g->popTransform();
			}
		}

		CBaseUITextbox::drawText();
	}

private:
	UString m_sSuggestion;
};

class ConsoleBoxSuggestionButton : public CBaseUIButton
{
public:
	ConsoleBoxSuggestionButton(float xPos, float yPos, float xSize, float ySize, UString name, const UString& text, UString helpText, ConsoleBox *consoleBox)
	    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), text)
	{
		m_sHelpText = std::move(helpText);
		m_consoleBox = consoleBox;
	}

protected:
	void drawText() override
	{
		if (m_font == NULL || m_sText.length() < 1)
			return;

		if (cv::consolebox_draw_helptext.getBool())
		{
			if (m_sHelpText.length() > 0)
			{
				const UString helpTextSeparator = "-";
				const int helpTextOffset =
				    std::round(2.0f * m_font->getStringWidth(helpTextSeparator) * ((float)m_font->getDPI() / 96.0f)); // NOTE: abusing font dpi
				const int helpTextSeparatorStringWidth = std::max(1, (int)m_font->getStringWidth(helpTextSeparator));
				const int helpTextStringWidth = std::max(1, (int)m_font->getStringWidth(m_sHelpText));

				g->pushTransform();
				{
					const float scale = std::min(1.0f, (std::max(1.0f, m_consoleBox->getTextbox()->getSize().x - m_fStringWidth - helpTextOffset * 1.5f -
					                                                       helpTextSeparatorStringWidth * 1.5f)) /
					                                       (float)helpTextStringWidth);

					g->scale(scale, scale);
					g->translate((int)(m_vPos.x + m_fStringWidth + helpTextOffset * scale / 2 + helpTextSeparatorStringWidth * scale),
					             (int)(m_vPos.y + m_vSize.y / 2.0f + m_fStringHeight / 2.0f - m_font->getHeight() * (1.0f - scale) / 2.0f));
					g->setColor(0xff444444);
					g->drawString(m_font, helpTextSeparator);
					g->translate(helpTextOffset * scale, 0);
					g->drawString(m_font, m_sHelpText);
				}
				g->popTransform();
			}
		}

		CBaseUIButton::drawText();
	}

private:
	ConsoleBox *m_consoleBox;
	UString m_sHelpText;
};

ConsoleBox::ConsoleBox() : WindowUIElement(0, 0, 0, 0, "")
{
	const float dpiScale = env->getDPIScale();

	McFont *font = resourceManager->getFont("FONT_DEFAULT");
	m_logFont = resourceManager->getFont("FONT_CONSOLE");

	m_textbox = new ConsoleBoxTextbox(5 * dpiScale, engine->getScreenHeight(), engine->getScreenWidth() - 10 * dpiScale, 26, "consoleboxtextbox");
	{
		m_textbox->setSizeY(m_textbox->getRelSize().y * dpiScale);
		m_textbox->setFont(font);
		m_textbox->setDrawBackground(true);
		m_textbox->setVisible(false);
		m_textbox->setBusy(true);
	}

	m_bRequireShiftToActivate = false;
	m_fConsoleAnimation = 0;
	m_bConsoleAnimateIn = false;
	m_bConsoleAnimateOut = false;
	m_fConsoleDelay = engine->getTime() + 0.2f;
	m_bConsoleAnimateOnce = false; // set to true for on-launch anim in

	m_suggestion = new CBaseUIScrollView(5 * dpiScale, engine->getScreenHeight(), engine->getScreenWidth() - 10 * dpiScale, 90 * dpiScale, "consoleboxsuggestion");
	{
		m_suggestion->setDrawBackground(true);
		m_suggestion->setBackgroundColor(rgb(0, 0, 0));
		m_suggestion->setFrameColor(rgb(255, 255, 255));
		m_suggestion->setHorizontalScrolling(false);
		m_suggestion->setVerticalScrolling(true);
		m_suggestion->setVisible(false);
	}
	m_fSuggestionAnimation = 0;
	m_fSuggestionY = 0.0f;
	m_fLogTime = 0.0f;
	m_bSuggestionAnimateIn = false;
	m_bSuggestionAnimateOut = false;

	m_iSuggestionCount = 0;
	m_iSelectedSuggestion = -1;

	m_iSelectedHistory = -1;

	m_fLogYPos = 0.0f;

	// initialize thread-safe log animation state
	m_bLogAnimationResetPending.store(false);
	m_fPendingLogTime.store(0.0f);
	m_bForceLogVisible.store(false);

	clearSuggestions();

	// convar callbacks
	cv::showconsolebox.setCallback(SA::MakeDelegate<&ConsoleBox::show>(this));
}

ConsoleBox::~ConsoleBox()
{
	SAFE_DELETE(m_textbox);
	SAFE_DELETE(m_suggestion);

	anim->deleteExistingAnimation(&m_fLogYPos);
}

void ConsoleBox::draw()
{
	// HACKHACK: legacy OpenGL fix
	g->setAntialiasing(false);

	g->pushTransform();
	{
		if (mouse->isMiddleDown())
			g->translate(0, mouse->getPos().y - engine->getScreenHeight());

		if (cv::console_overlay.getBool() || m_textbox->isVisible())
			drawLogOverlay();

		if (anim->isAnimating(&m_fConsoleAnimation))
		{
			g->push3DScene(McRect(m_textbox->getPos().x, m_textbox->getPos().y, m_textbox->getSize().x, m_textbox->getSize().y));
			{
				g->rotate3DScene(((m_fConsoleAnimation / getAnimTargetY()) * 130 - 130), 0, 0);
				g->translate3DScene(0, 0, ((m_fConsoleAnimation / getAnimTargetY()) * 500 - 500));
				m_textbox->draw();
				m_suggestion->draw();
			}
			g->pop3DScene();
		}
		else
		{
			m_suggestion->draw();
			m_textbox->draw();
		}
	}
	g->popTransform();
}

void ConsoleBox::drawLogOverlay()
{
	std::scoped_lock logGuard(m_logMutex);

	const float dpiScale = getDPIScale();

	const float logScale = std::round(dpiScale + 0.255f) * cv::console_overlay_scale.getFloat();

	const int shadowOffset = 1 * logScale;

	g->setColor(0xff000000);
	const float alpha = 1.0f - (m_fLogYPos / (m_logFont->getHeight() * (cv::console_overlay_lines.getInt() + 1)));
	if (m_fLogYPos != 0.0f)
		g->setAlpha(alpha);

	g->pushTransform();
	{
		g->scale(logScale, logScale);
		g->translate(2 * logScale + shadowOffset, -m_fLogYPos + shadowOffset);
		for (size_t i = 0; i < m_log.size(); i++)
		{
			g->translate(0, (int)((m_logFont->getHeight() + (i == 0 ? 0 : 2) + 1) * logScale));
			g->drawString(m_logFont, m_log[i].text);
		}
	}
	g->popTransform();

	g->setColor(0xffffffff);
	if (m_fLogYPos != 0.0f)
		g->setAlpha(alpha);

	g->pushTransform();
	{
		g->scale(logScale, logScale);
		g->translate(2 * logScale, -m_fLogYPos);
		for (size_t i = 0; i < m_log.size(); i++)
		{
			g->translate(0, (int)((m_logFont->getHeight() + (i == 0 ? 0 : 2) + 1) * logScale));
			g->setColor(m_log[i].textColor);
			g->setAlpha(alpha);
			g->drawString(m_logFont, m_log[i].text);
		}
	}
	g->popTransform();
}

void ConsoleBox::update()
{
	CBaseUIElement::update();

	// handle pending animation operations from logging threads
	processPendingLogAnimations();

	const bool mleft = mouse->isLeftDown();

	if (mleft && m_textbox->isMouseInside() && m_textbox->isVisible())
		m_textbox->setActive(true);

	// handle consolebox
	m_textbox->update();

	if (m_textbox->hitEnter())
	{
		processCommand(m_textbox->getText());
		m_textbox->clear();
		m_textbox->setSuggestion("");
	}

	if (m_bConsoleAnimateOnce)
	{
		if (engine->getTime() > m_fConsoleDelay)
		{
			m_bConsoleAnimateIn = true;
			m_bConsoleAnimateOnce = false;
			m_textbox->setVisible(true);
		}
	}

	if (m_bConsoleAnimateIn)
	{
		if (m_fConsoleAnimation < getAnimTargetY() && std::round((m_fConsoleAnimation / getAnimTargetY()) * 500) < 500.0f)
			m_textbox->setPosY(engine->getScreenHeight() - m_fConsoleAnimation);
		else
		{
			m_bConsoleAnimateIn = false;
			m_fConsoleAnimation = getAnimTargetY();
			m_textbox->setPosY(engine->getScreenHeight() - m_fConsoleAnimation);
			m_textbox->setActive(true);
			anim->deleteExistingAnimation(&m_fConsoleAnimation);
		}
	}

	if (m_bConsoleAnimateOut)
	{
		if (m_fConsoleAnimation > 0.0f && std::round((m_fConsoleAnimation / getAnimTargetY()) * 500) > 0.0f)
			m_textbox->setPosY(engine->getScreenHeight() - m_fConsoleAnimation);
		else
		{
			m_bConsoleAnimateOut = false;
			m_textbox->setVisible(false);
			m_fConsoleAnimation = 0.0f;
			m_textbox->setPosY(engine->getScreenHeight());
			anim->deleteExistingAnimation(&m_fConsoleAnimation);
		}
	}

	// handle suggestions
	m_suggestion->update();

	if (m_bSuggestionAnimateOut)
	{
		if (m_fSuggestionAnimation <= m_fSuggestionY)
		{
			m_suggestion->setPosY(engine->getScreenHeight() - (m_fSuggestionY - m_fSuggestionAnimation));
			m_fSuggestionAnimation += cv::consolebox_animspeed.getFloat();
		}
		else
		{
			m_bSuggestionAnimateOut = false;
			m_fSuggestionAnimation = m_fSuggestionY;
			m_suggestion->setVisible(false);
			m_suggestion->setPosY(engine->getScreenHeight());
		}
	}

	if (m_bSuggestionAnimateIn)
	{
		if (m_fSuggestionAnimation >= 0)
		{
			m_suggestion->setPosY(engine->getScreenHeight() - (m_fSuggestionY - m_fSuggestionAnimation));
			m_fSuggestionAnimation -= cv::consolebox_animspeed.getFloat();
		}
		else
		{
			m_bSuggestionAnimateIn = false;
			m_fSuggestionAnimation = 0.0f;
			m_suggestion->setPosY(engine->getScreenHeight() - m_fSuggestionY);
		}
	}

	if (mleft && !m_suggestion->isMouseInside() && !m_textbox->isActive() && !m_suggestion->isBusy())
		m_suggestion->setVisible(false);

	if (m_textbox->isActive() && mleft && m_textbox->isMouseInside() && m_iSuggestionCount > 0)
		m_suggestion->setVisible(true);

	// handle overlay animation and timeout
	// theres probably a better way to do it than yet another atomic boolean, but eh
	bool forceVisible = m_bForceLogVisible.exchange(false);
	if (!forceVisible && engine->getTime() > m_fLogTime)
	{
		if (!anim->isAnimating(&m_fLogYPos) && m_fLogYPos == 0.0f)
			anim->moveQuadInOut(&m_fLogYPos, m_logFont->getHeight() * (cv::console_overlay_lines.getFloat() + 1), 0.5f);

		if (m_fLogYPos == m_logFont->getHeight() * (cv::console_overlay_lines.getInt() + 1))
		{
			std::scoped_lock logGuard(m_logMutex);
			m_log.clear();
		}
	}
}

void ConsoleBox::processPendingLogAnimations()
{
	// check if we have pending animation reset from logging thread
	if (m_bLogAnimationResetPending.exchange(false))
	{
		// execute animation operations on main thread only
		anim->deleteExistingAnimation(&m_fLogYPos);
		m_fLogYPos = 0;
		m_fLogTime = m_fPendingLogTime.load();
	}
}

void ConsoleBox::onSuggestionClicked(CBaseUIButton *suggestion)
{
	UString text = suggestion->getName();

	ConVar *temp = ConVar::getConVarByName(text, false);
	if (temp != NULL && (temp->hasValue() || temp->hasCallbackArgs()))
		text.append(" ");

	m_textbox->setSuggestion("");
	m_textbox->setText(text);
	m_textbox->setCursorPosRight();
	m_textbox->setActive(true);
}

void ConsoleBox::onKeyDown(KeyboardEvent &e)
{
	// toggle visibility
	if ((e == KEY_F1 && (m_textbox->isActive() && m_textbox->isVisible() && !m_bConsoleAnimateOut ? true : keyboard->isShiftDown())) ||
	    (m_textbox->isActive() && m_textbox->isVisible() && !m_bConsoleAnimateOut && (e == KEY_ESCAPE || (Env::cfg(OS::WASM) && e == KEY_TILDE))))
		toggle(e);

	if (m_bConsoleAnimateOut)
		return;

	// textbox
	m_textbox->onKeyDown(e);

	// suggestion + command history hotkey handling
	if (m_iSuggestionCount > 0 && m_textbox->isActive() && m_textbox->isVisible())
	{
		// handle suggestion up/down buttons

		if (e == KEY_DOWN || (e == KEY_TAB && !keyboard->isShiftDown()))
		{
			if (m_iSelectedSuggestion < 1)
				m_iSelectedSuggestion = m_iSuggestionCount - 1;
			else
				m_iSelectedSuggestion--;

			if (m_iSelectedSuggestion > -1 && m_iSelectedSuggestion < m_vSuggestionButtons.size())
			{
				UString command = m_vSuggestionButtons[m_iSelectedSuggestion]->getName();

				ConVar *temp = ConVar::getConVarByName(command, false);
				if (temp != NULL && (temp->hasValue() || temp->hasCallbackArgs()))
					command.append(" ");

				m_textbox->setSuggestion("");
				m_textbox->setText(command);
				m_textbox->setCursorPosRight();
				m_suggestion->scrollToElement(m_vSuggestionButtons[m_iSelectedSuggestion]);

				for (size_t i = 0; i < m_vSuggestionButtons.size(); i++)
				{
					if (std::cmp_equal(i, m_iSelectedSuggestion))
					{
						m_vSuggestionButtons[i]->setTextColor(0xff00ff00);
						m_vSuggestionButtons[i]->setTextDarkColor(0xff000000);
					}
					else
						m_vSuggestionButtons[i]->setTextColor(0xffffffff);
				}
			}

			e.consume();
		}
		else if (e == KEY_UP || (e == KEY_TAB && keyboard->isShiftDown()))
		{
			if (m_iSelectedSuggestion > m_iSuggestionCount - 2)
				m_iSelectedSuggestion = 0;
			else
				m_iSelectedSuggestion++;

			if (m_iSelectedSuggestion > -1 && m_iSelectedSuggestion < m_vSuggestionButtons.size())
			{
				UString command = m_vSuggestionButtons[m_iSelectedSuggestion]->getName();

				ConVar *temp = ConVar::getConVarByName(command, false);
				if (temp != NULL && (temp->hasValue() || temp->hasCallbackArgs()))
					command.append(" ");

				m_textbox->setSuggestion("");
				m_textbox->setText(command);
				m_textbox->setCursorPosRight();
				m_suggestion->scrollToElement(m_vSuggestionButtons[m_iSelectedSuggestion]);

				for (size_t i = 0; i < m_vSuggestionButtons.size(); i++)
				{
					if (std::cmp_equal(i, m_iSelectedSuggestion))
					{
						m_vSuggestionButtons[i]->setTextColor(0xff00ff00);
						m_vSuggestionButtons[i]->setTextDarkColor(0xff000000);
					}
					else
						m_vSuggestionButtons[i]->setTextColor(0xffffffff);
				}
			}

			e.consume();
		}
	}
	else if (m_commandHistory.size() > 0 && m_textbox->isActive() && m_textbox->isVisible())
	{
		// handle command history up/down buttons

		if (e == KEY_DOWN)
		{
			if (m_iSelectedHistory > m_commandHistory.size() - 2)
				m_iSelectedHistory = 0;
			else
				m_iSelectedHistory++;

			if (m_iSelectedHistory > -1 && m_iSelectedHistory < m_commandHistory.size())
			{
				UString text = m_commandHistory[m_iSelectedHistory];
				m_textbox->setSuggestion("");
				m_textbox->setText(text);
				m_textbox->setCursorPosRight();
			}

			e.consume();
		}
		else if (e == KEY_UP)
		{
			if (m_iSelectedHistory < 1)
				m_iSelectedHistory = m_commandHistory.size() - 1;
			else
				m_iSelectedHistory--;

			if (m_iSelectedHistory > -1 && m_iSelectedHistory < m_commandHistory.size())
			{
				UString text = m_commandHistory[m_iSelectedHistory];
				m_textbox->setSuggestion("");
				m_textbox->setText(text);
				m_textbox->setCursorPosRight();
			}

			e.consume();
		}
	}
}

void ConsoleBox::onChar(KeyboardEvent &e)
{
	if (m_bConsoleAnimateOut && !m_bConsoleAnimateIn)
		return;
	if (e == KEY_TAB)
		return;

	m_textbox->onChar(e);

	if (m_textbox->isActive() && m_textbox->isVisible())
	{
		// rebuild suggestion list
		clearSuggestions();

		std::vector<ConVar *> suggestions = ConVar::getConVarByLetter(m_textbox->getText());
		for (size_t i = 0; i < suggestions.size(); i++)
		{
			UString suggestionText = suggestions[i]->getName();

			if (suggestions[i]->hasValue())
			{
				switch (suggestions[i]->getType())
				{
				case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
					suggestionText.append(UString::format(" %i", (int)suggestions[i]->getBool()));
					// suggestionText.append(UString::format(" ( def. \"%i\" )", (int)(suggestions[i]->getDefaultFloat() > 0)));
					break;
				case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
					suggestionText.append(UString::format(" %i", suggestions[i]->getInt()));
					// suggestionText.append(UString::format(" ( def. \"%i\" )", (int)suggestions[i]->getDefaultFloat()));
					break;
				case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
					suggestionText.append(UString::format(" %g", suggestions[i]->getFloat()));
					// suggestionText.append(UString::format(" ( def. \"%g\" )", suggestions[i]->getDefaultFloat()));
					break;
				case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING:
					suggestionText.append(" ");
					suggestionText.append(suggestions[i]->getString());
					// suggestionText.append(" ( def. \"");
					// suggestionText.append(suggestions[i]->getDefaultString());
					// suggestionText.append("\" )");
					break;
				}
			}

			addSuggestion(suggestionText, suggestions[i]->getHelpstring(), suggestions[i]->getName());
		}
		m_suggestion->setVisible(suggestions.size() > 0);

		if (suggestions.size() > 0)
		{
			m_suggestion->scrollToElement(m_suggestion->getContainer()->getElements()[0]);
			m_textbox->setSuggestion(suggestions[0]->getName());
		}
		else
			m_textbox->setSuggestion("");

		m_iSelectedSuggestion = -1;
	}
}

void ConsoleBox::onResolutionChange(Vector2 newResolution)
{
	const float dpiScale = getDPIScale();

	m_textbox->setSize(newResolution.x - 10 * dpiScale, m_textbox->getRelSize().y * dpiScale);
	m_textbox->setPos(5 * dpiScale, m_textbox->isVisible() ? newResolution.y - m_textbox->getSize().y - 6 * dpiScale : newResolution.y);

	m_suggestion->setPos(5 * dpiScale, newResolution.y - m_fSuggestionY);
	m_suggestion->setSizeX(newResolution.x - 10 * dpiScale);
}

void ConsoleBox::processCommand(const UString& command)
{
	clearSuggestions();
	m_iSelectedHistory = -1;

	if (command.length() > 0)
		m_commandHistory.push_back(command);

	Console::processCommand(command);
}

void ConsoleBox::execConfigFile(UString filename)
{
	Console::execConfigFile(std::move(filename));
}

bool ConsoleBox::isBusy()
{
	return (m_textbox->isBusy() || m_suggestion->isBusy()) && m_textbox->isVisible();
}

bool ConsoleBox::isActive()
{
	return (m_textbox->isActive() || m_suggestion->isActive()) && m_textbox->isVisible();
}

void ConsoleBox::addSuggestion(const UString &text, const UString &helpText, const UString &command)
{
	const float dpiScale = getDPIScale();

	const int vsize = m_vSuggestionButtons.size() + 1;
	const int bottomAdd = 3 * dpiScale;
	const int buttonheight = 22 * dpiScale;
	const int addheight = (17 + 8) * dpiScale;

	// create button and add it
	CBaseUIButton *button = new ConsoleBoxSuggestionButton(3 * dpiScale, (vsize - 1) * buttonheight + 2 * dpiScale, 100, addheight, command, text, helpText, this);
	{
		button->setDrawFrame(false);
		button->setSizeX(button->getFont()->getStringWidth(text));
		button->setClickCallback(SA::MakeDelegate<&ConsoleBox::onSuggestionClicked>(this));
		button->setDrawBackground(false);
	}
	m_suggestion->getContainer()->addBaseUIElement(button);
	m_vSuggestionButtons.insert(m_vSuggestionButtons.begin(), button);

	// update suggestion size
	const int gap = 10 * dpiScale;
	m_fSuggestionY = std::clamp<float>(buttonheight * vsize, 0, buttonheight * 4) + (engine->getScreenHeight() - m_textbox->getPos().y) + gap;

	if (buttonheight * vsize > buttonheight * 4)
	{
		m_suggestion->setSizeY(buttonheight * 4 + bottomAdd);
		m_suggestion->setScrollSizeToContent();
	}
	else
	{
		m_suggestion->setSizeY(buttonheight * vsize + bottomAdd);
		m_suggestion->setScrollSizeToContent();
	}

	m_suggestion->setPosY(engine->getScreenHeight() - m_fSuggestionY);

	m_iSuggestionCount++;
}

void ConsoleBox::clearSuggestions()
{
	m_iSuggestionCount = 0;
	m_suggestion->getContainer()->clear();
	m_vSuggestionButtons = std::vector<CBaseUIButton *>();
	m_suggestion->setVisible(false);
}

void ConsoleBox::show()
{
	if (!m_textbox->isVisible())
	{
		KeyboardEvent fakeEvent(KEY_F1);
		toggle(fakeEvent);
	}
}

void ConsoleBox::toggle(KeyboardEvent &e)
{
	if (m_textbox->isVisible() && !m_bConsoleAnimateIn && !m_bSuggestionAnimateIn)
	{
		m_bConsoleAnimateOut = true;
		anim->moveSmoothEnd(&m_fConsoleAnimation, 0, 2.0f * 0.8f);

		if (m_suggestion->getContainer()->getElements().size() > 0)
			m_bSuggestionAnimateOut = true;

		e.consume();
	}
	else if (!m_bConsoleAnimateOut && !m_bSuggestionAnimateOut)
	{
		m_textbox->setVisible(true);
		m_textbox->setActive(true);
		m_textbox->setBusy(true);
		m_bConsoleAnimateIn = true;

		anim->moveSmoothEnd(&m_fConsoleAnimation, getAnimTargetY(), 1.5f * 0.6f);

		if (m_suggestion->getContainer()->getElements().size() > 0)
		{
			m_bSuggestionAnimateIn = true;
			m_suggestion->setVisible(true);
		}

		e.consume();
	}

	// HACKHACK: force layout update
	onResolutionChange(engine->getScreenSize());
}

void ConsoleBox::log(UString text, Color textColor)
{
	std::scoped_lock logGuard(m_logMutex);

	// remove illegal chars
	{
		int newline = text.find("\n", 0);
		while (newline != -1)
		{
			text.erase(newline, 1);
			newline = text.find("\n", 0);
		}
	}

	// add log entry
	{
		LOG_ENTRY logEntry;
		{
			logEntry.text = text;
			logEntry.textColor = textColor;
		}
		m_log.push_back(logEntry);
	}

	while (m_log.size() > cv::console_overlay_lines.getInt())
	{
		m_log.erase(m_log.begin());
	}

	// defer animation operations to main thread to avoid data races
	// use force visibility flag to prevent immediate timeout on same frame (this is so dumb)
	m_fPendingLogTime.store(Timing::getTimeReal<float>() + 8.0f);
	m_bForceLogVisible.store(true);
	m_bLogAnimationResetPending.store(true);
}

float ConsoleBox::getAnimTargetY()
{
	return 32.0f * getDPIScale();
}

float ConsoleBox::getDPIScale()
{
	return ((float)std::max(env->getDPI(), m_textbox->getFont()->getDPI()) / 96.0f); // NOTE: abusing font dpi
}
