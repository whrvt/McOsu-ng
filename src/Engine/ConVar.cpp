//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		console variables
//
// $NoKeywords: $convar
//===============================================================================//

#include "ConVar.h"

#include <algorithm>
#include <utility>

#include "Engine.h"
#include "File.h"
// #define ALLOW_DEVELOPMENT_CONVARS // NOTE: comment this out on release
namespace cv::ConVars
{
ConVar sv_cheats("sv_cheats", true, FCVAR_NONE);
}

static std::vector<ConVar *> &_getGlobalConVarArray()
{
	static std::vector<ConVar *> g_vConVars; // (singleton)
	return g_vConVars;
}

static std::unordered_map<std::string, ConVar *> &_getGlobalConVarMap()
{
	static std::unordered_map<std::string, ConVar *> g_vConVarMap; // (singleton)
	return g_vConVarMap;
}

void ConVar::addConVar(ConVar *c)
{
	if (c->isFlagSet(FCVAR_UNREGISTERED))
		return;

	if (_getGlobalConVarArray().size() < 1)
		_getGlobalConVarArray().reserve(1024);

	if (_getGlobalConVarMap().find(std::string(c->getName().toUtf8(), c->getName().lengthUtf8())) == _getGlobalConVarMap().end())
	{
		_getGlobalConVarArray().push_back(c);
		_getGlobalConVarMap()[std::string(c->getName().toUtf8(), c->getName().lengthUtf8())] = c;
	}
	else
	{
		debugLog("FATAL: Duplicate ConVar name (\"{:s}\")\n", c->getName().toUtf8());
		std::exit(100);
	}
}

static ConVar *_getConVar(const UString &name)
{
	const auto result = _getGlobalConVarMap().find(std::string(name.toUtf8(), name.lengthUtf8()));
	if (result != _getGlobalConVarMap().end())
		return result->second;
	else
		return NULL;
}

ConVar::~ConVar()
{
	if (!isFlagSet(FCVAR_UNREGISTERED))
	{
		std::vector<ConVar *> &conVarArray = _getGlobalConVarArray();
		std::unordered_map<std::string, ConVar *> &conVarMap = _getGlobalConVarMap();

		auto it = std::ranges::find(conVarArray, this);
		if (it != conVarArray.end())
			conVarArray.erase(it);

		std::string nameStr(m_sName.toUtf8(), m_sName.lengthUtf8());
		auto mapIt = conVarMap.find(nameStr);
		if (mapIt != conVarMap.end() && mapIt->second == this)
			conVarMap.erase(mapIt);
	}
}

UString ConVar::typeToString(CONVAR_TYPE type)
{
	switch (type)
	{
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
		return "bool";
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
		return "int";
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
		return "float";
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING:
		return "string";
	}

	return "";
}

void ConVar::initBase(int flags)
{
	m_fValue = 0.0f;
	m_fDefaultValue = 0.0f;

	m_bHasValue = true;
	m_type = CONVAR_TYPE::CONVAR_TYPE_FLOAT;
	m_iFlags = flags;

#ifdef ALLOW_DEVELOPMENT_CONVARS
	m_iFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif

	// m_callback/m_changeCallback are default-init to std::monostate (i.e. nothing)
}

// command-only constructor
ConVar::ConVar(UString name)
{
	initBase(FCVAR_NONE);
	m_sName = std::move(name);
	m_bHasValue = false;
	m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
	m_iFlags = FCVAR_NONE;
	ConVar::addConVar(this);
}

void ConVar::exec()
{
	if (isFlagSet(FCVAR_CHEAT) && !(cv::ConVars::sv_cheats.getRaw() > 0))
		return;

	if (auto *cb = std::get_if<NativeConVarCallback>(&m_callback))
		(*cb)();
}

void ConVar::execArgs(UString args)
{
	if (isFlagSet(FCVAR_CHEAT) && !(cv::ConVars::sv_cheats.getRaw() > 0))
		return;

	if (auto *cb = std::get_if<NativeConVarCallbackArgs>(&m_callback))
		(*cb)(std::move(args));
}

void ConVar::execInt(float args)
{
	if (isFlagSet(FCVAR_CHEAT) && !(cv::ConVars::sv_cheats.getRaw() > 0))
		return;

	if (auto *cb = std::get_if<NativeConVarCallbackFloat>(&m_callback))
		(*cb)(args);
}

void ConVar::setDefaultFloat(float defaultValue)
{
	if (isFlagSet(FCVAR_HARDCODED))
		return;

	setDefaultFloatInt(defaultValue);
}

void ConVar::setDefaultFloatInt(float defaultValue)
{
	m_fDefaultValue = defaultValue;
	m_sDefaultValue = UString::format("%g", defaultValue);
}

void ConVar::setDefaultString(UString defaultValue)
{
	if (isFlagSet(FCVAR_HARDCODED))
		return;

	setDefaultStringInt(std::move(defaultValue));
}

void ConVar::setDefaultStringInt(UString defaultValue)
{
	m_sDefaultValue = std::move(defaultValue);
}

