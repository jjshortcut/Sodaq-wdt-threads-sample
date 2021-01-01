
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_ZTEST_TEST_H
#define Z_INCLUDE_SYSCALLS_ZTEST_TEST_H


#ifndef _ASMLANGUAGE

#include <syscall_list.h>
#include <syscall.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void z_impl_z_test_1cpu_start();
static inline void z_test_1cpu_start()
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		arch_syscall_invoke0(K_SYSCALL_Z_TEST_1CPU_START);
		return;
	}
#endif
	compiler_barrier();
	z_impl_z_test_1cpu_start();
}


extern void z_impl_z_test_1cpu_stop();
static inline void z_test_1cpu_stop()
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		arch_syscall_invoke0(K_SYSCALL_Z_TEST_1CPU_STOP);
		return;
	}
#endif
	compiler_barrier();
	z_impl_z_test_1cpu_stop();
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */