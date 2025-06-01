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

#include <cassert>
#include <concepts>
#include <type_traits>
#include <variant>

#include "ConVarDefs.h"

#define FCVAR_NONE 0                   // the default, no flags at all
#define FCVAR_UNREGISTERED (1 << 0)    // not added to g_vConVars list (not settable/gettable via console/help), not visible in find/listcommands results etc.
#define FCVAR_DEVELOPMENTONLY (1 << 1) // hidden in released products (like FCVAR_HIDDEN, but flag gets compiled out if ALLOW_DEVELOPMENT_CONVARS is defined)
#define FCVAR_HARDCODED (1 << 2)       // if this is set then the value can't and shouldn't ever be changed
#define FCVAR_HIDDEN (1 << 3)          // not visible in find/listcommands results etc., but is settable/gettable via console/help
#define FCVAR_CHEAT (1 << 4)   // always return default value and ignore callbacks unless sv_cheats is enabled (default value is changeable via setDefault*())
#define FCVAR_DYNAMIC (1 << 5) // set for convars which are created at runtime with e.g. "new" instead of in the global scope

#define MAKENEWCONVAR(name, defaultValue, ...) new ConVar((name), (defaultValue), FCVAR_DYNAMIC __VA_OPT__(, ) __VA_ARGS__)

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

	// callback typedefs
	using NativeConVarCallback = fastdelegate::FastDelegate0<>;
	using NativeConVarCallbackArgs = fastdelegate::FastDelegate1<UString>;
	using NativeConVarChangeCallback = fastdelegate::FastDelegate2<UString, UString>;
	using NativeConVarCallbackFloat = fastdelegate::FastDelegate1<float>;
	using NativeConVarChangeCallbackFloat = fastdelegate::FastDelegate2<float, float>;

	// polymorphic callback storage
	using ExecutionCallback = std::variant<std::monostate,           // empty
	                                       NativeConVarCallback,     // void()
	                                       NativeConVarCallbackArgs, // void(UString)
	                                       NativeConVarCallbackFloat // void(float)
	                                       >;

	using ChangeCallback = std::variant<std::monostate,                 // empty
	                                    NativeConVarChangeCallback,     // void(UString, UString)
	                                    NativeConVarChangeCallbackFloat // void(float, float)
	                                    >;

public:
	static UString typeToString(CONVAR_TYPE type);

private:
	[[nodiscard]] constexpr auto getRaw() const { return m_fValue.load(); } // forward def

	// type detection helper
	template <typename T>
	static constexpr CONVAR_TYPE getTypeFor()
	{
		if constexpr (std::is_same_v<std::decay_t<T>, bool>)
			return CONVAR_TYPE::CONVAR_TYPE_BOOL;
		else if constexpr (std::is_integral_v<std::decay_t<T>>)
			return CONVAR_TYPE::CONVAR_TYPE_INT;
		else if constexpr (std::is_floating_point_v<std::decay_t<T>>)
			return CONVAR_TYPE::CONVAR_TYPE_FLOAT;
		else
			return CONVAR_TYPE::CONVAR_TYPE_STRING;
	}

	static void addConVar(ConVar *);

