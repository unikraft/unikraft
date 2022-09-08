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

open Printf
open Util                               (* for failwithf *)

(* --------------------------------------------------------------------
 * We first introduce a `parse_enclave_ast' function (see below) to
 * parse a value of type `Ast.enclave' into a `enclave_content' record.
 * --------------------------------------------------------------------
 *)

(* This record type is used to better organize a value of Ast.enclave *)
type enclave_content = Ast.enclave_content = {
  file_shortnm : string; (* the short name of original EDL file *)
  enclave_name : string; (* the normalized C identifier *)

  include_list : string list; (* include another .h *)
  import_exprs : Ast.import_decl list;  (* always empty list after finished reduce_import. *)
  comp_defs    : Ast.composite_type list;
  tfunc_decls  : Ast.trusted_func   list;
  ufunc_decls  : Ast.untrusted_func list;
}

(* The generated code of ECalls and OCalls depends on some SGX utility
 * functions, each of which has two versions. The version to be used for an
 * ECall/OCall is determined by whether the ECAll/OCall is switchless or not.
 * For example, switchless ECalls use sgx_ecall_switchless() while ordinary ECalls use
 * sgx_ecall(). *)
type sgx_fn_id = SGX_ECALL | SGX_OCALL | SGX_OCALLOC | SGX_OCFREE
let get_sgx_fname fn_id is_switchless =
    let switchless_str = if is_switchless then "_switchless" else "" in
    match fn_id with
      SGX_ECALL -> "sgx_ecall" ^ switchless_str
    | SGX_OCALL -> "sgx_ocall" ^ switchless_str
    | SGX_OCALLOC -> "sgx_ocalloc"
    | SGX_OCFREE -> "sgx_ocfree"

(* Whether to prefix untrusted proxy with Enclave name *)
let g_use_prefix = ref false
let g_untrusted_dir = ref "."
let g_trusted_dir = ref "."

let empty_ec =
  { file_shortnm = "";
    enclave_name = "";
    include_list = [];
    import_exprs = [];
    comp_defs    = [];
    tfunc_decls  = [];
    ufunc_decls  = []; }

let get_tf_fname (tf: Ast.trusted_func) =
  tf.Ast.tf_fdecl.Ast.fname

let is_priv_ecall (tf: Ast.trusted_func) =
  tf.Ast.tf_is_priv

let is_switchless_ecall (tf: Ast.trusted_func) =
  tf.Ast.tf_is_switchless

let get_uf_fname (uf: Ast.untrusted_func) =
  uf.Ast.uf_fdecl.Ast.fname

let get_trusted_func_names (ec: enclave_content) =
  List.map get_tf_fname ec.tfunc_decls

let get_untrusted_func_names (ec: enclave_content) =
  List.map get_uf_fname ec.ufunc_decls

let tf_list_to_fd_list (tfs: Ast.trusted_func list) =
  List.map (fun (tf: Ast.trusted_func) -> tf.Ast.tf_fdecl) tfs

let tf_list_to_priv_list (tfs: Ast.trusted_func list) =
  List.map is_priv_ecall tfs

(* Get a list of names of all private ECALLs *)
let get_priv_ecall_names (tfs: Ast.trusted_func list) =
  List.filter is_priv_ecall tfs |> List.map get_tf_fname

let uf_list_to_fd_list (ufs: Ast.untrusted_func list) =
  List.map (fun (uf: Ast.untrusted_func) -> uf.Ast.uf_fdecl) ufs

