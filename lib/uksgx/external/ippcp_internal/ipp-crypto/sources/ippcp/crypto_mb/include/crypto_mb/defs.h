/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef DEFS_H
#define DEFS_H

/* data types */
typedef unsigned char int8u;
typedef unsigned int  int32u;
typedef unsigned long long int64u;

#ifndef NULL
   #define NULL ((void *)0)
#endif

/* alignment & inline */
#if defined(__GNUC__)
   #if !defined(__ALIGN64)
      #define __ALIGN64 __attribute__((aligned(64)))
   #endif

   #if !defined(__INLINE)
      #define __INLINE static __inline__
   #endif

   #if !defined(__NOINLINE)
      #define __NOINLINE __attribute__((noinline))
   #endif
#else
   #if !defined(__ALIGN64)
      #define __ALIGN64 __declspec(align(64))
   #endif

   #if !defined(__INLINE)
      #define __INLINE static __forceinline
   #endif

   #if !defined(__NOINLINE)
      #define __NOINLINE __declspec(noinline)
   #endif
#endif


/* externals */
#undef EXTERN_C

#ifdef __cplusplus
   #define EXTERN_C extern "C"
#else
   #define EXTERN_C
#endif

#endif /* DEFS_H */
