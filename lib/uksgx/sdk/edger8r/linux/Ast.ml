(*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *)

(* [unsigned] *)
type signedness = Signed | Unsigned

(* [short|long] *)
type shortness  = IShort | ILong | INone

(* [unsigned] [short|long] int *)
type int_attr = {
  ia_signedness : signedness;
  ia_shortness  : shortness;
}

(* All available types. *)
type atype =
  | Char  of signedness  (* 'char' *)
  | Long  of signedness  (* 'long' *)
  | LLong of signedness  (* 'long long' *)
  | Int   of int_attr
  | Float | Double | LDouble
  | Int8  | Int16  | Int32  | Int64
  | UInt8 | UInt16 | UInt32 | UInt64
  | Void  | WChar  | SizeT
  | Struct  of string
  | Union   of string
  | Enum    of string
  | Foreign of string (* typedef in other places *)
  | Ptr     of atype  (* Pointer to a type *)

(* Pointer parameter direction attribute 'in', 'out', 'inout' *)
type ptr_direction =
  | PtrIn | PtrOut | PtrInOut | PtrNoDirection

(* It holds possible values for a given attribute. *)
type attr_value =
  | AString of string  (* a varable *)
  | ANumber of int     (* a number *)

(* Pointer parameter can have option attribute of 'size=' and 'count='. *)
type ptr_size = {
  ps_size     : attr_value option;
  ps_count    : attr_value option;
}

let empty_ptr_size = {
  ps_size     = None;
  ps_count    = None;
}

(* Pointers have several special attributes. *)
type ptr_attr = {
  pa_direction  : ptr_direction;
  pa_size       : ptr_size;
  pa_isptr      : bool;       (* If a foreign type is a pointer type, 'isptr' *)
  pa_isary      : bool;       (* If a foreign type is an array, 'isary' *)
  pa_isstr      : bool;       (* 'char*' pointer with length of strlen(x), 'isstr' *)
  pa_iswstr     : bool;       (* 'wchar*' pointer with length of wcslen(x), 'iswstr' *)
  pa_rdonly     : bool;       (* If the pointer is 'const' qualified, 'readonly' *)
  pa_chkptr     : bool;       (* Whether to generate code to check pointer, 'user_check' *)
}

(* parameter type *)
type parameter_type =
  | PTVal of atype            (* Passed by value *)
  | PTPtr of atype * ptr_attr (* Passed by address *)

(* function calling convention *)  
type call_conv = CC_CDECL | CC_STDCALL | CC_FASTCALL | CC_NONE

let get_call_conv_str (cc: call_conv) =
  match cc with
    CC_CDECL    -> "CDECL"
  | CC_STDCALL  -> "STDCALL"
  | CC_FASTCALL -> "FASTCALL"
  | CC_NONE     -> "NOCONVENTION"

(* function attribute - only for untrusted functions *)
type func_attr = {
  fa_dllimport : bool;                   (* use 'dllimport'? *)
  fa_convention: call_conv;              (* the calling convention *)
}

