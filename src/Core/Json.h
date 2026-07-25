#pragma once

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mw::json {

class Value {
public:
    using Object = std::map<std::string, Value>;
    using Array = std::vector<Value>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Object, Array>;

    Value() : value_(nullptr) {}
    Value(std::nullptr_t) : value_(nullptr) {}
    Value(bool value) : value_(value) {}
    Value(int value) : value_(static_cast<double>(value)) {}
    Value(double value) : value_(value) {}
    Value(std::string value) : value_(std::move(value)) {}
    Value(const char* value) : value_(std::string(value)) {}
    Value(Object value) : value_(std::move(value)) {}
    Value(Array value) : value_(std::move(value)) {}

    [[nodiscard]] bool IsNull() const;
    [[nodiscard]] bool IsBool() const;
    [[nodiscard]] bool IsNumber() const;
    [[nodiscard]] bool IsString() const;
    [[nodiscard]] bool IsObject() const;
    [[nodiscard]] bool IsArray() const;

    [[nodiscard]] bool AsBool(bool fallback = false) const;
    [[nodiscard]] double AsNumber(double fallback = 0.0) const;
    [[nodiscard]] int AsInt(int fallback = 0) const;
    [[nodiscard]] std::string AsString(std::string fallback = {}) const;
    [[nodiscard]] const Object& AsObject() const;
    [[nodiscard]] const Array& AsArray() const;
    Object& AsObject();
    Array& AsArray();

    [[nodiscard]] const Value* Find(const std::string& key) const;
    Value& operator[](const std::string& key);

private:
    Storage value_;
};

struct ParseResult {
    std::optional<Value> value;
    std::string error;
    std::size_t errorOffset{0};
};

ParseResult Parse(const std::string& text, std::size_t maximumBytes = 1024 * 1024);
std::string Stringify(const Value& value, bool pretty = true, int indent = 0);

} // namespace mw::json
