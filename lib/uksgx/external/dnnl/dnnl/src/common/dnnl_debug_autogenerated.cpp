/*******************************************************************************
* Copyright 2018-2019 Intel Corporation
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

// DO NOT EDIT, AUTO-GENERATED

// clang-format off

#include <assert.h>

#include "dnnl_debug.h"
#include "dnnl_types.h"

const char *dnnl_status2str(dnnl_status_t v) {
    if (v == dnnl_success) return "success";
    if (v == dnnl_out_of_memory) return "out_of_memory";
    if (v == dnnl_invalid_arguments) return "invalid_arguments";
    if (v == dnnl_unimplemented) return "unimplemented";
    if (v == dnnl_iterator_ends) return "iterator_ends";
    if (v == dnnl_runtime_error) return "runtime_error";
    if (v == dnnl_not_required) return "not_required";
    assert(!"unknown status");
    return "unknown status";
}

const char *dnnl_dt2str(dnnl_data_type_t v) {
    if (v == dnnl_data_type_undef) return "undef";
    if (v == dnnl_f16) return "f16";
    if (v == dnnl_bf16) return "bf16";
    if (v == dnnl_f32) return "f32";
    if (v == dnnl_s32) return "s32";
    if (v == dnnl_s8) return "s8";
    if (v == dnnl_u8) return "u8";
    assert(!"unknown dt");
    return "unknown dt";
}

const char *dnnl_fmt_kind2str(dnnl_format_kind_t v) {
    if (v == dnnl_format_kind_undef) return "undef";
    if (v == dnnl_format_kind_any) return "any";
    if (v == dnnl_blocked) return "blocked";
    if (v == dnnl_format_kind_wino) return "wino";
    if (v == dnnl_format_kind_rnn_packed) return "rnn_packed";
    assert(!"unknown fmt_kind");
    return "unknown fmt_kind";
}

const char *dnnl_fmt_tag2str(dnnl_format_tag_t v) {
    if (v == dnnl_format_tag_undef) return "undef";
    if (v == dnnl_format_tag_any) return "format_tag_any";
    if (v == dnnl_a) return "a";
    if (v == dnnl_ab) return "ab";
    if (v == dnnl_abc) return "abc";
    if (v == dnnl_abcd) return "abcd";
    if (v == dnnl_abcde) return "abcde";
    if (v == dnnl_abcdef) return "abcdef";
    if (v == dnnl_abdec) return "abdec";
    if (v == dnnl_acb) return "acb";
    if (v == dnnl_acbde) return "acbde";
    if (v == dnnl_acdb) return "acdb";
    if (v == dnnl_acdeb) return "acdeb";
    if (v == dnnl_ba) return "ba";
    if (v == dnnl_bac) return "bac";
    if (v == dnnl_bacd) return "bacd";
    if (v == dnnl_bca) return "bca";
    if (v == dnnl_bcda) return "bcda";
    if (v == dnnl_bcdea) return "bcdea";
    if (v == dnnl_cba) return "cba";
    if (v == dnnl_cdba) return "cdba";
    if (v == dnnl_cdeba) return "cdeba";
    if (v == dnnl_decab) return "decab";
    if (v == dnnl_Abc16a) return "Abc16a";
    if (v == dnnl_ABc16a16b) return "ABc16a16b";
    if (v == dnnl_aBc16b) return "aBc16b";
    if (v == dnnl_ABc16b16a) return "ABc16b16a";
    if (v == dnnl_Abc4a) return "Abc4a";
    if (v == dnnl_aBc4b) return "aBc4b";
    if (v == dnnl_ABc4b16a4b) return "ABc4b16a4b";
    if (v == dnnl_ABc4b4a) return "ABc4b4a";
    if (v == dnnl_ABc8a16b2a) return "ABc8a16b2a";
    if (v == dnnl_ABc8a8b) return "ABc8a8b";
    if (v == dnnl_aBc8b) return "aBc8b";
    if (v == dnnl_ABc8b16a2b) return "ABc8b16a2b";
    if (v == dnnl_BAc8a16b2a) return "BAc8a16b2a";
    if (v == dnnl_ABc8b8a) return "ABc8b8a";
    if (v == dnnl_Abcd16a) return "Abcd16a";
    if (v == dnnl_ABcd16a16b) return "ABcd16a16b";
    if (v == dnnl_ABcd32a32b) return "ABcd32a32b";
    if (v == dnnl_aBcd16b) return "aBcd16b";
    if (v == dnnl_ABcd16b16a) return "ABcd16b16a";
    if (v == dnnl_aBCd16b16c) return "aBCd16b16c";
    if (v == dnnl_aBCd16c16b) return "aBCd16c16b";
    if (v == dnnl_Abcd4a) return "Abcd4a";
    if (v == dnnl_aBcd4b) return "aBcd4b";
    if (v == dnnl_ABcd4b16a4b) return "ABcd4b16a4b";
    if (v == dnnl_ABcd4b4a) return "ABcd4b4a";
    if (v == dnnl_aBCd4c16b4c) return "aBCd4c16b4c";
    if (v == dnnl_aBCd4c4b) return "aBCd4c4b";
    if (v == dnnl_ABcd8a16b2a) return "ABcd8a16b2a";
    if (v == dnnl_ABcd8a8b) return "ABcd8a8b";
    if (v == dnnl_aBcd8b) return "aBcd8b";
    if (v == dnnl_ABcd8b16a2b) return "ABcd8b16a2b";
    if (v == dnnl_aBCd8b16c2b) return "aBCd8b16c2b";
    if (v == dnnl_BAcd8a16b2a) return "BAcd8a16b2a";
    if (v == dnnl_ABcd8b8a) return "ABcd8b8a";
    if (v == dnnl_aBCd8b8c) return "aBCd8b8c";
    if (v == dnnl_aBCd8c16b2c) return "aBCd8c16b2c";
    if (v == dnnl_ABcde8a16b2a) return "ABcde8a16b2a";
    if (v == dnnl_aCBd8b16c2b) return "aCBd8b16c2b";
    if (v == dnnl_aBCd8c8b) return "aBCd8c8b";
    if (v == dnnl_Abcde16a) return "Abcde16a";
    if (v == dnnl_ABcde16a16b) return "ABcde16a16b";
    if (v == dnnl_BAcde8a16b2a) return "BAcde8a16b2a";
    if (v == dnnl_aBcde16b) return "aBcde16b";
    if (v == dnnl_ABcde16b16a) return "ABcde16b16a";
    if (v == dnnl_aBCde16b16c) return "aBCde16b16c";
    if (v == dnnl_aBCde16c16b) return "aBCde16c16b";
    if (v == dnnl_aBCde2c8b4c) return "aBCde2c8b4c";
    if (v == dnnl_Abcde4a) return "Abcde4a";
    if (v == dnnl_aBcde4b) return "aBcde4b";
    if (v == dnnl_ABcde4b4a) return "ABcde4b4a";
    if (v == dnnl_aBCde4b4c) return "aBCde4b4c";
    if (v == dnnl_aBCde4c16b4c) return "aBCde4c16b4c";
    if (v == dnnl_aBCde4c4b) return "aBCde4c4b";
    if (v == dnnl_Abcde8a) return "Abcde8a";
    if (v == dnnl_ABcde8a8b) return "ABcde8a8b";
    if (v == dnnl_BAcde16b16a) return "BAcde16b16a";
    if (v == dnnl_aBcde8b) return "aBcde8b";
    if (v == dnnl_ABcde8b16a2b) return "ABcde8b16a2b";
    if (v == dnnl_aBCde8b16c2b) return "aBCde8b16c2b";
    if (v == dnnl_aCBde8b16c2b) return "aCBde8b16c2b";
    if (v == dnnl_ABcde8b8a) return "ABcde8b8a";
    if (v == dnnl_aBCde8b8c) return "aBCde8b8c";
    if (v == dnnl_ABcd4a8b8a4b) return "ABcd4a8b8a4b";
    if (v == dnnl_ABcd2a8b8a2b) return "ABcd2a8b8a2b";
    if (v == dnnl_aBCde4b8c8b4c) return "aBCde4b8c8b4c";
    if (v == dnnl_aBCde2b8c8b2c) return "aBCde2b8c8b2c";
    if (v == dnnl_aBCde8c16b2c) return "aBCde8c16b2c";
    if (v == dnnl_aBCde8c8b) return "aBCde8c8b";
    if (v == dnnl_aBcdef16b) return "aBcdef16b";
    if (v == dnnl_aBCdef16b16c) return "aBCdef16b16c";
    if (v == dnnl_aBCdef16c16b) return "aBCdef16c16b";
    if (v == dnnl_aBcdef4b) return "aBcdef4b";
    if (v == dnnl_aBCdef4c4b) return "aBCdef4c4b";
    if (v == dnnl_aBCdef8b8c) return "aBCdef8b8c";
    if (v == dnnl_aBCdef8c16b2c) return "aBCdef8c16b2c";
    if (v == dnnl_aBCdef8b16c2b) return "aBCdef8b16c2b";
    if (v == dnnl_aCBdef8b16c2b) return "aCBdef8b16c2b";
    if (v == dnnl_aBCdef8c8b) return "aBCdef8c8b";
    if (v == dnnl_aBdc16b) return "aBdc16b";
    if (v == dnnl_aBdc4b) return "aBdc4b";
    if (v == dnnl_aBdc8b) return "aBdc8b";
    if (v == dnnl_aBdec16b) return "aBdec16b";
    if (v == dnnl_aBdec32b) return "aBdec32b";
    if (v == dnnl_aBdec4b) return "aBdec4b";
    if (v == dnnl_aBdec8b) return "aBdec8b";
    if (v == dnnl_aBdefc16b) return "aBdefc16b";
    if (v == dnnl_aCBdef16c16b) return "aCBdef16c16b";
    if (v == dnnl_aBdefc4b) return "aBdefc4b";
    if (v == dnnl_aBdefc8b) return "aBdefc8b";
    if (v == dnnl_Abcdef16a) return "Abcdef16a";
    if (v == dnnl_Acb16a) return "Acb16a";
    if (v == dnnl_Acb4a) return "Acb4a";
    if (v == dnnl_Acb8a) return "Acb8a";
    if (v == dnnl_aCBd16b16c) return "aCBd16b16c";
    if (v == dnnl_aCBd16c16b) return "aCBd16c16b";
    if (v == dnnl_aCBde16b16c) return "aCBde16b16c";
    if (v == dnnl_aCBde16c16b) return "aCBde16c16b";
    if (v == dnnl_Acdb16a) return "Acdb16a";
    if (v == dnnl_Acdb32a) return "Acdb32a";
    if (v == dnnl_Acdb4a) return "Acdb4a";
    if (v == dnnl_Acdb8a) return "Acdb8a";
    if (v == dnnl_Acdeb16a) return "Acdeb16a";
    if (v == dnnl_Acdeb4a) return "Acdeb4a";
    if (v == dnnl_Acdeb8a) return "Acdeb8a";
    if (v == dnnl_BAc16a16b) return "BAc16a16b";
    if (v == dnnl_BAc16b16a) return "BAc16b16a";
    if (v == dnnl_BAcd16a16b) return "BAcd16a16b";
    if (v == dnnl_BAcd16b16a) return "BAcd16b16a";
    if (v == dnnl_format_tag_last) return "format_tag_last";
    if (v == dnnl_x) return "x";
    if (v == dnnl_nc) return "nc";
    if (v == dnnl_cn) return "cn";
    if (v == dnnl_tn) return "tn";
    if (v == dnnl_nt) return "nt";
    if (v == dnnl_ncw) return "ncw";
    if (v == dnnl_nwc) return "nwc";
    if (v == dnnl_nchw) return "nchw";
    if (v == dnnl_nhwc) return "nhwc";
    if (v == dnnl_chwn) return "chwn";
    if (v == dnnl_ncdhw) return "ncdhw";
    if (v == dnnl_ndhwc) return "ndhwc";
    if (v == dnnl_oi) return "oi";
    if (v == dnnl_io) return "io";
    if (v == dnnl_oiw) return "oiw";
    if (v == dnnl_owi) return "owi";
    if (v == dnnl_wio) return "wio";
    if (v == dnnl_iwo) return "iwo";
    if (v == dnnl_oihw) return "oihw";
    if (v == dnnl_hwio) return "hwio";
    if (v == dnnl_ohwi) return "ohwi";
    if (v == dnnl_ihwo) return "ihwo";
    if (v == dnnl_iohw) return "iohw";
    if (v == dnnl_oidhw) return "oidhw";
    if (v == dnnl_dhwio) return "dhwio";
    if (v == dnnl_odhwi) return "odhwi";
    if (v == dnnl_idhwo) return "idhwo";
    if (v == dnnl_goiw) return "goiw";
    if (v == dnnl_goihw) return "goihw";
    if (v == dnnl_hwigo) return "hwigo";
    if (v == dnnl_giohw) return "giohw";
    if (v == dnnl_goidhw) return "goidhw";
    if (v == dnnl_tnc) return "tnc";
    if (v == dnnl_ntc) return "ntc";
    if (v == dnnl_ldnc) return "ldnc";
    if (v == dnnl_ldigo) return "ldigo";
    if (v == dnnl_ldgoi) return "ldgoi";
    if (v == dnnl_ldgo) return "ldgo";
    if (v == dnnl_nCdhw16c) return "nCdhw16c";
    if (v == dnnl_nCdhw4c) return "nCdhw4c";
    if (v == dnnl_nCdhw8c) return "nCdhw8c";
    if (v == dnnl_nChw16c) return "nChw16c";
    if (v == dnnl_nChw4c) return "nChw4c";
    if (v == dnnl_nChw8c) return "nChw8c";
    if (v == dnnl_nCw16c) return "nCw16c";
    if (v == dnnl_nCw4c) return "nCw4c";
    if (v == dnnl_nCw8c) return "nCw8c";
    if (v == dnnl_NCw16n16c) return "NCw16n16c";
    if (v == dnnl_NCdhw16n16c) return "NCdhw16n16c";
    if (v == dnnl_NChw16n16c) return "NChw16n16c";
    if (v == dnnl_NChw32n32c) return "NChw32n32c";
    if (v == dnnl_IOw16o16i) return "IOw16o16i";
    if (v == dnnl_IOw16i16o) return "IOw16i16o";
    if (v == dnnl_OIw16i16o) return "OIw16i16o";
    if (v == dnnl_OIw16o16i) return "OIw16o16i";
    if (v == dnnl_Oiw16o) return "Oiw16o";
    if (v == dnnl_OIw4i16o4i) return "OIw4i16o4i";
    if (v == dnnl_OIw4i4o) return "OIw4i4o";
    if (v == dnnl_Oiw4o) return "Oiw4o";
    if (v == dnnl_OIw8i16o2i) return "OIw8i16o2i";
    if (v == dnnl_OIw8i8o) return "OIw8i8o";
    if (v == dnnl_OIw8o16i2o) return "OIw8o16i2o";
    if (v == dnnl_IOw8o16i2o) return "IOw8o16i2o";
    if (v == dnnl_OIw8o8i) return "OIw8o8i";
    if (v == dnnl_Owi16o) return "Owi16o";
    if (v == dnnl_Owi4o) return "Owi4o";
    if (v == dnnl_Owi8o) return "Owi8o";
    if (v == dnnl_IOhw16i16o) return "IOhw16i16o";
    if (v == dnnl_IOhw16o16i) return "IOhw16o16i";
    if (v == dnnl_Ohwi16o) return "Ohwi16o";
    if (v == dnnl_Ohwi32o) return "Ohwi32o";
    if (v == dnnl_Ohwi4o) return "Ohwi4o";
    if (v == dnnl_Ohwi8o) return "Ohwi8o";
    if (v == dnnl_OIhw16i16o) return "OIhw16i16o";
    if (v == dnnl_OIhw16o16i) return "OIhw16o16i";
    if (v == dnnl_Oihw16o) return "Oihw16o";
    if (v == dnnl_OIhw4i16o4i) return "OIhw4i16o4i";
    if (v == dnnl_OIhw4i4o) return "OIhw4i4o";
    if (v == dnnl_Oihw4o) return "Oihw4o";
    if (v == dnnl_OIhw8i16o2i) return "OIhw8i16o2i";
    if (v == dnnl_OIhw8i8o) return "OIhw8i8o";
    if (v == dnnl_OIhw8o16i2o) return "OIhw8o16i2o";
    if (v == dnnl_IOhw8o16i2o) return "IOhw8o16i2o";
    if (v == dnnl_OIhw8o8i) return "OIhw8o8i";
    if (v == dnnl_Odhwi16o) return "Odhwi16o";
    if (v == dnnl_Odhwi4o) return "Odhwi4o";
    if (v == dnnl_Odhwi8o) return "Odhwi8o";
    if (v == dnnl_OIdhw16i16o) return "OIdhw16i16o";
    if (v == dnnl_OIdhw16o16i) return "OIdhw16o16i";
    if (v == dnnl_Oidhw16o) return "Oidhw16o";
    if (v == dnnl_OIdhw4i4o) return "OIdhw4i4o";
    if (v == dnnl_Oidhw4o) return "Oidhw4o";
    if (v == dnnl_OIdhw8i16o2i) return "OIdhw8i16o2i";
    if (v == dnnl_OIdhw8i8o) return "OIdhw8i8o";
    if (v == dnnl_OIdhw8o16i2o) return "OIdhw8o16i2o";
    if (v == dnnl_IOdhw8o16i2o) return "IOdhw8o16i2o";
    if (v == dnnl_OIdhw8o8i) return "OIdhw8o8i";
    if (v == dnnl_IOdhw16i16o) return "IOdhw16i16o";
    if (v == dnnl_Goiw16g) return "Goiw16g";
    if (v == dnnl_gIOw16o16i) return "gIOw16o16i";
    if (v == dnnl_gIOw16i16o) return "gIOw16i16o";
    if (v == dnnl_gOIw16i16o) return "gOIw16i16o";
    if (v == dnnl_gOIw16o16i) return "gOIw16o16i";
    if (v == dnnl_gOiw16o) return "gOiw16o";
    if (v == dnnl_gOIw4i16o4i) return "gOIw4i16o4i";
    if (v == dnnl_gOIw4i4o) return "gOIw4i4o";
    if (v == dnnl_gOiw4o) return "gOiw4o";
    if (v == dnnl_gOIw8i16o2i) return "gOIw8i16o2i";
    if (v == dnnl_gOIw8i8o) return "gOIw8i8o";
    if (v == dnnl_gOIw8o16i2o) return "gOIw8o16i2o";
    if (v == dnnl_gIOw8o16i2o) return "gIOw8o16i2o";
    if (v == dnnl_gOIw8o8i) return "gOIw8o8i";
    if (v == dnnl_gOwi16o) return "gOwi16o";
    if (v == dnnl_gOwi4o) return "gOwi4o";
    if (v == dnnl_gOwi8o) return "gOwi8o";
    if (v == dnnl_gIOhw16i16o) return "gIOhw16i16o";
    if (v == dnnl_gIOhw16o16i) return "gIOhw16o16i";
    if (v == dnnl_gOhwi16o) return "gOhwi16o";
    if (v == dnnl_gOhwi32o) return "gOhwi32o";
    if (v == dnnl_gOhwi4o) return "gOhwi4o";
    if (v == dnnl_gOhwi8o) return "gOhwi8o";
    if (v == dnnl_Goihw16g) return "Goihw16g";
    if (v == dnnl_gOIhw16i16o) return "gOIhw16i16o";
    if (v == dnnl_gOIhw16o16i) return "gOIhw16o16i";
    if (v == dnnl_gOihw16o) return "gOihw16o";
    if (v == dnnl_gOIhw2i8o4i) return "gOIhw2i8o4i";
    if (v == dnnl_gOIhw4i16o4i) return "gOIhw4i16o4i";
    if (v == dnnl_gOIhw4i4o) return "gOIhw4i4o";
    if (v == dnnl_gOIhw4o4i) return "gOIhw4o4i";
    if (v == dnnl_gOihw4o) return "gOihw4o";
    if (v == dnnl_Goihw8g) return "Goihw8g";
    if (v == dnnl_gOIhw8i16o2i) return "gOIhw8i16o2i";
    if (v == dnnl_gOIhw8i8o) return "gOIhw8i8o";
    if (v == dnnl_gOIhw8o16i2o) return "gOIhw8o16i2o";
    if (v == dnnl_gIOhw8o16i2o) return "gIOhw8o16i2o";
    if (v == dnnl_gOIhw8o8i) return "gOIhw8o8i";
    if (v == dnnl_OIhw4o8i8o4i) return "OIhw4o8i8o4i";
    if (v == dnnl_OIhw2o8i8o2i) return "OIhw2o8i8o2i";
    if (v == dnnl_gOIhw4o8i8o4i) return "gOIhw4o8i8o4i";
    if (v == dnnl_gOIhw2o8i8o2i) return "gOIhw2o8i8o2i";
    if (v == dnnl_gIOdhw16i16o) return "gIOdhw16i16o";
    if (v == dnnl_gOdhwi16o) return "gOdhwi16o";
    if (v == dnnl_gOdhwi4o) return "gOdhwi4o";
    if (v == dnnl_gOdhwi8o) return "gOdhwi8o";
    if (v == dnnl_gOIdhw16i16o) return "gOIdhw16i16o";
    if (v == dnnl_gOIdhw16o16i) return "gOIdhw16o16i";
    if (v == dnnl_gOidhw16o) return "gOidhw16o";
    if (v == dnnl_gOIdhw4i4o) return "gOIdhw4i4o";
    if (v == dnnl_gOidhw4o) return "gOidhw4o";
    if (v == dnnl_gOIdhw8i16o2i) return "gOIdhw8i16o2i";
    if (v == dnnl_gOIdhw8i8o) return "gOIdhw8i8o";
    if (v == dnnl_gOIdhw8o16i2o) return "gOIdhw8o16i2o";
    if (v == dnnl_gIOdhw8o16i2o) return "gIOdhw8o16i2o";
    if (v == dnnl_gOIdhw8o8i) return "gOIdhw8o8i";
    if (v == dnnl_Goidhw16g) return "Goidhw16g";
    assert(!"unknown fmt_tag");
    return "unknown fmt_tag";
}

const char *dnnl_prop_kind2str(dnnl_prop_kind_t v) {
    if (v == dnnl_prop_kind_undef) return "undef";
    if (v == dnnl_forward_training) return "forward_training";
    if (v == dnnl_forward_inference) return "forward_inference";
    if (v == dnnl_forward_scoring) return "forward_scoring";
    if (v == dnnl_forward) return "forward";
    if (v == dnnl_backward) return "backward";
    if (v == dnnl_backward_data) return "backward_data";
    if (v == dnnl_backward_weights) return "backward_weights";
    if (v == dnnl_backward_bias) return "backward_bias";
    assert(!"unknown prop_kind");
    return "unknown prop_kind";
}

const char *dnnl_prim_kind2str(dnnl_primitive_kind_t v) {
    if (v == dnnl_undefined_primitive) return "undef";
    if (v == dnnl_reorder) return "reorder";
    if (v == dnnl_shuffle) return "shuffle";
    if (v == dnnl_concat) return "concat";
    if (v == dnnl_sum) return "sum";
    if (v == dnnl_convolution) return "convolution";
    if (v == dnnl_deconvolution) return "deconvolution";
    if (v == dnnl_eltwise) return "eltwise";
    if (v == dnnl_softmax) return "softmax";
    if (v == dnnl_pooling) return "pooling";
    if (v == dnnl_lrn) return "lrn";
    if (v == dnnl_batch_normalization) return "batch_normalization";
    if (v == dnnl_layer_normalization) return "layer_normalization";
    if (v == dnnl_inner_product) return "inner_product";
    if (v == dnnl_rnn) return "rnn";
    if (v == dnnl_gemm) return "gemm";
    if (v == dnnl_binary) return "binary";
    assert(!"unknown prim_kind");
    return "unknown prim_kind";
}

const char *dnnl_alg_kind2str(dnnl_alg_kind_t v) {
    if (v == dnnl_alg_kind_undef) return "undef";
    if (v == dnnl_convolution_direct) return "convolution_direct";
    if (v == dnnl_convolution_winograd) return "convolution_winograd";
    if (v == dnnl_convolution_auto) return "convolution_auto";
    if (v == dnnl_deconvolution_direct) return "deconvolution_direct";
    if (v == dnnl_deconvolution_winograd) return "deconvolution_winograd";
    if (v == dnnl_eltwise_relu) return "eltwise_relu";
    if (v == dnnl_eltwise_tanh) return "eltwise_tanh";
    if (v == dnnl_eltwise_elu) return "eltwise_elu";
    if (v == dnnl_eltwise_square) return "eltwise_square";
    if (v == dnnl_eltwise_abs) return "eltwise_abs";
    if (v == dnnl_eltwise_sqrt) return "eltwise_sqrt";
    if (v == dnnl_eltwise_linear) return "eltwise_linear";
    if (v == dnnl_eltwise_bounded_relu) return "eltwise_bounded_relu";
    if (v == dnnl_eltwise_soft_relu) return "eltwise_soft_relu";
    if (v == dnnl_eltwise_logistic) return "eltwise_logistic";
    if (v == dnnl_eltwise_exp) return "eltwise_exp";
    if (v == dnnl_eltwise_gelu) return "eltwise_gelu";
    if (v == dnnl_eltwise_swish) return "eltwise_swish";
    if (v == dnnl_pooling_max) return "pooling_max";
    if (v == dnnl_pooling_avg_include_padding) return "pooling_avg_include_padding";
    if (v == dnnl_pooling_avg_exclude_padding) return "pooling_avg_exclude_padding";
    if (v == dnnl_pooling_avg) return "pooling_avg";
    if (v == dnnl_lrn_across_channels) return "lrn_across_channels";
    if (v == dnnl_lrn_within_channel) return "lrn_within_channel";
    if (v == dnnl_vanilla_rnn) return "vanilla_rnn";
    if (v == dnnl_vanilla_lstm) return "vanilla_lstm";
    if (v == dnnl_vanilla_gru) return "vanilla_gru";
    if (v == dnnl_lbr_gru) return "lbr_gru";
    if (v == dnnl_binary_add) return "binary_add";
    if (v == dnnl_binary_mul) return "binary_mul";
    assert(!"unknown alg_kind");
    return "unknown alg_kind";
}

const char *dnnl_rnn_flags2str(dnnl_rnn_flags_t v) {
    if (v == dnnl_rnn_flags_undef) return "undef";
    assert(!"unknown rnn_flags");
    return "unknown rnn_flags";
}

const char *dnnl_rnn_direction2str(dnnl_rnn_direction_t v) {
    if (v == dnnl_unidirectional_left2right) return "unidirectional_left2right";
    if (v == dnnl_unidirectional_right2left) return "unidirectional_right2left";
    if (v == dnnl_bidirectional_concat) return "bidirectional_concat";
    if (v == dnnl_bidirectional_sum) return "bidirectional_sum";
    if (v == dnnl_unidirectional) return "unidirectional";
    assert(!"unknown rnn_direction");
    return "unknown rnn_direction";
}

const char *dnnl_engine_kind2str(dnnl_engine_kind_t v) {
    if (v == dnnl_any_engine) return "any_engine";
    if (v == dnnl_cpu) return "cpu";
    if (v == dnnl_gpu) return "gpu";
    assert(!"unknown engine_kind");
    return "unknown engine_kind";
}

const char *dnnl_scratchpad_mode2str(dnnl_scratchpad_mode_t v) {
    if (v == dnnl_scratchpad_mode_library) return "scratchpad_mode_library";
    if (v == dnnl_scratchpad_mode_user) return "scratchpad_mode_user";
    assert(!"unknown scratchpad_mode");
    return "unknown scratchpad_mode";
}


