/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/diagnostics/exception_handler.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

//TODO
//FIX PLEASE SOMEONE FIX THIS IS DOGSHIT AND I DONT WANNA USE GDB
// - pupnoodle
// alright im just using gdb now but leaving this here cuz why not? :shrug:

#include "exception_handler.hpp"

#include <array>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <string>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>

namespace cathook::core
{

namespace
{

std::atomic<int> s_log_fd{ -1 };

struct signal_state
{
    std::array<struct sigaction, 4> previous_actions{};
    bool installed{};
};

signal_state& state()
{
    static signal_state instance{};
    return instance;
}

long current_thread_id()
{
    return static_cast<long>(::syscall(SYS_gettid));
}

void write_literal(const int fd, const char* const text)
{
    if (fd < 0 || !text)
    {
        return;
    }

    const std::size_t length{ std::strlen(text) };
    if (length == 0)
    {
        return;
    }

    static_cast<void>(::write(fd, text, length));
}

void write_unsigned_decimal(const int fd, std::uint64_t value)
{
    char buffer[32]{};
    std::size_t index{ std::size(buffer) };

    do
    {
        buffer[--index] = static_cast<char>('0' + (value % 10));
        value /= 10;
    }
    while (value != 0 && index > 0);

    static_cast<void>(::write(fd, buffer + index, std::size(buffer) - index));
}

void write_hex(const int fd, const std::uint64_t value)
{
    char buffer[18]{ '0', 'x' };
    constexpr char k_hex_digits[]{ "0123456789abcdef" };

    for (int index{}; index < 16; ++index)
    {
        const int shift{ (15 - index) * 4 };
        buffer[2 + index] = k_hex_digits[(value >> shift) & 0xF];
    }

    static_cast<void>(::write(fd, buffer, sizeof(buffer)));
}

void write_register(const int fd, const char* const name, const std::uint64_t value)
{
    write_literal(fd, name);
    write_literal(fd, "=");
    write_hex(fd, value);
    write_literal(fd, "\n");
}

void dump_memory_map(const int fd)
{
    const int maps_fd{ ::open("/proc/self/maps", O_RDONLY) };
    if (maps_fd < 0)
    {
        write_literal(fd, "maps: unavailable\n");
        return;
    }

    write_literal(fd, "\n/proc/self/maps\n");

    char buffer[4096]{};
    while (true)
    {
        const ssize_t read_size{ ::read(maps_fd, buffer, sizeof(buffer)) };
        if (read_size <= 0)
        {
            break;
        }

        static_cast<void>(::write(fd, buffer, static_cast<std::size_t>(read_size)));
    }

    ::close(maps_fd);
}

void dump_registers(const int fd, ucontext_t* const context)
{
    if (!context)
    {
        write_literal(fd, "registers: unavailable\n");
        return;
    }

#if defined(__x86_64__)
    const auto& registers{ context->uc_mcontext.gregs };
    write_literal(fd, "\nregisters\n");
    write_register(fd, "rip", static_cast<std::uint64_t>(registers[REG_RIP]));
    write_register(fd, "rsp", static_cast<std::uint64_t>(registers[REG_RSP]));
    write_register(fd, "rbp", static_cast<std::uint64_t>(registers[REG_RBP]));
    write_register(fd, "rax", static_cast<std::uint64_t>(registers[REG_RAX]));
    write_register(fd, "rbx", static_cast<std::uint64_t>(registers[REG_RBX]));
    write_register(fd, "rcx", static_cast<std::uint64_t>(registers[REG_RCX]));
    write_register(fd, "rdx", static_cast<std::uint64_t>(registers[REG_RDX]));
    write_register(fd, "rsi", static_cast<std::uint64_t>(registers[REG_RSI]));
    write_register(fd, "rdi", static_cast<std::uint64_t>(registers[REG_RDI]));
    write_register(fd, "r8", static_cast<std::uint64_t>(registers[REG_R8]));
    write_register(fd, "r9", static_cast<std::uint64_t>(registers[REG_R9]));
    write_register(fd, "r10", static_cast<std::uint64_t>(registers[REG_R10]));
    write_register(fd, "r11", static_cast<std::uint64_t>(registers[REG_R11]));
    write_register(fd, "r12", static_cast<std::uint64_t>(registers[REG_R12]));
    write_register(fd, "r13", static_cast<std::uint64_t>(registers[REG_R13]));
    write_register(fd, "r14", static_cast<std::uint64_t>(registers[REG_R14]));
    write_register(fd, "r15", static_cast<std::uint64_t>(registers[REG_R15]));
#elif defined(__i386__)
    const auto& registers{ context->uc_mcontext.gregs };
    write_literal(fd, "\nregisters\n");
    write_register(fd, "eip", static_cast<std::uint64_t>(registers[REG_EIP]));
    write_register(fd, "esp", static_cast<std::uint64_t>(registers[REG_ESP]));
    write_register(fd, "ebp", static_cast<std::uint64_t>(registers[REG_EBP]));
    write_register(fd, "eax", static_cast<std::uint64_t>(registers[REG_EAX]));
    write_register(fd, "ebx", static_cast<std::uint64_t>(registers[REG_EBX]));
    write_register(fd, "ecx", static_cast<std::uint64_t>(registers[REG_ECX]));
    write_register(fd, "edx", static_cast<std::uint64_t>(registers[REG_EDX]));
    write_register(fd, "esi", static_cast<std::uint64_t>(registers[REG_ESI]));
    write_register(fd, "edi", static_cast<std::uint64_t>(registers[REG_EDI]));
#else
    write_literal(fd, "\nregisters: unsupported architecture\n");
#endif
}

void dump_backtrace(const int fd)
{
    write_literal(fd, "\nbacktrace\n");

    void* frames[64]{};
    const int frame_count{ ::backtrace(frames, static_cast<int>(std::size(frames))) };
    if (frame_count <= 0)
    {
        write_literal(fd, "unavailable\n");
        return;
    }

    ::backtrace_symbols_fd(frames, frame_count, fd);
}

void signal_handler(const int signal_number, siginfo_t* const info, void* const context_ptr)
{
    const int fd{ s_log_fd.load() };
    if (fd >= 0)
    {
        write_literal(fd, "\n================ crash ================\n");
        write_literal(fd, "signal=");
        write_unsigned_decimal(fd, static_cast<std::uint64_t>(signal_number));
        write_literal(fd, " pid=");
        write_unsigned_decimal(fd, static_cast<std::uint64_t>(::getpid()));
        write_literal(fd, " tid=");
        write_unsigned_decimal(fd, static_cast<std::uint64_t>(current_thread_id()));
        write_literal(fd, "\n");

        if (info)
        {
            write_literal(fd, "code=");
            write_unsigned_decimal(fd, static_cast<std::uint64_t>(info->si_code));
            write_literal(fd, "fault_address=");
            write_hex(fd, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(info->si_addr)));
            write_literal(fd, "\n");
        }

        dump_registers(fd, static_cast<ucontext_t*>(context_ptr));
        dump_backtrace(fd);
        dump_memory_map(fd);
        write_literal(fd, "\n=======================================\n");
        static_cast<void>(::fsync(fd));
    }

