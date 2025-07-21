//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		console variables
//
// $NoKeywords: $convar
//===============================================================================//

#pragma once
#ifndef CONVAR_H
#define CONVAR_H

#include "ConVarDefs.h"
#include "UString.h"
#include "FastDelegate.h"

#include <cassert>
#include <type_traits>

#include <unordered_map>
#include <vector>
#include <variant>
#include <atomic>

#define FCVAR_NONE 0                   // the default, no flags at all
#define FCVAR_UNREGISTERED (1 << 0)    // not added to g_vConVars list (not settable/gettable via console/help), not visible in find/listcommands results etc.
#define FCVAR_DEVELOPMENTONLY (1 << 1) // hidden in released products (like FCVAR_HIDDEN, but flag gets compiled out if ALLOW_DEVELOPMENT_CONVARS is defined)
#define FCVAR_HARDCODED (1 << 2)       // if this is set then the value can't and shouldn't ever be changed
#define FCVAR_HIDDEN (1 << 3)          // not visible in find/listcommands results etc., but is settable/gettable via console/help
#define FCVAR_CHEAT (1 << 4)   // always return default value and ignore callbacks unless sv_cheats is enabled (default value is changeable via setDefault*())
#define FCVAR_DYNAMIC (1 << 5) // set for convars which are created at runtime with e.g. "new" instead of in the global scope

#define MAKENEWCONVAR(name, defaultValue, ...) new ConVar((name), (defaultValue), FCVAR_DYNAMIC __VA_OPT__(, ) __VA_ARGS__)

#ifdef CHECK_FOR_CHEATS
#define ALLOWCHEAT(_cvar_) !((_cvar_)->isFlagSet(FCVAR_CHEAT) && !(cv::ConVars::sv_cheats.getRaw() > 0))
#else
#define ALLOWCHEAT(_cvar_) true
#endif

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
	using NativeConVarCallbackArgs = fastdelegate::FastDelegate1<const UString &>;
	using NativeConVarChangeCallback = fastdelegate::FastDelegate2<const UString &, const UString &>;
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
	// global statics

	[[nodiscard]] static std::vector<ConVar *> &getConVarArray();
	[[nodiscard]] static std::unordered_map<UString, ConVar *> &getConVarMap();

	// helpers
	[[nodiscard]] static ConVar *getConVarByName(const UString &name, bool warnIfNotFound = true);
	[[nodiscard]] static std::vector<ConVar *> getConVarByLetter(const UString &letters);

	[[nodiscard]] static UString typeToString(CONVAR_TYPE type);
	[[nodiscard]] static UString flagsToString(uint8_t flags);

	// for restart without closing (TODO)
	static void resetAllConVarCallbacks();

