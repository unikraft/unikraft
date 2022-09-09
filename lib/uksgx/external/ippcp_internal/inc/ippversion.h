/*******************************************************************************
* Copyright 2001-2021 Intel Corporation
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

/*
// 
//              Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
//
//              Purpose: Describes the Intel IPP Cryptography version
//
*/


#if !defined( IPPVERSION_H__ )
#define IPPVERSION_H__

#define IPP_VERSION_MAJOR  2021
#define IPP_VERSION_MINOR  3
#define IPP_VERSION_UPDATE 0

// Major interface version
#define IPP_INTERFACE_VERSION_MAJOR 11
// Minor interface version
#define IPP_INTERFACE_VERSION_MINOR 1

#define IPP_VERSION_STR  STR(IPP_VERSION_MAJOR) "." STR(IPP_VERSION_MINOR) "." STR(IPP_VERSION_UPDATE) " (" STR(IPP_INTERFACE_VERSION_MAJOR) "." STR(IPP_INTERFACE_VERSION_MINOR) " )"

#endif /* IPPVERSION_H__ */
