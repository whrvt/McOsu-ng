//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		console variables
//
// $NoKeywords: $convar
//===============================================================================//

#pragma once
#ifndef CONVAR_H
#define CONVAR_H

#include "cbase.h"

#include <concepts>

class ConVars
{
public:
	// shared engine-wide cross-game convars for convenience access (for convars which don't fit anywhere else)
	static ConVar sv_cheats;
};

#define FCVAR_NONE				0		// the default, no flags at all
#define FCVAR_UNREGISTERED		(1<<0)	// not added to g_vConVars list (not settable/gettable via console/help), not visible in find/listcommands results etc.
#define FCVAR_DEVELOPMENTONLY	(1<<1)	// hidden in released products (like FCVAR_HIDDEN, but flag gets compiled out if ALLOW_DEVELOPMENT_CONVARS is defined)
#define FCVAR_HARDCODED			(1<<2)	// if this is set then the value can't and shouldn't ever be changed
#define FCVAR_HIDDEN			(1<<3)	// not visible in find/listcommands results etc., but is settable/gettable via console/help
#define FCVAR_CHEAT				(1<<4)	// always return default value and ignore callbacks unless sv_cheats is enabled (default value is changeable via setDefault*())

class ConVar
{
public:
	enum class CONVAR_TYPE : uint8_t
	{
		CONVAR_TYPE_BOOL,
		CONVAR_TYPE_INT,
		CONVAR_TYPE_FLOAT,
		CONVAR_TYPE_STRING
	};

	// raw callbacks
	typedef void (*ConVarCallback)(void);
	typedef void (*ConVarChangeCallback)(UString oldValue, UString newValue);
	typedef void (*ConVarCallbackArgs)(UString args);
	typedef void (*ConVarCallbackFloat)(float args);

	// delegate callbacks
	typedef fastdelegate::FastDelegate0<> NativeConVarCallback;
	typedef fastdelegate::FastDelegate1<UString> NativeConVarCallbackArgs;
	typedef fastdelegate::FastDelegate1<float> NativeConVarCallbackFloat;
	typedef fastdelegate::FastDelegate2<UString, UString> NativeConVarChangeCallback;

public:
	static UString typeToString(CONVAR_TYPE type);

private:
	[[nodiscard]] constexpr auto getRaw() const {return m_fValue.load();} // forward def

public:
	~ConVar();
	explicit ConVar(UString name);

	explicit ConVar(UString name, int flags, ConVarCallback callback);
	explicit ConVar(UString name, int flags, const char *helpString, ConVarCallback callback);

	explicit ConVar(UString name, int flags, ConVarCallbackArgs callbackARGS);
	explicit ConVar(UString name, int flags, const char *helpString, ConVarCallbackArgs callbackARGS);

	explicit ConVar(UString name, int flags, ConVarCallbackFloat callbackFLOAT);
	explicit ConVar(UString name, int flags, const char *helpString, ConVarCallbackFloat callbackFLOAT);

