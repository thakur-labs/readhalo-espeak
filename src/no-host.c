// SPDX-License-Identifier: GPL-3.0-or-later
#include <errno.h>
#include <stdint.h>
#include <wasi/api.h>

// The linked reactor can run without host imports. Optional libc calls have
// no descriptors, clock or environment. Exit traps instead of ending the host.
__wasi_errno_t __wrap___wasi_clock_time_get(__wasi_clockid_t clock, __wasi_timestamp_t precision, __wasi_timestamp_t *time)
{
    (void)clock;
    (void)precision;
    (void)time;
    return __WASI_ERRNO_NOSYS;
}

_Noreturn void __wrap___wasi_proc_exit(__wasi_exitcode_t code)
{
    (void)code;
    __builtin_trap();
}

__wasi_errno_t __wrap___wasi_fd_close(__wasi_fd_t fd)
{
    (void)fd;
    return __WASI_ERRNO_BADF;
}

__wasi_errno_t __wrap___wasi_fd_write(__wasi_fd_t fd, const __wasi_ciovec_t *iov, size_t length, size_t *written)
{
    (void)fd;
    (void)iov;
    (void)length;
    *written = 0;
    return __WASI_ERRNO_BADF;
}

__wasi_errno_t __wrap___wasi_fd_seek(__wasi_fd_t fd, __wasi_filedelta_t offset, __wasi_whence_t whence, __wasi_filesize_t *result)
{
    (void)fd;
    (void)offset;
    (void)whence;
    (void)result;
    return __WASI_ERRNO_BADF;
}

__wasi_errno_t __wrap___wasi_environ_sizes_get(size_t *count, size_t *size)
{
    *count = 0;
    *size = 0;
    return __WASI_ERRNO_SUCCESS;
}

__wasi_errno_t __wrap___wasi_environ_get(uint8_t **values, uint8_t *buffer)
{
    (void)values;
    (void)buffer;
    return __WASI_ERRNO_SUCCESS;
}

int __syscall_unlinkat(int fd, intptr_t path, int flags)
{
    (void)fd;
    (void)path;
    (void)flags;
    return -ENOSYS;
}

int __syscall_rmdir(intptr_t path)
{
    (void)path;
    return -ENOSYS;
}

void emscripten_notify_memory_growth(size_t index)
{
    (void)index;
}
