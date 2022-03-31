#include "log.h"
#include "file.h"
#include "config.h"
#include <iostream>
#include <functional>

namespace asco {

/********************* LogLevel *********************************/
static const char* logLevel[6] = { "TRACE","DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
static std::unordered_map<std::string_view, LogLevel::Level> mp {
        {"TRACE", LogLevel::Level::TRACE}, {"DEBUG", LogLevel::Level::DEBUG}, 
        {"INFO",  LogLevel::Level::INFO},  {"WARN",  LogLevel::Level::WARN},  
        {"ERROR", LogLevel::Level::ERROR}, {"FATAL", LogLevel::Level::FATAL}, 
        {"trace", LogLevel::Level::TRACE}, {"info",  LogLevel::Level::INFO}, 
        {"debug", LogLevel::Level::DEBUG}, {"warn",  LogLevel::Level::WARN}, 
        {"error", LogLevel::Level::ERROR} };

const char* LogLevel::ToString(LogLevel::Level level) {
    if (level >= 0 && level < 6) {
        return logLevel[level];
    }
    return "UNKONW";
}

LogLevel::Level LogLevel::FromString(std::string_view s) {
    auto it = mp.find(s);
    return it == mp.end() ? TRACE : it->second;
}

/******************** LogFormatter::FormatItem *********************/
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getCountent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << LogLevel::ToString(context->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getElapse();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getThreadName();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getLogger()->getName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(std::string_view format = "%Y-%m-%d %H:%M:%S") : m_format(format) {
        if (m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        struct tm tm;
        time_t time = context->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getFile();
    }
};

class FuncnameFormatItem : public LogFormatter::FormatItem {
public:
    FuncnameFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getFunc();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << context->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(std::string_view str = "") {}
    void format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) override {
        os << "\t";
    }
};

Logger::Logger(std::string_view name) : name_(name), formatter_(std::make_shared<LogFormatter>(
    "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n"
)) {}

void Logger::log(LogContext::ptr& contex) {
    auto level = contex->getLevel();
        if (level >= level_) {
        if (!appenders_.empty()) {
            LOCK_GUARD(mutex_);
            auto self = shared_from_this();
            for (auto& i : appenders_) {
                i->log(self, contex);
            }
        } else {
            if (root_) {
                root_->log(contex);
            }
        }
    }
}

void Logger::addAppender(const LogAppender::ptr& appender) {
    LOCK_GUARD(mutex_);
    if (!appender->getFormatter()) {
        LOCK_GUARD(appender->mutex_);
        appender->formatter_ = formatter_;
    }
    appenders_.insert(appender);
}

inline void Logger::delAppender(const LogAppender::ptr& appender) {
    LOCK_GUARD(mutex_);
    appenders_.erase(appender);
}

void Logger::setFormatter(const LogFormatter::ptr& val) {
    LOCK_GUARD(mutex_);
    formatter_ = val;
    for (auto& i : appenders_) {
        if (!i->hasFormatter_) {
            i->setFormatter(formatter_);
        }
    }
}

void Logger::setFormatter(std::string_view fmt) {
    auto new_value = std::make_shared<LogFormatter>(fmt);
    if (new_value->isError()) {
        printf("Logger setFormatter name = %s, value = %.*s invalid formatter", 
            name_.c_str(), (int)fmt.size(), fmt.data());
        return;
    }
    setFormatter(new_value);
}

inline auto Logger::getFormatter() const {
    LOCK_GUARD(mutex_);
    return formatter_;
}

// std::string Logger::toYamlString() const {
//     LOCK_GUARD(mutex_);
//     YAML::Node node;
//     node["name"] = name_;
//     node["level"] = LogLevel::ToString(level_);
//     if (formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     for (const auto& appender : appenders_) {
//         node["appenders"].push_back(YAML::Load(appender->toYamlString()));
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// std::string Logger::toJsonString() const {
//     LOCK_GUARD(mutex_);
//     Json::Value node;
//     node["name"] = name_;
//     node["level"] = LogLevel::ToString(level_);
//     if (formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     Json::Reader r;
//     for (const auto& appender : appenders_) {
//         Json::Value v;
//         r.parse(appender->toJsonString(), v);
//         node["appenders"].append(std::move(v));
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

/********************* LogAppender ************************************/

 void LogAppender::setFormatter(const LogFormatter::ptr& val) {
    LOCK_GUARD(mutex_);
    formatter_ = val;
    if (formatter_) {
        hasFormatter_ = true;
    } else {
        hasFormatter_ = false;
    }
 }

void StdoutLogAppender::log(Logger::ptr& logger, LogContext::ptr& contex) {
    auto level = contex->getLevel();
    if (level >= level_) {
        LOCK_GUARD(mutex_);
        formatter_->format(std::cout, logger, contex);
    }
}

// std::string StdoutLogAppender::toYamlString() const {
//     LOCK_GUARD(mutex_);
//     YAML::Node node;
//     node["type"] = "StdoutLogAppender";
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// std::string StdoutLogAppender::toJsonString() const {
//     LOCK_GUARD(mutex_);
//     Json::Value node;
//     node["type"] = "StdoutLogAppender";
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

FileLogAppender::FileLogAppender(std::string_view filename) : filename_(filename) {
    reopen();
}

void FileLogAppender::log(Logger::ptr& logger, LogContext::ptr& contex) {
    auto level = contex->getLevel();
    if (level >= level_) {
        LOCK_GUARD(mutex_);
        formatter_->format(filestream_, logger, contex);
    }
}

bool FileLogAppender::reopen() {
    LOCK_GUARD(mutex_);
    if (filestream_) {
        filestream_.close();
    }
    return FS::OpenForWrite<true>(filestream_, filename_, std::ios::app);
}

// std::string FileLogAppender::toYamlString() const {
//     LOCK_GUARD(mutex_);
//     YAML::Node node;
//     node["type"] = "FileLogAppender";
//     node["file"] = filename_;
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// std::string FileLogAppender::toJsonString() const {
//     LOCK_GUARD(mutex_);
//     Json::Value node;
//     node["type"] = "FileLogAppender";
//     node["file"] = filename_;
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// ServerLogAppender::ServerLogAppender(std::string_view host) {
//     addr_ = Address::LookupAnyIPAddress(host, AF_UNSPEC);
//     if (!addr_) {
//         ASCO_LOG_ERROR(ASCO_LOG_ROOT()) << "get address failed";
//     }
//     sock_ = Socket::CreateTCP(addr_->getFamily());
//     sock_->connect(addr_);
// }

// void ServerLogAppender::log(Logger::ptr& logger, LogContext::ptr& context) {
//     if (!sock_->isConnected()) {
//         sock_->connect(addr_);
//     }
//     auto level = context->getLevel();
//     if (level >= level_) {
//         LOCK_GUARD(mutex_);
//         sock_->send(formatter_->format(logger, context));
//     }
// }

// std::string ServerLogAppender::toYamlString() const {
//     LOCK_GUARD(mutex_);
//     YAML::Node node;
//     node["type"] = "FileLogAppender";
//     node["file"] = addr_->toString();
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// std::string ServerLogAppender::toJsonString() const {
//     LOCK_GUARD(mutex_);
//     Json::Value node;
//     node["type"] = "FileLogAppender";
//     node["file"] = addr_->toString();
//     node["level"] = LogLevel::ToString(level_);
//     if (hasFormatter_ && formatter_) {
//         node["formatter"] = formatter_->getPattern();
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

/*********************** LogFormatter *******************/

LogFormatter::LogFormatter(std::string_view pattern) : pattern_(pattern) {
    init();
}

void LogFormatter::init() {
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    size_t size = pattern_.size();
    for (size_t i = 0; i < size; ++i) {
        if (pattern_[i] != '%') {
            nstr.push_back(pattern_[i]);
            continue;
        }
        if (i + 1 < size && pattern_[i + 1] == '%') {
            nstr.push_back('%');
            i++;
            continue;
        }
        size_t j = i + 1;
        size_t fmt_status = 0, fmt_begin = 0;

        std::string str, fmt;
        for (; j < size; ++j) {
            if (!fmt_status && !isalpha(pattern_[j]) && pattern_[j] != '{' && pattern_[j] != '}') {
                str = pattern_.substr(i + 1, j - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if (pattern_[j] == '{') {
                    str = pattern_.substr(i + 1, j - i - 1);
                    fmt_status = 1;
                    fmt_begin = j;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (pattern_[j] == '}') {
                    fmt = pattern_.substr(fmt_begin + 1, j - fmt_begin - 1);
                    fmt_status = 0;
                    j++;
                    break;
                }
            }
            if (j == size - 1 && str.empty()) {
                str = pattern_.substr(i + 1);
            }
        }
        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.emplace_back(nstr, "", 0);
                nstr.clear();
            }
            vec.emplace_back(str, fmt, 1);
            i = j - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << pattern_ << " - " << pattern_.substr(i) << std::endl;
            error_ = true;
        }
    }
    if (!nstr.empty()) {
        vec.emplace_back(nstr, "", 0);
    }
    static std::unordered_map<std::string, std::function<FormatItem::ptr(std::string_view)>> s_format_items = {
    #define XX(str, C) \
        {#str, [](std::string_view fmt) { return FormatItem::ptr(new C(fmt)); } }
        XX(m, MessageFormatItem),           // %m 消息体
        XX(p, LevelFormatItem),             // %p level
        XX(r, ElapseFormatItem),            // %r 启动后的时间
        XX(c, NameFormatItem),              // %c:日志名称
        XX(t, ThreadIdFormatItem),          // %t 线程id
        XX(n, NewLineFormatItem),           // %n 回车换行
        XX(d, DateTimeFormatItem),          // %d 时间
        XX(f, FilenameFormatItem),          // %f 文件名
        XX(l, LineFormatItem),              // %l 行号
        XX(u, FuncnameFormatItem),          // %u 函数名
        XX(T, TabFormatItem),               // %T tab
        XX(F, FiberIdFormatItem),           // %F 协程id
        XX(N, ThreadNameFormatItem),        // %N 线程名称
    #undef XX
    };


    for (auto& [str, format, type] : vec) {
        if (type == 0) {
            items_.push_back(FormatItem::ptr(new StringFormatItem(str)));
        } else {
            auto it = s_format_items.find(str);
            if (it == s_format_items.end()) {
                items_.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + str + ">>")));
                error_ = true;
            } else {
                items_.push_back(it->second(format));
            }
        }
    }
}