private:
	[[nodiscard]] float getRaw() const { return m_fValue.load(); } // forward def

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

	ConVar &operator=(const ConVar &) = delete;
	ConVar &operator=(ConVar &&) = delete;
	ConVar(const ConVar &) = delete;
	ConVar(ConVar &&) = delete;

	// command-only constructor
	explicit ConVar(const std::string_view &name)
	{
		m_sName = m_sDefaultValue = name;
		m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
		addConVar(this);
	}

	// callback-only constructors (no value)
	template <typename Callback>
	explicit ConVar(const std::string_view &name, uint8_t flags, Callback callback)
	    requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float>
	{
		initCallback(name, flags, std::string_view{}, callback);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(const std::string_view &name, uint8_t flags, const std::string_view &helpString, Callback callback)
	    requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float>
	{
		initCallback(name, flags, helpString, callback);
		addConVar(this);
	}

	// value constructors handle all types uniformly
	template <typename T>
	explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags, const std::string_view &helpString = {})
	    requires(!std::is_same_v<std::decay_t<T>, const char *>)
	{
		initValue(name, defaultValue, flags, helpString, nullptr);
		addConVar(this);
	}

	template <typename T, typename Callback>
	explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags, const std::string_view &helpString, Callback callback)
	    requires(!std::is_same_v<std::decay_t<T>, const char *>) &&
	            (std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, const UString &, const UString &> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, helpString, callback);
		addConVar(this);
	}

	template <typename T, typename Callback>
	explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags, Callback callback)
	    requires(!std::is_same_v<std::decay_t<T>, const char *>) &&
	            (std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, const UString &, const UString &> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, {}, callback);
		addConVar(this);
	}

	// const char* specializations for string convars
	explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags, const std::string_view &helpString = {})
	{
		initValue(name, defaultValue, flags, helpString, nullptr);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags, const std::string_view &helpString, Callback callback)
	    requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, const UString &, const UString &> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, helpString, callback);
		addConVar(this);
	}

	template <typename Callback>
	explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags, Callback callback)
	    requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, const UString &, const UString &> || std::is_invocable_v<Callback, float, float>)
	{
		initValue(name, defaultValue, flags, {}, callback);
		addConVar(this);
	}

	// callbacks
	void exec();
	void execArgs(const UString &args);
	void execFloat(float args);

	// get
	template <typename T = int>
	[[nodiscard]] constexpr auto getDefaultVal() const
	{
		return static_cast<T>(m_fDefaultValue.load());
	}
	[[nodiscard]] constexpr float getDefaultFloat() const { return getDefaultVal<float>(); }
	[[nodiscard]] constexpr const UString &getDefaultString() const { return m_sDefaultValue; }
	[[nodiscard]] UString getFancyDefaultValue() const;

	[[nodiscard]] constexpr bool isFlagSet(int flag) const { return (m_iFlags & flag) == flag; }

	template <typename T = int>
	[[nodiscard]] constexpr auto getVal() const
	{
		return static_cast<T>(ALLOWCHEAT(this) ? m_fValue.load() : m_fDefaultValue.load());
	}

	[[nodiscard]] constexpr int getInt() const { return getVal<int>(); }
	[[nodiscard]] constexpr bool getBool() const { return getVal<bool>(); }
	[[nodiscard]] constexpr float getFloat() const { return getVal<float>(); }

	[[nodiscard]]
	constexpr const UString &getString() const
	{
		return ALLOWCHEAT(this) ? m_sValue : m_sDefaultValue;
	}

	[[nodiscard]] constexpr const UString &getHelpstring() const { return m_sHelpString; }
	[[nodiscard]] constexpr const UString &getName() const { return m_sName; }
	[[nodiscard]] constexpr CONVAR_TYPE getType() const { return m_type; }
	[[nodiscard]] constexpr int getFlags() const { return m_iFlags; }

	[[nodiscard]] constexpr bool hasValue() const { return m_bHasValue; }
	[[nodiscard]] bool hasCallbackArgs() const;

	// set
	void setDefaultFloat(float defaultValue);
	void setDefaultString(const UString &defaultValue);

	template <typename T>
	void setValue(T &&value, const bool &doCallback = true)
	{
		if (isFlagSet(FCVAR_HARDCODED) || !ALLOWCHEAT(this))
			return;
		setValueInt(std::forward<T>(value), doCallback);
	}

	// generic callback setter that auto-detects callback type
	template <typename Callback>
	void setCallback(Callback &&callback)
	    requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> || std::is_invocable_v<Callback, float> ||
	             std::is_invocable_v<Callback, const UString &, const UString &> || std::is_invocable_v<Callback, float, float>)
	{
		if constexpr (std::is_invocable_v<Callback>)
			m_callback = NativeConVarCallback(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, const UString &>)
			m_callback = NativeConVarCallbackArgs(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, float>)
			m_callback = NativeConVarCallbackFloat(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, const UString &, const UString &>)
			m_changeCallback = NativeConVarChangeCallback(std::forward<Callback>(callback));
		else if constexpr (std::is_invocable_v<Callback, float, float>)
			m_changeCallback = NativeConVarChangeCallbackFloat(std::forward<Callback>(callback));
		else
			static_assert(Env::always_false_v<Callback>, "Unsupported callback signature");
	}

	void setHelpString(const UString &helpString);

	// for restart support
	void resetCallbacks();

