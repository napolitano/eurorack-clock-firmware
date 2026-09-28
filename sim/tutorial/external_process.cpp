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
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
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

#ifdef _WIN32
std::wstring quoteWindowsArgument(const std::wstring_view argument) {
    if (argument.empty()) return L"\"\"";
    if (argument.find_first_of(L" \t\n\v\"") == std::wstring_view::npos) {
        return std::wstring(argument);
    }

    std::wstring quoted;
    quoted.push_back(L'\"');
    std::size_t backslashes = 0U;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            quoted.append(backslashes * 2U + 1U, L'\\');
            quoted.push_back(L'\"');
            backslashes = 0U;
            continue;
        }
        quoted.append(backslashes, L'\\');
        backslashes = 0U;
        quoted.push_back(character);
    }
    quoted.append(backslashes * 2U, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

std::wstring windowsCommandLine(const std::vector<std::wstring>& arguments) {
    std::wstring commandLine;
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
        if (index != 0U) commandLine.push_back(L' ');
        commandLine += quoteWindowsArgument(arguments[index]);
    }
    return commandLine;
}
#endif

int runHostProcess(const std::vector<std::string>& arguments) {
    if (arguments.empty() || arguments.front().empty()) {
        throw std::runtime_error("host process requires a non-empty executable argument");
    }
#ifdef _WIN32
    std::vector<std::wstring> wide;
    wide.reserve(arguments.size());
    for (const auto& argument : arguments) wide.push_back(std::filesystem::u8path(argument).wstring());
    std::wstring commandLine = windowsCommandLine(wide);
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(
        wide.front().c_str(), mutableCommandLine.data(), nullptr, nullptr, FALSE, 0U, nullptr, nullptr,
        &startup, &process);
    if (created == FALSE) {
        throw std::system_error(
            static_cast<int>(GetLastError()), std::system_category(),
            "failed to start host process: " + arguments.front());
    }

    CloseHandle(process.hThread);
    const DWORD waitResult = WaitForSingleObject(process.hProcess, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
        const DWORD error = GetLastError();
        CloseHandle(process.hProcess);
        throw std::system_error(
            static_cast<int>(error), std::system_category(),
            "failed waiting for host process: " + arguments.front());
    }

    DWORD exitCode = 0U;
    if (GetExitCodeProcess(process.hProcess, &exitCode) == FALSE) {
        const DWORD error = GetLastError();
        CloseHandle(process.hProcess);
        throw std::system_error(
            static_cast<int>(error), std::system_category(),
            "failed reading host process exit code: " + arguments.front());
    }
    CloseHandle(process.hProcess);
    return static_cast<int>(exitCode);
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
