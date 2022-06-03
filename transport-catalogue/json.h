#pragma once

#include <iostream>
#include <map>
#include <string_view>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;

    ParsingError(const std::string& s) : runtime_error(s) {};
    ParsingError(const char *s) : runtime_error(s) {};
};

const std::string_view null_value = "null";
const std::string_view true_value = "true";
const std::string_view false_value = "false";

class Node {
public:
    using ValueType = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

    Node();
    Node(std::nullptr_t);
    Node(int value);
    Node(double value);
    Node(const std::string& value);
    Node(std::string&& value);
    Node(bool value);
    Node(const Array& array);
    Node(Array&& array);
    Node(const Dict& map);
    Node(Dict&& map);

    Node(const Node&) noexcept = default;
    Node(Node&&) noexcept = default;
    Node& operator=(const Node&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;
    ~Node() = default;

    const ValueType& GetValue() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; // return double if value is int or double
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool IsInt() const;
    bool IsDouble() const; // int or double
    bool IsPureDouble() const; // double only
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

private:
    ValueType value_;
};

static_assert(std::is_copy_constructible<Node>::value);
static_assert(std::is_move_constructible<Node>::value);

bool operator==(const Node& v, const Node& w);

bool operator!=(const Node& v, const Node& w);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

bool operator==(const Document& v, const Document& w);

bool operator!=(const Document& v, const Document& w);

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json