void ConVar::setHelpString(UString helpString)
{
	m_sHelpString = std::move(helpString);
}

bool ConVar::hasCallbackArgs() const
{
	return std::holds_alternative<NativeConVarCallbackArgs>(m_callback) || !std::holds_alternative<std::monostate>(m_changeCallback);
}

void ConVar::resetCallbacks()
{
	m_callback = std::monostate{};
	m_changeCallback = std::monostate{};
}

//********************************//
//  ConVarHandler Implementation  //
//********************************//

namespace cv
{
ConVar emptyDummyConVar("emptyDummyConVar", 42.0f, FCVAR_NONE, "this placeholder convar is returned by convar->getConVarByName() if no matching convar is found");
}

ConVarHandler *convar = new ConVarHandler();

ConVarHandler::ConVarHandler()
{
	convar = this;
}

ConVarHandler::~ConVarHandler()
{
	convar = NULL;
}

const std::vector<ConVar *> &ConVarHandler::getConVarArray() const
{
	return _getGlobalConVarArray();
}

size_t ConVarHandler::getNumConVars() const
{
	return _getGlobalConVarArray().size();
}

ConVar *ConVarHandler::getConVarByName(const UString &name, bool warnIfNotFound) const
{
	ConVar *found = _getConVar(name);
	if (found != NULL)
		return found;

	if (warnIfNotFound)
	{
		debugLog(R"(ENGINE: ConVar "{}" does not exist...)"
		         "\n",
		         name.toUtf8());
		engine->showMessageWarning("Engine Error", UString::format(R"(ENGINE: ConVar "%s" does not exist...)"
		                                                           "\n",
		                                                           name.toUtf8()));
	}

	if (!warnIfNotFound)
		return NULL;
	else
		return &cv::emptyDummyConVar;
}

std::vector<ConVar *> ConVarHandler::getConVarByLetter(const UString &letters) const
{
	std::unordered_set<std::string> matchingConVarNames;
	std::vector<ConVar *> matchingConVars;
	{
		if (letters.length() < 1)
			return matchingConVars;

		const std::vector<ConVar *> &convars = getConVarArray();

		// first try matching exactly
		for (auto convar : convars)
		{
			if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
				continue;

			if (convar->getName().find(letters, 0, letters.length()) == 0)
			{
				if (letters.length() > 1)
					matchingConVarNames.insert(std::string(convar->getName().toUtf8(), convar->getName().lengthUtf8()));

				matchingConVars.push_back(convar);
			}
		}

		// then try matching substrings
		if (letters.length() > 1)
		{
			for (auto convar : convars)
			{
				if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
					continue;

				if (convar->getName().find(letters) != -1)
				{
					std::string stdName(convar->getName().toUtf8(), convar->getName().lengthUtf8());
					if (matchingConVarNames.find(stdName) == matchingConVarNames.end())
					{
						matchingConVarNames.insert(stdName);
						matchingConVars.push_back(convar);
					}
				}
			}
		}

		// (results should be displayed in vector order)
	}
	return matchingConVars;
}

UString ConVarHandler::flagsToString(int flags)
{
	UString string;
	{
		if (flags == FCVAR_NONE)
			string.append("no flags");
		else
		{
			if (flags & FCVAR_UNREGISTERED)
				string.append(string.length() > 0 ? " unregistered" : "unregistered");
			if (flags & FCVAR_DEVELOPMENTONLY)
				string.append(string.length() > 0 ? " developmentonly" : "developmentonly");
			if (flags & FCVAR_HARDCODED)
				string.append(string.length() > 0 ? " hardcoded" : "hardcoded");
			if (flags & FCVAR_HIDDEN)
				string.append(string.length() > 0 ? " hidden" : "hidden");
			if (flags & FCVAR_CHEAT)
				string.append(string.length() > 0 ? " cheat" : "cheat");
		}
	}
	return string;
}

void ConVarHandler::resetAllConVarCallbacks()
{
	const std::vector<ConVar *> &convars = getConVarArray();
	for (auto convar : convars)
	{
		convar->resetCallbacks();
	}
}

//*****************************//
//	ConVarHandler ConCommands  //
//*****************************//

static void _find(UString args)
{
	if (args.length() < 1)
	{
		debugLog("Usage:  find <string>");
		return;
	}

	const std::vector<ConVar *> &convars = convar->getConVarArray();

	std::vector<ConVar *> matchingConVars;
	for (auto convar : convars)
	{
		if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
			continue;

		const UString name = convar->getName();
		if (name.find(args, 0, name.length()) != -1)
			matchingConVars.push_back(convar);
	}

	if (matchingConVars.size() > 0)
		std::ranges::sort(matchingConVars, [](const ConVar *var1, const ConVar *var2) -> bool { return (var1->getName() < var2->getName()); });

	if (matchingConVars.size() < 1)
	{
		UString thelog = "No commands found containing \"";
		thelog.append(args);
		thelog.append("\".\n");
		debugLog("{:s}", thelog.toUtf8());
		return;
	}

	debugLog("----------------------------------------------\n");
	{
		UString thelog = "[ find : ";
		thelog.append(args);
		thelog.append(" ]\n");
		debugLog("{:s}", thelog.toUtf8());

		for (auto &matchingConVar : matchingConVars)
		{
			UString tstring = matchingConVar->getName();
			tstring.append("\n");
			debugLog("{:s}", tstring.toUtf8());
		}
	}
	debugLog("----------------------------------------------\n");
}

