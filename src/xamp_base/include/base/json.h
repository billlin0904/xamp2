//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

XAMP_BASE_NAMESPACE_BEGIN

class JsonArray;
class JsonObject;

class XAMP_BASE_API JsonElement final {
public:
	JsonElement();
	~JsonElement();

	JsonElement(const JsonElement&) noexcept;
	JsonElement(JsonElement&&) noexcept;
	JsonElement& operator=(const JsonElement&) noexcept;
	JsonElement& operator=(JsonElement&&) noexcept;

	std::optional<JsonObject> asObject() const;
	std::optional<JsonArray> asArray() const;
	std::optional<std::string> asString() const;
	std::optional<int64_t> asInt64() const;
	std::optional<uint64_t> asUInt64() const;
	std::optional<double> asDouble() const;
	std::optional<bool> asBool() const;

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;

	explicit JsonElement(std::shared_ptr<Impl> impl) noexcept;

	friend class JsonObject;
	friend class JsonArray;
};

class XAMP_BASE_API JsonArray final {
public:
	using Values = std::vector<JsonElement>;
	using const_iterator = Values::const_iterator;

	JsonArray();
	~JsonArray();

	JsonArray(const JsonArray&) noexcept;
	JsonArray(JsonArray&&) noexcept;
	JsonArray& operator=(const JsonArray&) noexcept;
	JsonArray& operator=(JsonArray&&) noexcept;

	const_iterator begin() const noexcept;
	const_iterator end() const noexcept;
	bool empty() const noexcept;
	size_t size() const noexcept;

private:
	Values values_;

	explicit JsonArray(Values values) noexcept;

	friend class JsonElement;
	friend class JsonObject;
};

class XAMP_BASE_API JsonObject final {
public:
	JsonObject();
	~JsonObject();

	JsonObject(const JsonObject&) noexcept;
	JsonObject(JsonObject&&) noexcept;
	JsonObject& operator=(const JsonObject&) noexcept;
	JsonObject& operator=(JsonObject&&) noexcept;

	std::optional<JsonElement> field(std::string_view name) const;
	std::optional<JsonObject> objectField(std::string_view name) const;
	std::optional<JsonArray> arrayField(std::string_view name) const;
	std::string stringField(std::string_view name) const;
	int intField(std::string_view name, int fallback = 0) const;
	double doubleField(std::string_view name, double fallback = 0) const;
	bool boolField(std::string_view name, bool fallback = false) const;
	std::vector<std::string> stringArrayField(std::string_view name) const;

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;

	explicit JsonObject(std::shared_ptr<Impl> impl) noexcept;

	friend class JsonElement;
	friend XAMP_BASE_API std::optional<JsonObject> parseJsonObject(std::string_view json);
	friend XAMP_BASE_API std::optional<JsonObject> parseJsonObject(const std::vector<uint8_t>& json);
};

XAMP_BASE_API std::optional<JsonObject> parseJsonObject(std::string_view json);

XAMP_BASE_API std::optional<JsonObject> parseJsonObject(const std::vector<uint8_t>& json);

XAMP_BASE_NAMESPACE_END