(* Get a list of names of all allowed ECALLs from `allow(...)' *)
let get_allowed_names (ufs: Ast.untrusted_func list) =
  let allow_lists =
    List.map (fun (uf: Ast.untrusted_func) -> uf.Ast.uf_allow_list) ufs
  in
    List.flatten allow_lists |> dedup_list

(* With `parse_enclave_ast', each enclave AST is traversed only once. *)
let parse_enclave_ast (e: Ast.enclave) =
  let ac_include_list = ref [] in
  let ac_import_exprs = ref [] in
  let ac_comp_defs = ref [] in
  let ac_tfunc_decls = ref [] in
  let ac_ufunc_decls = ref [] in
    List.iter (fun ex ->
      match ex with
          Ast.Composite x -> ac_comp_defs := x :: !ac_comp_defs
        | Ast.Include   x -> ac_include_list := x :: !ac_include_list
        | Ast.Importing x -> ac_import_exprs := x :: !ac_import_exprs
        | Ast.Interface xs ->
            List.iter (fun ef ->
              match ef with
                  Ast.Trusted f ->
                    ac_tfunc_decls := f :: !ac_tfunc_decls
                | Ast.Untrusted f ->
                    ac_ufunc_decls := f :: !ac_ufunc_decls) xs
      ) e.Ast.eexpr;
    { file_shortnm = e.Ast.ename;
      enclave_name = Util.to_c_identifier e.Ast.ename;
      include_list = List.rev !ac_include_list;
      import_exprs = List.rev !ac_import_exprs;
      comp_defs    = List.rev !ac_comp_defs;
      tfunc_decls  = List.rev !ac_tfunc_decls;
      ufunc_decls  = List.rev !ac_ufunc_decls; }

let is_foreign_array (pt: Ast.parameter_type) =
  match pt with
      Ast.PTVal _     -> false
    | Ast.PTPtr(t, a) ->
        match t with
            Ast.Foreign _ -> a.Ast.pa_isary
          | _             -> false

(* A naked function has neither parameters nor return value. *)
let is_naked_func (fd: Ast.func_decl) =
  fd.Ast.rtype = Ast.Void && fd.Ast.plist = []

(* 
 * If user only defined a trusted function w/o neither parameter nor
 * return value, the generated trusted bridge will not call any tRTS
 * routines.  If the real trusted function doesn't call tRTS function
 * either (highly possible), then the MSVC linker will not link tRTS
 * into the result enclave.
 *)
let tbridge_gen_dummy_variable (ec: enclave_content) =
  let _dummy_variable =
    sprintf "\n#ifdef _MSC_VER\n\
\t/* In case enclave `%s' doesn't call any tRTS function. */\n\
\tvolatile int force_link_trts = sgx_is_within_enclave(NULL, 0);\n\
\t(void) force_link_trts; /* avoid compiler warning */\n\
#endif\n\n" ec.enclave_name
  in
    if ec.ufunc_decls <> [] then ""
    else
      if List.for_all (fun tfd -> is_naked_func tfd.Ast.tf_fdecl) ec.tfunc_decls
      then _dummy_variable
      else ""

(* This function is used to convert Array form into Pointer form.
 * e.g.: int array[10][20]   =>  [count = 200] int* array
 *
 * This function is called when generating proxy/bridge code and
 * the marshaling structure.
 *)
let conv_array_to_ptr (pd: Ast.pdecl): Ast.pdecl =
  let (pt, declr) = pd in
  let get_count_attr ilist =
    (* XXX: assume the size of each dimension will be > 0. *)
    Ast.ANumber (List.fold_left (fun acc i -> acc*i) 1 ilist)
  in
    match pt with
      Ast.PTVal _        ->  (pt, declr)
    | Ast.PTPtr(aty, pa) ->
      if Ast.is_array declr then
        let tmp_declr = { declr with Ast.array_dims = [] } in
        let tmp_aty = Ast.Ptr aty in
        let tmp_cnt = get_count_attr declr.Ast.array_dims in
        let tmp_pa = { pa with Ast.pa_size = { Ast.empty_ptr_size with Ast.ps_count = Some tmp_cnt } }
        in (Ast.PTPtr(tmp_aty, tmp_pa), tmp_declr)
      else (pt, declr)

(* ------------------------------------------------------------------
 * Code generation for edge-routines.
 * ------------------------------------------------------------------
 *)

(* Little functions for naming of a struct and its members etc *)
let retval_name = "retval"
let retval_declr = { Ast.identifier = retval_name; Ast.array_dims = []; }
let eid_name = "eid"
let ms_ptr_name = "pms"
let ms_struct_val = "ms"
let ms_in_struct_val = "__in_ms"
let mk_ms_member_name (pname: string) = "ms_" ^ pname
let mk_ms_struct_name (fname: string) = "ms_" ^ fname ^ "_t"
let ms_retval_name = mk_ms_member_name retval_name
let mk_tbridge_name (fname: string) = "sgx_" ^ fname
let mk_parm_accessor name = sprintf "%s->%s" ms_struct_val (mk_ms_member_name name)
let mk_in_parm_accessor name = sprintf "%s.%s" ms_in_struct_val (mk_ms_member_name name)
let mk_tmp_var name = "_tmp_" ^ name
let mk_tmp_var2 name1 name2 = "_tmp_" ^ name1 ^ "_" ^ name2
let mk_len_var name = "_len_" ^ name
let mk_len_var2 name1 name2 = "_len_" ^ name1 ^ "_" ^ name2
let mk_in_var name = "_in_" ^ name
let mk_in_var2 name1 name2 = "_in_" ^ name1 ^ "_" ^ name2
let mk_ocall_table_name enclave_name = "ocall_table_" ^ enclave_name

(* Un-trusted bridge name is prefixed with enclave file short name. *)
let mk_ubridge_name (enclave_name: string) (funcname: string) =
  sprintf "%s_%s" enclave_name funcname

let mk_ubridge_proto (enclave_name: string) (funcname: string) =
  sprintf "static sgx_status_t SGX_CDECL %s(void* %s)"
          (mk_ubridge_name enclave_name funcname) ms_ptr_name

(* Common macro definitions. *)
let common_macros = "#include <stdlib.h> /* for size_t */\n\n\
#define SGX_CAST(type, item) ((type)(item))\n\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n"

(* Header footer *)
let header_footer = "\n#ifdef __cplusplus\n}\n#endif /* __cplusplus */\n\n#endif\n"

(* Little functions for generating file names. *)
let get_uheader_short_name (file_shortnm: string) = file_shortnm ^ "_u.h"
let get_uheader_name (file_shortnm: string) =
  !g_untrusted_dir ^ separator_str ^ (get_uheader_short_name file_shortnm)

let get_usource_name (file_shortnm: string) =
  !g_untrusted_dir ^ separator_str ^ file_shortnm ^ "_u.c"

let get_theader_short_name (file_shortnm: string) = file_shortnm ^ "_t.h"
let get_theader_name (file_shortnm: string) =
  !g_trusted_dir ^ separator_str ^ (get_theader_short_name file_shortnm)

let get_tsource_name (file_shortnm: string) =
  !g_trusted_dir ^ separator_str ^ file_shortnm ^ "_t.c"

(* Construct the string of structure definition *)
let mk_struct_decl (fs: string) (name: string) =
  sprintf "typedef struct %s {\n%s} %s;\n" name fs name

(* Construct the string of structure definition with #ifndef surrounded *)
let mk_struct_decl_with_macro (fs: string) (name: string) =
  sprintf "#ifndef _%s\n#define _%s\ntypedef struct %s {\n%s} %s;\n#endif\n" name name name fs name

(* Construct the string of union definition with #ifndef surrounded *)
let mk_union_decl_with_macro (fs: string) (name: string) =
  sprintf "#ifndef _%s\n#define _%s\ntypedef union %s {\n%s} %s;\n#endif\n" name name name fs name

(* Generate a definition of enum with #ifndef surrounded *)
let mk_enum_def (e: Ast.enum_def) =
  let gen_enum_ele_str (ele: Ast.enum_ele) =
    let k, v = ele in
      match v with
          Ast.EnumValNone -> k
        | Ast.EnumVal ev  -> sprintf "%s = %s" k (Ast.attr_value_to_string ev)
  in
  let enname = e.Ast.enname in
  let enbody = e.Ast.enbody in
  let enbody_str =
    if enbody = [] then ""
    else List.fold_left (fun acc ele ->
               acc ^ "\t" ^ gen_enum_ele_str ele ^ ",\n") "" enbody
  in
    if enname = "" then sprintf "enum {\n%s};\n" enbody_str
    else sprintf "typedef enum %s {\n%s} %s;\n" enname enbody_str enname

let get_array_dims (ns: int list) =
  (* Get the array declaration from a list of array dimensions.
   * Empty `ns' indicates the corresponding declarator is a simple identifier.
   * Element of value -1 means that user does not specify the dimension size.
   *)
  let get_dim n = if n = -1 then "[]" else sprintf "[%d]" n
  in
    if ns = [] then ""
    else List.fold_left (fun acc n -> acc ^ get_dim n) "" ns

let get_typed_declr_str (ty: Ast.atype) (declr: Ast.declarator) =
  let tystr = Ast.get_tystr  ty in
  let dmstr = get_array_dims declr.Ast.array_dims in
    sprintf "%s %s%s" tystr declr.Ast.identifier dmstr

(* Construct a member declaration string *)
let mk_member_decl (ty: Ast.atype) (declr: Ast.declarator) =
  sprintf "\t%s;\n" (get_typed_declr_str ty declr)

(* Check whether given parameter is `const' specified. *)
let is_const_ptr (pt: Ast.parameter_type) =
    match pt with
      Ast.PTVal _      -> false
    | Ast.PTPtr(aty, pa) ->
      if not pa.Ast.pa_rdonly then false
      else
        match aty with
          Ast.Foreign _ -> false
        | _             -> true

(* Note that, for a foreign array type `foo_array_t' we will generate
 *   foo_array_t* ms_field;
 * in the marshaling data structure to keep the pass-by-address scheme
 * as in the C programming language.
*)
let mk_ms_member_decl (pt: Ast.parameter_type) (declr: Ast.declarator) (isecall: bool) =
  let qual = if is_const_ptr pt then "const " else "" in
  let aty = Ast.get_param_atype pt in
  let tystr = Ast.get_tystr aty in
  let ptr = if is_foreign_array pt then "* " else "" in
  let field = mk_ms_member_name declr.Ast.identifier in
  (* String attribute is available for in/inout both ecall and ocall.
   * For ocall ,strlen is called in trusted proxy ocde, so no need to defense it.
   *)
  let need_str_len_var (pt: Ast.parameter_type) =
    match pt with
    Ast.PTVal _        -> false
    | Ast.PTPtr(_, pa) ->
    if pa.Ast.pa_isstr || pa.Ast.pa_iswstr then
        match pa.Ast.pa_direction with
        Ast.PtrInOut | Ast.PtrIn ->  if isecall then true else false
        | _ -> false
    else false
  in
  let str_len = if need_str_len_var pt then sprintf "\tsize_t %s_len;\n" field else ""
  in
  let dmstr = get_array_dims declr.Ast.array_dims in
    sprintf "\t%s%s%s %s%s;\n%s" qual tystr ptr field dmstr str_len

(* It is used to save the structures defined in EDLs. *)
let defined_structure = ref []
let is_structure_defined s = List.exists (fun (( i, _ ): (Ast.struct_def * bool)) -> i.Ast.sname = s) !defined_structure
let get_struct_def s = List.find (fun (( i, _ ): (Ast.struct_def * bool)) -> i.Ast.sname = s) !defined_structure

let is_structure_deep_copy (s:Ast.struct_def) = 
    let is_deep_copy (pd: Ast.pdecl) = 
      let (pt, _)    = pd in
      match pt with
        | Ast.PTVal _      -> false
        | Ast.PTPtr (_, attr) -> if attr.Ast.pa_size = Ast.empty_ptr_size then false else true
    in
    List.exists is_deep_copy s.Ast.smlist


let is_foreign_a_structure (pt: Ast.parameter_type) =
  let rec is_foreign_atype (atype: Ast.atype) =
      match atype with
        | Ast.Ptr(ptr)      -> is_foreign_atype ptr
        | Ast.Foreign(name) -> (is_structure_defined name, name)
        | _                 -> (false, "")
  in
  match pt with
    | Ast.PTVal(atype)
    | Ast.PTPtr(atype, _) -> is_foreign_atype atype
    | _                   -> (false, "")

(* Check duplicated structure definition and illegal usage.
 *)
let check_structure (ec: enclave_content) =
  let trusted_fds = tf_list_to_fd_list ec.tfunc_decls in
  let untrusted_fds = uf_list_to_fd_list ec.ufunc_decls in
  List.iter(fun (st: Ast.composite_type) ->
      match st with
        Ast.StructDef s -> 
            if is_structure_defined s.Ast.sname then 
                failwithf "duplicated structure definition `%s'" s.Ast.sname
            else 
                defined_structure := (s, is_structure_deep_copy s) :: !defined_structure;
        | _ -> ()
      )  ec.comp_defs;
  List.iter  (fun (fd: Ast.func_decl) ->
        List.iter (fun (pd: Ast.pdecl) ->
            let (pt, _)= pd in
            match pt with
              | Ast.PTVal (Ast.Struct(s))     ->
                if is_structure_defined s then
                      let (struct_def, deep_copy) = get_struct_def s
                      in
                      if deep_copy then 
                        failwithf "the structure declaration \"%s\" specifies a deep copy is expected. Referenced by value in function \"%s\" detected."s fd.Ast.fname
                      else
                        if List.exists (fun (pt, _) ->
                           match pt with
                             Ast.PTVal _        -> false
                           | Ast.PTPtr _        -> true       )  struct_def.Ast.smlist
                        then (eprintf "warning: the structure \"%s\" is referenced by value in function \"%s\". Part of the data may not be copied.\n"s fd.Ast.fname)
                        else ()
                 else ()
            | Ast.PTPtr (Ast.Ptr(Ast.Struct(s)), attr) ->
                if is_structure_defined s then
                      let (_, deep_copy) = get_struct_def s
                      in
                      if deep_copy && attr.Ast.pa_direction = Ast.PtrOut then 
                        failwithf "the structure declaration \"%s\" specifies a deep copy, should not be used with an `out' attribute in function \"%s\"."s fd.Ast.fname
                      else ()
                 else ()
            | a -> let (found, name) = is_foreign_a_structure a
                in
                    if found then
                      let (_, deep_copy) = get_struct_def name
                      in
                      if deep_copy then
                        failwithf "`%s' in function `%s' is a structure and it specifies a deep copy. Use `struct %s' instead." name fd.Ast.fname name
                      else
                        (eprintf "warning: `%s' in function `%s' is a structure. Use `struct %s' instead.\n" name fd.Ast.fname name)
                else ()
            ) fd.Ast.plist
        ) (trusted_fds @ untrusted_fds)

(* Generate code using given function if a parameter is structure pointer. *)
let invoke_if_struct (ty: Ast.atype) (param_direction: Ast.ptr_direction) (var_name: string)
    (pre:string -> string -> string)
    (generator: Ast.ptr_direction -> string -> string -> Ast.atype -> Ast.ptr_attr -> Ast.declarator -> string )
    (post:string)=
    match ty with
        Ast.Ptr(Ast.Struct(struct_type)) ->
            if is_structure_defined struct_type then
              let (struct_def, deep_copy)= get_struct_def struct_type
              in
              if deep_copy then
                let body = 
                  List.fold_left (
                    fun acc (pty, declr) ->
                     match pty with
                        Ast.PTVal _          -> acc
                      | Ast.PTPtr (member_ty, member_attr) ->
                        if member_attr.Ast.pa_size <> Ast.empty_ptr_size then
                          acc ^ generator param_direction struct_type var_name member_ty member_attr declr
                        else acc 
                    ) "" struct_def.Ast.smlist
                in
                if body = "" then ""
                else (pre struct_type var_name) ^ body ^ post
              else ""
            else ""
       | _ -> ""

(* Generate data structure definition *)
let gen_comp_def (st: Ast.composite_type) =
  let gen_union_member_list mlist =
    List.fold_left (fun acc (ty, pdeclr) ->
                 acc ^ mk_member_decl ty pdeclr) "" mlist
  in
  let gen_struct_member_list mlist =
    List.fold_left (fun acc (pt, declr) ->
                acc ^ mk_member_decl (Ast.get_param_atype pt) declr) "" mlist
  in
    match st with
        Ast.StructDef s -> mk_struct_decl_with_macro (gen_struct_member_list s.Ast.smlist) s.Ast.sname
      | Ast.UnionDef  u -> mk_union_decl_with_macro (gen_union_member_list u.Ast.umlist) u.Ast.uname
      | Ast.EnumDef   e -> mk_enum_def    e

(* Generate a list of '#include' *)
let gen_include_list (xs: string list) =
  List.fold_left (fun acc s -> acc ^ sprintf "#include \"%s\"\n" s) "" xs

(* Get the type string from 'parameter_type' *)
let get_param_tystr (pt: Ast.parameter_type) =
  Ast.get_tystr (Ast.get_param_atype pt)

(* Generate marshaling structure definition *)
let gen_marshal_struct (fd: Ast.func_decl) (errno: string) (isecall: bool) =
    let member_list_str = errno ^
    let new_param_list = List.map conv_array_to_ptr fd.Ast.plist in
    List.fold_left (fun acc (pt, declr) ->
            acc ^ mk_ms_member_decl pt declr isecall) "" new_param_list in
  let struct_name = mk_ms_struct_name fd.Ast.fname in
    match fd.Ast.rtype with
        (* A function w/o return value and parameters doesn't need
           a marshaling struct. *)
        Ast.Void -> if fd.Ast.plist = [] && errno = "" then ""
                    else mk_struct_decl member_list_str struct_name
      | _ -> let rv_str = mk_ms_member_decl (Ast.PTVal fd.Ast.rtype) retval_declr isecall
             in mk_struct_decl (rv_str ^ member_list_str) struct_name

let gen_ecall_marshal_struct (tf: Ast.trusted_func) =
    gen_marshal_struct tf.Ast.tf_fdecl "" true

let gen_ocall_marshal_struct (uf: Ast.untrusted_func) =
    let errno_decl = if uf.Ast.uf_propagate_errno then "\tint ocall_errno;\n" else "" in
    gen_marshal_struct uf.Ast.uf_fdecl errno_decl false

(* Generate parameter representation. *)
let gen_parm_str (p: Ast.pdecl) =
  let (pt, (declr : Ast.declarator)) = p in
  let aty = Ast.get_param_atype pt in
  let str = get_typed_declr_str aty declr in
    if is_const_ptr pt then "const " ^ str else str

(* Generate parameter representation of return value. *)
let gen_parm_retval (rt: Ast.atype) =
  if rt = Ast.Void then ""
  else Ast.get_tystr rt ^ "* " ^ retval_name

(* ---------------------------------------------------------------------- *)

(* `gen_ecall_table' is used to generate ECALL table with the following form:
    SGX_EXTERNC const struct {
       size_t nr_ecall;    /* number of ECALLs */
       struct {
           void   *ecall_addr;
           uint8_t is_priv;
           uint8_t is_switchless;
       } ecall_table [nr_ecall];
   } g_ecall_table = {
       2, { {sgx_foo, 1, 0}, {sgx_bar, 0, 1} }
   };
*)
let gen_ecall_table (tfs: Ast.trusted_func list) =
  let ecall_table_name = "g_ecall_table" in
  let ecall_table_size = List.length tfs in
  let trusted_fds = tf_list_to_fd_list tfs in
  let tbridge_names = List.map (fun (fd: Ast.func_decl) ->
                                  mk_tbridge_name fd.Ast.fname) trusted_fds in
  let priv_switchless_bits = List.map (fun (tf: Ast.trusted_func ) ->
                                  (is_priv_ecall tf, is_switchless_ecall tf) ) tfs in
  let ecall_table =
    let bool_to_int b = if b then 1 else 0 in
    let inner_table =
      List.fold_left2 (fun acc s (b1, b2) ->
        sprintf "%s\t\t{(void*)(uintptr_t)%s, %d, %d},\n" acc s (bool_to_int b1) (bool_to_int b2)) "" tbridge_names priv_switchless_bits
    in "\t{\n" ^ inner_table ^ "\t}\n"
  in
    sprintf "SGX_EXTERNC const struct {\n\
\tsize_t nr_ecall;\n\
\tstruct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[%d];\n\
} %s = {\n\
\t%d,\n\
%s};\n" ecall_table_size
      ecall_table_name
      ecall_table_size
      (if ecall_table_size = 0 then "" else ecall_table)

(* `gen_entry_table' is used to generate Dynamic Entry Table with the form:
   SGX_EXTERNC const struct {
       /* number of OCALLs (number of ECALLs can be found in ECALL table) */
       size_t nr_ocall;

       /* entry_table[m][n] = 1 iff. ECALL n is allowed in the OCALL m. */
       uint8_t entry_table[NR_OCALL][NR_ECALL];
   } g_dyn_entry_table = {
       3, {{0, 0}, {0, 1}, {1, 0}}
   };
*)
let gen_entry_table (ec: enclave_content) =
  let dyn_entry_table_name = "g_dyn_entry_table" in
  let ocall_table_size = List.length ec.ufunc_decls in
  let trusted_func_names = get_trusted_func_names ec in
  let ecall_table_size = List.length trusted_func_names in
  let get_entry_array (allowed_ecalls: string list) =
    List.fold_left (fun acc name ->
                      acc ^ (if List.exists (fun x -> x=name) allowed_ecalls
                             then "1"
                             else "0") ^ ", ") "" trusted_func_names in
  let entry_table =
    let inner_table =
      List.fold_left (fun acc (uf: Ast.untrusted_func) ->
                        let entry_array = get_entry_array uf.Ast.uf_allow_list
                        in acc ^ "\t\t{" ^ entry_array ^ "},\n") "" ec.ufunc_decls
    in
      "\t{\n" ^ inner_table ^ "\t}\n"
  in
    (* Generate dynamic entry table iff. both sgx_ecall/ocall_table_size > 0 *)
  let gen_table_p = (ecall_table_size > 0) && (ocall_table_size > 0) in
    (* When NR_ECALL is 0, or NR_OCALL is 0, there will be no entry table field. *)
  let entry_table_field =
    if gen_table_p then
      sprintf "\tuint8_t entry_table[%d][%d];\n" ocall_table_size ecall_table_size
    else
      ""
  in
    sprintf "SGX_EXTERNC const struct {\n\
\tsize_t nr_ocall;\n%s\
} %s = {\n\
\t%d,\n\
%s};\n" entry_table_field
        dyn_entry_table_name
        ocall_table_size
        (if gen_table_p then entry_table else "")

(* ---------------------------------------------------------------------- *)

(* Generate the function prototype for untrusted proxy in COM style.
 * For example, un-trusted functions
 *   int foo(double d);
 *   void bar(float f);
 *
 * will have an untrusted proxy like below:
 *   sgx_status_t foo(int* retval, double d);
 *   sgx_status_t bar(float f);
 *)
let gen_tproxy_proto (fd: Ast.func_decl) =
  let parm_list =
    match fd.Ast.plist with
        [] -> ""
      | x :: xs ->
          List.fold_left (fun acc pd ->
                            acc ^ ", " ^ gen_parm_str pd) (gen_parm_str x) xs
  in
  let retval_parm_str = gen_parm_retval fd.Ast.rtype in
    if fd.Ast.plist = [] && fd.Ast.rtype = Ast.Void then
      sprintf "sgx_status_t SGX_CDECL %s(void)" fd.Ast.fname
    else if fd.Ast.plist = [] then
      sprintf "sgx_status_t SGX_CDECL %s(%s)" fd.Ast.fname retval_parm_str
    else if fd.Ast.rtype = Ast.Void then
           sprintf "sgx_status_t SGX_CDECL %s(%s)" fd.Ast.fname parm_list
         else
           sprintf "sgx_status_t SGX_CDECL %s(%s, %s)" fd.Ast.fname retval_parm_str parm_list

(* Generate the function prototype for untrusted proxy in COM style.
 * For example, trusted functions
 *   int foo(double d);
 *   void bar(float f);
 *
 * will have an untrusted proxy like below:
 *   sgx_status_t foo(sgx_enclave_id_t eid, int* retval, double d);
 *   sgx_status_t foo(sgx_enclave_id_t eid, float f);
 *
 * When `g_use_prefix' is true, the untrusted proxy name is prefixed
 * with the `prefix' parameter.
 *
 *)
let gen_uproxy_com_proto (fd: Ast.func_decl) (prefix: string) =
  let retval_parm_str = gen_parm_retval fd.Ast.rtype in

  let eid_parm_str =
    if fd.Ast.rtype = Ast.Void then sprintf "(sgx_enclave_id_t %s" eid_name
    else sprintf "(sgx_enclave_id_t %s, " eid_name in
  let parm_list =
    List.fold_left (fun acc pd -> acc ^ ", " ^ gen_parm_str pd)
      retval_parm_str fd.Ast.plist in
  let fname =
    if !g_use_prefix then sprintf "%s_%s" prefix fd.Ast.fname
    else fd.Ast.fname
  in "sgx_status_t " ^ fname ^ eid_parm_str ^ parm_list ^ ")"

let get_ret_tystr (fd: Ast.func_decl) = Ast.get_tystr fd.Ast.rtype
let get_plist_str (fd: Ast.func_decl) =
  if fd.Ast.plist = [] then "void"
  else List.fold_left (fun acc pd -> acc ^ ", " ^ gen_parm_str pd)
                      (gen_parm_str (List.hd fd.Ast.plist))
                      (List.tl fd.Ast.plist)

(* Generate the function prototype as is. *)
let gen_func_proto (fd: Ast.func_decl) =
  let ret_tystr = get_ret_tystr fd in
  let plist_str = get_plist_str fd in
    sprintf "%s %s(%s)" ret_tystr fd.Ast.fname plist_str

(* Generate prototypes for untrusted function. *)
let gen_ufunc_proto (uf: Ast.untrusted_func) =
  let dllimport = if uf.Ast.uf_fattr.Ast.fa_dllimport then "SGX_DLLIMPORT " else "" in
  let ret_tystr = get_ret_tystr uf.Ast.uf_fdecl in
  let cconv_str = "SGX_" ^ Ast.get_call_conv_str uf.Ast.uf_fattr.Ast.fa_convention in
  let func_name = uf.Ast.uf_fdecl.Ast.fname in
  let plist_str = get_plist_str uf.Ast.uf_fdecl in
  let func_guard = sprintf "%s_DEFINED__" (String.uppercase func_name) in 
    sprintf "#ifndef %s\n#define %s\n%s%s SGX_UBRIDGE(%s, %s, (%s));\n#endif"
            func_guard func_guard dllimport ret_tystr cconv_str func_name plist_str

(* The preemble contains common include expressions. *)
let gen_uheader_preemble (guard: string) (inclist: string)=
  let grd_hdr = sprintf "#ifndef %s\n#define %s\n\n" guard guard in
  let inc_exp = "#include <stdint.h>\n\
#include <wchar.h>\n\
#include <stddef.h>\n\
#include <string.h>\n\
#include \"sgx_edger8r.h\" /* for sgx_status_t etc. */\n" in
    grd_hdr ^ inc_exp ^ "\n" ^ inclist ^ "\n" ^ common_macros

let ms_writer out_chan ec =
  let ms_struct_ecall = List.map gen_ecall_marshal_struct ec.tfunc_decls in
  let ms_struct_ocall = List.map gen_ocall_marshal_struct ec.ufunc_decls in
  let output_struct s = 
    match s with
        "" -> s
      | _  -> sprintf "%s\n" s
  in
    List.iter (fun s -> output_string out_chan (output_struct s)) ms_struct_ecall;
    List.iter (fun s -> output_string out_chan (output_struct s)) ms_struct_ocall


(* Generate untrusted header for enclave *)
let gen_untrusted_header (ec: enclave_content) =
  let header_fname = get_uheader_name ec.file_shortnm in
  let guard_macro = sprintf "%s_U_H__" (String.uppercase ec.enclave_name) in
  let preemble_code =
    let include_list = gen_include_list (ec.include_list @ !untrusted_headers) in
      gen_uheader_preemble guard_macro include_list
  in
  let comp_def_list   = List.map gen_comp_def ec.comp_defs in
  let func_proto_ufunc = List.map gen_ufunc_proto ec.ufunc_decls in
  let uproxy_com_proto =
      List.map (fun (tf: Ast.trusted_func) ->
                  gen_uproxy_com_proto tf.Ast.tf_fdecl ec.enclave_name)
        ec.tfunc_decls
  in
  let out_chan = open_out header_fname in
    output_string out_chan (preemble_code ^ "\n");
    List.iter (fun s -> output_string out_chan (s ^ "\n")) comp_def_list;
    List.iter (fun s -> output_string out_chan (s ^ "\n")) func_proto_ufunc;
    output_string out_chan "\n";
    List.iter (fun s -> output_string out_chan (s ^ ";\n")) uproxy_com_proto;
    output_string out_chan header_footer;
    close_out out_chan

(* It generates preemble for trusted header file. *)
let gen_theader_preemble (guard: string) (inclist: string) =
  let grd_hdr = sprintf "#ifndef %s\n#define %s\n\n" guard guard in
  let inc_exp = "#include <stdint.h>\n\
#include <wchar.h>\n\
#include <stddef.h>\n\
#include \"sgx_edger8r.h\" /* for sgx_ocall etc. */\n\n" in
    grd_hdr ^ inc_exp ^ inclist ^ "\n" ^ common_macros

(* Generate trusted header for enclave *)
let gen_trusted_header (ec: enclave_content) =
  let header_fname = get_theader_name ec.file_shortnm in
  let guard_macro = sprintf "%s_T_H__" (String.uppercase ec.enclave_name) in
  let guard_code =
    let include_list = gen_include_list (ec.include_list @ !trusted_headers) in
      gen_theader_preemble guard_macro include_list in
  let comp_def_list   = List.map gen_comp_def ec.comp_defs in
  let func_proto_list = List.map gen_func_proto (tf_list_to_fd_list ec.tfunc_decls) in
  let func_tproxy_list= List.map gen_tproxy_proto (uf_list_to_fd_list ec.ufunc_decls) in

  let out_chan = open_out header_fname in
    output_string out_chan (guard_code ^ "\n");
    List.iter (fun s -> output_string out_chan (s ^ "\n")) comp_def_list;
    List.iter (fun s -> output_string out_chan (s ^ ";\n")) func_proto_list;
    output_string out_chan "\n";
    List.iter (fun s -> output_string out_chan (s ^ ";\n")) func_tproxy_list;
    output_string out_chan header_footer;
    close_out out_chan

(* It generates function invocation expression. *)
let mk_parm_name_raw (pt: Ast.parameter_type) (declr: Ast.declarator) (tbridge: bool)=
  let cast_expr =
    let tystr = get_param_tystr pt in
    if Ast.is_array declr && List.length declr.Ast.array_dims > 1
    then
      let dims = get_array_dims (List.tl declr.Ast.array_dims) in
        sprintf "(%s (*)%s)"  tystr dims
    else ""
  in
    cast_expr ^ (if tbridge then mk_in_parm_accessor else mk_parm_accessor) declr.Ast.identifier

(* We passed foreign array `foo_array_t foo' as `&foo[0]', thus we
 * need to get back `foo' by '* array_ptr' where
 *   array_ptr = &foo[0]
*)
let add_foreign_array_ptrref
    (arg: string)
    (pt: Ast.parameter_type) =
    if is_foreign_array pt
    then sprintf "(%s != NULL) ? (*%s) : NULL" arg arg
    else arg

let mk_parm_name_ubridge (pt: Ast.parameter_type) (declr: Ast.declarator) =
  add_foreign_array_ptrref (mk_parm_name_raw pt declr false) pt

let mk_parm_name_ext (pt: Ast.parameter_type) (declr: Ast.declarator) =
  let name = declr.Ast.identifier in
    match pt with
        Ast.PTVal _ -> mk_parm_name_raw pt declr true
      | Ast.PTPtr (_, attr) ->
          match attr.Ast.pa_direction with
            | Ast.PtrNoDirection -> mk_parm_name_raw pt declr true
            | _ -> mk_in_var name

let gen_func_invoking (fd: Ast.func_decl)
                      (mk_parm_name: Ast.parameter_type -> Ast.declarator -> string) =
    match fd.Ast.plist with
      [] -> sprintf "%s();" fd.Ast.fname
    | (pt, (declr : Ast.declarator)) :: ps ->
        sprintf "%s(%s);"
          fd.Ast.fname
          (let p0 = mk_parm_name pt declr in
             List.fold_left (fun acc (pty, dlr) ->
                               acc ^ ", " ^ mk_parm_name pty dlr) p0 ps)

(* Generate untrusted bridge code for a given untrusted function. *)
let gen_func_ubridge (enclave_name: string) (ufunc: Ast.untrusted_func) =
  let fd = ufunc.Ast.uf_fdecl in
  let propagate_errno = ufunc.Ast.uf_propagate_errno in
  let func_open = sprintf "%s\n{\n" (mk_ubridge_proto enclave_name fd.Ast.fname) in
  let func_close = "\treturn SGX_SUCCESS;\n}\n" in
  let set_errno = if propagate_errno then "\tms->ocall_errno = errno;" else "" in
  let ms_struct_name = mk_ms_struct_name fd.Ast.fname in
  let declare_ms_ptr = sprintf "%s* %s = SGX_CAST(%s*, %s);"
                               ms_struct_name
                               ms_struct_val
                               ms_struct_name
                               ms_ptr_name in
  let call_with_pms =
    let invoke_func = gen_func_invoking fd mk_parm_name_ubridge in
      if fd.Ast.rtype = Ast.Void then invoke_func
      else sprintf "%s = %s" (mk_parm_accessor retval_name) invoke_func
  in
    if (is_naked_func fd) && (propagate_errno = false) then
      let check_pms =
        sprintf "if (%s != NULL) return SGX_ERROR_INVALID_PARAMETER;" ms_ptr_name
      in
        sprintf "%s\t%s\n\t%s\n%s" func_open check_pms call_with_pms func_close
      else
        sprintf "%s\t%s\n\t%s\n%s\n%s" func_open declare_ms_ptr call_with_pms set_errno func_close

let fill_ms_field (isptr: bool) (pd: Ast.pdecl) =
  let accessor       = if isptr then "->" else "." in
  let (pt, declr)    = pd in
  let param_name     = declr.Ast.identifier in
  let ms_member_name = mk_ms_member_name param_name in
  let assignment_str (aty: Ast.atype) =
    sprintf "%s%s%s = %s;" ms_struct_val accessor ms_member_name param_name
  in
  let gen_setup_foreign_array aty =
    sprintf "%s%s%s = (%s *)&%s[0];"
      ms_struct_val accessor ms_member_name (Ast.get_tystr aty) param_name
  in
  let gen_setup_foreign_str aty =
    sprintf "%s%s%s_len = %s ? strlen(%s) + 1 : 0;"
      ms_struct_val accessor ms_member_name param_name param_name
  in
  let gen_setup_foreign_wstr aty =
    sprintf "%s%s%s_len = %s ? (wcslen(%s) + 1) * sizeof(wchar_t) : 0;"
      ms_struct_val accessor ms_member_name param_name param_name
  in
    if declr.Ast.array_dims = [] then
      match pt with
          Ast.PTVal(aty)        -> assignment_str aty
        | Ast.PTPtr(aty, pattr) ->
            if pattr.Ast.pa_isary
            then gen_setup_foreign_array aty
            else if pattr.Ast.pa_isstr
            then assignment_str aty ^ "\n\t" ^ gen_setup_foreign_str aty
            else if pattr.Ast.pa_iswstr
            then assignment_str aty ^ "\n\t" ^ gen_setup_foreign_wstr aty
            else assignment_str aty
    else
      (* Arrays are passed by address. *)
      let tystr = Ast.get_tystr (Ast.Ptr (Ast.get_param_atype pt)) in
      sprintf "%s%s%s = (%s)%s;" ms_struct_val accessor ms_member_name tystr param_name

(* Generate untrusted proxy code for a given trusted function. *)
let gen_func_uproxy (tf: Ast.trusted_func) (idx: int) (ec: enclave_content) =
  let fd = tf.Ast.tf_fdecl in
  let func_open  =
    gen_uproxy_com_proto fd ec.enclave_name ^
      "\n{\n\tsgx_status_t status;\n"
  in
  let func_close = "\treturn status;\n}\n" in
  let ocall_table_name  = mk_ocall_table_name ec.enclave_name in
  let ms_struct_name  = mk_ms_struct_name fd.Ast.fname in
  let declare_ms_expr = sprintf "%s %s;" ms_struct_name ms_struct_val in
  let ocall_table_ptr =
    sprintf "&%s" ocall_table_name in
  let sgx_ecall_fn = get_sgx_fname SGX_ECALL tf.Ast.tf_is_switchless in

  (* Normal case - do ECALL with marshaling structure*)
  let ecall_with_ms = sprintf "status = %s(%s, %d, %s, &%s);"
                              sgx_ecall_fn eid_name idx ocall_table_ptr ms_struct_val in

  (* Rare case - the trusted function doesn't have parameter nor return value.
   * In this situation, no marshaling structure is required - passing in NULL.
   *)
  let ecall_null = sprintf "status = %s(%s, %d, %s, NULL);"
                           sgx_ecall_fn eid_name idx ocall_table_ptr
  in
  let update_retval = sprintf "if (status == SGX_SUCCESS && %s) *%s = %s.%s;"
                              retval_name retval_name ms_struct_val ms_retval_name in
  let func_body = ref [] in
    if is_naked_func fd then
      sprintf "%s\t%s\n%s" func_open ecall_null func_close
    else
      begin
        func_body := declare_ms_expr :: !func_body;
        List.iter (fun pd -> func_body := fill_ms_field false pd :: !func_body) fd.Ast.plist;
        func_body := ecall_with_ms :: !func_body;
        if fd.Ast.rtype <> Ast.Void then func_body := update_retval :: !func_body;
          List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") func_open (List.rev !func_body) ^ func_close
      end

(* Generate an expression to check the pointers. *)
let mk_check_ptr (name: string) (lenvar: string) =
  let checker = "CHECK_UNIQUE_POINTER"
  in sprintf "\t%s(%s, %s);\n" checker name lenvar

(* Pointer to marshaling structure should never be NULL. *)
let mk_check_pms (fname: string) =
  let lenvar = sprintf "sizeof(%s)" (mk_ms_struct_name fname)
  in sprintf "\t%s(%s, %s);%s" "CHECK_REF_POINTER" ms_ptr_name lenvar
        "\n\t//\n\t// fence after pointer checks\n\t//\n\tsgx_lfence();\n"

(* Generate code to get the size of the pointer. *)
let gen_ptr_size (ty: Ast.atype) (pattr: Ast.ptr_attr) (name: string) (get_parm: string -> string) =
  let len_var = mk_len_var name in
  let parm_name = get_parm name in

  let mk_len_size v =
    match v with
        Ast.AString s -> get_parm s
      | Ast.ANumber n -> sprintf "%d" n in

  let mk_len_count v size_str =
    match v with
        Ast.AString s -> sprintf "%s * %s" (get_parm s) size_str
      | Ast.ANumber n -> sprintf "%d * %s" n size_str in

  (* size_str:
             [size = n] -> n
             int ptr[] -> sizeof(int)
             int* ptr -> sizeof(int)
             int **ptr -> sizeof(int* )
             Mystruct struct -> sizeof(Mystruct)
             pMystruct ptr -> sizeof( *ptr)
  *)
  let do_ps_attribute (sattr: Ast.ptr_size) =
    let size_str =
      match sattr.Ast.ps_size with
        Some a -> mk_len_size a
      | None   -> 
        match ty with
          Ast.Ptr ty  -> 
            sprintf "sizeof(%s)" (Ast.get_tystr ty)
        | _ -> 
          if pattr.Ast.pa_isptr then
            sprintf "sizeof(*%s)" parm_name
          else
            sprintf "sizeof(%s)"  (Ast.get_tystr ty)
      in
      match sattr.Ast.ps_count with
        None   -> size_str
      | Some a -> mk_len_count a size_str
    in
    sprintf "size_t %s = %s;\n"
          len_var
          (if pattr.Ast.pa_isary then
             sprintf "sizeof(%s)" (Ast.get_tystr ty)
           else
             (* genrerate ms_parm_len only for ecall with string/wstring in _t.c.*)
             if (pattr.Ast.pa_isstr || pattr.Ast.pa_iswstr) && parm_name <> name then
                 sprintf "%s_len " (mk_in_parm_accessor name)
               else
                 (* genrerate strlen(param)/wcslen(param) only for ocall with string/wstring in _t.c.*)
                 if pattr.Ast.pa_isstr then
                   sprintf "%s ? strlen(%s) + 1 : 0" parm_name parm_name
                 else 
                   if pattr.Ast.pa_iswstr then
                     sprintf "%s ? (wcslen(%s) + 1) * sizeof(wchar_t) : 0" parm_name parm_name
                   else do_ps_attribute pattr.Ast.pa_size)

(* Find the data type of a parameter. *)
let find_param_type (name: string) (plist: Ast.pdecl list) =
  try
    let (pt, _) = List.find (fun (pd: Ast.pdecl) ->
                            let (pt, declr) = pd
                            in declr.Ast.identifier = name) plist
    in get_param_tystr pt
  with
     Not_found -> failwithf "parameter `%s' not found." name

(* Generate code to check the length of buffers. *)
let gen_check_tbridge_length_overflow (plist: Ast.pdecl list) =
  let gen_check_length (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let tmp_ptr_name= mk_tmp_var name in
 
    let mk_len_size v =
      match v with
        Ast.AString s -> mk_tmp_var s
      | Ast.ANumber n -> sprintf "%d" n in

    let gen_check_overflow cnt size_str =
      let if_statement =
        match cnt with
            Ast.AString s -> sprintf "\tif (%s != 0 &&\n\t\t(size_t)%s > (SIZE_MAX / %s)) {\n" size_str (mk_tmp_var s) size_str
          | Ast.ANumber n -> sprintf "\tif (%s != 0 &&\n\t\t%d > (SIZE_MAX / %s)) {\n" size_str n size_str
      in
        sprintf "%s\t\treturn SGX_ERROR_INVALID_PARAMETER;\n\t}" if_statement
    in
      let size_str =
        match attr.Ast.pa_size.Ast.ps_size with
          Some a -> mk_len_size a
        | None   -> sprintf "sizeof(*%s)" tmp_ptr_name
      in
        match attr.Ast.pa_size.Ast.ps_count with
          None   -> ""
        | Some a -> sprintf "%s\n\n" (gen_check_overflow a size_str) 
  in
    List.fold_left
      (fun acc (pty, declr) ->
         match pty with
             Ast.PTVal _         -> acc
           | Ast.PTPtr(ty, attr) -> acc ^ gen_check_length ty attr declr) "" plist

(* Generate code to check all function parameters which are pointers. *)
let gen_check_tbridge_ptr_parms (plist: Ast.pdecl list) =
  let gen_check_ptr (ty: Ast.atype) (pattr: Ast.ptr_attr) (declr: Ast.declarator) =
    if not pattr.Ast.pa_chkptr then ""
    else
      let name = declr.Ast.identifier in
      let len_var = mk_len_var name in
      let parm_name = mk_tmp_var name in
        if pattr.Ast.pa_chkptr
        then mk_check_ptr parm_name len_var
        else ""
  in
  let new_param_list = List.map conv_array_to_ptr plist
  in
  let pointer_checkings =
    List.fold_left
      (fun acc (pty, declr) ->
         match pty with
             Ast.PTVal _         -> acc
           | Ast.PTPtr(ty, attr) -> acc ^ gen_check_ptr ty attr declr) "" new_param_list
  in
  if pointer_checkings = "" then ""
  else pointer_checkings ^ "\n\t//\n\t// fence after pointer checks\n\t//\n\tsgx_lfence();\n"

(* If a foreign type is a readonly pointer, we cast it to 'void*' for memcpy() and free() *)
let mk_in_ptr_dst_name (ty: Ast.atype) (attr: Ast.ptr_attr) (ptr_name: string) =
  let rdonly = 
    match ty with
      Ast.Foreign _ -> attr.Ast.pa_rdonly
    | _             -> false
  in
  if rdonly then "(void*)" ^ ptr_name
  else ptr_name

let gen_struct_ptr_size (ty: Ast.atype) (pattr: Ast.ptr_attr) (name: string) (get_parm: string -> string) =
  let mk_len_size v =
    match v with
        Ast.AString s -> get_parm s
      | Ast.ANumber n -> sprintf "%d" n in
  let mk_len_count v size_str =
    match v with
        Ast.AString s -> sprintf "%s * %s" (get_parm s) size_str
      | Ast.ANumber n -> sprintf "%d * %s" n size_str in
  let do_ps_attribute (sattr: Ast.ptr_size) =
    let size_str =
      match sattr.Ast.ps_size with
        Some a -> mk_len_size a
      | None   -> 
        match ty with
          Ast.Ptr ty  -> 
            sprintf "sizeof(%s)" (Ast.get_tystr ty)
        | _ -> 
          if pattr.Ast.pa_isptr then
            sprintf "sizeof(*%s)" (get_parm name)
          else
            sprintf "sizeof(%s)"  (Ast.get_tystr ty)
    in
      match sattr.Ast.ps_count with
        None   -> size_str
      | Some a -> mk_len_count a size_str
  in
  sprintf "%s" (do_ps_attribute pattr.Ast.pa_size)
  
(* Generate code to check the length of buffers in structure. *)
let gen_check_member_length (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) (get_parm: string -> string) (indent: string) (break_out: string list) =
    let name        = declr.Ast.identifier in
    let mk_len_size v =
      match v with
        Ast.AString s -> get_parm s
      | Ast.ANumber n -> sprintf "%d" n in

    let gen_check_overflow cnt size_str =
      let if_statement =
        match cnt with
            Ast.AString s -> sprintf "if (%s != 0 &&\n%s\t\t(size_t)%s > (SIZE_MAX / %s)) {\n" size_str indent (get_parm s) size_str
          | Ast.ANumber n -> sprintf "if (%s != 0 &&\n%s\t\t%d > (SIZE_MAX / %s)) {\n" size_str indent n size_str
      in
        (List.fold_left (fun acc s -> acc ^ indent ^ s ^ "\n") if_statement break_out) ^ indent ^ "}"
    in
    let size_str =
      match attr.Ast.pa_size.Ast.ps_size with
        Some a -> mk_len_size a
      | None   -> 
        match ty with
          Ast.Ptr ty  -> 
            sprintf "sizeof(%s)" (Ast.get_tystr ty)
        | _ -> 
          if attr.Ast.pa_isptr then
            sprintf "sizeof(*%s)" (get_parm name)
          else
            sprintf "sizeof(%s)"  (Ast.get_tystr ty)
      in
        match attr.Ast.pa_size.Ast.ps_count with
          None   -> ""
        | Some a -> (gen_check_overflow a size_str) 

(* Generate the code to handle structure pointer deep copy,
 * which is to be inserted before actually calling the trusted function.
 *)
let gen_struct_ptr_direction_pre_calculate (param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let in_struct_name = mk_in_var struct_name in
    let in_ptr_name = mk_in_var2 struct_name name in
    let len_var     = mk_len_var2 struct_name name in
    let in_struct = sprintf "(%s + i)->%s" in_struct_name in
    let in_struct_member = sprintf "%s" (in_struct name)  in
    let gen_tmp_var_for_out = 
        match param_direction with
            Ast.PtrInOut ->
                  [
                    sprintf "if (ADD_ASSIGN_OVERFLOW(_%s_malloc_size, sizeof(void*) + sizeof(size_t))) {" struct_name;
                    "\tstatus = SGX_ERROR_INVALID_PARAMETER;";
                    "\tgoto err;";
                    "}";
                  ]
            | _ -> []
    in
    let check_size =
        match ty with
        Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_)  | Ast.Ptr(Ast.Struct(_)) -> []
        | _ ->
            [
            sprintf "\tif (%s %% sizeof(*%s) != 0) {" len_var in_ptr_name; (* "size x count" is a multiple of sizeof type *)
            "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
            "\t\tgoto err;";
            "\t}";
            ]
    in
    let code_template =
            [
            gen_check_member_length ty attr declr in_struct "\t\t\t" ["\tstatus = SGX_ERROR_INVALID_PARAMETER;";"\tgoto err;"];
            sprintf "if (%s != NULL && (%s = %s) != 0) {" in_struct_member len_var (gen_struct_ptr_size ty attr name in_struct);
            sprintf "\tif (!sgx_is_outside_enclave(%s, %s)) {" in_struct_member len_var;
            "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
            "\t\tgoto err;";
            "\t}\n";
            ]
            @ check_size @
            [
            sprintf "\tif (ADD_ASSIGN_OVERFLOW(_%s_malloc_size, %s)) {" struct_name len_var;
            "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
            "\t\tgoto err;";
            "\t}";
            "}";
            ]
    in
    List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "" (gen_tmp_var_for_out @ code_template)

(* Generate the code to handle structure pointer deep copy,
 * which is to be inserted before actually calling the trusted function.
 *)
let gen_struct_ptr_direction_pre_copy (param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let in_struct_name = mk_in_var struct_name in
    let in_ptr_name = mk_in_var2 struct_name name in
    let len_var     = mk_len_var2 struct_name name in
    let in_ptr_dst_name = mk_in_ptr_dst_name ty attr in_ptr_name in
    let in_struct = sprintf "(%s + i)->%s" in_struct_name in
    let in_struct_member = sprintf "%s" (in_struct name)  in
    let code_template =
        let gen_tmp_var_for_out = 
            match param_direction with
                Ast.PtrInOut -> 
                    [
                    sprintf "*(%s*)__tmp_%s = %s;" (Ast.get_tystr ty) in_struct_name in_ptr_name ;
                    sprintf "__tmp_%s = (void *)((size_t)__tmp_%s + sizeof(void*));" in_struct_name in_struct_name;
                    sprintf "*(size_t*)__tmp_%s = %s;" in_struct_name len_var;
                    sprintf "__tmp_%s= (void *)((size_t)__tmp_%s + sizeof(size_t));" in_struct_name in_struct_name;
                    ]
                | _ -> []
        in
        let check_size =
                match ty with
                Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_)  | Ast.Ptr(Ast.Struct(_)) -> []
                | _ ->
                    [
                    sprintf "\tif (%s %% sizeof(*%s) != 0) {" len_var in_struct_member; (* "size x count" is a multiple of sizeof type *)
                    "\t\t\tstatus = SGX_ERROR_UNEXPECTED;";
                    "\t\t\tgoto err;";
                    "\t}";
                    ]
        in
        [
        sprintf "%s = %s;" in_ptr_name in_struct_member;
        sprintf "%s = %s;" len_var (gen_struct_ptr_size ty attr name in_struct);
        ] @ gen_tmp_var_for_out @
        [
        sprintf "if (%s != NULL && %s != 0) {" in_ptr_dst_name len_var;
        ] @ check_size @
        [
        sprintf "\tif (memcpy_s(__tmp_%s, %s, %s, %s)) {" in_struct_name len_var in_struct_member len_var;
        "\t\tstatus = SGX_ERROR_UNEXPECTED;";
        "\t\tgoto err;";
        sprintf "\t}";
        sprintf "\t%s = __tmp_%s;" in_struct_member in_struct_name;
        sprintf "\t__tmp_%s = (void *)((size_t)__tmp_%s + %s);" in_struct_name in_struct_name len_var ;
        "}";
        "else";
        sprintf "\t%s = NULL;" in_struct_member;
        ]
    in
    List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "" code_template

(* Generate the code to handle function pointer parameter direction,
 * which is to be inserted before actually calling the trusted function.
 *)
let gen_parm_ptr_direction_pre (plist: Ast.pdecl list) =
  let clone_in_ptr (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let is_ary      = (Ast.is_array declr || attr.Ast.pa_isary) in
    let in_ptr_name = mk_in_var name in
    let in_ptr_type = sprintf "%s%s" (Ast.get_tystr ty) (if is_ary then "*"  else "") in
    let len_var     = mk_len_var name in
    let in_ptr_dst_name = mk_in_ptr_dst_name ty attr in_ptr_name in
    let tmp_ptr_name= mk_tmp_var name in
    let malloc_and_copy pre_indent =
      let check_size =
          if attr.Ast.pa_isstr then [] else
          if attr.Ast.pa_iswstr then
                  [
                  sprintf "\tif (%s %% sizeof(wchar_t) != 0)" len_var;
                  "\t{";
                  "\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\tgoto err;";
                  "\t}";
                  ]
          else
              match ty with
                Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_) -> []
              | Ast.Ptr(Ast.Struct(struct_type)) ->
                    if is_structure_defined struct_type then
                      let (struct_def, deep_copy)= get_struct_def struct_type
                      in
                      if deep_copy then
                          [
                          sprintf "\tif ( %s %% sizeof(*%s) != 0)" len_var tmp_ptr_name;
                          "\t{";
                          "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
                          "\t\tgoto err;";
                          "\t}";
                          ]
                      else []
                    else []
              | _ -> 
                  [
                  sprintf "\tif ( %s %% sizeof(*%s) != 0)" len_var tmp_ptr_name;
                  "\t{";
                  "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
                  "\t\tgoto err;";
                  "\t}";
                  ]
      in
      match attr.Ast.pa_direction with
          Ast.PtrIn | Ast.PtrInOut ->
            let struct_deep_copy_pre = 
                let struct_calculate = 
                    (invoke_if_struct ty attr.Ast.pa_direction name 
                        (fun struct_type name ->
                            sprintf "\t\tfor (i = 0; i < %s / sizeof(struct %s); i++){\n"  (mk_len_var name) struct_type
                        )
                    gen_struct_ptr_direction_pre_calculate "\t\t}\n")
                in
                let struct_malloc =
                  let code_template = [
                      sprintf "\t__tmp_%s = malloc(_%s_malloc_size);"(mk_in_var name) name;
                      sprintf "\tif (__tmp_%s == NULL) {" (mk_in_var name);
                      "\t\tstatus = SGX_ERROR_OUT_OF_MEMORY;";
                      "\t\tgoto err;";
                      "\t}";
                      ]
                  in
                  List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                in
                let struct_copy =
                    (invoke_if_struct ty attr.Ast.pa_direction name 
                    (fun struct_type name ->
                        sprintf "\t\t_in_member_%s = __tmp_%s;\n\t\tfor (i = 0; i < %s / sizeof(struct %s); i++){\n"  name (mk_in_var name) (mk_len_var name) struct_type 
                    ) gen_struct_ptr_direction_pre_copy "\t\t}\n")
                in
                struct_calculate ^ (if struct_calculate = "" then "" else struct_malloc) ^ struct_copy
            in
            let code_template = 
              [
              sprintf "if (%s != NULL && %s != 0) {" tmp_ptr_name len_var;
              ]
              @ check_size @
              [
              sprintf "\t%s = (%s)malloc(%s);" in_ptr_name in_ptr_type len_var;
              sprintf "\tif (%s == NULL) {" in_ptr_name;
              "\t\tstatus = SGX_ERROR_OUT_OF_MEMORY;";
              "\t\tgoto err;";
              "\t}\n";
              sprintf "\tif (memcpy_s(%s, %s, %s, %s)) {" in_ptr_dst_name len_var tmp_ptr_name len_var;
              "\t\tstatus = SGX_ERROR_UNEXPECTED;";
              "\t\tgoto err;";
              sprintf "\t}\n%s" struct_deep_copy_pre;
              ]
            in
            let s1 = List.fold_left (fun acc s -> acc ^ pre_indent ^ s ^ "\n") "" code_template in
            let s2 =
              if attr.Ast.pa_isstr then
                let code_template2  = [
                  sprintf "\t%s[%s - 1] = '\\0';" in_ptr_name len_var;
                  sprintf "\tif (%s != strlen(%s) + 1)" len_var in_ptr_name;
                  "\t{";
                  "\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\tgoto err;";
                  "\t}";
                  ]
                in
                s1 ^ List.fold_left (fun acc s -> acc ^ pre_indent ^ s ^ "\n") "" code_template2
              else if attr.Ast.pa_iswstr then
                let code_template3  = [
                  sprintf "\t%s[(%s - sizeof(wchar_t))/sizeof(wchar_t)] = (wchar_t)0;" in_ptr_name len_var;
                  sprintf "\tif ( %s / sizeof(wchar_t) != wcslen(%s) + 1)" len_var in_ptr_name;
                  "\t{";
                  "\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\tgoto err;";
                  "\t}";
                  ]
                in
                s1 ^ List.fold_left (fun acc s -> acc ^ pre_indent ^ s ^ "\n") "" code_template3
              else s1 in
            sprintf "%s\t}\n" s2
        | Ast.PtrOut ->
            let code_template =
              [
              sprintf "if (%s != NULL && %s != 0) {" tmp_ptr_name len_var;
              ]
              @ check_size @
              [
              sprintf "\tif ((%s = (%s)malloc(%s)) == NULL) {" in_ptr_name in_ptr_type len_var;
              "\t\tstatus = SGX_ERROR_OUT_OF_MEMORY;";
              "\t\tgoto err;";
              "\t}\n";
              sprintf "\tmemset((void*)%s, 0, %s);" in_ptr_name len_var;
              "}"]
            in
              List.fold_left (fun acc s -> acc ^ pre_indent ^ s ^ "\n") "" code_template
        | _ -> ""
    in
      malloc_and_copy "\t"
  in List.fold_left
       (fun acc (pty, declr) ->
          match pty with
              Ast.PTVal _          -> acc
            | Ast.PTPtr (ty, attr) -> acc ^ clone_in_ptr ty attr declr) "" plist

(* Generate the code to handle tructure pointer deep copy,
 * which is to be inserted after finishing calling the trusted function.
 *)
let gen_struct_ptr_direction_post (param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string) (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let in_struct_name = mk_in_var struct_name in
    let in_ptr_name = mk_in_var2 struct_name name in
    let in_struct = sprintf "(%s + i)->%s" in_struct_name in
    let in_struct_member = sprintf "%s" (in_struct name)  in
    let in_len_ptr_var = mk_len_var2 struct_name name in
    let out_len_ptr_var = mk_len_var2 ("out_" ^ struct_name) name in
      match param_direction with
          Ast.PtrIn -> ""
        | Ast.PtrInOut ->
            let check_size =
                match ty with
                Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_)  | Ast.Ptr(Ast.Struct(_)) -> ""
                | _ -> sprintf " || (%s %% sizeof(*%s) != 0)" in_len_ptr_var in_struct_member (* "size x count" is a multiple of sizeof type *)
            in
            let code_template  = [
                sprintf "%s = *(%s*)__tmp_%s;" in_ptr_name (Ast.get_tystr ty) in_struct_name;
                sprintf "__tmp_%s = (void *)((size_t)__tmp_%s + sizeof(void*));" in_struct_name in_struct_name;
                sprintf "%s = *(size_t*)__tmp_%s;" in_len_ptr_var in_struct_name;
                sprintf "__tmp_%s = (void *)((size_t)__tmp_%s + sizeof(size_t));" in_struct_name in_struct_name;
                gen_check_member_length ty attr declr in_struct "\t\t\t" ["\tstatus = SGX_ERROR_INVALID_PARAMETER;";"\tbreak;"];
                sprintf "size_t %s = %s;" out_len_ptr_var  (gen_struct_ptr_size ty attr name in_struct);
                sprintf "if(%s!= NULL &&" in_struct_member;
                sprintf "\t\t%s != 0) {"  out_len_ptr_var;
                sprintf "\tif (%s > %s%s) {" out_len_ptr_var  in_len_ptr_var check_size;
                "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
                "\t\tbreak;";
                "\t}";
                sprintf "\tif (!sgx_is_within_enclave(%s, %s) ||" in_struct_member out_len_ptr_var;
                sprintf "\t\t\t!sgx_is_outside_enclave(%s, %s)) {" in_ptr_name in_len_ptr_var;
                "\t\tstatus = SGX_ERROR_INVALID_PARAMETER;";
                "\t\tbreak;";
                "\t}";
                sprintf "\tif (memcpy_verw_s(%s, %s, %s, %s)) {" in_ptr_name in_len_ptr_var in_struct_member out_len_ptr_var;
                sprintf "\t\tstatus = SGX_ERROR_UNEXPECTED;";
                "\t\tbreak;";
                "\t}";
                "}";
                sprintf "%s = %s;" in_struct_member in_ptr_name;
                sprintf "__tmp_%s = (void *)((size_t)__tmp_%s + %s);" in_struct_name in_struct_name in_len_ptr_var;
                ]
          in
          List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "" code_template
        | _ -> ""

(* Generate the code to handle function pointer parameter direction,
 * which is to be inserted after finishing calling the trusted function.
 *)
let gen_parm_ptr_direction_post (plist: Ast.pdecl list) =
  let copy_and_free (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let in_ptr_name = mk_in_var name in
    let len_var     = mk_len_var name in
    let struct_deep_copy_post = 
          let pre struct_type name =
            let code_template  = [
                sprintf "__tmp_%s = _in_member_%s;" (mk_in_var name) name;
                sprintf "for (i = 0; i < %s / sizeof(struct %s); i++){"  (mk_len_var name) struct_type;
                "\tif (status != SGX_SUCCESS)";
                "\t\tbreak;";
                ]
            in
            List.fold_left (fun acc s -> acc ^ "\t\t" ^ s ^ "\n") "" code_template
          in
          invoke_if_struct ty attr.Ast.pa_direction name pre gen_struct_ptr_direction_post "\t\t}\n"
      in
      match attr.Ast.pa_direction with
        Ast.PtrIn -> ""
        | Ast.PtrInOut | Ast.PtrOut ->
          if attr.Ast.pa_isstr then
                let code_template  = [
                  sprintf "\tif (%s)" in_ptr_name;
                  "\t{";
                  sprintf "\t\t%s[%s - 1] = '\\0';" in_ptr_name len_var;
                  sprintf "\t\t%s = strlen(%s) + 1;" len_var in_ptr_name;
                  sprintf "\t\tif (memcpy_verw_s((void*)%s, %s, %s, %s)) {" (mk_tmp_var name) len_var in_ptr_name len_var;
                  "\t\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\t\tgoto err;";
                  "\t\t}";
                  "\t}";
                  ]
                in
                List.fold_left (fun acc s -> acc ^ s ^ "\n") "" code_template
              else if attr.Ast.pa_iswstr then
                let code_template  = [ 
                  sprintf "\tif (%s)" in_ptr_name;
                  "\t{";
                  sprintf "\t\t%s[(%s - sizeof(wchar_t))/sizeof(wchar_t)] = (wchar_t)0;" in_ptr_name len_var;
                  sprintf "\t\t%s = (wcslen(%s) + 1) * sizeof(wchar_t);" len_var in_ptr_name;
                  sprintf "\t\tif (memcpy_verw_s((void*)%s, %s, %s, %s)) {" (mk_tmp_var name) len_var in_ptr_name len_var;
                  "\t\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\t\tgoto err;";
                  "\t\t}";
                  "\t}";
                  ]
                in  
                List.fold_left (fun acc s -> acc ^ s ^ "\n") "" code_template
            else
                let code_template  = [ 
                  sprintf "\tif (%s) {" in_ptr_name;
                  sprintf "%s\t\tif (memcpy_verw_s(%s, %s, %s, %s)) {" struct_deep_copy_post (mk_tmp_var name) len_var in_ptr_name len_var;
                  "\t\t\tstatus = SGX_ERROR_UNEXPECTED;";
                  "\t\t\tgoto err;";
                  "\t\t}";
                  "\t}";
                  ]
                in
                List.fold_left (fun acc s -> acc ^ s ^ "\n") "" code_template
        | _ -> ""
  in
  List.fold_left
      (fun acc (pty, declr) ->
          match pty with
              Ast.PTVal _          -> acc
            | Ast.PTPtr (ty, attr) -> acc ^ copy_and_free ty attr declr) "" plist


(* Generate an "err:" goto mark if necessary. *)
let gen_err_mark (fd: Ast.func_decl) =
  let has_inout_p (attr: Ast.ptr_attr): bool =
    attr.Ast.pa_direction <> Ast.PtrNoDirection
  in
    if fd.rtype <> Ast.Void || List.exists (fun (pt, name) ->
                      match pt with
                          Ast.PTVal _        -> false
                        | Ast.PTPtr(_, attr) -> has_inout_p attr) fd.plist
    then "err:"
    else ""

(* Generate the code to handle function pointer parameter direction,
 * which is to be inserted after finishing calling the trusted function.
 *)
let gen_parm_ptr_free_post (plist: Ast.pdecl list) =
  let copy_and_free (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let name        = declr.Ast.identifier in
    let in_ptr_name = mk_in_var name in
    let in_ptr_dst_name = mk_in_ptr_dst_name ty attr in_ptr_name in
      match attr.Ast.pa_direction with
        Ast.PtrIn ->
            let struct_free =
                match ty with
                  Ast.Ptr(Ast.Struct(struct_type)) ->
                        if is_structure_defined struct_type then
                          let (_, deep_copy)= get_struct_def struct_type
                          in
                          if deep_copy then
                             sprintf "\tif (_in_member_%s) free(_in_member_%s);\n" name name
                          else ""
                        else ""
                  | _ -> ""
            in
            sprintf "\tif (%s) free(%s);\n%s" in_ptr_name in_ptr_dst_name struct_free
        | Ast.PtrInOut | Ast.PtrOut ->
                  sprintf "\tif (%s) free(%s);\n" in_ptr_name in_ptr_name
        | _ -> ""
  in
  List.fold_left
      (fun acc (pty, declr) ->
          match pty with
              Ast.PTVal _          -> acc
            | Ast.PTPtr (ty, attr) -> acc ^ copy_and_free ty attr declr) "" plist

(* It is used to save the parameters used as the value of size/count attribute. *)
let param_cache = Hashtbl.create 1
let is_in_param_cache s = Hashtbl.mem param_cache s

(* Try to generate a temporary value to save the size of the buffer. *)
let gen_tmp_size (pattr: Ast.ptr_attr) (plist: Ast.pdecl list) =
  let do_gen_temp_var (s: string) =
    if is_in_param_cache s then ""
    else
      let param_tystr = find_param_type s plist in
      let tmp_var     = mk_tmp_var s in
      let parm_str    = mk_in_parm_accessor s in
        Hashtbl.add param_cache s true;
        sprintf "\t%s %s = %s;\n" param_tystr tmp_var parm_str
  in
  let gen_temp_var (v: Ast.attr_value) =
    match v with
        Ast.ANumber _ -> ""
      | Ast.AString s -> do_gen_temp_var s
  in
  let tmp_size_str =
    match pattr.Ast.pa_size.Ast.ps_size with
        Some v -> gen_temp_var v
      | None   -> ""
  in
  let tmp_count_str =
    match pattr.Ast.pa_size.Ast.ps_count with
        Some v -> gen_temp_var v
      | None   -> ""
  in
    sprintf "%s%s" tmp_size_str tmp_count_str

let is_ptr (pt: Ast.parameter_type) =
  match pt with
      Ast.PTVal _ -> false
    | Ast.PTPtr _ -> true

let is_ptr_type (aty: Ast.atype) =
  match aty with
    Ast.Ptr _ -> true
  | _         -> false

let ptr_has_direction (pt: Ast.parameter_type) =
  match pt with
      Ast.PTVal _     -> false
    | Ast.PTPtr(_, a) -> a.Ast.pa_direction <> Ast.PtrNoDirection

let tbridge_mk_parm_name_ext (pt: Ast.parameter_type) (declr: Ast.declarator) =
  let cast_expr =
    let tystr = get_param_tystr pt in
    if Ast.is_array declr && List.length declr.Ast.array_dims > 1
    then
      let dims = get_array_dims (List.tl declr.Ast.array_dims) in
        sprintf "(%s (*)%s)" tystr dims
    else if is_const_ptr pt then
      sprintf "(const %s)" tystr
    else ""
  in
  if is_in_param_cache declr.Ast.identifier || (is_ptr pt && (not (is_foreign_array pt)))
  then
    if ptr_has_direction pt
    then cast_expr ^ mk_in_var declr.Ast.identifier
    else cast_expr ^ mk_tmp_var declr.Ast.identifier
  else mk_parm_name_ext pt declr

let mk_parm_name_tbridge (pt: Ast.parameter_type) (declr: Ast.declarator) =
  add_foreign_array_ptrref (tbridge_mk_parm_name_ext pt declr) pt

(* Generate local variables required for the trusted bridge. *)
let gen_tbridge_local_vars (plist: Ast.pdecl list) =
  let status_var = "\tsgx_status_t status = SGX_SUCCESS;\n" in
  let do_gen_local_var (pt: Ast.parameter_type) (attr: Ast.ptr_attr) (name: string) =
    let qual = if is_const_ptr pt then "const " else "" in
    let ty = Ast.get_param_atype pt in
    let tmp_var =
      (* Save a copy of pointer in case it might be modified in the marshaling structure. *)
      sprintf "\t%s%s %s = %s;\n" qual (Ast.get_tystr ty) (mk_tmp_var name) (mk_in_parm_accessor name)
    in
    let len_var =
      if not attr.Ast.pa_chkptr then ""
      else gen_tmp_size attr plist ^ "\t" ^ gen_ptr_size ty attr name mk_tmp_var in
    let in_ptr =
      match attr.Ast.pa_direction with
          Ast.PtrNoDirection -> ""
        | _ -> sprintf "\t%s %s = NULL;\n" (Ast.get_tystr ty) (mk_in_var name)
    in
    let in_ptr_struct_var =
      if not attr.Ast.pa_chkptr then ""
      else
       let gen_struct_local_var (param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string) (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator)=
           let in_ptr_name = mk_in_var2 struct_name declr.Ast.identifier in
           let in_len_ptr_name = mk_len_var2 struct_name declr.Ast.identifier in
           sprintf "\t%s %s = NULL;\n\tsize_t %s = 0;\n" (Ast.get_tystr ty) in_ptr_name in_len_ptr_name 
       in
       invoke_if_struct ty attr.Ast.pa_direction name  (fun _ name -> sprintf "\tvoid* __tmp_%s = NULL;\n\tsize_t _%s_malloc_size = 0;\n\tvoid* _in_member_%s = NULL;\n" (mk_in_var name) name name) gen_struct_local_var ""
    in
      (tmp_var ^ len_var ^ in_ptr ^ in_ptr_struct_var, if in_ptr_struct_var <> "" then true else false)
  in
  let gen_local_var_for_foreign_array (ty: Ast.atype) (attr: Ast.ptr_attr) (name: string) =
    let tystr = Ast.get_tystr ty in
    let tmp_var =
      sprintf "\t%s* %s = %s;\n" tystr (mk_tmp_var name) (mk_in_parm_accessor name)
    in
    let len_var = sprintf "\tsize_t %s = sizeof(%s);\n" (mk_len_var name) tystr
    in
    let in_ptr = sprintf "\t%s* %s = NULL;\n" tystr (mk_in_var name)
    in
      match attr.Ast.pa_direction with
        Ast.PtrNoDirection -> ""
      | _ -> tmp_var ^ len_var ^ in_ptr
  in
  let gen_local_var (pd: Ast.pdecl) =
    let (pty, declr) = pd in
      match pty with
          Ast.PTVal _          -> ("", false)
        | Ast.PTPtr (ty, attr) ->
            if is_foreign_array pty
            then (gen_local_var_for_foreign_array ty attr declr.Ast.identifier, false)
            else do_gen_local_var pty attr declr.Ast.identifier
  in
  let new_param_list = List.map conv_array_to_ptr plist
  in
  let (str, deep_copy) =
    Hashtbl.clear param_cache;
    List.fold_left (fun acc pd -> let (str, deep_copy) =  (gen_local_var pd) in (fst acc ^ str, snd acc || deep_copy)) (status_var, false) new_param_list
  in
    str ^ if deep_copy then "\tsize_t i = 0;\n" else ""

(* It generates trusted bridge code for a trusted function. *)
let gen_func_tbridge (fd: Ast.func_decl) (dummy_var: string) =
  let func_open = sprintf "static sgx_status_t SGX_CDECL %s(void* %s)\n{\n"
                          (mk_tbridge_name fd.Ast.fname)
                          ms_ptr_name in
  let local_vars = gen_tbridge_local_vars fd.Ast.plist ^
                   if fd.rtype <> Ast.Void
                     then sprintf "\t%s %s;\n" (Ast.get_tystr fd.rtype) (mk_in_var retval_name)
                     else "" in
  let func_close = "\treturn status;\n}\n" in

  let ms_struct_name = mk_ms_struct_name fd.Ast.fname in
  let declare_ms_ptr = sprintf "%s* %s = SGX_CAST(%s*, %s);"
                               ms_struct_name
                               ms_struct_val
                               ms_struct_name
                               ms_ptr_name in
  let declare_ms = sprintf "%s %s;"
                               ms_struct_name
                               ms_in_struct_val in
  let copy_ms =
    let code_template =[
                     sprintf "if (memcpy_s(&%s, sizeof(%s), %s, sizeof(%s))) {"
                        ms_in_struct_val
                        ms_struct_name
                        ms_struct_val
                        ms_struct_name;
                     "\treturn SGX_ERROR_UNEXPECTED;";
                     "}";
                    ]
    in 
    List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template in

  let invoke_func   = gen_func_invoking fd mk_parm_name_tbridge in
  
  let update_retval =
    let code_template =[
                     sprintf "%s = %s"(mk_in_var retval_name) invoke_func;
                     sprintf "if (memcpy_verw_s(&%s, sizeof(%s), &%s, sizeof(%s))) {"
                        (mk_parm_accessor retval_name)
                        (mk_parm_accessor retval_name)
                        (mk_in_var retval_name)
                        (mk_in_var retval_name);
                     "\tstatus = SGX_ERROR_UNEXPECTED;";
                     "\tgoto err;";
                     "}";
                    ]
    in 
    List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template in

    if is_naked_func fd then
      let check_pms =
        sprintf "if (%s != NULL) return SGX_ERROR_INVALID_PARAMETER;" ms_ptr_name
      in
        sprintf "%s%s%s\t%s\n\t%s\n%s" func_open local_vars dummy_var check_pms invoke_func func_close
    else
      sprintf "%s%s\t%s\n\t%s\n%s%s\n%s%s\n%s%s%s\n%s\n%s%s"
        func_open
        (mk_check_pms fd.Ast.fname)
        declare_ms_ptr
        declare_ms
        copy_ms
        local_vars
        (gen_check_tbridge_length_overflow fd.Ast.plist)
        (gen_check_tbridge_ptr_parms fd.Ast.plist)
        (gen_parm_ptr_direction_pre fd.Ast.plist)
        (if fd.rtype <> Ast.Void then update_retval else sprintf "\t%s\n" invoke_func)
        (gen_parm_ptr_direction_post fd.Ast.plist)
        (gen_err_mark fd)
        (gen_parm_ptr_free_post fd.Ast.plist)
        func_close

let tproxy_fill_ms_field (pd: Ast.pdecl) (is_ocall_switchless: bool) =
  let (pt, declr)   = pd in
  let name          = declr.Ast.identifier in
  let len_var       = mk_len_var name in
  let parm_accessor = mk_parm_accessor name in
  let sgx_ocfree_fn = get_sgx_fname SGX_OCFREE is_ocall_switchless in
  let copy_ms_val_filed = [
    sprintf "\tif (memcpy_verw_s(&%s, sizeof(%s), &%s, sizeof(%s))) {"
      parm_accessor
      parm_accessor
      name
      name;
    sprintf "\t\t%s();" sgx_ocfree_fn;
    "\t\treturn SGX_ERROR_UNEXPECTED;";
    "\t}";
    ] in
    match pt with
        Ast.PTVal _ -> List.fold_left (fun acc s -> acc ^ s ^ "\n") "" copy_ms_val_filed
      | Ast.PTPtr(ty, attr) ->
              let is_ary = (Ast.is_array declr || attr.Ast.pa_isary) in
              let tystr = sprintf "%s%s%s" (if is_const_ptr pt then "const " else"")(get_param_tystr pt) (if is_ary then "*" else "") in
              if not attr.Ast.pa_chkptr then (* [user_check] specified *) 
                List.fold_left (fun acc s -> acc ^ s ^ "\n") "" copy_ms_val_filed
              else
                let check_size =
                    match ty with
                    Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_)  -> []
                   | Ast.Ptr(Ast.Struct(struct_type)) ->
                            if is_structure_defined struct_type then
                              let (_, deep_copy)= get_struct_def struct_type
                              in
                              if deep_copy then
                                [
                                   sprintf "\tif (%s %% sizeof(*%s) != 0) {" len_var name;
                                   sprintf "\t\t%s();" sgx_ocfree_fn;
                                   "\t\treturn SGX_ERROR_INVALID_PARAMETER;";
                                   "\t}";
                                ]
                                else []
                            else []
                   | _ ->
                    [
                       sprintf "\tif (%s %% sizeof(*%s) != 0) {" len_var name;
                       sprintf "\t\t%s();" sgx_ocfree_fn;
                       "\t\treturn SGX_ERROR_INVALID_PARAMETER;";
                       "\t}";
                    ]
                in
                let copy_out =
                    let deep_copy_out =
                        let pre struct_type name =
                          let code_template =[
                                           sprintf "for (i = 0; i < %s / sizeof(struct %s); i++){"  (mk_len_var name) struct_type;
                                           sprintf "\t\tif (memcpy_s(&__local_%s, sizeof(__local_%s), %s + i, sizeof(struct %s))) {" name name name struct_type;
                                           sprintf "\t\t\t%s();" sgx_ocfree_fn;
                                           "\t\t\treturn SGX_ERROR_UNEXPECTED;";
                                           "\t\t}";
                                          ]
                          in
                          List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                        in
                        let clear_member(param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
                            let member_name        = declr.Ast.identifier in
                            sprintf "\t\t\t__local_%s.%s = NULL;\n" struct_name member_name
                        in
                        let post =
                            let code_template =[
                                             sprintf "\tif (memcpy_verw_s((void *)((size_t)__tmp + sizeof(__local_%s) * i), sizeof(__local_%s), &__local_%s, sizeof(__local_%s))) {" name name name name;
                                             sprintf "\t\t%s();" sgx_ocfree_fn;
                                             "\t\treturn SGX_ERROR_UNEXPECTED;";
                                             "\t}";
                                             "}";
                                             sprintf "memset(&__local_%s, 0, sizeof(__local_%s));" name name;
                                            ]
                            in
                            List.fold_left (fun acc s -> acc ^ "\t\t" ^ s ^ "\n") "" code_template
                        in
                        invoke_if_struct ty attr.Ast.pa_direction name  pre clear_member post
                    in
                    let non_deep_copy_out =
                        let code_template =
                          [
                           sprintf "if (memcpy_verw_s(__tmp, ocalloc_size, %s, %s)) {"  name len_var;
                           sprintf "\t\t%s();" sgx_ocfree_fn;
                           "\t\treturn SGX_ERROR_UNEXPECTED;";
                           "\t}";
                          ]
                        in List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                    in
                    if deep_copy_out = "" then non_deep_copy_out else deep_copy_out
                in
                let assign_tmp_to_ptr = [
                    sprintf "\tif (memcpy_verw_s(&%s, sizeof(%s), &__tmp, sizeof(%s))) {"
                      parm_accessor
                      tystr
                      tystr;
                    sprintf "\t\t%s();" sgx_ocfree_fn;
                    "\t\treturn SGX_ERROR_UNEXPECTED;";
                    "\t}";
                    ]
                in
                match attr.Ast.pa_direction with
                  Ast.PtrOut ->
                    let code_template =
                      [sprintf "if (%s != NULL) {" name;]
                      @ assign_tmp_to_ptr @ 
                      [
                       sprintf "\t__tmp_%s = __tmp;" name;
                      ]
                      @ check_size @
                      [
                       sprintf "\tmemset_verw(__tmp_%s, 0, %s);" name len_var;
                       sprintf "\t__tmp = (void *)((size_t)__tmp + %s);" len_var;
                       sprintf "\tocalloc_size -= %s;" len_var;
                       "} else {";
                       sprintf "\t%s = NULL;" parm_accessor;
                       "}"
                      ]
                    in List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                | Ast.PtrInOut ->
                    let code_template =
                      [sprintf "if (%s != NULL) {" name;]
                      @ assign_tmp_to_ptr @ 
                      [
                       sprintf "\t__tmp_%s = __tmp;" name;
                      ]
                      @ check_size @
                      [
                       sprintf "%s\t\t__tmp = (void *)((size_t)__tmp + %s);"copy_out len_var;
                       sprintf "\tocalloc_size -= %s;" len_var;
                       "} else {";
                       sprintf "\t%s = NULL;" parm_accessor;
                       "}"
                      ]
                    in List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                | _ ->
                    let code_template =
                      [sprintf "if (%s != NULL) {" name;
                      ]
                      @ assign_tmp_to_ptr @ check_size @
                      [
                       sprintf "%s\t\t__tmp = (void *)((size_t)__tmp + %s);" copy_out len_var;
                       sprintf "\tocalloc_size -= %s;" len_var;
                       "} else {";
                       sprintf "\t%s = NULL;" parm_accessor;
                       "}"
                      ]
                    in List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template

(* Attach data pointed by structure member pointer at the end of ms. *)
let tproxy_fill_structure(pd: Ast.pdecl) (is_ocall_switchless: bool)=
  let (pt, declr)   = pd in
  let name          = declr.Ast.identifier in
  let parm_accessor = mk_parm_accessor name in
  let sgx_ocfree_fn = get_sgx_fname SGX_OCFREE is_ocall_switchless in
  let fill_structure(param_direction: Ast.ptr_direction) (struct_type: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
    let member_name        = declr.Ast.identifier in
    let len_member_name = mk_len_var2 struct_name member_name in
    let in_struct = sprintf "(%s + i)->%s" parm_accessor in
    let in_struct_member = sprintf "%s" (in_struct member_name)  in
    let para_struct = sprintf "(%s + i)->%s" name in
    let para_struct_member = sprintf "%s" (para_struct member_name)  in
                match param_direction with
                | Ast.PtrInOut | Ast.PtrIn  ->
                    let code_template =
                      [
                       sprintf "%s = %s;"  len_member_name (gen_struct_ptr_size ty attr name para_struct);
                       sprintf "\tif (%s != NULL && %s != 0) {" para_struct_member len_member_name;
                       sprintf "\t\tif (memcpy_verw_s(__tmp, %s, %s, %s) ||" len_member_name para_struct_member len_member_name;
                       sprintf "\t\t\tmemcpy_verw_s(&%s, sizeof(%s), &__tmp, sizeof(%s))) {" in_struct_member (Ast.get_tystr ty) (Ast.get_tystr ty);
                       sprintf "\t\t\t%s();" sgx_ocfree_fn;
                       "\t\t\treturn SGX_ERROR_UNEXPECTED;";
                       "\t\t}";
                       sprintf "\t\t__tmp = (void *)((size_t)__tmp + %s);" len_member_name;
                       sprintf "\t\tocalloc_size -= %s;" len_member_name;
                       "\t} else {";
                       sprintf "\t\t%s = NULL;" in_struct_member;
                       "\t}"
                      ]
                    in List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "" code_template
                | _ -> ""
    in
        match pt with
              Ast.PTPtr(ty, attr) ->
                let pre struct_type name = 
                  let code_template =
                    if attr.Ast.pa_direction = Ast.PtrInOut then
                                  [sprintf "if (%s != NULL && %s != 0 ) {" name (mk_len_var name);
                                   sprintf "\t__tmp_member_%s = __tmp;" name;
                                   sprintf "\tfor (i = 0; i < %s / sizeof(struct %s); i++){"  (mk_len_var name) struct_type
                                  ]
                    else
                                  [sprintf "if (%s != NULL && %s != 0 ) {" name (mk_len_var name);
                                   sprintf "\tfor (i = 0; i < %s / sizeof(struct %s); i++){"  (mk_len_var name) struct_type
                                  ]
                  in 
                  List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
                  in
                  invoke_if_struct ty attr.Ast.pa_direction name  pre fill_structure "\t\t}\n\t}\n"
              | _ -> ""

(* Generate local variables required for the trusted proxy, inclidng variables required by structure deep copy. *)
let gen_tproxy_local_vars (plist: Ast.pdecl list) =
  let status_var = "\tsgx_status_t status = SGX_SUCCESS;\n" in
  let do_gen_local_vars (ty: Ast.atype) (attr: Ast.ptr_attr) (name: string) =
    let do_gen_local_var =
      if not attr.Ast.pa_chkptr then ""
      else "\t" ^ gen_ptr_size ty attr name (fun x -> x)
    in
    let in_ptr_struct_var =
      if not attr.Ast.pa_chkptr then ""
      else
         let gen_in_ptr_struct_var (_: Ast.ptr_direction) (_: string) (struct_name: string) (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
           let member_name = declr.Ast.identifier in
           let in_len_ptr_name = mk_len_var2 struct_name member_name in
           sprintf "\tsize_t %s = 0;\n" in_len_ptr_name 
         in
         invoke_if_struct ty attr.Ast.pa_direction name  (fun struct_type name ->
            sprintf "\tstruct %s __local_%s;\n" struct_type name ^
            if attr.Ast.pa_direction = Ast.PtrInOut then
                sprintf"\tvoid* __tmp_member_%s = NULL;\n" name else
                ""
         ) gen_in_ptr_struct_var ""
    in 
    (do_gen_local_var ^ in_ptr_struct_var, if in_ptr_struct_var <> "" then true else false)
  in
  let gen_local_var (pd: Ast.pdecl) =
    let (pty, declr) = pd in
      match pty with
          Ast.PTVal _          -> ("", false)
        | Ast.PTPtr (ty, attr) -> do_gen_local_vars ty attr declr.Ast.identifier
  in
  let new_param_list = List.map conv_array_to_ptr plist
  in
  let (str, deep_copy) =
    List.fold_left (fun acc pd -> let (str, deep_copy) =  (gen_local_var pd) in (fst acc ^ str, snd acc || deep_copy)) (status_var, false) new_param_list
  in
    str ^ if deep_copy then "\tsize_t i = 0;\n" else ""
    

(* Generate only one ocalloc block required for the trusted proxy. *)
let gen_ocalloc_block (fname: string) (plist: Ast.pdecl list) (is_switchless: bool) =
  let ms_struct_name = mk_ms_struct_name fname in
  let new_param_list = List.map conv_array_to_ptr plist in
  let local_vars_block = sprintf "\t%s* %s = NULL;\n\tsize_t ocalloc_size = sizeof(%s);\n\tvoid *__tmp = NULL;\n\n" ms_struct_name ms_struct_val ms_struct_name in
  let local_var (ty: Ast.atype) (attr: Ast.ptr_attr) (name: string) =
    if not attr.Ast.pa_chkptr then ""
    else
      match attr.Ast.pa_direction with
        Ast.PtrOut | Ast.PtrInOut -> sprintf "\tvoid *__tmp_%s = NULL;\n" name
      | _ -> ""
  in
  let do_local_var (pd: Ast.pdecl) =
    let (pty, declr) = pd in
      match pty with
        Ast.PTVal _         -> ""
      | Ast.PTPtr (ty, attr) -> local_var ty attr declr.Ast.identifier
  in
  let mk_check_enclave_ptr (name: string) (lenvar: string) =
    let checker = "CHECK_ENCLAVE_POINTER" in
    sprintf "%s(%s, %s)" checker name lenvar
  in
  let check_enclave_ptr_block =
    let check_enclave_ptr (pd: Ast.pdecl) =
      let (pty, declr) = pd in
        match pty with
          Ast.PTVal _          -> ""
        | Ast.PTPtr (ty, attr) -> 
           if not attr.Ast.pa_chkptr then ""
           else "\t" ^ mk_check_enclave_ptr declr.Ast.identifier (mk_len_var declr.Ast.identifier) ^ ";\n"
    in 
    let do_check_enclave_ptr = List.fold_left (fun acc pd -> acc ^ check_enclave_ptr pd) "" new_param_list in
    let break_line = if do_check_enclave_ptr = "" then "" else "\n" in
    break_line ^ do_check_enclave_ptr ^ break_line
  in

  let count_ocalloc_size (ty: Ast.atype) (attr: Ast.ptr_attr) (name: string) =
    if not attr.Ast.pa_chkptr then ""
    else sprintf "\tif (ADD_ASSIGN_OVERFLOW(ocalloc_size, (%s != NULL) ? %s : 0))\n\t\treturn SGX_ERROR_INVALID_PARAMETER;\n" name (mk_len_var name)
  in
  let do_count_ocalloc_size (pd: Ast.pdecl) =
    let (pty, declr) = pd in
      match pty with
        Ast.PTVal _          -> ""
      | Ast.PTPtr (ty, attr) -> count_ocalloc_size ty attr declr.Ast.identifier
  in
  let sgx_ocalloc_fn = get_sgx_fname SGX_OCALLOC is_switchless in
  let sgx_ocfree_fn = get_sgx_fname SGX_OCFREE is_switchless in
  let do_gen_ocalloc_block = [
      sprintf "\n\t__tmp = %s(ocalloc_size);\n" sgx_ocalloc_fn;
      "\tif (__tmp == NULL) {\n";
      sprintf "\t\t%s();\n" sgx_ocfree_fn;
      "\t\treturn SGX_ERROR_UNEXPECTED;\n";
      "\t}\n";
      sprintf "\t%s = (%s*)__tmp;\n" ms_struct_val ms_struct_name;
      sprintf "\t__tmp = (void *)((size_t)__tmp + sizeof(%s));\n" ms_struct_name;
      sprintf "\tocalloc_size -= sizeof(%s);\n" ms_struct_name;
      ]
  in
  let s1 = List.fold_left (fun acc pd -> acc ^ do_local_var pd) local_vars_block new_param_list in
  let s2 = List.fold_left (fun acc pd -> acc ^ do_count_ocalloc_size pd) (s1 ^ check_enclave_ptr_block) new_param_list in
     List.fold_left (fun acc s -> acc ^ s) s2 do_gen_ocalloc_block

(* Generate only one ocalloc block required for the trusted proxy. *)
let gen_ocalloc_block_struct_deep_copy (fname: string) (plist: Ast.pdecl list) (is_ocall_switchless: bool)=
  let new_param_list = List.map conv_array_to_ptr plist in
  let sgx_ocalloc_fn = get_sgx_fname SGX_OCALLOC is_ocall_switchless in
  let sgx_ocfree_fn = get_sgx_fname SGX_OCFREE is_ocall_switchless in
  let count_ocalloc_size (ty: Ast.atype) (attr: Ast.ptr_attr) (name: string) =
    if not attr.Ast.pa_chkptr then ""
    else 
      let count_struct_ocalloc_size =  
         let gen_member_size (_: Ast.ptr_direction) (_: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
           let in_len_ptr_var = mk_len_var2 struct_name declr.Ast.identifier in
           let para_struct = sprintf "(%s + i)->%s"  struct_name in
           let check_size =
                match ty with
                Ast.Ptr(Ast.Void) | Ast.Ptr(Ast.Foreign(_)) | Ast.Foreign(_)  | Ast.Ptr(Ast.Struct(_)) -> []
                | _ ->
                    [
                       sprintf "if (%s %% sizeof(*%s) != 0) {" in_len_ptr_var (para_struct declr.Ast.identifier); (* "size x count" is a multiple of sizeof type *)
                       sprintf "\t%s();" sgx_ocfree_fn;
                       "\treturn SGX_ERROR_INVALID_PARAMETER;";
                       "}";
                    ]
           in
           let code_template = 
               [
               gen_check_member_length ty attr declr para_struct "\t\t\t" [(sprintf "\t%s();" sgx_ocfree_fn);"\treturn SGX_ERROR_INVALID_PARAMETER;"];
               sprintf "%s = %s;" in_len_ptr_var (gen_struct_ptr_size ty attr struct_name para_struct);
               ]
               @ check_size @
               [
               sprintf "if (%s && ! sgx_is_within_enclave(%s, %s)) {" (para_struct declr.Ast.identifier)  (para_struct declr.Ast.identifier) in_len_ptr_var;
               sprintf "\t%s();" sgx_ocfree_fn;
               "\treturn SGX_ERROR_INVALID_PARAMETER;";
               "}";
               sprintf "\tif (ADD_ASSIGN_OVERFLOW(ocalloc_size, (%s != NULL) ? %s : 0)) {" (para_struct declr.Ast.identifier) in_len_ptr_var;
               sprintf "\t%s();" sgx_ocfree_fn;
               "\treturn SGX_ERROR_INVALID_PARAMETER;";
               "}";
               ]
           in
           List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "" code_template
         in
         invoke_if_struct ty attr.Ast.pa_direction name (fun struct_type name -> sprintf "\t\tfor (i = 0; i < %s / sizeof(struct %s); i++){\n"  (mk_len_var name) struct_type) gen_member_size "\t\t}\n"
      in
      if count_struct_ocalloc_size <> "" then
                  sprintf "\tif (%s != NULL && %s != 0){\n" name (mk_len_var name) ^
                  sprintf "%s" count_struct_ocalloc_size ^
                  "\t}\n"
      else ""
  in
  let do_count_ocalloc_size (pd: Ast.pdecl) =
    let (pty, declr) = pd in
      match pty with
        Ast.PTVal _          -> ""
      | Ast.PTPtr (ty, attr) -> count_ocalloc_size ty attr declr.Ast.identifier
  in
  let do_gen_ocalloc_block = [
      sprintf "\n\t__tmp = %s(ocalloc_size);\n" sgx_ocalloc_fn;
      "\tif (__tmp == NULL) {\n";
      sprintf "\t\t%s();\n" sgx_ocfree_fn;
      "\t\treturn SGX_ERROR_UNEXPECTED;\n";
      "\t}\n";
      ]
  in
  let s2 = List.fold_left (fun acc pd -> acc ^ do_count_ocalloc_size pd) "" new_param_list in
  if s2 = "" then ""
  else
     List.fold_left (fun acc s -> acc ^ s) s2 do_gen_ocalloc_block

(* Generate trusted proxy code for a given untrusted function. *)
let gen_func_tproxy (ufunc: Ast.untrusted_func) (idx: int) =
  let fd = ufunc.Ast.uf_fdecl in
  let propagate_errno = ufunc.Ast.uf_propagate_errno in
  let func_open = sprintf "%s\n{\n" (gen_tproxy_proto fd) in
  let local_vars = gen_tproxy_local_vars fd.Ast.plist in
  let ocalloc_ms_struct = gen_ocalloc_block fd.Ast.fname fd.Ast.plist ufunc.Ast.uf_is_switchless in
  let ocalloc_struct_deep_copy = gen_ocalloc_block_struct_deep_copy fd.Ast.fname fd.Ast.plist in
  let sgx_ocfree_fn = get_sgx_fname SGX_OCFREE ufunc.Ast.uf_is_switchless in
  let gen_ocfree rtype plist =
    if rtype = Ast.Void && plist = [] && propagate_errno = false then "" else sprintf "\t%s();\n" sgx_ocfree_fn
  in
  let handle_out_ptr plist =
    let copy_memory (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) =
      let name = declr.Ast.identifier in
        match attr.Ast.pa_direction with
            Ast.PtrInOut | Ast.PtrOut ->
              if attr.Ast.pa_isstr then
                let code_template  = [
                  sprintf "\tif (%s) {" name;
                  sprintf "\t\tsize_t __tmp%s;" (mk_len_var name);
                  sprintf "\t\tif (memcpy_s((void*)%s, %s, __tmp_%s, %s)) {"  name (mk_len_var name) name (mk_len_var name);
                  sprintf "\t\t\t%s();" sgx_ocfree_fn;
                  "\t\t\treturn SGX_ERROR_UNEXPECTED;";
                  "\t\t}";
                  sprintf "\t\t((char*)%s)[%s - 1] = '\\0';" name (mk_len_var name);
                  sprintf "\t\t__tmp%s = strlen(%s) + 1;" (mk_len_var name) name;
                  sprintf "\t\tmemset(%s +__tmp%s - 1, 0, %s -__tmp%s);" name (mk_len_var name) (mk_len_var name) (mk_len_var name);
                  "\t}";
                  ]
                in
                List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
              else if attr.Ast.pa_iswstr then
                let code_template  = [ 
                  sprintf "\tif (%s) {" name;
                  sprintf "\t\tsize_t __tmp%s;" (mk_len_var name);
                  sprintf "\t\tif (memcpy_s((void*)%s, %s, __tmp_%s, %s)) {"  name (mk_len_var name) name (mk_len_var name);
                  sprintf "\t\t\t%s();" sgx_ocfree_fn;
                  "\t\t\treturn SGX_ERROR_UNEXPECTED;";
                  "\t\t}";
                  sprintf "\t\t((wchar_t*)%s)[(%s - sizeof(wchar_t))/sizeof(wchar_t)] = (wchar_t)0;" name (mk_len_var name);
                  sprintf "\t\t__tmp%s = (wcslen(%s) + 1) * sizeof(wchar_t);" (mk_len_var name) name;
                  sprintf "\t\tmemset(((uint8_t*)%s) + __tmp%s - sizeof(wchar_t), 0, %s -__tmp%s);" name (mk_len_var name) (mk_len_var name) (mk_len_var name);
                  "\t}";
                  ]
                in  
                List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template
              else
                let struct_deep_copy_post =
                    let copy_and_free_struct_member  (_: Ast.ptr_direction) (_: string) (struct_name: string)  (ty: Ast.atype) (attr: Ast.ptr_attr) (declr: Ast.declarator) = 
                        let name        = declr.Ast.identifier in
                        let para_struct = sprintf "(%s + i)->%s" struct_name in
                        let para_struct_member = sprintf "%s" (para_struct name)  in
                        let local_struct = sprintf "__local_%s.%s" struct_name in
                        let in_len_ptr_var = mk_len_var2 struct_name name in
                        let out_len_ptr_var = mk_len_var2 ("out_" ^ struct_name) name in
                        let check_size =
                            match ty with
                            Ast.Ptr(Ast.Void) ->  ""
                            | _ -> sprintf " || (%s %% sizeof(*%s) != 0)" out_len_ptr_var para_struct_member (* "size x count" is a multiple of sizeof type *)
                        in
                        let code_template  = [
                            sprintf "size_t %s = 0;" out_len_ptr_var;
                            gen_check_member_length ty attr declr local_struct "\t\t\t\t" [(sprintf "\t%s();" sgx_ocfree_fn);"\treturn SGX_ERROR_INVALID_PARAMETER;"];
                            sprintf "%s = %s;"  in_len_ptr_var (gen_struct_ptr_size ty attr name para_struct);
                            sprintf "if(%s!= NULL &&" para_struct_member;
                            sprintf "\t\t(%s = %s) != 0) {"  out_len_ptr_var (gen_struct_ptr_size ty attr name local_struct);
                            sprintf "\tif (%s != __tmp_member_%s ||" (local_struct name) struct_name;(*pointer is not changed by untrusted code *)
                            sprintf "\t\t\t%s > %s%s) {"  out_len_ptr_var in_len_ptr_var check_size;
                            sprintf "\t\t%s();" sgx_ocfree_fn;
                            "\t\treturn SGX_ERROR_INVALID_PARAMETER;";
                            "\t}";
                            sprintf "\tif (memcpy_s(%s, %s, __tmp_member_%s, %s)) {" para_struct_member in_len_ptr_var struct_name out_len_ptr_var;
                            sprintf "\t\t%s();" sgx_ocfree_fn;
                            "\t\treturn SGX_ERROR_UNEXPECTED;";
                            "\t}";
                            "}";
                            sprintf "%s = %s;" (local_struct name) para_struct_member;
                            sprintf "__tmp_member_%s = (void *)((size_t)__tmp_member_%s + (%s != NULL? %s : 0));" struct_name struct_name para_struct_member in_len_ptr_var;

                            ]
                        in
                        List.fold_left (fun acc s -> acc ^ "\t\t\t\t" ^ s ^ "\n") "" code_template
                    in
                    let pre (struct_type: string) (name: string) =
                        let code_template2 = [
                            sprintf "for (i = 0; i < %s / sizeof(struct %s); i++){" (mk_len_var name) struct_type;
                            sprintf "\tif (memcpy_s(&__local_%s, sizeof(%s), ((%s*)__tmp_%s + i), sizeof(%s))) {"name struct_type struct_type name struct_type;
                            sprintf "\t\t%s();" sgx_ocfree_fn;
                            "\t\treturn SGX_ERROR_UNEXPECTED;";
                            "\t}";
                            ]
                        in
                        List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "\n" code_template2
                    in
                    let post =
                        let code_template3 = [
                            sprintf "\tif (memcpy_s((void*)(%s + i), sizeof(__local_%s), &__local_%s, sizeof(__local_%s))) {" name name name name;
                            sprintf "\t\t%s();" sgx_ocfree_fn;
                            "\t\treturn SGX_ERROR_UNEXPECTED;";
                            "\t}";
                            "}";
                            ]
                        in
                        List.fold_left (fun acc s -> acc ^ "\t\t\t" ^ s ^ "\n") "\n" code_template3
                    in
                    invoke_if_struct ty attr.Ast.pa_direction name pre copy_and_free_struct_member post
                in
                let code_template  = 
                  if struct_deep_copy_post = "" then
                      [
                      sprintf "\tif (%s) {" name;
                      sprintf "\t\tif (memcpy_s((void*)%s, %s, __tmp_%s, %s)) {" name (mk_len_var name) name (mk_len_var name);
                      sprintf "\t\t\t%s();" sgx_ocfree_fn;
                      "\t\t\treturn SGX_ERROR_UNEXPECTED;";
                      "\t\t}";
                      "\t}" ;
                      ]
                  else
                      [
                      sprintf "\tif (%s) {%s" name struct_deep_copy_post;
                      "\t}" ;
                      ]
                in
                List.fold_left (fun acc s -> acc ^ "\t" ^ s ^ "\n") "" code_template

          | _ -> ""
    in
    List.fold_left (fun acc (pty, declr) ->
             match pty with
                             Ast.PTVal _ -> acc
               | Ast.PTPtr(ty, attr) -> acc ^ copy_memory ty attr declr) "" plist in

  let set_errno = if propagate_errno then sprintf "%s\n%s\n%s\n%s\n"
                      "\t\tif (memcpy_s((void*)&errno, sizeof(errno), &ms->ocall_errno, sizeof(ms->ocall_errno))) {"
                      (sprintf "\t\t\t%s();" sgx_ocfree_fn)
                      "\t\t\treturn SGX_ERROR_UNEXPECTED;"
                      "\t\t}"
                  else "" in
  let func_close = sprintf "%s%s%s\n%s%s\n"
                           (handle_out_ptr fd.Ast.plist)
                           set_errno
                           "\t}"
                           (gen_ocfree fd.Ast.rtype fd.Ast.plist)
                           "\treturn status;\n}" in
  let sgx_ocall_fn = get_sgx_fname SGX_OCALL ufunc.Ast.uf_is_switchless in
  let ocall_null = sprintf "\tstatus = %s(%d, NULL);\n" sgx_ocall_fn idx in
  let ocall_with_ms = sprintf "\tstatus = %s(%d, %s);\n" sgx_ocall_fn idx ms_struct_val in
  let update_retval = sprintf "%s\n%s\n%s\n%s\n%s\n%s"
                        (sprintf "\t\tif (%s) {" retval_name)
                        (sprintf "\t\t\tif (memcpy_s((void*)%s, sizeof(*%s), &%s, sizeof(%s))) {" retval_name retval_name (mk_parm_accessor retval_name) (mk_parm_accessor retval_name))
                        (sprintf "\t\t\t\t%s();" sgx_ocfree_fn)
                        "\t\t\t\treturn SGX_ERROR_UNEXPECTED;"
                        "\t\t\t}"
                        "\t\t}" in
  let func_body = ref [] in
    if (is_naked_func fd) && (propagate_errno = false) then
        sprintf "%s%s%s%s" func_open local_vars ocall_null "\n\treturn status;\n}"
    else
      begin
        func_body := local_vars :: !func_body;
        func_body := ocalloc_ms_struct:: !func_body;
        List.iter (fun pd -> func_body := tproxy_fill_ms_field pd ufunc.Ast.uf_is_switchless :: !func_body ) fd.Ast.plist;
        func_body := ocalloc_struct_deep_copy ufunc.Ast.uf_is_switchless :: !func_body;
        List.iter (fun pd -> func_body := tproxy_fill_structure pd ufunc.Ast.uf_is_switchless:: !func_body) fd.Ast.plist;
        func_body := ocall_with_ms :: !func_body;
        func_body := "\tif (status == SGX_SUCCESS) {" :: !func_body;
        if fd.Ast.rtype <> Ast.Void then func_body := update_retval :: !func_body;
        List.fold_left (fun acc s -> if s = "" then acc else acc ^ s ^ "\n") func_open (List.rev !func_body) ^ func_close
      end

(* It generates OCALL table and the untrusted proxy to setup OCALL table. *)
let gen_ocall_table (ec: enclave_content) =
  let func_proto_ubridge = List.map (fun (uf: Ast.untrusted_func) ->
                                       let fd : Ast.func_decl = uf.Ast.uf_fdecl in
                                         mk_ubridge_name ec.enclave_name fd.Ast.fname)
                                    ec.ufunc_decls in
  let nr_ocall = List.length ec.ufunc_decls in
  let ocall_table_name = mk_ocall_table_name ec.enclave_name in
  let ocall_table =
    let ocall_members =
      List.fold_left
        (fun acc proto -> acc ^ "\t\t(void*)" ^ proto ^ ",\n") "" func_proto_ubridge
    in "\t{\n" ^ ocall_members ^ "\t}\n"
  in
    sprintf "static const struct {\n\
\tsize_t nr_ocall;\n\
\tvoid * table[%d];\n\
} %s = {\n\
\t%d,\n\
%s};\n" (max nr_ocall 1)
      ocall_table_name
      nr_ocall
      (if nr_ocall <> 0 then ocall_table else "\t{ NULL },\n")

(* It generates untrusted code to be saved in a `.c' file. *)
let gen_untrusted_source (ec: enclave_content) =
  let code_fname = get_usource_name ec.file_shortnm in
  let include_hd = "#include \"" ^ get_uheader_short_name ec.file_shortnm ^ "\"\n" in
  let include_errno = "#include <errno.h>\n" in
  let uproxy_list =
    List.map2 (fun tf ecall_idx -> gen_func_uproxy tf ecall_idx ec)
      ec.tfunc_decls
      (Util.mk_seq 0 (List.length ec.tfunc_decls - 1))
  in
  let ubridge_list =
    List.map (fun fd -> gen_func_ubridge ec.enclave_name fd)
      (ec.ufunc_decls) in
  let out_chan = open_out code_fname in
    output_string out_chan (include_hd ^ include_errno ^ "\n");
    ms_writer out_chan ec;
    List.iter (fun s -> output_string out_chan (s ^ "\n")) ubridge_list;
    output_string out_chan (gen_ocall_table ec);
    List.iter (fun s -> output_string out_chan (s ^ "\n")) uproxy_list;
    close_out out_chan

(* It generates trusted code to be saved in a `.c' file. *)
let gen_trusted_source (ec: enclave_content) =
  let code_fname = get_tsource_name ec.file_shortnm in
  let include_hd = "#include \"" ^ get_theader_short_name ec.file_shortnm ^ "\"\n\n\
#include \"sgx_trts.h\" /* for sgx_ocalloc, sgx_is_outside_enclave */\n\
#include \"sgx_lfence.h\" /* for sgx_lfence */\n\n\
#include <errno.h>\n\
#include <mbusafecrt.h> /* for memcpy_s etc */\n\
#include <stdlib.h> /* for malloc/free etc */\n\
\n\
#define CHECK_REF_POINTER(ptr, siz) do {\t\\\n\
\tif (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))\t\\\n\
\t\treturn SGX_ERROR_INVALID_PARAMETER;\\\n\
} while (0)\n\
\n\
#define CHECK_UNIQUE_POINTER(ptr, siz) do {\t\\\n\
\tif ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))\t\\\n\
\t\treturn SGX_ERROR_INVALID_PARAMETER;\\\n\
} while (0)\n\
\n\
#define CHECK_ENCLAVE_POINTER(ptr, siz) do {\t\\\n\
\tif ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))\t\\\n\
\t\treturn SGX_ERROR_INVALID_PARAMETER;\\\n\
} while (0)\n\
\n\
#define ADD_ASSIGN_OVERFLOW(a, b) (\t\\\n\
\t((a) += (b)) < (b)\t\\\n\
)\n\
\n"
  in
  let trusted_fds = tf_list_to_fd_list ec.tfunc_decls in
  let tbridge_list =
    let dummy_var = tbridge_gen_dummy_variable ec in
    List.map (fun tfd -> gen_func_tbridge tfd dummy_var) trusted_fds in
  let ecall_table = gen_ecall_table ec.tfunc_decls in
  let entry_table = gen_entry_table ec in
  let tproxy_list = List.map2
                      (fun fd idx -> gen_func_tproxy fd idx)
                      (ec.ufunc_decls)
                      (Util.mk_seq 0 (List.length ec.ufunc_decls - 1)) in
  let out_chan = open_out code_fname in
    output_string out_chan (include_hd ^ "\n");
    ms_writer out_chan ec;
    List.iter (fun s -> output_string out_chan (s ^ "\n")) tbridge_list;
    output_string out_chan (ecall_table ^ "\n");
    output_string out_chan (entry_table ^ "\n");
    output_string out_chan "\n";
    List.iter (fun s -> output_string out_chan (s ^ "\n")) tproxy_list;
    close_out out_chan

(* We use a stack to keep record of imported files.
 *
 * A file will be pushed to the stack before we parsing it,
 * and we will pop the stack after each `parse_import_file'.
 *)
let already_read = SimpleStack.create ()
let save_file fullpath =
  if SimpleStack.mem fullpath already_read
  then failwithf "detected circled import for `%s'" fullpath
  else SimpleStack.push fullpath already_read

(* The entry point of the Edger8r parser front-end.
 * ------------------------------------------------
 *)
let start_parsing (fname: string) : Ast.enclave =
  let set_initial_pos lexbuf filename =
    lexbuf.Lexing.lex_curr_p <- {
      lexbuf.Lexing.lex_curr_p with Lexing.pos_fname = fname;
    }
  in
    try
      let fullpath = Util.get_file_path fname in
      let preprocessed =
        save_file fullpath;  Preprocessor.processor_macro(fullpath) in
      let lexbuf = 
        match preprocessed with
          | None -> 
            let chan =  open_in fullpath in
            Lexing.from_channel chan
          | Some(preprocessed_string) -> Lexing.from_string preprocessed_string
      in
        try
          set_initial_pos lexbuf fname;
          let e : Ast.enclave = Parser.start_parsing Lexer.tokenize lexbuf in
          let short_name = Util.get_short_name fname in
            if short_name = ""
            then (eprintf "error: %s: file short name is empty\n" fname; exit 1;)
            else
              let res =  { e with Ast.ename = short_name } in
                if Util.is_c_identifier short_name then res
                else (eprintf "warning: %s: file short name `%s' is not a valid C identifier\n" fname short_name; res)
        with exn ->
          begin match exn with
            | Parsing.Parse_error ->
                let curr = lexbuf.Lexing.lex_curr_p in
                let line = curr.Lexing.pos_lnum in
                let cnum = curr.Lexing.pos_cnum - curr.Lexing.pos_bol in
                let tok = Lexing.lexeme lexbuf in
                  failwithf "%s:%d:%d: unexpected token: %s\n" fname line cnum tok
            | _ -> raise exn
          end
    with Sys_error s -> failwithf "%s\n" s

(* Check duplicated ECALL/OCALL names.
 *
 * This is a pretty simple implementation - to improve it, the
 * location information of each token should be carried to AST.
 *)
let check_duplication (ec: enclave_content) =
  let dict = Hashtbl.create 10 in
  let trusted_fds = tf_list_to_fd_list ec.tfunc_decls in
  let untrusted_fds = uf_list_to_fd_list ec.ufunc_decls in
  let check_and_add fname =
    if Hashtbl.mem dict fname then
      failwithf "Multiple definition of function \"%s\" detected." fname
    else
      Hashtbl.add dict fname true
  in
    List.iter (fun (fd: Ast.func_decl) ->
                 check_and_add fd.Ast.fname) (trusted_fds @ untrusted_fds)

(* For each untrusted functions, check that allowed ECALL does exist. *)
let check_allow_list (ec: enclave_content) =
  let trusted_func_names = get_trusted_func_names ec in
  let do_check_allow_list fname allowed_ecalls =
    List.iter (fun trusted_func ->
                 if List.exists (fun x -> x = trusted_func) trusted_func_names
                 then ()
                 else
                   failwithf "\"%s\" declared to allow unknown function \"%s\"."
                     fname trusted_func) allowed_ecalls
  in
    List.iter (fun (uf: Ast.untrusted_func) ->
                 let fd = uf.Ast.uf_fdecl in
                 let allowed_ecalls = uf.Ast.uf_allow_list in
                   do_check_allow_list fd.Ast.fname allowed_ecalls) ec.ufunc_decls

(* Report private ECALL not used in any "allow(...)" expression. *)
let report_orphaned_priv_ecall (ec: enclave_content) =
  let priv_ecall_names = get_priv_ecall_names ec.tfunc_decls in
  let allowed_names = get_allowed_names ec.ufunc_decls in
  let check_ecall n = if List.mem n allowed_names then ()
                      else eprintf "warning: private ECALL `%s' is not used by any OCALL\n" n
  in
    List.iter check_ecall priv_ecall_names

(* Check that there is at least one public ECALL function. *)
let check_priv_funcs (ec: enclave_content) =
  let priv_bits = tf_list_to_priv_list ec.tfunc_decls in
  if List.for_all (fun is_priv -> is_priv) priv_bits
  then failwithf "the enclave `%s' contains no public root ECALL.\n" ec.file_shortnm
  else report_orphaned_priv_ecall ec

(* When generating edge-routines, it need first to check whether there
 * are `import' expressions inside EDL.  If so, it will parse the given
 * importing file to get an `enclave_content' record, recursively.
 *
 * `ec' is the toplevel `enclave_content' record.

 * Here, a tree reduce algorithm is used. `ec' is the root-node, each
 * `import' expression is considered as a children.
 *)
let reduce_import (ec: enclave_content) =
  (* Append a EDL list to another. Keep the first element and replace the
   second one with empty element contains functions not in the first one 
   if both lists contain a same EDL. The function sequence is backwards compatible.*)
  let join (ec1: enclave_content list) (ec2: enclave_content list) =
    let join_one (acc: enclave_content list) (ec: enclave_content) =
      if List.exists (fun (x: enclave_content) -> x.enclave_name = ec.enclave_name) acc
      then
        let match_ec = List.find (fun (x: enclave_content) -> x.enclave_name = ec.enclave_name) acc in
        let filter_one func_decls decl= List.filter(fun x -> not (x = decl)) func_decls in
        let filtered_ec =
          {empty_ec with
            tfunc_decls   =  List.fold_left(filter_one) ec.tfunc_decls match_ec.tfunc_decls;
            ufunc_decls   =  List.fold_left(filter_one) ec.ufunc_decls match_ec.ufunc_decls; }
        in
        acc @ filtered_ec::[]
      else
        acc @ ec ::[]
    in
    List.fold_left(join_one) ec1 ec2
  in
  let parse_import_file fname =
    parse_enclave_ast (start_parsing fname)
  in
  let check_funs funcs (ec: enclave_content list) =
    (* Check whether `funcs' are listed in head of `ec'.  It returns a
       production (x, y), where:
         x - functions not listed in  head `ec';
         y - a new `ec' that its head contains functions from `funcs' listed in `ec'.
    *)
    let enclave_funcs =
      let trusted_func_names = get_trusted_func_names (List.hd ec) in
      let untrusted_func_names = get_untrusted_func_names (List.hd ec) in
        trusted_func_names @ untrusted_func_names
    in
    let in_ec_def name = List.exists (fun x -> x = name) enclave_funcs in
    let in_import_list name = List.exists (fun x -> x = name) funcs in
    let x = List.filter (fun name -> not (in_ec_def name)) funcs in
    let y =
      { (List.hd ec) with
          tfunc_decls = List.filter (fun tf ->
                                       in_import_list (get_tf_fname tf)) (List.hd ec).tfunc_decls;
          ufunc_decls = List.filter (fun uf ->
                                       in_import_list (get_uf_fname uf)) (List.hd ec).ufunc_decls; }
    in (x, y::(List.tl ec))
  in
  (* Import functions listed in `funcs' from `importee'. *)
  let rec import_funcs (funcs: string list) (importee: enclave_content list) =
    (* A `*' means importing all the functions. *)
    if List.exists (fun x -> x = "*") funcs
    then
      let finished_ec = List.fold_left (fun acc (ipd: Ast.import_decl) ->
                   let next_ec = parse_import_file ipd.Ast.mname
                   in join acc (import_funcs ipd.Ast.flist (next_ec::[]))) importee (List.hd importee).import_exprs
      in
      (SimpleStack.pop already_read |> ignore; finished_ec)
    else
      let (x, y) = check_funs funcs importee
      in
          match (List.hd importee).import_exprs with
              [] -> 
                if x = [] 
                then (SimpleStack.pop already_read |> ignore;y)    (* Resolved all importings *)
                else failwithf "import failed - functions `%s' not found" (List.hd x)
              | ex -> 
                (* Continue importing even if all function importings resolved to avoid circled import.*)
                let finished_ec = List.fold_left (fun acc (ipd: Ast.import_decl) ->
                                 let next_ec = parse_import_file ipd.Ast.mname
                                 in join acc (import_funcs x (next_ec::[]))) y ex
                in
                (SimpleStack.pop already_read |> ignore; finished_ec)
  in
  let imported_ec_list = import_funcs ["*"] (ec::[])
  in
  (* combine two EDLs by appending items except import. *)
  let combine (acc: enclave_content) (ec2: enclave_content) =
    { acc with
        include_list = acc.include_list @ ec2.include_list;
        import_exprs = [];
        comp_defs    = acc.comp_defs   @ ec2.comp_defs;
        tfunc_decls  = acc.tfunc_decls @ ec2.tfunc_decls;
        ufunc_decls  = acc.ufunc_decls @ ec2.ufunc_decls; }
  in
  List.fold_left (combine) (List.hd imported_ec_list) (List.tl imported_ec_list)

(* Generate the Enclave code. *)
let gen_enclave_code (e: Ast.enclave) (ep: edger8r_params) =
  let ec = reduce_import (parse_enclave_ast e) in
    g_use_prefix := ep.use_prefix;
    g_untrusted_dir := ep.untrusted_dir;
    g_trusted_dir := ep.trusted_dir;
    create_dir ep.untrusted_dir;
    create_dir ep.trusted_dir;
    check_duplication ec;
    check_structure ec;
    check_allow_list ec;
    (if not ep.header_only then check_priv_funcs ec);
    if Plugin.available() then
      Plugin.gen_edge_routines ec ep
    else (
      (if ep.gen_untrusted then (gen_untrusted_header ec; if not ep.header_only then gen_untrusted_source ec));
      (if ep.gen_trusted then (gen_trusted_header ec; if not ep.header_only then gen_trusted_source ec))
    )
