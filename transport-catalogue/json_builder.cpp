#include "json_builder.h"

#include <utility>
#include <exception>
#include <cassert>
#include <string>

namespace json {

using namespace std;

Builder::Builder() {
}

DictValueContext Builder::Key(const string& key) {
    return Key(string(key));
}

DictValueContext Builder::Key(string&& key) {
    if (nodes_stack_.empty() || !nodes_stack_.back().IsDict())
        throw logic_error("Dict expected");
    assert(!keys_stack_.empty());
    if (keys_stack_.back().has_value())
        throw logic_error("duplicate key");

    keys_stack_.back() = move(key);
    return DictValueContext(*this);
}

Builder& Builder::Value(const Node& value) {
    return Value(Node(value));
}

Builder& Builder::Value(Node&& value) {
    if (nodes_stack_.empty()) {
        nodes_stack_.push_back(move(value));
    } else {
        auto& last_node = nodes_stack_.back();
        if (last_node.IsArray()) {
            get<Array>(last_node.Value()).push_back(move(value));
        } else if (last_node.IsDict()) {
            if (!keys_stack_.back().has_value())
                throw logic_error("no key");
            get<Dict>(last_node.Value()).insert({move(*keys_stack_.back()), move(value)});
            keys_stack_.back().reset();
        } else {
            assert(false);
        }
    }
    return *this;
}

DictKeyContext Builder::StartDict() {
    nodes_stack_.emplace_back(Dict{});
    keys_stack_.emplace_back(nullopt);
    return DictKeyContext(*this);
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back().IsDict())
        throw logic_error("dict expected");
    assert(!keys_stack_.empty());
    if (keys_stack_.back().has_value())
        throw logic_error("excessive key");

    auto dict_node{move(nodes_stack_.back())};
    nodes_stack_.pop_back();
    keys_stack_.pop_back();
    return Value(move(dict_node));
}

ArrayContext Builder::StartArray() {
    nodes_stack_.emplace_back(Array{});
    return ArrayContext(*this);
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back().IsArray())
        throw logic_error("array expected");

    auto array_node{move(nodes_stack_.back())};
    nodes_stack_.pop_back();
    return Value(move(array_node));
}

Node Builder::Build() {
    // there must be one node of Array (see ctor)
    if (nodes_stack_.size() != 1)
        throw logic_error("json must contain one root node");
    if (keys_stack_.size() > 0)
        throw logic_error("unused key");

    return move(nodes_stack_.back());
}

DictValueContext::DictValueContext(Builder& builder)
    : builder_(builder) {
}

DictKeyContext DictValueContext::Value(const Node& value) {
    return DictKeyContext(builder_.Value(value));
}

DictKeyContext DictValueContext::Value(Node&& value) {
    return DictKeyContext(builder_.Value(move(value)));
}

DictKeyContext DictValueContext::StartDict() {
    return DictKeyContext(builder_.StartDict());
}

ArrayContext DictValueContext::StartArray() {
    return ArrayContext(builder_.StartArray());
}

DictKeyContext::DictKeyContext(Builder& builder)
    : builder_(builder) {
}

DictValueContext DictKeyContext::Key(const string& key) {
    return DictValueContext(builder_.Key(key));
}

DictValueContext DictKeyContext::Key(string&& key) {
    return DictValueContext(builder_.Key(move(key)));
}

Builder& DictKeyContext::EndDict() {
    return builder_.EndDict();
}

ArrayContext::ArrayContext(Builder& builder)
    : builder_(builder) {
}

ArrayContext ArrayContext::Value(const Node& value) {
    return ArrayContext(builder_.Value(value));
}

ArrayContext ArrayContext::Value(Node&& value) {
    return ArrayContext(builder_.Value(move(value)));
}

DictKeyContext ArrayContext::StartDict() {
    return DictKeyContext(builder_.StartDict());
}

ArrayContext ArrayContext::StartArray() {
    return ArrayContext(builder_.StartArray());
}

Builder& ArrayContext::EndArray() {
    return builder_.EndArray();
}

} // namespace json