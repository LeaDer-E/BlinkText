#include "SimpleJson.h"

#include <cctype>
#include <cwctype>
#include <sstream>

namespace simplejson {
namespace {

void AppendCodePoint(std::wstring& output, unsigned int code_point) {
    if constexpr (sizeof(wchar_t) == 2) {
        if (code_point <= 0xFFFF) {
            output.push_back(static_cast<wchar_t>(code_point));
            return;
        }

        code_point -= 0x10000;
        output.push_back(static_cast<wchar_t>(0xD800 + ((code_point >> 10) & 0x3FF)));
        output.push_back(static_cast<wchar_t>(0xDC00 + (code_point & 0x3FF)));
        return;
    }

    output.push_back(static_cast<wchar_t>(code_point));
}

bool IsHexDigit(wchar_t ch) {
    return (ch >= L'0' && ch <= L'9') ||
        (ch >= L'a' && ch <= L'f') ||
        (ch >= L'A' && ch <= L'F');
}

unsigned int HexValue(wchar_t ch) {
    if (ch >= L'0' && ch <= L'9') {
        return static_cast<unsigned int>(ch - L'0');
    }
    if (ch >= L'a' && ch <= L'f') {
        return 10u + static_cast<unsigned int>(ch - L'a');
    }
    return 10u + static_cast<unsigned int>(ch - L'A');
}

std::wstring MakeIndent(int count) {
    return std::wstring(static_cast<size_t>(count), L' ');
}

std::wstring EscapeString(const std::wstring& value) {
    std::wstring escaped;
    escaped.reserve(value.size() + 8);

    for (wchar_t ch : value) {
        switch (ch) {
        case L'\"':
            escaped += L"\\\"";
            break;
        case L'\\':
            escaped += L"\\\\";
            break;
        case L'\b':
            escaped += L"\\b";
            break;
        case L'\f':
            escaped += L"\\f";
            break;
        case L'\n':
            escaped += L"\\n";
            break;
        case L'\r':
            escaped += L"\\r";
            break;
        case L'\t':
            escaped += L"\\t";
            break;
        default:
            if (static_cast<unsigned int>(ch) < 0x20) {
                wchar_t buffer[7] = {};
                swprintf(buffer, 7, L"\\u%04X", static_cast<unsigned int>(ch));
                escaped += buffer;
            } else {
                escaped.push_back(ch);
            }
            break;
        }
    }

    return escaped;
}

std::wstring StringifyImpl(const Value& value, int indent_size, int depth);

std::wstring StringifyArray(const Value& value, int indent_size, int depth) {
    if (value.array_value.empty()) {
        return L"[]";
    }

    std::wstring text = L"[\n";
    const std::wstring indent = MakeIndent((depth + 1) * indent_size);
    const std::wstring closing_indent = MakeIndent(depth * indent_size);

    for (size_t index = 0; index < value.array_value.size(); ++index) {
        text += indent + StringifyImpl(value.array_value[index], indent_size, depth + 1);
        if (index + 1 < value.array_value.size()) {
            text += L",";
        }
        text += L"\n";
    }

    text += closing_indent + L"]";
    return text;
}

std::wstring StringifyObject(const Value& value, int indent_size, int depth) {
    if (value.object_value.empty()) {
        return L"{}";
    }

    std::wstring text = L"{\n";
    const std::wstring indent = MakeIndent((depth + 1) * indent_size);
    const std::wstring closing_indent = MakeIndent(depth * indent_size);

    for (size_t index = 0; index < value.object_value.size(); ++index) {
        const auto& entry = value.object_value[index];
        text += indent + L"\"" + EscapeString(entry.first) + L"\": " +
            StringifyImpl(entry.second, indent_size, depth + 1);
        if (index + 1 < value.object_value.size()) {
            text += L",";
        }
        text += L"\n";
    }

    text += closing_indent + L"}";
    return text;
}

std::wstring StringifyImpl(const Value& value, int indent_size, int depth) {
    switch (value.type) {
    case Value::Type::Null:
        return L"null";
    case Value::Type::Bool:
        return value.bool_value ? L"true" : L"false";
    case Value::Type::Number: {
        std::wostringstream stream;
        stream.precision(15);
        stream << value.number_value;
        return stream.str();
    }
    case Value::Type::String:
        return L"\"" + EscapeString(value.string_value) + L"\"";
    case Value::Type::Array:
        return StringifyArray(value, indent_size, depth);
    case Value::Type::Object:
        return StringifyObject(value, indent_size, depth);
    }

    return L"null";
}

class Parser {
public:
    explicit Parser(const std::wstring& source)
        : source_(source) {}

