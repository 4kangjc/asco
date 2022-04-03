#pragma once

#include <jsoncc/json>
#include <list>
#include <set>
#include <unordered_set>
#include <unordered_map>

// #include <boost/lexical_cast.hpp>

namespace asco {

template <typename F, typename T>
struct LexicalCastJson {
    T operator() (const F& v) {
        // return boost::lexical_cast<T>(v);
        T result;
        std::stringstream ss;
        ss << v;
        ss >> result;
        return result;
    }
};

// vector
template <typename T>
struct LexicalCastJson<std::string, std::vector<T>> {
    decltype(auto) operator() (const std::string& v) {
        auto root_ptr = json::value::parse(v);
        if (root_ptr) {
            std::stringstream ss;
            std::vector<T> vec;
            for (const auto& node : *root_ptr) {
                ss.str("");
                ss << node;
                vec.push_back(LexicalCastJson<std::string, T>()(ss.str()));
            }
            return vec;
        } 
        return {};
    }
};

template <typename T>
struct LexicalCastJson<std::vector<T>, std::string> {
    decltype(auto) operator() (const std::vector<T>& vec) {
        json::value root;
        for (auto& v : vec) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(v));
            if (node_ptr) {
                root.push_back(std::move(*node_ptr));
            }
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};


template <typename T>
struct LexicalCastJson<std::string, std::list<T>> {
    decltype(auto) operator() (const std::string& v) {
        auto node_ptr = json::value::parse(v);
        if (node_ptr) {
            std::stringstream ss;
            std::list<T> vec;
            for (const auto& node : *node_ptr) {
                ss.str("");
                ss << node;
                vec.push_back(LexicalCastJson<std::string, T>()(ss.str()));
            }
            return vec;
        } 
        return {};
    }
};

template <typename T>
struct LexicalCastJson<std::list<T>, std::string> {
    decltype(auto) operator() (const std::vector<T>& vec) {
        json::value root;
        for (auto& v : vec) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(v));
            if (node_ptr) {
                root.push_back(std::move(*node_ptr));
            }
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};


// set 
template <typename T>
struct LexicalCastJson<std::string, std::set<T>> {
    decltype(auto) operator() (const std::string& v) {
        auto node_ptr = json::value::parse(v);
        std::set<T> vec;
        if (node_ptr) {
            std::stringstream ss;
            for (const auto& node : *node_ptr) {
                ss.str("");
                ss << node;
                vec.insert(LexicalCastJson<std::string, T>()(ss.str()));
            }
        } 
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::set<T>, std::string> {
    decltype(auto) operator() (const std::set<T>& vec) {
        json::value root;
        for (auto& v : vec) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(v));
            if (node_ptr) {
                root.push_back(std::move(*node_ptr));
            }
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};

// unordered_set 
template <typename T>
struct LexicalCastJson<std::string, std::unordered_set<T>> {
    decltype(auto) operator() (const std::string& v) {
        auto node_ptr = json::value::parse(v);
        std::unordered_set<T> vec;
        if (node_ptr) {
            std::stringstream ss;
            for (const auto& node : *node_ptr) {
                ss.str("");
                ss << node;
                vec.insert(LexicalCastJson<std::string, T>()(ss.str()));
            }
        } 
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::unordered_set<T>, std::string> {
    decltype(auto) operator() (const std::unordered_set<T>& vec) {
        json::value root;
        for (auto& v : vec) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(v));
            if (node_ptr) {
                root.push_back(std::move(*node_ptr));
            }
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};


// map
template <typename T>
struct LexicalCastJson<std::string, std::map<std::string, T>> {
    decltype(auto) operator() (const std::string& v) {
        auto root_ptr = json::value::parse(v);
        if (root_ptr) {
            std::map<std::string, T> mp;
            std::stringstream ss;
            for (auto it = root_ptr->begin(); it != root_ptr->end(); ++it) {
                ss.str("");
                ss << *it;
                mp.emplace(it.key(), LexicalCastJson<std::string, T>()(ss.str()));
            }
            return mp;
        }
        return {};
    }
};

template <typename T>
struct LexicalCastJson<std::map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::map<std::string, T>& mp) {
        json::value root;
        for (const auto& [x, y] : mp) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(y));
            root[x] = std::move(*node_ptr);
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};

// unordered_map
template <typename T>
struct LexicalCastJson<std::string, std::unordered_map<std::string, T>> {
    decltype(auto) operator() (const std::string& v) {
        auto root_ptr = json::value::parse(v);
        if (root_ptr) {
            std::unordered_map<std::string, T> mp;
            std::stringstream ss;
            for (auto it = root_ptr->begin(); it != root_ptr->end(); ++it) {
                ss.str("");
                ss << *it;
                mp.emplace(it.key(), LexicalCastJson<std::string, T>()(ss.str()));
            }
            return mp;
        }
        return {};
    }
};

template <typename T>
struct LexicalCastJson<std::unordered_map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::unordered_map<std::string, T>& mp) {
        json::value root;
        for (const auto& [x, y] : mp) {
            auto node_ptr = json::value::parse(LexicalCastJson<T, std::string>()(y));
            root[x] = std::move(*node_ptr);
        }
        std::stringstream ss;
        ss << root;
        return ss.str();
    }
};

template <typename T>
using JsonFromStr = LexicalCastJson<std::string, T>;

template <typename T>
using JsonToStr = LexicalCastJson<T, std::string>;

} // namespace asco