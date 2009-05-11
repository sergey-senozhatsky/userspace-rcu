#ifndef _ARCH_X86_H
#define _ARCH_X86_H

/*
 * arch_x86.h: trivial definitions for the x86 architecture.
 *
 * Copyright (c) 2009 Paul E. McKenney, IBM Corporation.
 * Copyright (c) 2009 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
*
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <compiler.h>

/* Assume P4 or newer */
#define CONFIG_HAVE_FENCE 1
#define CONFIG_HAVE_MEM_COHERENCY

#ifndef BITS_PER_LONG
#define BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#endif

#ifdef CONFIG_HAVE_FENCE
#define mb()    asm volatile("mfence":::"memory")
#define rmb()   asm volatile("lfence":::"memory")
#define wmb()   asm volatile("sfence"::: "memory")
#else
/*
 * Some non-Intel clones support out of order store. wmb() ceases to be a
 * nop for these.
 */
#define mb()    asm volatile("lock; addl $0,0(%%esp)":::"memory")
#define rmb()   asm volatile("lock; addl $0,0(%%esp)":::"memory")
#define wmb()   asm volatile("lock; addl $0,0(%%esp)"::: "memory")
#endif

/*
 * Architectures without cache coherency need something like the following:
 *
 * #define mb()		mc()
 * #define rmb()	rmc()
 * #define wmb()	wmc()
 * #define mc()		arch_cache_flush()
 * #define rmc()	arch_cache_flush_read()
 * #define wmc()	arch_cache_flush_write()
 */

#define mc()	barrier()
#define rmc()	barrier()
#define wmc()	barrier()

/* Assume SMP machine, given we don't have this information */
#define CONFIG_SMP 1

#ifdef CONFIG_SMP
#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()
#define smp_mc()	mc()
#define smp_rmc()	rmc()
#define smp_wmc()	wmc()
#else
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#define smp_mc()	barrier()
#define smp_rmc()	barrier()
#define smp_wmc()	barrier()
#endif

/* Nop everywhere except on alpha. */
#define smp_read_barrier_depends()

/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void rep_nop(void)
{
	asm volatile("rep; nop" ::: "memory");
}

static inline void cpu_relax(void)
{
	rep_nop();
}

#define xchg(ptr, v)	\
	((__typeof__(*(ptr)))__xchg((ptr), (unsigned long)(v), sizeof(*(ptr))))

struct __xchg_ptr_as_array {
	unsigned long a[100];
};

#define __xchg_ptr_as_array(x) ((struct __xchg_ptr_as_array *)(x))

/*
 * xchg always implies a "lock" prefix, even on UP. See Intel documentation.
 * volatile attribute is neccessary due to xchg side effect.
 * *ptr is an output argument.
 * x is considered local, ptr is considered remote.
 */
static inline unsigned long __xchg(volatile void *ptr, unsigned long x,
				   int size)
{
	switch (size) {
	case 1:
		asm volatile("xchgb %b0,%1"
			     : "=q" (x)
			     : "m" (*__xchg_ptr_as_array(ptr)), "0" (x)
			     : "memory");
		break;
	case 2:
		asm volatile("xchgw %w0,%1"
			     : "=r" (x)
			     : "m" (*__xchg_ptr_as_array(ptr)), "0" (x)
			     : "memory");
		break;
	case 4:
		asm volatile("xchgl %k0,%1"
			     : "=r" (x)
			     : "m" (*__xchg_ptr_as_array(ptr)), "0" (x)
			     : "memory");
		break;
#if (BITS_PER_LONG == 64)
	case 8:
		asm volatile("xchgq %0,%1"
			     : "=r" (x)
			     : "m" (*__xchg_ptr_as_array(ptr)), "0" (x)
			     : "memory");
		break;
#endif
	}
	smp_wmc();
	return x;
}

#define rdtscll(val)							  \
	do {						  		  \
	     unsigned int __a, __d;					  \
	     asm volatile("rdtsc" : "=a" (__a), "=d" (__d));		  \
	     (val) = ((unsigned long long)__a)				  \
			| (((unsigned long long)__d) << 32);		  \
	} while(0)

typedef unsigned long long cycles_t;

static inline cycles_t get_cycles(void)
{
        cycles_t ret = 0;

        rdtscll(ret);
        return ret;
}

#endif /* _ARCH_X86_H */