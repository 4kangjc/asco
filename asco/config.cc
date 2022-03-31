#include "config.h"
#include "file.h"

namespace asco {

static auto g_logger = ASCO_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    READLOCK(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    READLOCK(GetMutex());
    ConfigVarMap& m = GetDatas();
    for (auto& [key, ptr] : m) {
        cb(ptr);
    }
}

static void ListAllMember(const std::string& prefix, const json::value& node, std::vector<std::pair<std::string, const json::value*>>& output) {
    if (prefix.find_first_not_of("abcdefghijkmlnopqrstuvwxyz._0123456789") != prefix.npos) {
        ASCO_LOG_INFO(g_logger) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.emplace_back(prefix, &node);
    if (node.is_object()) {
        for (auto it = node.cbegin(); it != node.cend(); ++it) {
            ListAllMember(prefix.empty() ? it.key() : prefix + "." + it.key(), *it, output);
        }
    }
}

void Config::LoadFromJson(const json::value& root) {
    std::vector<std::pair<std::string, const json::value*>> all_nodes;
    ListAllMember("", root, all_nodes);
    for (auto& [key, node] : all_nodes) {
        if (key.empty()) {
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        auto var = LookupBase(key);
        if (var) {
            std::stringstream ss;
            ss << *node;
            var->fromString(ss.str());
        }
    }
}

static std::map<std::string, uint64_t> s_filelastmodtime;
static mutex s_mutex;

void Config::LoadFromConDir(std::string_view path) {
    std::string absolute_path;
    absolute_path = FS::AbsolutePath(path);

    std::vector<std::string> files;

    FS::ListAllFile(files, absolute_path, ".json");

    for (const auto& file : files) {
        auto t = FS::LastWriteTime(file);
        {
            LOCK_GUARD(s_mutex);
            if (s_filelastmodtime[file] == t) {
                continue;
            }
            s_filelastmodtime[file] = t;
        }
        try {
            std::ifstream is(file);
            auto root_ptr = json::value::parse(is);
            LoadFromJson(*root_ptr);
    
            ASCO_LOG_INFO(g_logger) << "LoadConfFile file = " << file << " ok";
        } catch (...) {
            ASCO_LOG_ERROR(g_logger) << "LoadConfFile file = " << file << " failed";
        }
    }
}


} // namespace asco