    bool ParseRoot(Value& out, std::wstring* error) {
        SkipWhitespace();
        if (!ParseValue(out, error)) {
            return false;
        }
        SkipWhitespace();
        if (position_ != source_.size()) {
            if (error != nullptr) {
                *error = L"Unexpected trailing characters at position " + std::to_wstring(position_);
            }
            return false;
        }
        return true;
    }

private:
    bool ParseValue(Value& out, std::wstring* error) {
        SkipWhitespace();
        if (position_ >= source_.size()) {
            return SetError(error, L"Unexpected end of JSON input.");
        }

        const wchar_t ch = source_[position_];
        if (ch == L'{') {
            return ParseObject(out, error);
        }
        if (ch == L'[') {
            return ParseArray(out, error);
        }
        if (ch == L'"') {
            std::wstring text;
            if (!ParseString(text, error)) {
                return false;
            }
            out = Value::String(std::move(text));
            return true;
        }
        if (ch == L't') {
            return ParseLiteral(L"true", Value::Bool(true), out, error);
        }
        if (ch == L'f') {
            return ParseLiteral(L"false", Value::Bool(false), out, error);
        }
        if (ch == L'n') {
            return ParseLiteral(L"null", Value::Null(), out, error);
        }
        if (ch == L'-' || (ch >= L'0' && ch <= L'9')) {
            return ParseNumber(out, error);
        }

        return SetError(error, L"Unexpected token at position " + std::to_wstring(position_));
    }

    bool ParseObject(Value& out, std::wstring* error) {
        ++position_;
        out = Value::Object();
        SkipWhitespace();

        if (position_ < source_.size() && source_[position_] == L'}') {
            ++position_;
            return true;
        }

        while (position_ < source_.size()) {
            std::wstring key;
            if (!ParseString(key, error)) {
                return false;
            }
            SkipWhitespace();
            if (position_ >= source_.size() || source_[position_] != L':') {
                return SetError(error, L"Expected ':' at position " + std::to_wstring(position_));
            }
            ++position_;

            Value child;
            if (!ParseValue(child, error)) {
                return false;
            }

            out.object_value.emplace_back(std::move(key), std::move(child));
            SkipWhitespace();
            if (position_ >= source_.size()) {
                break;
            }
            if (source_[position_] == L'}') {
                ++position_;
                return true;
            }
            if (source_[position_] != L',') {
                return SetError(error, L"Expected ',' at position " + std::to_wstring(position_));
            }
            ++position_;
            SkipWhitespace();
        }

        return SetError(error, L"Unterminated object.");
    }

    bool ParseArray(Value& out, std::wstring* error) {
        ++position_;
        out = Value::Array();
        SkipWhitespace();

        if (position_ < source_.size() && source_[position_] == L']') {
            ++position_;
            return true;
        }

        while (position_ < source_.size()) {
            Value child;
            if (!ParseValue(child, error)) {
                return false;
            }
            out.array_value.push_back(std::move(child));
            SkipWhitespace();
            if (position_ >= source_.size()) {
                break;
            }
            if (source_[position_] == L']') {
                ++position_;
                return true;
            }
            if (source_[position_] != L',') {
                return SetError(error, L"Expected ',' at position " + std::to_wstring(position_));
            }
            ++position_;
            SkipWhitespace();
        }

        return SetError(error, L"Unterminated array.");
    }

    bool ParseString(std::wstring& out, std::wstring* error) {
        if (position_ >= source_.size() || source_[position_] != L'"') {
            return SetError(error, L"Expected string at position " + std::to_wstring(position_));
        }
        ++position_;
        out.clear();

        while (position_ < source_.size()) {
            const wchar_t ch = source_[position_++];
            if (ch == L'"') {
                return true;
            }
            if (ch == L'\\') {
                if (position_ >= source_.size()) {
                    return SetError(error, L"Invalid escape sequence.");
                }
                const wchar_t escaped = source_[position_++];
                switch (escaped) {
                case L'"':
                case L'\\':
                case L'/':
                    out.push_back(escaped);
                    break;
                case L'b':
                    out.push_back(L'\b');
                    break;
                case L'f':
                    out.push_back(L'\f');
                    break;
                case L'n':
                    out.push_back(L'\n');
                    break;
                case L'r':
                    out.push_back(L'\r');
                    break;
                case L't':
                    out.push_back(L'\t');
                    break;
                case L'u': {
                    if (position_ + 4 > source_.size()) {
                        return SetError(error, L"Incomplete unicode escape.");
                    }

                    unsigned int code_point = 0;
                    for (int i = 0; i < 4; ++i) {
                        const wchar_t hex = source_[position_++];
                        if (!IsHexDigit(hex)) {
                            return SetError(error, L"Invalid unicode escape.");
                        }
                        code_point = (code_point << 4) | HexValue(hex);
                    }

                    if (code_point >= 0xD800 && code_point <= 0xDBFF) {
                        const size_t save = position_;
                        if (position_ + 6 <= source_.size() &&
                            source_[position_] == L'\\' &&
                            source_[position_ + 1] == L'u') {
                            position_ += 2;
                            unsigned int low = 0;
                            for (int i = 0; i < 4; ++i) {
                                const wchar_t hex = source_[position_++];
                                if (!IsHexDigit(hex)) {
                                    position_ = save;
                                    return SetError(error, L"Invalid unicode surrogate pair.");
                                }
                                low = (low << 4) | HexValue(hex);
                            }

                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                code_point = 0x10000 + (((code_point - 0xD800) << 10) | (low - 0xDC00));
                            } else {
                                position_ = save;
                            }
                        }
                    }

                    AppendCodePoint(out, code_point);
                    break;
                }
                default:
                    return SetError(error, L"Unknown escape sequence.");
                }
                continue;
            }

            out.push_back(ch);
        }

