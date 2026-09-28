/**
 * @file external_process.cpp
 * @brief Implements shell-free host process execution for CLOCK Storybook publication tooling.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/external_process.h"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace clockfw::sim::tutorial {
namespace {

#ifdef _WIN32
constexpr char kPathSeparator = ';';
#else
constexpr char kPathSeparator = ':';
#endif

bool executableCandidate(const std::filesystem::path& candidate) {
    std::error_code error;
    return std::filesystem::is_regular_file(candidate, error) && !error;
}

std::vector<std::filesystem::path> pathDirectories() {
    std::vector<std::filesystem::path> directories;
    const char* raw = std::getenv("PATH");
    if (raw == nullptr) return directories;
    std::string path(raw);
    std::size_t start = 0U;
    while (start <= path.size()) {
        const std::size_t end = path.find(kPathSeparator, start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty()) directories.emplace_back(part);
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return directories;
}

}  // namespace

std::optional<std::filesystem::path> findHostExecutable(const std::string& name) {
    if (name.empty()) return std::nullopt;
    const std::filesystem::path supplied(name);
    if (supplied.has_parent_path() && executableCandidate(supplied)) return supplied;
    for (const auto& directory : pathDirectories()) {
        const auto candidate = directory / supplied;
        if (executableCandidate(candidate)) return candidate;
#ifdef _WIN32
        const auto executable = directory / (name + ".exe");
        if (executableCandidate(executable)) return executable;
#endif
    }
    return std::nullopt;
}

int runHostProcess(const std::vector<std::string>& arguments) {
    if (arguments.empty() || arguments.front().empty()) {
        throw std::runtime_error("host process requires a non-empty executable argument");
    }
#ifdef _WIN32
    std::vector<std::wstring> wide;
    wide.reserve(arguments.size());
    for (const auto& argument : arguments) wide.push_back(std::filesystem::u8path(argument).wstring());
    std::vector<const wchar_t*> argv;
    argv.reserve(wide.size() + 1U);
    for (const auto& argument : wide) argv.push_back(argument.c_str());
    argv.push_back(nullptr);
    const intptr_t result = _wspawnv(_P_WAIT, wide.front().c_str(), argv.data());
    if (result == -1) throw std::runtime_error("failed to start host process: " + arguments.front());
    return static_cast<int>(result);
#else
    const pid_t child = fork();
    if (child < 0) throw std::runtime_error("failed to fork host publication process");
    if (child == 0) {
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1U);
        for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);
        execv(arguments.front().c_str(), argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) throw std::runtime_error("failed waiting for host publication process");
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 128;
#endif
}

}  // namespace clockfw::sim::tutorial
