//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		console variables
//
// $NoKeywords: $convar
//===============================================================================//

#include "ConVar.h"

#include <algorithm>

#include "Engine.h"
#include "File.h"
#include <mutex>

// #define ALLOW_DEVELOPMENT_CONVARS // NOTE: comment this out on release
namespace cv::ConVars
{
ConVar sv_cheats("sv_cheats", true, FCVAR_NONE);
}

namespace cv
{
ConVar emptyDummyConVar("emptyDummyConVar", 42.0f, FCVAR_NONE, "this placeholder convar is returned by ConVar::getConVarByName() if no matching convar is found");
}

// lazy init on first use
std::vector<ConVar *> &ConVar::getConVarArray()
{
	static std::once_flag reserved;
	static std::vector<ConVar *> _;

	std::call_once(reserved, []() { _.reserve(1024); });

	return _;
}

std::unordered_map<UString, ConVar *> &ConVar::getConVarMap()
{
	static std::unordered_map<UString, ConVar *> _;
	return _;
}

// public helpers
ConVar *ConVar::getConVarByName(const UString &name, bool warnIfNotFound)
{
	const auto it = ConVar::getConVarMap().find(name);
	if (it != ConVar::getConVarMap().end())
		return it->second;
	else
		return nullptr;

	if (warnIfNotFound)
	{
		debugLog(R"(ENGINE: ConVar "{:s}" does not exist...)"
		         "\n",
		         name);
		engine->showMessageWarning("Engine Error", UString::fmt(R"(ENGINE: ConVar "{:s}" does not exist...)"
		                                                        "\n",
		                                                        name));
	}

	if (!warnIfNotFound)
		return NULL;
	else
		return &cv::emptyDummyConVar;
}