private:
	// unified init for callback-only convars
	template <typename Callback>
	void initCallback(const std::string_view &name, uint8_t flags, const std::string_view &helpString, Callback callback)
	{
		m_iFlags = flags
#ifdef ALLOW_DEVELOPMENT_CONVARS
		           & ~FCVAR_DEVELOPMENTONLY
#endif
		    ;

		m_sName = UString{name};
		m_sHelpString = UString{helpString};
		m_bHasValue = false;

		if constexpr (std::is_invocable_v<Callback>)
		{
			m_callback = NativeConVarCallback(callback);
			m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
		}
		else if constexpr (std::is_invocable_v<Callback, const UString &>)
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
	template <typename T, typename Callback>
	void initValue(const std::string_view &name, const T &defaultValue, uint8_t flags, const std::string_view &helpString, Callback callback)
	{
		m_iFlags = flags
#ifdef ALLOW_DEVELOPMENT_CONVARS
		           & ~FCVAR_DEVELOPMENTONLY
#endif
		    ;
		m_sName = name;
		m_sHelpString = helpString;
		m_type = getTypeFor<T>();

		// set default value
		if constexpr (std::is_convertible_v<std::decay_t<T>, float> && !std::is_same_v<std::decay_t<T>, UString> &&
		              !std::is_same_v<std::decay_t<T>, std::string_view> && !std::is_same_v<std::decay_t<T>, const char *>)
			setDefaultFloatInt(static_cast<float>(defaultValue));
		else
			setDefaultStringInt(UString{defaultValue});

		// set initial value (without triggering callbacks)
		if constexpr (std::is_convertible_v<std::decay_t<T>, float> && !std::is_same_v<std::decay_t<T>, UString> &&
		              !std::is_same_v<std::decay_t<T>, std::string_view> && !std::is_same_v<std::decay_t<T>, const char *>)
			setValueInt(static_cast<float>(defaultValue));
		else
			setValueInt(UString{defaultValue});

		// set callback if provided
		if constexpr (!std::is_same_v<Callback, std::nullptr_t>)
		{
			if constexpr (std::is_invocable_v<Callback>)
				m_callback = NativeConVarCallback(callback);
			else if constexpr (std::is_invocable_v<Callback, const UString &>)
				m_callback = NativeConVarCallbackArgs(callback);
			else if constexpr (std::is_invocable_v<Callback, float>)
				m_callback = NativeConVarCallbackFloat(callback);
			else if constexpr (std::is_invocable_v<Callback, const UString &, const UString &>)
				m_changeCallback = NativeConVarChangeCallback(callback);
			else if constexpr (std::is_invocable_v<Callback, float, float>)
				m_changeCallback = NativeConVarChangeCallbackFloat(callback);
		}
	}

	void setDefaultFloatInt(float defaultValue);
	void setDefaultStringInt(const UString &defaultValue);

	template <typename T>
	void setValueInt(T &&value, const bool &doCallback = true) // no flag checking, setValue (user-accessible) already does that
	{
		m_bHasValue = true;

		// determine float and string representations depending on whether setValue("string") or setValue(float) was called
		const auto [newFloat, newString] = [&]() -> std::pair<float, UString> {
			if constexpr (std::is_convertible_v<std::decay_t<T>, float> && !std::is_same_v<std::decay_t<T>, UString> &&
			              !std::is_same_v<std::decay_t<T>, std::string_view> && !std::is_same_v<std::decay_t<T>, const char *>)
			{
				const auto f = static_cast<float>(value);
				return std::make_pair(f, UString::format("%g", f));
			}
			else
			{
				const UString s{std::forward<T>(value)};
				const float f = !s.isEmpty() ? s.toFloat() : 0.0f;
				return std::make_pair(f, s);
			}
		}();

		// backup previous values
		const float oldFloat = m_fValue.load();
		const UString oldString = m_sValue;

		// set new values
		m_fValue = newFloat;
		m_sValue = newString;

		if (doCallback)
		{
			// handle possible execution callbacks
			if (!std::holds_alternative<std::monostate>(m_callback))
			{
				std::visit(
				    [&](auto &&callback) {
					    using CallbackType = std::decay_t<decltype(callback)>;
					    if constexpr (std::is_same_v<CallbackType, NativeConVarCallback>)
						    callback();
					    else if constexpr (std::is_same_v<CallbackType, NativeConVarCallbackArgs>)
						    callback(newString);
					    else if constexpr (std::is_same_v<CallbackType, NativeConVarCallbackFloat>)
						    callback(newFloat);
				    },
				    m_callback);
			}

			// handle possible change callbacks
			if (!std::holds_alternative<std::monostate>(m_changeCallback))
			{
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
		}
	}

private:
	bool m_bHasValue{false};
	CONVAR_TYPE m_type{CONVAR_TYPE::CONVAR_TYPE_FLOAT};
	uint8_t m_iFlags{FCVAR_NONE};

	UString m_sName;
	UString m_sHelpString;

	std::atomic<float> m_fValue{0.0f};
	std::atomic<float> m_fDefaultValue{0.0f};

	UString m_sValue;
	UString m_sDefaultValue;

	// callback storage (allow having 1 "change" callback and 1 single value (or void) callback)
	ExecutionCallback m_callback;
	ChangeCallback m_changeCallback;
};

#endif
