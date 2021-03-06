#pragma once

#include "lexical.h"
#include "mutex.h"
#include "util.h"
#include "log.h"

namespace asco {

class ConfigVarBase {
public:
    using ptr = std::shared_ptr<ConfigVarBase>;
    ConfigVarBase(std::string_view name, const std::string_view description) : 
        name_(name), description_(description) {}
    virtual ~ConfigVarBase() {}
    auto& getName() const { return name_; }
    auto& getDescription() const { return description_; }
    
    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const = 0;
protected:
    std::string name_;
    std::string description_;
};


template <typename T, typename FromStr = JsonFromStr<T>, typename ToStr = JsonToStr<T>>
class ConfigVar : public ConfigVarBase {
public: 
    using on_change_cb = std::function<void(const T& old_value, const T& new_value)>;
    ConfigVar(std::string_view name, const T& default_val, std::string_view description = "")
        : ConfigVarBase(name, description), val_(default_val) { }

    std::string toString() override {
        try {
            READLOCK(mutex_);
            return ToStr()(val_);
        } catch (std::exception& e) {
            ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "ConfigVar::toString exception " << e.what() 
             << ", convert: " << TypeToName<T>() << " to String, name =  " << name_;
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
            setValue(FromStr()(val));
        } catch (std::exception& e) {
            ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what()
             << ", convert: string to " << TypeToName<T>() << ", name = " << name_ << ", val = " << val;
        }
        return false;
    }

    std::string getTypeName() const override {
        return TypeToName<T>();
    }

    const T& getValue() const {
        READLOCK(mutex_);
        return val_;
    }

    void setValue(const T& value) {
        {
            READLOCK(mutex_);
            if (value == val_) {
                return;
            }
            for (auto& [key, cb] : cbs_) {
                cb(val_, value);
            }
        }
        WRITELOCK(mutex_);
        val_ = value;
    }

    uint64_t addListener(on_change_cb cb) {
        static uint64_t key = 0;
        WRITELOCK(mutex_);
        ++key;
        cbs_[key] = cb;
        return key;
    }

    void delListener(uint64_t key) {
        WRITELOCK(mutex_);
        cbs_.erase(key);
    }

    auto getListener(uint64_t key) const {
        READLOCK(mutex_);
        auto it = cbs_.find(key);
        return it == cbs_.end() ? nullptr : it->second;
    }

    void clearListener() {
        WRITELOCK(mutex_);
        cbs_.clear();
    }
private:
    T val_;
    std::unordered_map<uint64_t, on_change_cb> cbs_;
    mutable rw_mutex mutex_;
};

class Config {
public:
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

    template<typename T, typename FromStr = JsonFromStr<T>, typename ToStr = JsonToStr<T>>
    static std::shared_ptr<ConfigVar<T, FromStr, ToStr>> Lookup(const std::string& name, 
            const T& default_val, std::string_view description = "") {
        WRITELOCK(GetMutex());
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T, FromStr, ToStr>>(it->second);
            if (tmp) {
                ASCO_LOG_INFO(ASCO_LOG_ROOT()) << "Lookup name = " << name << " exits";
            } else {
                ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "Lookup name = " << name << " exits but type not "
                 << TypeToName<T>() << ", real_type = " << it->second->getTypeName() << " value = "
                 << it->second->toString();
                return nullptr;
            }
        }

        if (name.find_first_not_of("abcdefghijkmlnopqrstuvwxyz._0123456789") != name.npos) {
            ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "Lookup me invalid " << name;
            throw std::invalid_argument(name);
        }

        auto v = std::make_shared<ConfigVar<T, FromStr, ToStr>>(name, default_val, description);
        GetDatas()[name] = v;
        return v;
    }
    static void LoadFromJson(const json::value& root);
    static void LoadFromConDir(std::string_view path);

    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
private:
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }
    static rw_mutex& GetMutex() {
        static rw_mutex mutex_;
        return mutex_;
    }
};

} // namespace asco