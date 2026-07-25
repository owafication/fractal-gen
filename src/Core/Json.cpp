#include "Core/Json.h"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace mw::json {
namespace {

const Value::Object kEmptyObject;
const Value::Array kEmptyArray;

class Parser {
public:
    Parser(const std::string& text, std::size_t maximumDepth)
        : text_(text), maximumDepth_(maximumDepth) {}

    ParseResult Run() {
        SkipWhitespace();
        auto value = ParseValue(0);
        if (!error_.empty()) return {std::nullopt, error_, position_};
        SkipWhitespace();
        if (position_ != text_.size()) return {std::nullopt, "Unexpected trailing content.", position_};
        return {std::move(value), {}, 0};
    }

private:
    std::optional<Value> ParseValue(std::size_t depth) {
        if (depth > maximumDepth_) return Fail("Maximum JSON nesting depth exceeded.");
        SkipWhitespace();
        if (position_ >= text_.size()) return Fail("Unexpected end of JSON input.");
        const char ch = text_[position_];
        if (ch == '{') return ParseObject(depth + 1);
        if (ch == '[') return ParseArray(depth + 1);
        if (ch == '"') {
            auto string = ParseString();
            if (!string) return std::nullopt;
            return Value(std::move(*string));
        }
        if (ch == 't' && ConsumeLiteral("true")) return Value(true);
        if (ch == 'f' && ConsumeLiteral("false")) return Value(false);
        if (ch == 'n' && ConsumeLiteral("null")) return Value(nullptr);
        if (ch == '-' || (ch >= '0' && ch <= '9')) return ParseNumber();
        return Fail("Unexpected JSON token.");
    }

    std::optional<Value> ParseObject(std::size_t depth) {
        ++position_;
        Value::Object object;
        SkipWhitespace();
        if (Consume('}')) return std::optional<Value>(std::in_place, Value::Object(std::move(object)));
        while (position_ < text_.size()) {
            SkipWhitespace();
            if (position_ >= text_.size() || text_[position_] != '"') return Fail("Expected an object key.");
            auto key = ParseString();
            if (!key) return std::nullopt;
            SkipWhitespace();
            if (!Consume(':')) return Fail("Expected ':' after object key.");
            auto value = ParseValue(depth);
            if (!value) return std::nullopt;
            object[*key] = std::move(*value);
            SkipWhitespace();
            if (Consume('}')) return std::optional<Value>(std::in_place, Value::Object(std::move(object)));
            if (!Consume(',')) return Fail("Expected ',' or '}' in object.");
        }
        return Fail("Unterminated object.");
    }

    std::optional<Value> ParseArray(std::size_t depth) {
        ++position_;
        Value::Array array;
        SkipWhitespace();
        if (Consume(']')) return std::optional<Value>(std::in_place, Value::Array(std::move(array)));
        while (position_ < text_.size()) {
            auto value = ParseValue(depth);
            if (!value) return std::nullopt;
            array.push_back(std::move(*value));
            SkipWhitespace();
            if (Consume(']')) return std::optional<Value>(std::in_place, Value::Array(std::move(array)));
            if (!Consume(',')) return Fail("Expected ',' or ']' in array.");
        }
        return Fail("Unterminated array.");
    }