std::vector<ConVar *> ConVar::getConVarByLetter(const UString &letters)
{
	std::unordered_set<UString> matchingConVarNames;
	std::vector<ConVar *> matchingConVars;
	{
		if (letters.length() < 1)
			return matchingConVars;

		const std::vector<ConVar *> &convars = ConVar::getConVarArray();

		// first try matching exactly
		int i = 0;
		for (auto convar : convars)
		{
			if (!convar)
			{
				debugLog("CONVAR {} IS NULL!\n", i);
				continue;
			}
			i++;
			if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
				continue;

			if (convar->getName().find(letters, 0, letters.length()) == 0)
			{
				if (letters.length() > 1)
					matchingConVarNames.insert(convar->getName());

				matchingConVars.push_back(convar);
			}
		}

		// then try matching substrings
		i = 0;
		if (letters.length() > 1)
		{
			for (auto convar : convars)
			{
				if (!convar)
				{
					debugLog("CONVAR {} IS NULL!\n", i);
					continue;
				}
				i++;
				if (convar->isFlagSet(FCVAR_HIDDEN) || convar->isFlagSet(FCVAR_DEVELOPMENTONLY))
					continue;

				if (convar->getName().find(letters) != -1)
				{
					if (matchingConVarNames.find(convar->getName()) == matchingConVarNames.end())
					{
						matchingConVarNames.insert(convar->getName());
						matchingConVars.push_back(convar);
					}
				}
			}
		}

		// (results should be displayed in vector order)
	}
	return matchingConVars;
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

UString ConVar::flagsToString(uint8_t flags)
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

void ConVar::resetAllConVarCallbacks()
{
	const std::vector<ConVar *> &convars = ConVar::getConVarArray();
	for (auto convar : convars)
	{
		convar->resetCallbacks();
	}
}

// private init helper
void ConVar::addConVar(ConVar *c)
{
	if (c->isFlagSet(FCVAR_UNREGISTERED))
		return;

	const UString &cvarName = c->getName();
	if (ConVar::getConVarMap().find(cvarName) == ConVar::getConVarMap().end())
	{
		ConVar::getConVarArray().push_back(c);
		ConVar::getConVarMap()[cvarName] = c;
	}
	else
	{
		debugLog("FATAL: Duplicate ConVar name (\"{:s}\")\n", cvarName);
		std::exit(100);
	}
}

ConVar::~ConVar()
{
	if (!isFlagSet(FCVAR_UNREGISTERED))
	{
		auto it = std::ranges::find(ConVar::getConVarArray(), this);
		if (it != ConVar::getConVarArray().end())
			ConVar::getConVarArray().erase(it);

		auto mapIt = ConVar::getConVarMap().find(m_sName);
		if (mapIt != ConVar::getConVarMap().end() && mapIt->second == this)
			ConVar::getConVarMap().erase(mapIt);
	}
}

void ConVar::exec()
{
	if (!ALLOWCHEAT(this))
		return;

	if (auto *cb = std::get_if<NativeConVarCallback>(&m_callback))
		(*cb)();
}

void ConVar::execArgs(const UString &args)
{
	if (!ALLOWCHEAT(this))
		return;

	if (auto *cb = std::get_if<NativeConVarCallbackArgs>(&m_callback))
		(*cb)(args);
}

void ConVar::execFloat(float args)
{
	if (!ALLOWCHEAT(this))
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

void ConVar::setDefaultString(const UString &defaultValue)
{
	if (isFlagSet(FCVAR_HARDCODED))
		return;

	setDefaultStringInt(defaultValue);
}

void ConVar::setDefaultStringInt(const UString &defaultValue)
{
	m_sDefaultValue = defaultValue;
}

void ConVar::setHelpString(const UString &helpString)
{
	m_sHelpString = helpString;
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

//*****************************//
//	ConVarHandler ConCommands  //
//*****************************//

static void _find(const UString &args)
{
	if (args.length() < 1)
	{
		Engine::logRaw("Usage:  find <string>\n");
		return;
	}

	const std::vector<ConVar *> &convars = ConVar::getConVarArray();

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
		Engine::logRaw("{:s}", thelog);
		return;
	}

	Engine::logRaw("----------------------------------------------\n");
	{
		UString thelog = "[ find : ";
		thelog.append(args);
		thelog.append(" ]\n");
		Engine::logRaw("{:s}", thelog);

		for (auto &matchingConVar : matchingConVars)
		{
			UString tstring = matchingConVar->getName();
			tstring.append("\n");
			Engine::logRaw("{:s}", tstring);
		}
	}
	Engine::logRaw("----------------------------------------------\n");
}

static void _help(const UString &args)
{
	const UString argsCopy{args.trim()};

	if (argsCopy.length() < 1)
	{
		Engine::logRaw("Usage:  help <cvarname>\n");
		Engine::logRaw("To get a list of all available commands, type \"listcommands\".\n");
		return;
	}

	const std::vector<ConVar *> matches = ConVar::getConVarByLetter(argsCopy);

	if (matches.size() < 1)
	{
		UString thelog = "ConVar \"";
		thelog.append(argsCopy);
		thelog.append("\" does not exist.\n");
		Engine::logRaw("{:s}", thelog);
		return;
	}

	// use closest match
	size_t index = 0;
	for (size_t i = 0; i < matches.size(); i++)
	{
		if (matches[i]->getName() == argsCopy)
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
		Engine::logRaw("{:s}", thelog);
		return;
	}

	UString thelog = match->getName();
	{
		if (match->hasValue())
		{
			thelog.append(UString::fmt(" = {:s} ( def. \"{:s}\" , ", match->getString(), match->getDefaultString()));
			thelog.append(ConVar::typeToString(match->getType()));
			thelog.append(", ");
			thelog.append(ConVar::flagsToString(match->getFlags()));
			thelog.append(" )");
		}

		thelog.append(" - ");
		thelog.append(match->getHelpstring());
	}
	Engine::logRaw("{:s}\n", thelog);
}

static void _listcommands(void)
{
	debugLog("----------------------------------------------\n");
	{
		std::vector<ConVar *> convars = ConVar::getConVarArray();
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
					tstring.append(UString::fmt(" = {:s} ( def. \"{:s}\" , ", var->getString(), var->getDefaultString()));
					tstring.append(ConVar::typeToString(var->getType()));
					tstring.append(", ");
					tstring.append(ConVar::flagsToString(var->getFlags()));
					tstring.append(" )");
				}

				if (var->getHelpstring().length() > 0)
				{
					tstring.append(" - ");
					tstring.append(var->getHelpstring());
				}

				tstring.append("\n");
			}
			debugLog("{:s}", tstring);
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
	std::vector<ConVar *> convars = ConVar::getConVarArray();
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
