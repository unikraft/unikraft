/*############################################################################
  # Copyright 2017 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/
/// Common SDK macro definitions
/*! \file */
#ifndef EPID_COMMON_EPIDDEFS_H_
#define EPID_COMMON_EPIDDEFS_H_

#if defined(SHARED)
#if defined(_WIN32)
#ifdef EXPORT_EPID_APIS
#define EPID_API __declspec(dllexport)
#else
#define EPID_API __declspec(dllimport)
#endif
#else  // defined(_WIN32)
#if __GNUC__ >= 4
#define EPID_API __attribute__((visibility("default")))
#else
#define EPID_API
#endif
#endif  // defined(_WIN32)
#else   // defined(SHARED)
#define EPID_API
#endif  // defined(SHARED)

#endif  // EPID_COMMON_EPIDDEFS_H_
