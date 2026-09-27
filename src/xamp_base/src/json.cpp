#include <base/json.h>

#include <simdjson.h>

#include <memory>
#include <utility>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	struct JsonDocumentState final {
		simdjson::padded_string padded;
		simdjson::dom::parser parser;

		explicit JsonDocumentState(std::string_view json)
			: padded(json.data(), json.size()) {
		}
	};
}

struct JsonElement::Impl final {
	std::shared_ptr<JsonDocumentState> state;
	simdjson::dom::element element;

	Impl(std::shared_ptr<JsonDocumentState> state, simdjson::dom::element element)
		: state(std::move(state))
		, element(element) {
	}
};

struct JsonObject::Impl final {
	std::shared_ptr<JsonDocumentState> state;
	simdjson::dom::object object;

	Impl(std::shared_ptr<JsonDocumentState> state, simdjson::dom::object object)
		: state(std::move(state))
		, object(object) {
	}
};

JsonElement::JsonElement() = default;

JsonElement::~JsonElement() = default;

JsonElement::JsonElement(const JsonElement&) noexcept = default;

JsonElement::JsonElement(JsonElement&&) noexcept = default;

JsonElement& JsonElement::operator=(const JsonElement&) noexcept = default;

JsonElement& JsonElement::operator=(JsonElement&&) noexcept = default;

JsonElement::JsonElement(std::shared_ptr<Impl> impl) noexcept
	: impl_(std::move(impl)) {
}

std::optional<JsonObject> JsonElement::asObject() const {
	if (!impl_) {
		return std::nullopt;
	}

	simdjson::dom::object object;
	if (impl_->element.get(object)) {
		return std::nullopt;
	}
	return JsonObject(std::make_shared<JsonObject::Impl>(impl_->state, object));
}

std::optional<JsonArray> JsonElement::asArray() const {
	if (!impl_) {
		return std::nullopt;
	}

	simdjson::dom::array array;
	if (impl_->element.get(array)) {
		return std::nullopt;
	}

	JsonArray::Values values;
	for (const auto value : array) {
		values.push_back(JsonElement(std::make_shared<JsonElement::Impl>(impl_->state, value)));
	}
	return JsonArray(std::move(values));
}

std::optional<std::string> JsonElement::asString() const {
	if (!impl_) {
		return std::nullopt;
	}

	std::string_view text;
	if (impl_->element.get(text)) {
		return std::nullopt;
	}
	return std::string(text);
}

std::optional<int64_t> JsonElement::asInt64() const {
	if (!impl_) {
		return std::nullopt;
	}

	int64_t value = 0;
	if (impl_->element.get(value)) {
		return std::nullopt;
	}
	return value;
}

std::optional<uint64_t> JsonElement::asUInt64() const {
	if (!impl_) {
		return std::nullopt;
	}

	uint64_t value = 0;
	if (impl_->element.get(value)) {
		return std::nullopt;
	}
	return value;
}

std::optional<double> JsonElement::asDouble() const {
	if (!impl_) {
		return std::nullopt;
	}

	double value = 0;
	if (impl_->element.get(value)) {
		return std::nullopt;
	}
	return value;
}

std::optional<bool> JsonElement::asBool() const {
	if (!impl_) {
		return std::nullopt;
	}

	bool value = false;
	if (impl_->element.get(value)) {
		return std::nullopt;
	}
	return value;
}

JsonArray::JsonArray() = default;

JsonArray::~JsonArray() = default;

JsonArray::JsonArray(const JsonArray&) noexcept = default;

JsonArray::JsonArray(JsonArray&&) noexcept = default;

JsonArray& JsonArray::operator=(const JsonArray&) noexcept = default;

JsonArray& JsonArray::operator=(JsonArray&&) noexcept = default;

JsonArray::JsonArray(Values values) noexcept
	: values_(std::move(values)) {
}

JsonArray::const_iterator JsonArray::begin() const noexcept {
	return values_.begin();
}

JsonArray::const_iterator JsonArray::end() const noexcept {
	return values_.end();
}

bool JsonArray::empty() const noexcept {
	return values_.empty();
}

size_t JsonArray::size() const noexcept {
	return values_.size();
}

JsonObject::JsonObject() = default;

JsonObject::~JsonObject() = default;

JsonObject::JsonObject(const JsonObject&) noexcept = default;

JsonObject::JsonObject(JsonObject&&) noexcept = default;

JsonObject& JsonObject::operator=(const JsonObject&) noexcept = default;

JsonObject& JsonObject::operator=(JsonObject&&) noexcept = default;

JsonObject::JsonObject(std::shared_ptr<Impl> impl) noexcept
	: impl_(std::move(impl)) {
}

std::optional<JsonElement> JsonObject::field(std::string_view name) const {
	if (!impl_) {
		return std::nullopt;
	}

	const auto result = impl_->object[name];
	if (result.error()) {
		return std::nullopt;
	}
	return JsonElement(std::make_shared<JsonElement::Impl>(impl_->state, result.value_unsafe()));
}

std::optional<JsonObject> JsonObject::objectField(std::string_view name) const {
	const auto value = field(name);
	return value ? value->asObject() : std::nullopt;
}

std::optional<JsonArray> JsonObject::arrayField(std::string_view name) const {
	const auto value = field(name);
	return value ? value->asArray() : std::nullopt;
}

std::string JsonObject::stringField(std::string_view name) const {
	const auto value = field(name);
	if (!value) {
		return {};
	}
	return value->asString().value_or(std::string{});
}

int JsonObject::intField(std::string_view name, int fallback) const {
	const auto value = field(name);
	if (!value) {
		return fallback;
	}

	if (const auto signedValue = value->asInt64()) {
		return static_cast<int>(*signedValue);
	}
	if (const auto unsignedValue = value->asUInt64()) {
		return static_cast<int>(*unsignedValue);
	}
	if (const auto doubleValue = value->asDouble()) {
		return static_cast<int>(*doubleValue);
	}
	return fallback;
}

double JsonObject::doubleField(std::string_view name, double fallback) const {
	const auto value = field(name);
	if (!value) {
		return fallback;
	}

	if (const auto doubleValue = value->asDouble()) {
		return *doubleValue;
	}
	if (const auto signedValue = value->asInt64()) {
		return static_cast<double>(*signedValue);
	}
	if (const auto unsignedValue = value->asUInt64()) {
		return static_cast<double>(*unsignedValue);
	}
	return fallback;
}

bool JsonObject::boolField(std::string_view name, bool fallback) const {
	const auto value = field(name);
	if (!value) {
		return fallback;
	}
	return value->asBool().value_or(fallback);
}

std::vector<std::string> JsonObject::stringArrayField(std::string_view name) const {
	std::vector<std::string> values;
	const auto array = arrayField(name);
	if (!array) {
		return values;
	}

	values.reserve(array->size());
	for (const auto& value : *array) {
		if (const auto text = value.asString()) {
			values.push_back(*text);
		}
	}
	return values;
}

std::optional<JsonObject> parseJsonObject(std::string_view json) {
	auto state = std::make_shared<JsonDocumentState>(json);

	simdjson::dom::element root;
	if (state->parser.parse(state->padded).get(root)) {
		return std::nullopt;
	}

	simdjson::dom::object object;
	if (root.get(object)) {
		return std::nullopt;
	}
	return JsonObject(std::make_shared<JsonObject::Impl>(std::move(state), object));
}

std::optional<JsonObject> parseJsonObject(const std::vector<uint8_t>& json) {
	return parseJsonObject(std::string_view(
		reinterpret_cast<const char*>(json.data()),
		json.size()));
}

XAMP_BASE_NAMESPACE_END