    std::optional<std::string> ParseString() {
        if (!Consume('"')) return FailString("Expected string.");
        std::string output;
        while (position_ < text_.size()) {
            const unsigned char ch = static_cast<unsigned char>(text_[position_++]);
            if (ch == '"') return output;
            if (ch < 0x20) return FailString("Control character in JSON string.");
            if (ch != '\\') {
                output.push_back(static_cast<char>(ch));
                continue;
            }
            if (position_ >= text_.size()) return FailString("Unterminated escape sequence.");
            const char escaped = text_[position_++];
            switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > text_.size()) return FailString("Incomplete unicode escape.");
                unsigned codePoint = 0;
                for (int i = 0; i < 4; ++i) {
                    const char hex = text_[position_++];
                    codePoint <<= 4;
                    if (hex >= '0' && hex <= '9') codePoint += static_cast<unsigned>(hex - '0');
                    else if (hex >= 'a' && hex <= 'f') codePoint += static_cast<unsigned>(hex - 'a' + 10);
                    else if (hex >= 'A' && hex <= 'F') codePoint += static_cast<unsigned>(hex - 'A' + 10);
                    else return FailString("Invalid unicode escape.");
                }
                AppendUtf8(output, codePoint);
                break;
            }
            default: return FailString("Invalid JSON escape sequence.");
            }
        }
        return FailString("Unterminated string.");
    }

    std::optional<Value> ParseNumber() {
        const std::size_t start = position_;
        if (text_[position_] == '-') ++position_;
        if (position_ >= text_.size()) return Fail("Incomplete number.");
        if (text_[position_] == '0') {
            ++position_;
        } else {
            if (text_[position_] < '1' || text_[position_] > '9') return Fail("Invalid number.");
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }
        if (position_ < text_.size() && text_[position_] == '.') {
            ++position_;
            if (position_ >= text_.size() || text_[position_] < '0' || text_[position_] > '9') return Fail("Invalid decimal number.");
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            if (position_ >= text_.size() || text_[position_] < '0' || text_[position_] > '9') return Fail("Invalid exponent.");
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }
        try {
            const double value = std::stod(text_.substr(start, position_ - start));
            if (!std::isfinite(value)) return Fail("Non-finite numbers are not allowed.");
            return Value(value);
        } catch (...) {
            return Fail("Invalid number.");
        }
    }

    void SkipWhitespace() {
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') ++position_;
            else break;
        }
    }

    bool Consume(char expected) {
        SkipWhitespace();
        if (position_ < text_.size() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    bool ConsumeLiteral(const char* literal) {
        const std::size_t start = position_;
        while (*literal) {
            if (position_ >= text_.size() || text_[position_] != *literal) {
                position_ = start;
                return false;
            }
            ++position_;
            ++literal;
        }
        return true;
    }

    std::optional<Value> Fail(std::string message) {
        if (error_.empty()) error_ = std::move(message);
        return std::nullopt;
    }

    std::optional<std::string> FailString(std::string message) {
        if (error_.empty()) error_ = std::move(message);
        return std::nullopt;
    }

    static void AppendUtf8(std::string& output, unsigned codePoint) {
        if (codePoint <= 0x7F) {
            output.push_back(static_cast<char>(codePoint));
        } else if (codePoint <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        } else {
            output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
    }

    const std::string& text_;
    std::size_t position_{0};
    std::size_t maximumDepth_{32};
    std::string error_;
};

std::string EscapeString(const std::string& text) {
    std::ostringstream stream;
    stream << '"';
    for (const char raw : text) {
        const auto ch = static_cast<unsigned char>(raw);
        switch (ch) {
        case '"': stream << "\\\""; break;
        case '\\': stream << "\\\\"; break;
        case '\b': stream << "\\b"; break;
        case '\f': stream << "\\f"; break;
        case '\n': stream << "\\n"; break;
        case '\r': stream << "\\r"; break;
        case '\t': stream << "\\t"; break;
        default:
            if (ch < 0x20) {
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
            } else {
                stream << static_cast<char>(ch);
            }
        }
    }
    stream << '"';
    return stream.str();
}

std::string Indent(int level) {
    return std::string(static_cast<std::size_t>(level) * 2, ' ');
}

} // namespace

bool Value::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Value::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Value::IsNumber() const { return std::holds_alternative<double>(value_); }
bool Value::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Value::IsObject() const { return std::holds_alternative<Object>(value_); }
bool Value::IsArray() const { return std::holds_alternative<Array>(value_); }

bool Value::AsBool(bool fallback) const { return IsBool() ? std::get<bool>(value_) : fallback; }
double Value::AsNumber(double fallback) const { return IsNumber() ? std::get<double>(value_) : fallback; }
int Value::AsInt(int fallback) const {
    if (!IsNumber()) return fallback;
    const double value = std::get<double>(value_);
    if (value < static_cast<double>(std::numeric_limits<int>::min()) || value > static_cast<double>(std::numeric_limits<int>::max())) return fallback;
    return static_cast<int>(value);
}
std::string Value::AsString(std::string fallback) const { return IsString() ? std::get<std::string>(value_) : std::move(fallback); }
const Value::Object& Value::AsObject() const { return IsObject() ? std::get<Object>(value_) : kEmptyObject; }
const Value::Array& Value::AsArray() const { return IsArray() ? std::get<Array>(value_) : kEmptyArray; }
Value::Object& Value::AsObject() {
    if (!IsObject()) value_ = Object{};
    return std::get<Object>(value_);
}
Value::Array& Value::AsArray() {
    if (!IsArray()) value_ = Array{};
    return std::get<Array>(value_);
}
const Value* Value::Find(const std::string& key) const {
    if (!IsObject()) return nullptr;
    const auto& object = std::get<Object>(value_);
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}
Value& Value::operator[](const std::string& key) { return AsObject()[key]; }

ParseResult Parse(const std::string& text, std::size_t maximumBytes) {
    if (text.size() > maximumBytes) return {std::nullopt, "JSON input exceeds the size limit.", maximumBytes};
    Parser parser(text, 32);
    return parser.Run();
}

std::string Stringify(const Value& value, bool pretty, int indent) {
    if (value.IsNull()) return "null";
    if (value.IsBool()) return value.AsBool() ? "true" : "false";
    if (value.IsNumber()) {
        std::ostringstream stream;
        stream << std::setprecision(17) << value.AsNumber();
        return stream.str();
    }
    if (value.IsString()) return EscapeString(value.AsString());
    if (value.IsArray()) {
        const auto& array = value.AsArray();
        if (array.empty()) return "[]";
        std::ostringstream stream;
        stream << '[';
        for (std::size_t i = 0; i < array.size(); ++i) {
            if (pretty) stream << '\n' << Indent(indent + 1);
            stream << Stringify(array[i], pretty, indent + 1);
            if (i + 1 < array.size()) stream << ',';
        }
        if (pretty) stream << '\n' << Indent(indent);
        stream << ']';
        return stream.str();
    }
    const auto& object = value.AsObject();
    if (object.empty()) return "{}";
    std::ostringstream stream;
    stream << '{';
    std::size_t index = 0;
    for (const auto& [key, child] : object) {
        if (pretty) stream << '\n' << Indent(indent + 1);
        stream << EscapeString(key) << (pretty ? ": " : ":") << Stringify(child, pretty, indent + 1);
        if (++index < object.size()) stream << ',';
    }
    if (pretty) stream << '\n' << Indent(indent);
    stream << '}';
    return stream.str();
}

} // namespace mw::json
