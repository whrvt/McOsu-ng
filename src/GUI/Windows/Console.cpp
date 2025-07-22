//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		console window, also handles ConVar parsing
//
// $NoKeywords: $con
//===============================================================================//

#include "Console.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"

#include "CBaseUIContainer.h"
#include "CBaseUITextbox.h"
#include "CBaseUIScrollView.h"
#include "CBaseUITextField.h"
#include "CBaseUIButton.h"
#include "CBaseUILabel.h"

#include "File.h"

#include <mutex>

#include <utility>

#define CFG_FOLDER "cfg/"

#define CONSOLE_BORDER 6
namespace cv {
ConVar console_logging("console_logging", true, FCVAR_NONE);
ConVar clear("clear");
}

std::vector<UString> Console::g_commandQueue;

std::mutex g_consoleLogMutex;

Console::Console() : CBaseUIWindow(350, 100, 620, 550, "Console")
{
	// convar bindings
	cv::clear.setCallback( SA::MakeDelegate<&Console::clear>(this) );

	// resources
	m_logFont = resourceManager->getFont("FONT_CONSOLE");
	McFont *textboxFont = resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE_TEXTBOX", 9.0f, false);
	McFont *titleFont = resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE_TITLE", 10.0f, false);

	// colors
	//Color frameColor = 0xff9a9a9a;
	Color brightColor = 0xffb7b7b7;
	Color darkColor = 0xff343434;
	Color backgroundColor = 0xff555555;
	Color windowBackgroundColor = 0xff7b7b7b;

	setTitleFont(titleFont);
	setDrawTitleBarLine(false);
	setDrawFrame(false);
	setRoundedRectangle(true);

	setBackgroundColor(windowBackgroundColor);

	//setFrameColor(frameColor);
	setFrameDarkColor(0xff9a9a9a);
	setFrameBrightColor(0xff8a8a8a);

	getCloseButton()->setBackgroundColor(0xffbababa);
	getCloseButton()->setDrawBackground(false);

	int textboxHeight = 20;

	// log scrollview
	m_log = new CBaseUIScrollView(CONSOLE_BORDER, 0, m_vSize.x - 2*CONSOLE_BORDER, m_vSize.y - getTitleBarHeight() - 2*CONSOLE_BORDER - textboxHeight - 1, "consolelog");
	m_log->setHorizontalScrolling(false);
	m_log->setVerticalScrolling(true);
	m_log->setBackgroundColor(backgroundColor);
	m_log->setFrameDarkColor(darkColor);
	m_log->setFrameBrightColor(brightColor);
	getContainer()->addBaseUIElement(m_log);

	/*
	m_newLog = new CBaseUITextField(CONSOLE_BORDER, 0, m_vSize.x - 2*CONSOLE_BORDER, m_vSize.y - getTitleBarHeight() - 2*CONSOLE_BORDER - textboxHeight - 1, "newconsolelog", "");
	m_newLog->setFont(m_logFont);
	m_newLog->setHorizontalScrolling(false);
	m_newLog->setVerticalScrolling(true);
	m_newLog->setBackgroundColor(backgroundColor);
	m_newLog->setFrameDarkColor(darkColor);
	m_newLog->setFrameBrightColor(brightColor);
	getContainer()->addBaseUIElement(m_newLog);
	*/

	// textbox
	m_textbox = new CBaseUITextbox(CONSOLE_BORDER, m_vSize.y - getTitleBarHeight() - textboxHeight - CONSOLE_BORDER, m_vSize.x - 2*CONSOLE_BORDER, textboxHeight, "consoletextbox");
	m_textbox->setText("");
	m_textbox->setFont(textboxFont);
	m_textbox->setBackgroundColor(backgroundColor);
	m_textbox->setFrameDarkColor(darkColor);
	m_textbox->setFrameBrightColor(brightColor);
	m_textbox->setCaretWidth(1);
	getContainer()->addBaseUIElement(m_textbox);

	// notify engine, exec autoexec
	engine->setConsole(this);
	Console::execConfigFile("autoexec.cfg");
}

Console::~Console()
{
}

void Console::processCommand(UString command)
{
	if (command.length() < 1) return;

	// remove empty space at beginning if it exists
	if (command.find(" ", 0, 1) != -1)
		command.erase(0, 1);

	// handle multiple commands separated by semicolons
	if (command.find(";") != -1 && command.find("echo") == -1)
	{
		const std::vector<UString> commands = command.split(";");
		for (const auto & command : commands)
		{
			processCommand(command);
		}

		return;
	}

	// separate convar name and value
	const std::vector<UString> tokens = command.split(" ");
	UString commandName;
	UString commandValue;
	for (size_t i=0; i<tokens.size(); i++)
	{
		if (i == 0)
			commandName = tokens[i];
		else
		{
			commandValue.append(tokens[i]);
			if (std::cmp_less(i ,(tokens.size()-1)))
				commandValue.append(" ");
		}
	}

	// get convar
	ConVar *var = ConVar::getConVarByName(commandName, false);
	if (var == NULL)
	{
		debugLog("Unknown command: {:s}\n", commandName.toUtf8());
		return;
	}

	if (!ALLOWCHEAT(var))
		return;

	// set new value (this handles all callbacks internally)
	// except for help, don't set a value for that, just run the callback
	if (commandValue.length() > 0)
	{
		if (commandName == "help")
			var->execArgs(commandValue);
		else
			var->setValue(commandValue);
	}
	else
	{
		var->exec();
		var->execArgs("");
		var->execFloat(var->getFloat());
	}

	// log
	if (cv::console_logging.getBool())
	{
		UString logMessage;

		bool doLog = false;
		if (commandValue.length() < 1)
		{
			doLog = var->hasValue(); // assume ConCommands never have helpstrings

			logMessage = commandName;

			if (var->hasValue())
			{
				logMessage.append(UString::fmt(" = {:s} ( def. \"{:s}\" , ", var->getString(), var->getDefaultString()));
				logMessage.append(ConVar::typeToString(var->getType()));
				logMessage.append(", ");
				logMessage.append(ConVar::flagsToString(var->getFlags()));
				logMessage.append(" )");
			}

			if (var->getHelpstring().length() > 0)
			{
				logMessage.append(" - ");
				logMessage.append(var->getHelpstring());
			}
		}
		else if (var->hasValue())
		{
			doLog = true;

			logMessage = commandName;
			logMessage.append(" : ");
			logMessage.append(var->getString());
		}

		if (logMessage.length() > 0 && doLog)
			debugLog("{:s}\n", logMessage.toUtf8());
	}
}

