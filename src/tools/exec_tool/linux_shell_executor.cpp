#ifdef __linux__
#include "linux_shell_executor.h"

#include <array>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <expected>

std::expected<ExecResult, std::string>
LinuxShellExecutor::execute(const std::string& command)
{
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) == -1)
        return std::unexpected("Failed to create stdout pipe.");

    if (pipe(stderr_pipe) == -1)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return std::unexpected("Failed to create stderr pipe.");
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        return std::unexpected("Failed to fork.");
    }

    if (pid == 0)
    {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        execl("/bin/sh",
              "sh",
              "-c",
              command.c_str(),
              nullptr);

        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    ExecResult result;
    result.exit_code = -1;

    std::array<char, 4096> buffer;

    ssize_t bytes;

    while ((bytes = read(stdout_pipe[0], buffer.data(), buffer.size())) > 0)
    {
        result.stdout_text.append(buffer.data(), bytes);
    }

    while ((bytes = read(stderr_pipe[0], buffer.data(), buffer.size())) > 0)
    {
        result.stderr_text.append(buffer.data(), bytes);
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status;

    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        result.exit_code = WEXITSTATUS(status);
    else
        result.exit_code = -1;

    return result;
}
#endif