static void _help(UString args)
{
	args = args.trim();

	if (args.length() < 1)
	{
		debugLog("Usage:  help <cvarname>\n");
		debugLog("To get a list of all available commands, type \"listcommands\".\n");
		return;
	}

	const std::vector<ConVar *> matches = convar->getConVarByLetter(args);

	if (matches.size() < 1)
	{
		UString thelog = "ConVar \"";
		thelog.append(args);
		thelog.append("\" does not exist.\n");
		debugLog("{:s}", thelog.toUtf8());
		return;
	}

	// use closest match
	size_t index = 0;
	for (size_t i = 0; i < matches.size(); i++)
	{
		if (matches[i]->getName() == args)
		{
			index = i;
			break;
		}
	}
	const ConVar *match = matches[index];

	if (match->getHelpstring().length() < 1)
	{
		UString thelog = "ConVar \"";
		thelog.append(match->getName());
		thelog.append("\" does not have a helpstring.\n");
		debugLog("{:s}", thelog.toUtf8());
		return;
	}

	UString thelog = match->getName();
	{
		if (match->hasValue())
		{
			thelog.append(UString::format(" = %s ( def. \"%s\" , ", match->getString().toUtf8(), match->getDefaultString().toUtf8()));
			thelog.append(ConVar::typeToString(match->getType()));
			thelog.append(", ");
			thelog.append(ConVarHandler::flagsToString(match->getFlags()));
			thelog.append(" )");
		}

		thelog.append(" - ");
		thelog.append(match->getHelpstring());
	}
	debugLog("{:s}", thelog.toUtf8());
}

static void _listcommands(void)
{
	debugLog("----------------------------------------------\n");
	{
		std::vector<ConVar *> convars = convar->getConVarArray();
		std::ranges::sort(convars, [](const ConVar *var1, const ConVar *var2) -> bool { return (var1->getName() < var2->getName()); });

		for (auto &convar : convars)
		{
			if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
				continue;

			const ConVar *var = convar;

			UString tstring = var->getName();
			{
				if (var->hasValue())
				{
					tstring.append(UString::format(" = %s ( def. \"%s\" , ", var->getString().toUtf8(), var->getDefaultString().toUtf8()));
					tstring.append(ConVar::typeToString(var->getType()));
					tstring.append(", ");
					tstring.append(ConVarHandler::flagsToString(var->getFlags()));
					tstring.append(" )");
				}

				if (var->getHelpstring().length() > 0)
				{
					tstring.append(" - ");
					tstring.append(var->getHelpstring());
				}

				tstring.append("\n");
			}
			debugLog("{:s}", tstring.toUtf8());
		}
	}
	debugLog("----------------------------------------------\n");
}

UString ConVar::getFancyDefaultValue() const
{
	switch (getType())
	{
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
		return m_fDefaultValue == 0 ? "false" : "true";
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
		return UString::fmt("{:d}", static_cast<int>(m_fDefaultValue.load()));
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
		return UString::fmt("{:.4f}", m_fDefaultValue.load());
	case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING: {
		return UString::fmt("\"{:s}\"", m_sDefaultValue);
	}
	}

	return "unreachable";
}

static void _dumpcommands(void)
{
	std::vector<ConVar *> convars = convar->getConVarArray();
	std::ranges::sort(convars, [](const ConVar *var1, const ConVar *var2) -> bool { return (var1->getName() < var2->getName()); });
	{
		McFile commands_htm("commands.htm", McFile::TYPE::WRITE);
		if (!commands_htm.canWrite())
		{
			debugLog("Failed to open commands.htm for writing\n");
			return;
		}

		for (auto var : convars)
		{
			if (!commands_htm.writeLine(UString::fmt("<h4>{:s}</h4>{:s}<pre>\n{{\n\t\"default\": {:s}\n\t\"runtime_allocated\": {:s}\n}}\n</pre>", var->getName(),
			                                         var->getHelpstring(), var->getFancyDefaultValue(), var->isFlagSet(FCVAR_DYNAMIC) ? "true" : "false")))
			{
				debugLog("failed to write var: {:s}, not writing out any more commands\n", var->getName());
				break;
			}
		}
	}
	debugLog("Commands dumped to commands.htm\n");
}

namespace cv
{
ConVar find("find", FCVAR_NONE, _find);
ConVar help("help", FCVAR_NONE, _help);
ConVar listcommands("listcommands", FCVAR_NONE, _listcommands);
ConVar dumpcommands("dumpcommands", FCVAR_NONE, _dumpcommands);
} // namespace cv