void Console::execConfigFile(UString filename)
{
	// handle extension
	filename.insert(0, CFG_FOLDER);
	if (filename.find(".cfg", (filename.length() - 4), filename.length()) == -1)
		filename.append(".cfg");

	McFile configFile(filename, McFile::TYPE::READ);
	if (!configFile.canRead())
	{
		debugLog("error, file \"{:s}\" not found!\n", filename.toUtf8());
		return;
	}

	// collect commands first (preserving original behavior)
	std::vector<UString> cmds;
	while (true)
	{
		UString line = configFile.readLine();

		// if canRead() is false after readLine(), we hit EOF
		if (!configFile.canRead())
			break;

		// only process non-empty lines (matching original: if (line.size() > 0))
		if (!line.isEmpty())
		{
			// handle comments - find "//" and remove everything after
			const int commentIndex = line.find("//");
			if (commentIndex != -1)
				line.erase(commentIndex, line.length() - commentIndex);

			// add command (original adds all processed lines, even if they become empty after comment removal)
			cmds.push_back(line);
		}
	}

	// process the collected commands
	for (const auto &cmd : cmds)
		processCommand(cmd);
}

void Console::update()
{
	CBaseUIWindow::update();
	if (!m_bVisible) return;

	// TODO: this needs proper callbacks in the textbox class
	if (m_textbox->hitEnter())
	{
		processCommand(m_textbox->getText());
		m_textbox->clear();
	}
}

void Console::log(UString text, Color textColor)
{
	std::lock_guard<std::mutex> lk(g_consoleLogMutex);

	if (text.length() < 1) return;

	// delete illegal characters
	int newline = text.find("\n", 0);
	while (newline != -1)
	{
		text.erase(newline, 1);
		newline = text.find("\n", 0);
	}

	// get index
	const int index = m_log->getContainer()->getElements().size();
	int height = 13;

	// create new label, add it
	CBaseUILabel *logEntry = new CBaseUILabel(3, height*index - 1, 150, height, text, text);
	logEntry->setDrawFrame(false);
	logEntry->setDrawBackground(false);
	logEntry->setTextColor(textColor);
	logEntry->setFont(m_logFont);
	logEntry->setSizeToContent(1, 4);
	m_log->getContainer()->addBaseUIElement(logEntry);

	// update scrollsize, scroll to bottom, clear textbox
	m_log->setScrollSizeToContent();
	m_log->scrollToBottom();

	///m_newLog->append(text);
	///m_newLog->scrollToBottom();
}

void Console::clear()
{
	m_log->clear();
	///m_newLog->clear();
}

void Console::onResized()
{
	CBaseUIWindow::onResized();

	m_log->setSize(m_vSize.x - 2*CONSOLE_BORDER, m_vSize.y - getTitleBarHeight() - 2*CONSOLE_BORDER - m_textbox->getSize().y - 1);
	///m_newLog->setSize(m_vSize.x - 2*CONSOLE_BORDER, m_vSize.y - getTitleBarHeight() - 2*CONSOLE_BORDER - m_textbox->getSize().y - 1);
	m_textbox->setSize(m_vSize.x - 2*CONSOLE_BORDER, m_textbox->getSize().y);
	m_textbox->setRelPosY(m_log->getRelPos().y + m_log->getSize().y + CONSOLE_BORDER + 1);

	m_log->scrollToY(m_log->getScrollPosY());
	//m_newLog->scrollY(m_newLog->getScrollPosY());
}



//***********************//
//	Console ConCommands  //
//***********************//

void _exec(const UString &args)
{
	Console::execConfigFile(args);
}

void _echo(const UString &args)
{
	if (args.length() > 0)
	{
		UString argsCopy{args};
		argsCopy.append("\n");
		debugLog("{:s}", argsCopy.toUtf8());
	}
}

void _fizzbuzz(void)
{
	for (int i=1; i<101; i++)
	{
		if (i % 3 == 0 && i % 5 == 0)
			debugLog("{} fizzbuzz\n",i);
		else if (i % 3 == 0)
			debugLog("{} fizz\n",i);
		else if (i % 5 == 0)
			debugLog("{} buzz\n",i);
		else
			debugLog("{}\n",i);
	}
}
namespace cv {
ConVar exec("exec", FCVAR_NONE, CFUNC(_exec));
ConVar echo("echo", FCVAR_NONE, CFUNC(_echo));
ConVar fizzbuzz("fizzbuzz", FCVAR_NONE, CFUNC(_fizzbuzz));
}
