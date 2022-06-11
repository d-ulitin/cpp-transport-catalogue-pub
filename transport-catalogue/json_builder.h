#pragma once

#include "json.h"

#include <optional>
#include <vector>
#include <variant>
#include <optional>

namespace json {

class DictValueContext;
class DictKeyContext;
class ArrayContext;

class Builder {

public:
    Builder();

    DictValueContext Key(std::string key);
    Builder& Value(Node value);
    DictKeyContext StartDict();
    Builder& EndDict();
    ArrayContext StartArray();
    Builder& EndArray();
    Node Build();

private:

    std::vector<Node> nodes_stack_;
    std::vector<std::optional<std::string>> keys_stack_;

}; // class Builder

class DictValueContext {
public:
    DictValueContext(Builder& builder);
    DictKeyContext Value(Node value);
    DictKeyContext StartDict();
    ArrayContext StartArray();
private:
    Builder& builder_;
};

class DictKeyContext {
public:
    DictKeyContext(Builder& builder);
    DictValueContext Key(std::string key);
    Builder& EndDict();
private:
    Builder& builder_;
};

class ArrayContext {
public:
    ArrayContext(Builder& builder);
    ArrayContext Value(Node value);
    DictKeyContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();
private:
    Builder& builder_;
};

} // namespace json