	explicit ConVar(UString name, float defaultValue, int flags);
	explicit ConVar(UString name, float defaultValue, int flags, ConVarChangeCallback callback);
	explicit ConVar(UString name, float defaultValue, int flags, const char *helpString);
	explicit ConVar(UString name, float defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

	explicit ConVar(UString name, int defaultValue, int flags);
	explicit ConVar(UString name, int defaultValue, int flags, ConVarChangeCallback callback);
	explicit ConVar(UString name, int defaultValue, int flags, const char *helpString);
	explicit ConVar(UString name, int defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

	explicit ConVar(UString name, bool defaultValue, int flags);
	explicit ConVar(UString name, bool defaultValue, int flags, ConVarChangeCallback callback);
	explicit ConVar(UString name, bool defaultValue, int flags, const char *helpString);
	explicit ConVar(UString name, bool defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

	explicit ConVar(UString name, const char *defaultValue, int flags);
	explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString);
	explicit ConVar(UString name, const char *defaultValue, int flags, ConVarChangeCallback callback);
	explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

	// callbacks
	void exec();
	void execArgs(UString args);
	void execInt(float args);

	// get
	template <typename T = int>
	[[nodiscard]] constexpr auto getDefaultVal() const {return static_cast<T>(m_fDefaultValue.load());}
	[[nodiscard]] constexpr float getDefaultFloat() const {return getDefaultVal<float>();}
	[[nodiscard]] constexpr const UString &getDefaultString() const {return m_sDefaultValue;}

	[[nodiscard]] constexpr bool isFlagSet(int flag) const {return (bool)(m_iFlags & flag);}

	template <typename T = int>
	[[nodiscard]] constexpr auto getVal(bool cheat = false) const {return static_cast<T>((!cheat || !!static_cast<int>(ConVars::sv_cheats.getRaw())) ? m_fValue.load() : m_fDefaultValue.load());}

	[[nodiscard]] constexpr int getInt() const	   	{return getVal<int>(isFlagSet(FCVAR_CHEAT));}
	[[nodiscard]] constexpr bool getBool() const	{return getVal<bool>(isFlagSet(FCVAR_CHEAT));}
	[[nodiscard]] constexpr float getFloat() const	{return	getVal<float>(isFlagSet(FCVAR_CHEAT));}

	[[nodiscard]]
	constexpr const UString &getString() const		 {return (isFlagSet(FCVAR_CHEAT) && !static_cast<int>(ConVars::sv_cheats.getRaw()) ? m_sDefaultValue : m_sValue);}

	[[nodiscard]] constexpr const UString &getHelpstring() const {return m_sHelpString;}
	[[nodiscard]] constexpr const UString &getName() const {return m_sName;}
	[[nodiscard]] constexpr CONVAR_TYPE getType() const {return m_type;}
	[[nodiscard]] constexpr int getFlags() const {return m_iFlags;}

	[[nodiscard]] constexpr bool hasValue() const {return m_bHasValue;}
	[[nodiscard]] constexpr bool hasCallbackArgs() const {return (m_callbackfuncargs || m_changecallback);}

	// set
	void setDefaultFloat(float defaultValue);
	void setDefaultString(UString defaultValue);

	template <typename T>
		requires (std::is_same_v<T, float> || std::convertible_to<T, float>)
	constexpr void setValue(T value)
	{
		if (isFlagSet(FCVAR_HARDCODED) || (isFlagSet(FCVAR_CHEAT) && !static_cast<int>(ConVars::sv_cheats.getRaw()))) return;
		setValueInt(value); // setValueInt(ernal)...
	}

	void setValue(UString sValue);

	void setCallback(NativeConVarCallback callback);
	void setCallback(NativeConVarCallbackArgs callback);
	void setCallback(NativeConVarCallbackFloat callback);
	void setCallback(NativeConVarChangeCallback callback);

	void setHelpString(UString helpString);

	// for restart support
	void resetCallbacks()
	{
		m_callbackfunc = NULL;
		m_callbackfuncargs = NULL;
		m_callbackfuncfloat = NULL;
		m_changecallback = NULL;
	}
private:
	void init(int flags);
	void init(UString &name, int flags);
	void init(UString &name, int flags, ConVarCallback callback);
	void init(UString &name, int flags, UString helpString, ConVarCallback callback);
	void init(UString &name, int flags, ConVarCallbackArgs callbackARGS);
	void init(UString &name, int flags, UString helpString, ConVarCallbackArgs callbackARGS);
	void init(UString &name, int flags, ConVarCallbackFloat callbackARGS);
	void init(UString &name, int flags, UString helpString, ConVarCallbackFloat callbackARGS);
	void init(UString &name, float defaultValue, int flags, UString helpString, ConVarChangeCallback callback);
	void init(UString &name, UString defaultValue, int flags, UString helpString, ConVarChangeCallback callback);

	void setDefaultFloatInt(float defaultValue);
	void setDefaultStringInt(UString defaultValue);

	void setValueInt(float value);
	void setValueInt(UString sValue);

private:
	bool m_bHasValue;
	CONVAR_TYPE m_type;
	int m_iFlags;

	UString m_sName;
	UString m_sHelpString;

	std::atomic<float> m_fValue;
	std::atomic<float> m_fDefaultValue;

	UString m_sValue;
	UString m_sDefaultValue;

	NativeConVarCallback m_callbackfunc;
	NativeConVarCallbackArgs m_callbackfuncargs;
	NativeConVarCallbackFloat m_callbackfuncfloat;
	NativeConVarChangeCallback m_changecallback;
};



//*******************//
//  Searching/Lists  //
//*******************//

class ConVarHandler
{
public:
	static UString flagsToString(int flags);

public:
	ConVarHandler();
	~ConVarHandler();

	const std::vector<ConVar*> &getConVarArray() const;
	int getNumConVars() const;

	ConVar *getConVarByName(UString name, bool warnIfNotFound = true) const;
	std::vector<ConVar*> getConVarByLetter(UString letters) const;

	void resetAllConVarCallbacks();
};

extern ConVarHandler *convar;

#endif
