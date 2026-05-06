#pragma once

#include <string>
#include <utility>
#include <vector>

namespace simplejson {

struct Value {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    Type type = Type::Null;
    bool bool_value = false;
    double number_value = 0.0;
    std::wstring string_value;
    std::vector<Value> array_value;
    std::vector<std::pair<std::wstring, Value>> object_value;

    static Value Null();
    static Value Bool(bool value);
    static Value Number(double value);
    static Value String(std::wstring value);
    static Value Array();
    static Value Object();

    const Value* Find(const std::wstring& key) const;
    Value* Find(const std::wstring& key);
};

bool Parse(const std::wstring& text, Value& out, std::wstring* error = nullptr);
std::wstring Stringify(const Value& value, int indent_size = 4);

}  // namespace simplejson