std::string LogFormatter::format(Logger::ptr& logger, LogContext::ptr& context) {
    std::stringstream ss;
    for (auto& item : items_) {
        item->format(ss, logger, context);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& os, Logger::ptr& logger, LogContext::ptr& context) {
    for (auto& item : items_) {
        item->format(os, logger, context);
    }
    return os;
}

/************************* LogManager  ************************************/
LoggerManager::LoggerManager() : root_(new Logger) {
    root_->addAppender(LogAppender::ptr(new StdoutLogAppender));
    loggers_[root_->name_] = root_;
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    LOCK_GUARD(mutex_);
    if (auto it = loggers_.find(name); it != loggers_.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));   
    logger->root_ = root_;
    loggers_[name] = logger;
    return logger;
}

// std::string LoggerManager::toYamlString() {
//     LOCK_GUARD(mutex_);
//     YAML::Node node;
//     for (const auto& [_, logger] : loggers_) {
//         node.push_back(YAML::Load(logger->toYamlString()));
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

// std::string LoggerManager::toJsonString() {
//     LOCK_GUARD(mutex_);
//     Json::Value node;
//     Json::Reader r;
//     for (const auto& [_, logger] : loggers_) {
//         Json::Value v;
//         r.parse(logger->toYamlString(), v);
//         node.append(std::move(v));
//     }
//     std::stringstream ss;
//     ss << node;
//     return ss.str();
// }

