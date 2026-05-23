#pragma once
#include "json.h"

#include<cassert>

namespace json {
class BaseBuilder;
class DictItemContext;
class DictValueContext;
class ArrayItemContext;
template <typename T>
class EndConteiner;

class Builder{
public:
    Builder() = default;

    DictValueContext Key(std::string key);
    BaseBuilder Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseBuilder EndDict();
    BaseBuilder EndArray();
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
    bool first_ = true;
    bool wait_build_ = false;

    void AddKey(Node* node, std::string&& key);
    void StartArrayAndDict(Node&& node, const std::string& error_text);

    friend class EndConteiner<json::Dict>;
    friend class EndConteiner<json::Array>;

};

class BaseBuilder{
public:
    BaseBuilder(Builder& builder): builder_(builder){}
    DictValueContext Key(std::string key);
    ArrayItemContext Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseBuilder EndDict();
    BaseBuilder EndArray();
    Node Build();
    
protected:
    Builder& GetBuilder();
private:
    Builder& builder_;
};

class DictItemContext: public BaseBuilder{
public:
    DictItemContext(Builder& builder): BaseBuilder(builder){}
    ArrayItemContext Value(Node value) = delete;
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    BaseBuilder EndArray() = delete;
    Node Build() = delete;
};

class DictValueContext: public BaseBuilder{
public:
    DictValueContext(Builder& builder): BaseBuilder(builder){}
    DictValueContext Key(std::string key) = delete;
    DictItemContext Value(Node value);
    BaseBuilder EndDict() = delete;
    BaseBuilder EndArray() = delete;
    Node Build() = delete;
};

class ArrayItemContext: public BaseBuilder{
public:
    ArrayItemContext(Builder& builder): BaseBuilder(builder){}
    DictValueContext Key(std::string key) = delete;
    BaseBuilder EndDict() = delete;
    Node Build() = delete;
};

template <typename T>
class EndConteiner {
public:
    EndConteiner(Builder& builder): builder_(builder){}

    void operator() (const std::string& error_text) {
        if (builder_.nodes_stack_.empty()) {
            if (CheckType(builder_.root_)) {
                builder_.wait_build_ = true;
                return;
            }
        } else if (CheckType(*builder_.nodes_stack_.back())) {
            builder_.nodes_stack_.pop_back();
            return;
        }
        throw std::logic_error(error_text);
    }

private:
    Builder& builder_;

    bool CheckType(const Node& node) const{
        return std::is_same_v<T, json::Array> ? node.IsArray(): node.IsDict();
    }
};

}