public:
	~ConVar();

	// command-only constructor
	explicit ConVar(UString name);

	// callback-only constructors (no value)
	template <typename Callback>
	explicit ConVar(UString name, int flags, Callback callback)
	    requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, UString> || std::is_invocable_v<Callback, float>
	{
		initCallback(name, flags, UString(""), callback);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(UString name, int flags, const char *helpString, Callback callback)
	    requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, UString> || std::is_invocable_v<Callback, float>
	{
		initCallback(name, flags, UString(helpString), callback);
		addConVar(this);
	}

	// value constructors handle all types uniformly
	template <typename T>
	explicit ConVar(UString name, T defaultValue, int flags, const char *helpString = "")
	    requires(!std::is_same_v<std::decay_t<T>, const char *>)
	{
		initValue(name, defaultValue, flags, UString(helpString), nullptr, nullptr);
		addConVar(this);
	}

	template <typename T, typename Callback>
	explicit ConVar(UString name, T defaultValue, int flags, const char *helpString, Callback callback)
	    requires(!std::is_same_v<std::decay_t<T>, const char *>) && (std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, UString(helpString), callback, nullptr);
		addConVar(this);
	}

	template <typename T, typename Callback>
	explicit ConVar(UString name, T defaultValue, int flags, Callback callback)
	    requires(!std::is_same_v<std::decay_t<T>, const char *>) && (std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, UString(""), callback, nullptr);
		addConVar(this);
	}

	// const char* specializations for string convars
	explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString = "")
	{
		initValue(name, UString(defaultValue), flags, UString(helpString), nullptr, nullptr);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString, Callback callback)
	    requires(std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, UString(defaultValue), flags, UString(helpString), callback, nullptr);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(UString name, const char *defaultValue, int flags, Callback callback)
	    requires(std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, UString(defaultValue), flags, UString(""), callback, nullptr);
		addConVar(this);
	}

	// callbacks
	void exec();
	void execArgs(UString args);
	void execInt(float args);

	// get
	template <typename T = int>
	[[nodiscard]] constexpr auto getDefaultVal() const
	{
		return static_cast<T>(m_fDefaultValue.load());
	}
	[[nodiscard]] constexpr float getDefaultFloat() const { return getDefaultVal<float>(); }
	[[nodiscard]] constexpr const UString &getDefaultString() const { return m_sDefaultValue; }
	[[nodiscard]] UString getFancyDefaultValue() const;

	[[nodiscard]] constexpr bool isFlagSet(int flag) const { return (bool)(m_iFlags & flag); }

	template <typename T = int>
	[[nodiscard]] constexpr auto getVal(bool cheat = false) const
	{
		return static_cast<T>((!cheat || !!static_cast<int>(cv::ConVars::sv_cheats.getRaw())) ? m_fValue.load() : m_fDefaultValue.load());
	}

	[[nodiscard]] constexpr int getInt() const { return getVal<int>(isFlagSet(FCVAR_CHEAT)); }
	[[nodiscard]] constexpr bool getBool() const { return getVal<bool>(isFlagSet(FCVAR_CHEAT)); }
	[[nodiscard]] constexpr float getFloat() const { return getVal<float>(isFlagSet(FCVAR_CHEAT)); }

	[[nodiscard]]
	constexpr const UString &getString() const
	{
		return (isFlagSet(FCVAR_CHEAT) && !static_cast<int>(cv::ConVars::sv_cheats.getRaw()) ? m_sDefaultValue : m_sValue);
	}

	[[nodiscard]] constexpr const UString &getHelpstring() const { return m_sHelpString; }
	[[nodiscard]] constexpr const UString &getName() const { return m_sName; }
	[[nodiscard]] constexpr CONVAR_TYPE getType() const { return m_type; }
	[[nodiscard]] constexpr int getFlags() const { return m_iFlags; }

	[[nodiscard]] constexpr bool hasValue() const { return m_bHasValue; }
	[[nodiscard]] bool hasCallbackArgs() const;

	// set
	void setDefaultFloat(float defaultValue);
	void setDefaultString(UString defaultValue);

	template <typename T>
	void setValue(T &&value)
	{
		if (isFlagSet(FCVAR_HARDCODED) || (isFlagSet(FCVAR_CHEAT) && !static_cast<int>(cv::ConVars::sv_cheats.getRaw())))
			return;
		setValueInt(std::forward<T>(value));
	}

	// generic callback setter that auto-detects callback type
	template <typename Callback>
	void setCallback(Callback &&callback)
	    requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, UString> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>)
	{
		if constexpr (std::is_invocable_v<Callback>)
			m_callback = NativeConVarCallback(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, UString>)
			m_callback = NativeConVarCallbackArgs(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, float>)
			m_callback = NativeConVarCallbackFloat(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, UString, UString>)
			m_changeCallback = NativeConVarChangeCallback(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, float, float>)
			m_changeCallback = NativeConVarChangeCallbackFloat(std::forward<Callback>(callback));
		else
			static_assert(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, UString> || std::is_invocable_v<Callback, float> ||
			                  std::is_invocable_v<Callback, UString, UString> || std::is_invocable_v<Callback, float, float>,
			              "Unsupported callback signature");
	}

	void setHelpString(UString helpString);

	// for restart support
	void resetCallbacks();

