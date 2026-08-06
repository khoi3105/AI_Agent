
#ifdef _WIN32
#include <windows.h>
#include "windows_shell_executor.h"

std::expected<ExecResult, std::string>
WindowsShellExecutor::execute(const std::string& command)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0))
        return std::unexpected("CreatePipe(stdout) failed.");
    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0))
    {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        return std::unexpected("CreatePipe(stderr) failed.");
    }

    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    std::string cmd = "cmd.exe /C " + command;

    if (!CreateProcessA(
            nullptr,
            cmd.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi))
    {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);

        return std::unexpected("CreateProcess failed.");
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    ExecResult result;

    char buffer[4096];

    DWORD bytesRead;

    while (ReadFile(stdoutRead,
                    buffer,
                    sizeof(buffer),
                    &bytesRead,
                    nullptr) &&
           bytesRead > 0)
    {
        result.stdout_text.append(buffer, bytesRead);
    }

    while (ReadFile(stderrRead,
                    buffer,
                    sizeof(buffer),
                    &bytesRead,
                    nullptr) &&
           bytesRead > 0)
    {
        result.stderr_text.append(buffer, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;

    GetExitCodeProcess(pi.hProcess, &exitCode);

    result.exit_code = static_cast<int>(exitCode);

    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return result;
}
#endif