(* A declarator can be an identifier or an identifier with array form.
 * For a simlpe identifier, the `array_dims' is an empty list `[]'.
 * A dimension with size -1 means that user explicitly declared `ary[]'.
 *)
type declarator = {
  identifier : string;
  array_dims : int list;
}

let is_array (declr: declarator) = declr.array_dims <> []

(* Parameter and Struct declaration. *)
type pdecl = parameter_type * declarator

(* Union member declaration *)
type mdecl = atype * declarator

(* Definition of a struct *)
type struct_def = {
  sname : string;       (* structure name. *)
  smlist : pdecl list;   (* structure members. *)
}

(* Definition of a union *)
type union_def = {
  uname : string;       (* union name. *)
  umlist : mdecl list;   (* union members. *)
}

(* Definition of a enum *)
type enum_val = EnumValNone | EnumVal of attr_value
type enum_ele = string * enum_val

type enum_def = {
  enname: string;              (* enum name - "" for anonymous enum *)
  enbody: enum_ele list;       (* elements of enum *)
}

(* Composite type - the form for struct/union definition are the same. *)
type composite_type =
    StructDef of struct_def
  | UnionDef  of union_def
  | EnumDef   of enum_def

(* Function declaration. *)
type func_decl = {
  fname : string;       (* function name. *)
  rtype : atype;        (* return type. *)
  plist : pdecl list;   (* parameter list. *)
}

(* The untrusted functions might have a string list that specifying
 * trusted ECALLs possibly to be made.  While the trusted functions
 * have a bool tag to identify whether it is private or not (private
 * means it can only be accessed by an OCALL).
 *)
(* Trust function(aka ecall) declaration. *)
type trusted_func = {
  tf_fdecl   : func_decl;
  tf_is_priv : bool;   (* private or not, see above comment *)
  tf_is_switchless : bool;
}

(* untrust function(aka ocall) declaration. *)
type untrusted_func = {
  uf_fdecl      : func_decl;
  uf_fattr      : func_attr; (* function attribute *)
  uf_allow_list : string list; (* allow list, see above comment *)
  uf_propagate_errno : bool; (* whether this function changes errno *)
  uf_is_switchless    : bool;
}

type enclave_func =
  | Trusted   of trusted_func
  | Untrusted of untrusted_func

(* Module import declaration. 'import' *)
type import_decl = {
  mname : string;       (* from which to import functions. *)
  flist : string list;  (* a list of functions to be imported. *)
}

(* All valid expressions. *)
type expr =
  | Interface of enclave_func list
  | Composite of composite_type
  | Importing of import_decl
  | Include   of string

(* The definition of an Enclave *)
type enclave = {
  ename : string;           (* enclave name. *)
  eexpr : expr list;        (* expressions inside enclave. *)
}

(*
  Plugin.ml operates on an enclave_content instance.
  CodeGen.ml calls Plugin.ml if a plugin is installed and hence
  depends on Plugin.ml.
  To prevent cyclic dependency between Plugin.ml and Codegen.ml,
  enclave_content is defined here. Additionally, the enclave_content 
  type in CodeGen.ml is defined to be equivalent to the enclave_content 
  type defined here.
*)
type enclave_content = {
  file_shortnm : string; (* the short name of original EDL file *)
  enclave_name : string; (* the normalized C identifier *)

  include_list : string list;
  import_exprs : import_decl list;
  comp_defs    : composite_type list;
  tfunc_decls  : trusted_func   list;
  ufunc_decls  : untrusted_func list;
}

(* -------------------------------------------------------------------
 *  Some utility function to manupulate types defined in AST.
 * -------------------------------------------------------------------
 *)

(* Get the string representation of a type. *)
let rec get_tystr (ty: atype) =
  match ty with
    | Char sn ->
        (match sn with
            Signed   -> "char"
          | Unsigned -> "unsigned char")
    | Long sn ->
        (match sn with
            Signed   -> "long"
          | Unsigned -> "unsigned long")
    | LLong sn ->
        (match sn with
            Signed   -> "long long"
          | Unsigned -> "unsigned long long")
    | Int  ia ->
        Printf.sprintf "%s%sint"
          (if ia.ia_signedness = Unsigned then "unsigned " else "")
          (match ia.ia_shortness with
               IShort -> "short "
             | ILong  -> "long "
             | INone  -> "")
    | Float     -> "float"
    | Double    -> "double"
    | LDouble   -> "long double"
    | Int8      -> "int8_t"
    | Int16     -> "int16_t"
    | Int32     -> "int32_t"
    | Int64     -> "int64_t"
    | UInt8     -> "uint8_t"
    | UInt16    -> "uint16_t"
    | UInt32    -> "uint32_t"
    | UInt64    -> "uint64_t"
    | Void      -> "void"
    | SizeT     -> "size_t"
    | WChar     -> "wchar_t"
    | Struct id -> "struct " ^ id
    | Union  id -> "union "  ^ id
    | Enum   id -> "enum "   ^ id
    | Foreign s -> s
    | Ptr ty    -> get_tystr(ty) ^ "*"

(* Get the plain `atype' from a `parameter_type'. *)
let get_param_atype (pt: parameter_type) =
  match pt with
    | PTVal t      -> t
    | PTPtr (t, _) -> t

(* Convert attr_value to string *)
let attr_value_to_string (attr: attr_value) =
  match attr with
      ANumber n -> Printf.sprintf "%d" n
    | AString s -> s