        return SetError(error, L"Unterminated string.");
    }

    bool ParseNumber(Value& out, std::wstring* error) {
        const size_t start = position_;
        if (source_[position_] == L'-') {
            ++position_;
        }

        if (position_ >= source_.size()) {
            return SetError(error, L"Invalid number.");
        }

        if (source_[position_] == L'0') {
            ++position_;
        } else {
            if (source_[position_] < L'1' || source_[position_] > L'9') {
                return SetError(error, L"Invalid number.");
            }
            while (position_ < source_.size() && source_[position_] >= L'0' && source_[position_] <= L'9') {
                ++position_;
            }
        }

        if (position_ < source_.size() && source_[position_] == L'.') {
            ++position_;
            if (position_ >= source_.size() || source_[position_] < L'0' || source_[position_] > L'9') {
                return SetError(error, L"Invalid fractional number.");
            }
            while (position_ < source_.size() && source_[position_] >= L'0' && source_[position_] <= L'9') {
                ++position_;
            }
        }

        if (position_ < source_.size() && (source_[position_] == L'e' || source_[position_] == L'E')) {
            ++position_;
            if (position_ < source_.size() && (source_[position_] == L'+' || source_[position_] == L'-')) {
                ++position_;
            }
            if (position_ >= source_.size() || source_[position_] < L'0' || source_[position_] > L'9') {
                return SetError(error, L"Invalid exponent.");
            }
            while (position_ < source_.size() && source_[position_] >= L'0' && source_[position_] <= L'9') {
                ++position_;
            }
        }

        const std::wstring slice = source_.substr(start, position_ - start);
        try {
            out = Value::Number(std::stod(slice));
            return true;
        } catch (...) {
            return SetError(error, L"Could not parse number.");
        }
    }

    bool ParseLiteral(const wchar_t* token, Value value, Value& out, std::wstring* error) {
        const std::wstring literal(token);
        if (source_.compare(position_, literal.size(), literal) != 0) {
            return SetError(error, L"Unexpected token at position " + std::to_wstring(position_));
        }
        position_ += literal.size();
        out = std::move(value);
        return true;
    }

    void SkipWhitespace() {
        while (position_ < source_.size() && std::iswspace(source_[position_])) {
            ++position_;
        }
    }

    bool SetError(std::wstring* error, std::wstring text) const {
        if (error != nullptr) {
            *error = std::move(text);
        }
        return false;
    }

    const std::wstring& source_;
    size_t position_ = 0;
};

}  // namespace

Value Value::Null() {
    return Value{};
}

Value Value::Bool(bool value) {
    Value result;
    result.type = Type::Bool;
    result.bool_value = value;
    return result;
}

Value Value::Number(double value) {
    Value result;
    result.type = Type::Number;
    result.number_value = value;
    return result;
}

Value Value::String(std::wstring value) {
    Value result;
    result.type = Type::String;
    result.string_value = std::move(value);
    return result;
}

Value Value::Array() {
    Value result;
    result.type = Type::Array;
    return result;
}

Value Value::Object() {
    Value result;
    result.type = Type::Object;
    return result;
}

const Value* Value::Find(const std::wstring& key) const {
    if (type != Type::Object) {
        return nullptr;
    }
    for (const auto& entry : object_value) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

Value* Value::Find(const std::wstring& key) {
    if (type != Type::Object) {
        return nullptr;
    }
    for (auto& entry : object_value) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

bool Parse(const std::wstring& text, Value& out, std::wstring* error) {
    std::wstring source = text;
    if (!source.empty() && source.front() == 0xFEFF) {
        source.erase(source.begin());
    }

    Parser parser(source);
    return parser.ParseRoot(out, error);
}

std::wstring Stringify(const Value& value, int indent_size) {
    return StringifyImpl(value, indent_size, 0);
}

}  // namespace simplejson