private:
	// unified init for callback-only convars
	template <typename Callback>
	void initCallback(UString &name, int flags, UString helpString, Callback callback)
	{
		initBase(flags);
		m_sName = name;
		m_sHelpString = std::move(helpString);
		m_bHasValue = false;

		if constexpr (std::is_invocable_v<Callback>)
		{
			m_callback = NativeConVarCallback(callback);
			m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
		}
		else if constexpr (std::is_invocable_v<Callback, UString>)
		{
			m_callback = NativeConVarCallbackArgs(callback);
			m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
		}
		else if constexpr (std::is_invocable_v<Callback, float>)
		{
			m_callback = NativeConVarCallbackFloat(callback);
			m_type = CONVAR_TYPE::CONVAR_TYPE_INT;
		}
	}

	// unified init for value convars
	template <typename T, typename ChangeCallback>
	void initValue(UString &name, const T &defaultValue, int flags, UString helpString, ChangeCallback changeCallback, void * /*unused*/)
	{
		initBase(flags);
		m_sName = name;
		m_sHelpString = std::move(helpString);
		m_type = getTypeFor<T>();

		// set default value
		if constexpr (std::is_same_v<std::decay_t<T>, UString> || std::is_same_v<std::decay_t<T>, const char *>)
			setDefaultStringInt(defaultValue);
		else
			setDefaultFloatInt(static_cast<float>(defaultValue));

		// set initial value (without triggering callbacks)
		if constexpr (std::is_same_v<std::decay_t<T>, UString>)
			setValueInt(defaultValue);
		else
			setValueInt(static_cast<float>(defaultValue));

		// set change callback if provided
		if constexpr (!std::is_same_v<ChangeCallback, std::nullptr_t>)
		{
			if constexpr (std::is_invocable_v<ChangeCallback, UString, UString>)
				m_changeCallback = NativeConVarChangeCallback(changeCallback);
			else if constexpr (std::is_invocable_v<ChangeCallback, float, float>)
				m_changeCallback = NativeConVarChangeCallbackFloat(changeCallback);
		}
	}

	void initBase(int flags);
	void setDefaultFloatInt(float defaultValue);
	void setDefaultStringInt(UString defaultValue);

	template <typename T>
	void setValueInt(T &&value) // no flag checking, setValue (user-accessible) already does that
	{
		// determine float and string representations depending on whether setValue("string") or setValue(float) was called
		const auto [newFloat, newString] = [&]() {
			if constexpr (std::is_convertible_v<std::decay_t<T>, float> && !std::is_same_v<std::decay_t<T>, UString>)
			{
				const auto f = static_cast<float>(value);
				return std::make_pair(f, UString::format("%g", f));
			}
			else
			{
				const UString s = std::forward<T>(value);
				const float f = (s.length() > 0) ? s.toFloat() : 0.0f;
				return std::make_pair(f, s);
			}
		}();

		// backup previous values
		const float oldFloat = m_fValue.load();
		const UString oldString = m_sValue;

		// set new values
		m_fValue = newFloat;
		m_sValue = newString;

		// handle possible execution callbacks
		std::visit(
		    [&](auto &&callback) {
			    using CallbackType = std::decay_t<decltype(callback)>;
			    if constexpr (std::is_same_v<CallbackType, NativeConVarCallback>)
				    callback();
			    else if constexpr (std::is_same_v<CallbackType, NativeConVarCallbackArgs>)
				    callback(newString);
			    else if constexpr (std::is_same_v<CallbackType, NativeConVarCallbackFloat>)
				    callback(newFloat);
			    // std::monostate case does nothing
		    },
		    m_callback);

		// handle possible change callbacks
		std::visit(
		    [&](auto &&callback) {
			    using CallbackType = std::decay_t<decltype(callback)>;
			    if constexpr (std::is_same_v<CallbackType, NativeConVarChangeCallback>)
				    callback(oldString, newString);
			    else if constexpr (std::is_same_v<CallbackType, NativeConVarChangeCallbackFloat>)
				    callback(oldFloat, newFloat);
		    },
		    m_changeCallback);
	}

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

	// callback storage (allow having 1 "change" callback and 1 single value (or void) callback)
	ExecutionCallback m_callback;
	ChangeCallback m_changeCallback;
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

	[[nodiscard]] const std::vector<ConVar *> &getConVarArray() const;
	[[nodiscard]] size_t getNumConVars() const;

	[[nodiscard]] ConVar *getConVarByName(const UString &name, bool warnIfNotFound = true) const;
	[[nodiscard]] std::vector<ConVar *> getConVarByLetter(const UString &letters) const;

	void resetAllConVarCallbacks();
};

extern ConVarHandler *convar;

#endif
