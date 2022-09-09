/* Copyright (C) 2012-2021 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _X86GPRINTRIN_H_INCLUDED
# error "Never use <fxsrintrin.h> directly; include <x86gprintrin.h> instead."
#endif

#ifndef _FXSRINTRIN_H_INCLUDED
#define _FXSRINTRIN_H_INCLUDED

#ifndef __FXSR__
#pragma GCC push_options
#pragma GCC target("fxsr")
#define __DISABLE_FXSR__
#endif /* __FXSR__ */

extern __inline void
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
_fxsave (void *__P)
{
  __builtin_ia32_fxsave (__P);
}

extern __inline void
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
_fxrstor (void *__P)
{
  __builtin_ia32_fxrstor (__P);
}

#ifdef __x86_64__
extern __inline void
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
_fxsave64 (void *__P)
{
  __builtin_ia32_fxsave64 (__P);
}

extern __inline void
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
_fxrstor64 (void *__P)
{
  __builtin_ia32_fxrstor64 (__P);
}
#endif

#ifdef __DISABLE_FXSR__
#undef __DISABLE_FXSR__
#pragma GCC pop_options
#endif /* __DISABLE_FXSR__ */


#endif /* _FXSRINTRIN_H_INCLUDED */
