#include "json_builder.h"

namespace json {
const std::string ERROR_ON_VALUE = "Error on add value";
const std::string ERROR_ON_KEY = "Error on add key";
const std::string ERROR_ON_START_ARRAY = "Error on start array";
const std::string ERROR_ON_START_DIRT = "Error on start map";
const std::string ERROR_ON_END_ARRAY = "Error on end array";
const std::string ERROR_ON_END_DIRT = "Error on end map";
const std::string ERROR_BUILD = "Error in build";

void AddNodeValue(Node* node, Node&& value) {
    if (node->IsArray()) {
        node->AsArray().emplace_back(std::move(value));
        return;
    }
    if (!node->IsDict()) {
        *node = Node(value);
    } else {
        throw std::logic_error(ERROR_ON_VALUE);
    }
}

DictValueContext json::Builder::Key(std::string key)
{
    if (nodes_stack_.empty()) {
        AddKey(&root_, std::move(key));
    } else {
        AddKey(nodes_stack_.back(), std::move(key));
    }
    return *this;
}

BaseBuilder Builder::Value(Node value)
{
    if (nodes_stack_.empty()) {
        if (wait_build_) {
            throw std::logic_error(ERROR_ON_VALUE);
        }
        if (first_) {
            first_ = false;
            wait_build_ = true;
        }
        AddNodeValue(&root_, std::move(value));
    } else {
        if (!nodes_stack_.back()->IsArray()) {
            AddNodeValue(nodes_stack_.back(), std::move(value));
            nodes_stack_.pop_back();
        } else {
            AddNodeValue(nodes_stack_.back(), std::move(value));
        }
    }
    return *this;
}

DictItemContext Builder::StartDict()
{
    StartArrayAndDict(Node(Dict()), ERROR_ON_START_ARRAY);
    return *this;
}

ArrayItemContext Builder::StartArray()
{
    StartArrayAndDict(Node(Array()), ERROR_ON_START_DIRT);
    return *this;
}

BaseBuilder Builder::EndDict()
{
    EndConteiner<json::Dict>{*this}(ERROR_ON_END_DIRT);
    return *this;
}

BaseBuilder Builder::EndArray()
{
    EndConteiner<json::Array>{*this}(ERROR_ON_END_ARRAY);
    return *this;
}

Node Builder::Build()
{
    if (!nodes_stack_.empty() || !wait_build_) {
        throw std::logic_error(ERROR_BUILD);
    }
    return root_;
}

void Builder::AddKey(Node *node, std::string &&key)
{
    if (node->IsDict()) {
        nodes_stack_.push_back(&node->AsDict()[std::move(key)]);
    } else {
        throw std::logic_error(ERROR_ON_KEY);
    }
}

void Builder::StartArrayAndDict(Node &&node, const std::string &error_text)
{
    if (nodes_stack_.empty()) {
        if (first_) {
            first_ = false;
            root_ = std::move(node);
        } else if (root_.IsArray()) {
            root_.AsArray().emplace_back(std::move(node));
            nodes_stack_.push_back(&root_.AsArray().back());
        } else {
            throw std::logic_error(error_text);
        }
    } else {
        if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(std::move(node));
            nodes_stack_.push_back(&nodes_stack_.back()->AsArray().back());
        } else if (!nodes_stack_.back()->IsDict()) {
            *nodes_stack_.back() = std::move(node);
        } else {
            throw std::logic_error(error_text);
        }
    }
}

DictValueContext BaseBuilder::Key(std::string key)
{
    GetBuilder().Key(std::move(key));
    return GetBuilder();
}

ArrayItemContext BaseBuilder::Value(Node value)
{
    GetBuilder().Value(std::move(value));
    return GetBuilder();
}

DictItemContext BaseBuilder::StartDict()
{
    GetBuilder().StartDict();
    return GetBuilder();
}

ArrayItemContext BaseBuilder::StartArray()
{
    GetBuilder().StartArray();
    return GetBuilder();
}

BaseBuilder BaseBuilder::EndDict()
{
    GetBuilder().EndDict();
    return GetBuilder();
}

BaseBuilder BaseBuilder::EndArray()
{
    GetBuilder().EndArray();
    return GetBuilder();
}

Node BaseBuilder::Build()
{
    return builder_.Build();
}

Builder &BaseBuilder::GetBuilder()
{
    return builder_;
}

DictItemContext DictValueContext::Value(Node value)
{
    GetBuilder().Value(std::move(value));
    return GetBuilder();
}

}//namespace json
