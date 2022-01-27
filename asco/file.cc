#include "file.h"
// #include "util.h"
#include "log.h"

#include <filesystem>
#include <unistd.h>
#include <string.h>
#include <signal.h>

namespace fs = std::filesystem;

static auto g_logger = ASCO_LOG_NAME("system");

namespace asco {

template <bool v>
std::string FS::AbsolutePath(std::string_view filename) {
    if (filename[0] == '/') {
        return std::string(filename);
    }
    if constexpr (v) {
        char path[512] = {0};
        ssize_t count = readlink("/proc/self/exe", path, 512);
        if (count <= 0) {
            return "";
        }
        // int n = filename.size();
        // char* c = find(&path[0], path + count, '/', false);
        // memcpy(c + 1, filename.data(), n);
        // c[n + 1] = '\0';
        // return std::string(path, c - path + n + 2);
        auto p = fs::path(path).parent_path()/filename;
        return p;
    }
    return fs::absolute(filename);
}

template <bool v>
bool FS::Mv(std::string_view from, std::string_view to) {
    try {
        if constexpr (v) {
            auto __from = AbsolutePath(from);
            auto __to = AbsolutePath(to);
            fs::rename(__from, __to);
        } else {
            fs::rename(from, to);
        }
    } catch (std::exception& ex) {
        ASCO_LOG_FMT_ERROR(g_logger, "FS Mv Except: {}", ex.what());
        return false;
    } catch (...) {
        ASCO_LOG_ERROR(g_logger) << "FS Mv Except";
        return false;
    }
    return true;
}

// 暂时不提供v, Env模块转换绝对路径
void FS::ListAllFile(std::vector<std::string>& files, std::string_view path, 
                std::string_view subfix) {
    fs::path dir(path);
    if (!fs::exists(dir)) {
        return;
    }
    fs::directory_iterator di(dir);
    for (auto& it : di) {
        if (it.status().type() == fs::file_type::directory) {
            ListAllFile(files, it.path().c_str(), subfix);
        } else if (it.status().type() == fs::file_type::regular) {
            if (subfix.empty()) {
                files.push_back(it.path());
            } else {
                auto s = it.path().c_str();
                auto len = strlen(s);
                if (strncmp(s + len - subfix.size(), subfix.data(), subfix.size()) == 0) {
                    files.push_back(it.path());
                }
            }
        }
    }
}

template <bool v>
bool FS::Mkdir(std::string_view dirname) {
    if constexpr (v) {
        auto s = AbsolutePath(dirname);
        return Mkdir(s);
    }
    fs::path dir(dirname);
    if (fs::exists(dir)) {
        return true;
    }
    return fs::create_directories(dirname);
}

template <bool v>
bool FS::Rm(std::string_view path) {
    if (v) {
        auto s = AbsolutePath(path);
        return Rm(s);
    }
    try {
        fs::remove_all(path);
    } catch (std::exception& ex) {
        ASCO_LOG_FMT_ERROR(g_logger, "FS Rm Except: {}", ex.what());
        return false;
    } catch (...) {
        ASCO_LOG_ERROR(g_logger) << "Fs Rm Except";
        return false;
    }
    return true;
}

bool FS::IsRunningPidfile(std::string_view pidfile) {
    std::ifstream ifs(pidfile.data());
    std::string line;
    if (!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if (line.empty()) {
        return false;
    }
    pid_t pid = ::atoi(line.c_str());
    if (pid <= 1) {
        return false;
    }
    if (kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

template <bool v>
bool FS::OpenForWrite(std::ofstream& ofs, std::string_view filename,
            std::ios_base::openmode mode) {
    if constexpr (v) {
        return FS::OpenForWrite(ofs, AbsolutePath(filename), mode);
    }
    ofs.open(filename.data(), mode);
    if (!ofs.is_open()) {
        fs::path file(filename);
        fs::create_directories(file.parent_path());
        ofs.open(filename.data(), mode);
    }
    return ofs.is_open();
}

template <bool v>
bool FS::OpenForRead(std::ifstream& ifs, std::string_view filename,
            std::ios_base::openmode mode) {
    if constexpr (v) {
        return FS::OpenForRead(ifs, AbsolutePath(filename), mode);
    }
    ifs.open(filename.data(), mode);
    return ifs.is_open();
}

template <bool v>
uint64_t FS::LastWriteTime(std::string_view filename) {
    if constexpr (v) {
        return FS::LastWriteTime(AbsolutePath(filename));
    }
    auto ftime = fs::last_write_time(filename);
    return std::chrono::duration_cast<std::chrono::microseconds>(ftime.time_since_epoch()).count();
}

namespace {

// 实例化模板 为了编译通过 并不调用
void __FILE__INIT__() {
    FS::AbsolutePath<false>("");
    FS::AbsolutePath<true>("");
    FS::LastWriteTime<false>("");
    FS::LastWriteTime<true>("");
    FS::Mkdir<false>("");
    FS::Mkdir<true>("");
    FS::Mv<false>("", "");
    FS::Mv<true>("", "");
    std::ifstream ifs;
    FS::OpenForRead<true>(ifs, "", std::ios::app);
    FS::OpenForRead<false>(ifs, "", std::ios::app);
    std::ofstream ofs;
    FS::OpenForWrite<true>(ofs, "", std::ios::app);
    FS::OpenForWrite<false>(ofs, "", std::ios::app);
}

}

} // namespace asco