/*************************** Log Config  ***********************************/

struct LogAppenderDefine {
    int type = 0;               // 1 file 2 stdout 3 server
    LogLevel::Level level = LogLevel::Level::TRACE;
    std::string formatter;
    std::string fist;           // 1: filename or 3: hostname + port

    bool operator==(const LogAppenderDefine& other) const {
        return type == other.type && level == other.level 
        && formatter == other.formatter && fist == other.fist;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::Level::TRACE;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& other) const {
        return name == other.name && level == other.level 
        && formatter == other.formatter 
        && appenders == other.appenders;
    }

    bool operator<(const LogDefine& other) const {
        return name < other.name;
    }
};


template <>
struct LexicalCastJson<std::string, LogDefine> {
    decltype(auto) operator() (const std::string& v) {
        auto root_ptr = json::value::parse(v);
        auto& root = *root_ptr;

        if (!root_ptr->count("name")) {
            std::cout << "log config error: name is null, " << root << std::endl;
            throw std::logic_error("log config name is null");
        }


        std::stringstream ss;
        LogDefine l;
        l.name = root["name"].as_string();
        if (auto it = root.find("level"); it != root.end()) {
            l.level = LogLevel::FromString(it->as_string());
        } else {
            l.level = LogLevel::FromString("");
        }
        
        if (auto it = root.find("formatter"); it != root.end()) {
            l.formatter = it->as_string();
        }
        
        if (auto it = root.find("appenders"); it != root.end()) {
            for (auto& a : *it) {
                if (!a.count("type")) {
                    std::cout << "log config error appender type is null, " << a << std::endl;
                    continue;
                }

                LogAppenderDefine lad;
                if (auto lit = a.find("level"); lit != a.end()) {
                    lad.level = LogLevel::FromString(lit->as_string());
                } else {
                    lad.level = l.level;
                }
                
                auto type = a["type"].as_string();
                if (type == "FileLogAppender") {
                    lad.type = 1;
                    if (auto fit = a.find("file"); fit == a.end()) {
                        std::cout << "log config error: fileappender file is null, " << a << std::endl;
                        continue;
                    } else {
                        lad.fist = fit->as_string();
                    }
                } else if (type == "StdoutLogAppender") {
                    lad.type = 2;
                } else if (type == "ServerLogAppender") {
                    lad.type = 3;
                    if (auto hit = a.find("host"); hit == a.end()) {
                        std::cout << "log config error: serverappender host is null, " << a << std::endl;
                        continue;
                    } else {
                        lad.fist = hit->as_string();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a << std::endl;
                    continue;
                }

                if (auto fit = a.find("formatter"); fit != a.end()) {
                    lad.formatter = fit->as_string();
                }

                l.appenders.push_back(std::move(lad));
            }
        }
        
        return l;
    }
};

template <>
struct LexicalCastJson<LogDefine, std::string> {
    decltype(auto) operator() (const LogDefine& l) {
        json::value root;
        root["name"] = l.name;
        root["level"] = LogLevel::ToString(l.level);

        if (!l.formatter.empty()) {
            root["formatter"] = l.formatter;
        }

        for (const auto& appender : l.appenders) {
            json::value na;
            switch (appender.type) {
                case 1 : {
                    na["type"] = "FileLogAppender";
                    na["file"] = appender.fist;
                    break;
                }
                case 2 : {
                    na["type"] = "StdoutLogAppender";
                    break;
                }
                case 3 : {
                    na["type"] = "ServerLogAppender";
                    na["host"] = appender.fist;
                    break;
                }
                default:
                    break;
            }

            if (appender.level != LogLevel::UNKOWN) {
                na["level"] = LogLevel::ToString(appender.level);
            }
            if (!appender.formatter.empty()) {
                na["formatter"] = appender.formatter;
            }

            root["appenders"].push_back(std::move(na));
        }

        std::stringstream ss;
        ss << root;
        return ss.str();        
    }
};

using log_conf_type = std::set<LogDefine>;
auto g_log_defines = Config::Lookup("logs", std::set<LogDefine>(), "logs config for json");

struct LogIniter {
    static void on_log_change(const log_conf_type& old_value, const log_conf_type& new_value) {
        ASCO_LOG_INFO(ASCO_LOG_ROOT()) << "on_logger_conf_changed";
        for (auto& i : new_value) {
            auto it = old_value.find(i);
            Logger::ptr logger;
            if (it == old_value.end()) {                // 新增logger
                logger = ASCO_LOG_NAME(i.name);
            } else {                                    // 修改logger 
                if (!(i == *it)) {
                    logger = ASCO_LOG_NAME(i.name);
                } else {
                    continue;
                }
            }
            logger->setLevel(i.level);
            if (!i.formatter.empty()) {
                logger->setFormatter(i.formatter);
            }
            logger->clearAppender();
            for (auto& a : i.appenders) {
                LogAppender::ptr ap;
                switch (a.type) {
                    case 1 : 
                        ap.reset(new FileLogAppender(a.fist));
                        break;
                    case 2 :
                        ap.reset(new StdoutLogAppender);
                        break;
                    case 3 :
                        // ap.reset(new ServerLogAppender(a.fist));
                        break;
                    default:
                        break;
                }
                ap->setLevel(a.level);
                if (!a.formatter.empty()) {
                    auto fmt = std::make_shared<LogFormatter>(a.formatter);
                    if (fmt->isError()) {
                        std::cout << "log.name = " << i.name << " appender type = " << a.type
                        << " formatter = " << a.formatter << " is invalid" << std::endl;
                    } else {
                        ap->setFormatter(fmt);
                    }
                }
                logger->addAppender(ap);
            }
        }
        
        for (auto& i : old_value) {
            auto it = new_value.find(i);
            if (it == new_value.end()) {
                auto logger = ASCO_LOG_NAME(i.name);
                logger->setLevel(LogLevel::UNKOWN);
                logger->clearAppender();
            }
        }
    }

    LogIniter() {
        g_log_defines->addListener(on_log_change);
    }
};

static LogIniter __log_init;


} // namespace asco