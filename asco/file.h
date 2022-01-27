#pragma once
#include <string>
#include <vector>
#include <fstream>

namespace asco {

// 文件系统 实现依靠C++17 filesystem
// v: 是否与与运行目录无关
struct FS {
    // 得到绝对路径
    template <bool v = true>
    static std::string AbsolutePath(std::string_view filename);
    // 列出path路径下的所有以subfix结尾的文件
    static void ListAllFile(std::vector<std::string>& files, std::string_view path, 
                std::string_view subfix);
    // 建立文件夹
    template <bool v = false>
    static bool Mkdir(std::string_view dirname);
    // 删除文件或文件夹
    template <bool v = false>
    static bool Rm(std::string_view path);
    // 移动或重命名文件或文件夹
    template <bool v = false>
    static bool Mv(std::string_view from, std::string_view to);

    static bool IsRunningPidfile(std::string_view pidfile);
    template <bool v = false>
    // open不支持std::string_view, 注意std::string_view 以 \\0 结尾
    static bool OpenForRead(std::ifstream& ifs, std::string_view  filename,
                std::ios_base::openmode mode);
    // open不支持std::string_view, 注意std::string_view 以 \\0 结尾
    template <bool v = false>
    static bool OpenForWrite(std::ofstream& ofs, std::string_view filename,
                std::ios_base::openmode mode);
    // 上次文件修改时间
    template <bool v = false>
    static uint64_t LastWriteTime(std::string_view filename);
};

} // namespace asco