    ::_exit(128 + signal_number);
}

} // namespace

void exception_handler::install(const std::filesystem::path& log_file_path)
{
    auto& handler_state{ state() };
    if (handler_state.installed)
    {
        return;
    }

    std::error_code error{};
    std::filesystem::create_directories(log_file_path.parent_path(), error);

    const int fd
    {
        ::open(
            log_file_path.string().c_str(),
            O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC,
            0644)
    };
    if (fd < 0)
    {
        return;
    }

    struct sigaction action{};
    action.sa_sigaction = &signal_handler;
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    ::sigemptyset(&action.sa_mask);

    constexpr int k_signals[]{ SIGSEGV, SIGABRT, SIGBUS, SIGILL };
    for (std::size_t index{}; index < std::size(k_signals); ++index)
    {
        if (::sigaction(k_signals[index], &action, &handler_state.previous_actions[index]) != 0)
        {
            for (std::size_t restore_index{}; restore_index < index; ++restore_index)
            {
                static_cast<void>(::sigaction(k_signals[restore_index], &handler_state.previous_actions[restore_index], nullptr));
            }

            ::close(fd);
            return;
        }
    }

    s_log_fd.store(fd);
    handler_state.installed = true;
}

void exception_handler::uninstall()
{
    auto& handler_state{ state() };
    if (!handler_state.installed)
    {
        return;
    }

    constexpr int k_signals[]{ SIGSEGV, SIGABRT, SIGBUS, SIGILL };
    for (std::size_t index{}; index < std::size(k_signals); ++index)
    {
        static_cast<void>(::sigaction(k_signals[index], &handler_state.previous_actions[index], nullptr));
    }

    const int fd{ s_log_fd.exchange(-1) };
    if (fd >= 0)
    {
        ::close(fd);
    }

    handler_state.installed = false;
}

} // namespace cathook::core
