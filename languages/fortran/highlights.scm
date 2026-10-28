; Fortran highlights, shared by the fixed-form and free-form languages.
; Template: `make queries` replaces "integer" "real" "double" "precision" "doubleprecision" "complex" "doublecomplex" "logical" "character" "byte", "program" "module" "submodule" "subroutine" "function" "procedure" "end" "endif" "enddo" "endselect" "endwhere" "endforall" "endassociate" "endblock" "endcritical" "endmodule" "endsubmodule" "endprogram" "endsubroutine" "endfunction" "endinterface" "endtype" "endenum" "endprocedure" "endteam" "blockdata" "endblockdata" "contains" "use" "implicit" "if" "then" "else" "elseif" "elsewhere" "do" "call" "return" "stop" "continue" "goto" "go" "where" "forall" "associate" "allocate" "deallocate" "nullify" "interface" "recursive" "pure" "elemental" "impure" "non_recursive" "include" "common" "equivalence" "namelist" "import" "generic" "deferred" "non_overridable" "nopass" "enum" "enumerator" "inquire" "rewind" "backspace" "endfile" "selectcase" "selecttype" "sync", "allocatable" "dimension" "intent" "optional" "parameter" "pointer" "public" "private" "protected" "external" "intrinsic" "contiguous" "volatile" "asynchronous" "sequence" "bind" "extends" "abstract" "non_intrinsic" "codimension" "pass" and
; "type" "class" "select" "case" with the keyword tokens of ts/common/keywords.js.
;
; In Zed the last matching pattern wins, so general patterns come first.
; Keep patterns cheap: a pattern whose first step is a plain (word) runs at
; every word on screen, and unanchored sibling steps compare every pair of
; nodes in a statement. Start from a keyword token or anchor with `.`.
; Keyword tokens are anonymous nodes, which `.` skips: `(statement . "type")`
; also matches `END TYPE`.

; ---------------------------------------------------------------- literals

(number) @number
(boz) @number
(string) @string

((dot_operator) @boolean
  (#match? @boolean "(?i)^\\.(true|false)\\.$"))
((dot_operator) @operator
  (#not-match? @operator "(?i)^\\.(true|false)\\.$"))

; kind parameters and named constants of the standard modules, MPI_*, OMP_*
((word) @constant
  (#match? @constant "(?i)^(int8|int16|int32|int64|real32|real64|real128|c_int|c_short|c_long|c_long_long|c_signed_char|c_size_t|c_int8_t|c_int16_t|c_int32_t|c_int64_t|c_intptr_t|c_float|c_double|c_long_double|c_float_complex|c_double_complex|c_bool|c_char|c_ptr|c_funptr|c_null_ptr|c_null_funptr|c_null_char|c_new_line|input_unit|output_unit|error_unit|iostat_end|iostat_eor|character_storage_size|numeric_storage_size|file_storage_size|(mpi|omp)_\\w+)$"))
(macro_identifier) @constant

; ------------------------------------------------------------- comments

(comment) @comment
((comment) @comment.doc
  (#match? @comment.doc "^!(>|<|!)"))

; ----------------------------------------------------- line structure

(label) @label
(continuation) @punctuation.special

("goto"
  .
  (number) @label)
("go"
  .
  (word)
  .
  (number) @label)
(statement
  .
  "do"
  .
  (number) @label)

; ---------------------------------------------------------- operators

(operator) @operator
"%" @operator
[
  "("
  ")"
  "["
  "]"
] @punctuation.bracket
[
  (comma)
  (colon)
  (double_colon)
  ";"
] @punctuation.delimiter

; ----------------------------------------------------------- keywords

["integer" "real" "double" "precision" "doubleprecision" "complex" "doublecomplex" "logical" "character" "byte"] @type.builtin
["program" "module" "submodule" "subroutine" "function" "procedure" "end" "endif" "enddo" "endselect" "endwhere" "endforall" "endassociate" "endblock" "endcritical" "endmodule" "endsubmodule" "endprogram" "endsubroutine" "endfunction" "endinterface" "endtype" "endenum" "endprocedure" "endteam" "blockdata" "endblockdata" "contains" "use" "implicit" "if" "then" "else" "elseif" "elsewhere" "do" "call" "return" "stop" "continue" "goto" "go" "where" "forall" "associate" "allocate" "deallocate" "nullify" "interface" "recursive" "pure" "elemental" "impure" "non_recursive" "include" "common" "equivalence" "namelist" "import" "generic" "deferred" "non_overridable" "nopass" "enum" "enumerator" "inquire" "rewind" "backspace" "endfile" "selectcase" "selecttype" "sync"] @keyword
["allocatable" "dimension" "intent" "optional" "parameter" "pointer" "public" "private" "protected" "external" "intrinsic" "contiguous" "volatile" "asynchronous" "sequence" "bind" "extends" "abstract" "non_intrinsic" "codimension" "pass"] @attribute

; Keywords that are also common names: as the first word of a statement...
(statement
  .
  [
    "type"
    "class"
    "select"
    "case"
  ] @keyword)
(statement
  .
  (word) @keyword
  (#match? @keyword "(?i)^(read|write|print|open|close|flush|wait|format|entry|exit|cycle|data|assign|pause|block|lock|unlock|error|critical|rank|event|form|change|final)$"))

; ...after a logical IF...
(statement
  .
  "if"
  .
  (parens)
  .
  (word) @keyword
  (#match? @keyword "(?i)^(read|write|print|open|close|flush|wait|exit|cycle|assign|pause|error|lock|unlock)$"))

; ...or inside compound keywords.
("go"
  .
  (word) @keyword
  (#match? @keyword "(?i)^to$"))
([
  "select"
  "end"
]
  .
  [
    "case"
    "type"
    "select"
  ] @keyword)
("select"
  .
  (word) @keyword
  (#match? @keyword "(?i)^rank$"))
([
  "case"
  "class"
]
  .
  (word) @keyword
  (#match? @keyword "(?i)^default$"))
([
  "type"
  "class"
]
  .
  (word) @keyword
  (#match? @keyword "(?i)^is$"))
("do"
  .
  (word) @keyword
  (#match? @keyword "(?i)^(while|concurrent)$"))
("end"
  .
  (word) @keyword
  (#match? @keyword "(?i)^(block|critical|team|rank)$"))
("implicit"
  .
  (word) @keyword
  (#match? @keyword "(?i)^none$"))
([
  "interface"
  "generic"
]
  .
  (word) @keyword
  (#match? @keyword "(?i)^(operator|assignment)$"))
("sync"
  .
  (word) @keyword
  (#match? @keyword "(?i)^(all|images|memory|team)$"))
((comma)
  .
  (word) @keyword
  .
  (colon)
  (#match? @keyword "(?i)^only$"))
("function"
  .
  (word)
  .
  (parens)
  .
  (word) @keyword
  .
  (parens)
  (#match? @keyword "(?i)^result$"))

; SAVE, TARGET and VALUE as attributes: `, TARGET ::`, `, VALUE,` or leading
; the statement.
(statement
  (comma)
  .
  (word) @attribute
  .
  [
    (comma)
    (double_colon)
  ]
  (#match? @attribute "(?i)^(save|target|value|automatic|static)$"))
(statement
  .
  (word) @attribute
  (#match? @attribute "(?i)^(save|target|value)$"))

; INTENT(IN), INTENT(OUT), INTENT(INOUT), INTENT(IN OUT)
("intent"
  .
  (parens
    .
    (word) @attribute
    (#match? @attribute "(?i)^(in|out|inout)$")))
("intent"
  .
  (parens
    .
    (word)
    .
    (word) @attribute
    (#match? @attribute "(?i)^out$")))

; TYPE(...) and CLASS(...) in declarations are type specifiers.
([
  "type"
  "class"
] @type.builtin
  .
  (parens))

; ------------------------------------------------------- named things

; TYPE(T), CLASS(T), EXTENDS(T), TYPE IS (T), CLASS IS (T)
([
  "type"
  "class"
  "extends"
]
  .
  (parens
    .
    (word) @type))
([
  "type"
  "class"
]
  .
  (word)
  .
  (parens
    .
    (word) @type))

; TYPE T, TYPE, EXTENDS(B) :: T, END TYPE T
(statement
  .
  "type"
  .
  (word) @type
  (#not-match? @type "(?i)^is$"))
(statement
  .
  "type"
  .
  (comma)
  (double_colon)
  .
  (word) @type)
("end"
  .
  "type"
  .
  (word) @type)
("endtype"
  .
  (word) @type)

; MODULE M, PROGRAM P, USE M, END MODULE M
([
  "module"
  "program"
  "submodule"
  "use"
  "endmodule"
  "endprogram"
  "endsubmodule"
  "blockdata"
]
  .
  (word) @type)
(statement
  .
  "use"
  (double_colon)
  .
  (word) @type)
("submodule"
  .
  (parens
    .
    (word) @type))
("submodule"
  .
  (parens)
  .
  (word) @type)

; SUBROUTINE S, FUNCTION F, END SUBROUTINE S, MODULE PROCEDURE P, ENTRY E
([
  "subroutine"
  "function"
  "procedure"
  "endsubroutine"
  "endfunction"
  "endprocedure"
]
  .
  (word) @function)
(statement
  .
  (word) @_entry
  .
  (word) @function
  (#match? @_entry "(?i)^entry$"))

; PROCEDURE :: NAME => IMPL, GENERIC :: NAME => A, B, FINAL :: F
(statement
  .
  [
    "procedure"
    "generic"
  ]
  (double_colon)
  .
  (word) @function)
(statement
  .
  (word) @_final
  (double_colon)
  .
  (word) @function
  (#match? @_final "(?i)^final$"))
(statement
  .
  [
    "procedure"
    "generic"
  ]
  (operator) @_arrow
  .
  (word) @function
  (#eq? @_arrow "=>"))

; INTERFACE NAME, END INTERFACE NAME (generic interfaces)
([
  "interface"
  "endinterface"
]
  .
  (word) @function
  (#not-match? @function "(?i)^(operator|assignment|read|write)$"))
("end"
  .
  "interface"
  .
  (word) @function
  (#not-match? @function "(?i)^(operator|assignment|read|write)$"))

; ---------------------------------------------------------------- calls

("call"
  .
  (word) @function)

; CALL OBJ%METHOD(...)
("call"
  .
  (word) @variable
  .
  (member))
("call"
  .
  (word)
  .
  (member
    (field) @function)
  .
  (parens))

; intrinsic subroutines and library calls (MPI, OpenMP)
("call"
  .
  (word) @function.builtin
  (#match? @function.builtin "(?i)^(cpu_time|date_and_time|random_number|random_seed|random_init|system_clock|move_alloc|mvbits|get_command|get_command_argument|get_environment_variable|execute_command_line|c_f_pointer|c_f_procpointer|event_query|abort|exit|flush|sleep|atomic_\\w+|co_\\w+|ieee_\\w+|mpi_\\w+|omp_\\w+)$"))

; __MACRO__ SAFE_DEALLOC(X): the macro after a cpp marker
((macro_identifier)
  .
  (word) @function)

; intrinsic functions
((word) @function.builtin
  .
  (parens)
  (#match? @function.builtin "(?i)^(abs|achar|acos|acosh|adjustl|adjustr|aimag|aint|all|allocated|anint|any|asin|asinh|associated|atan|atan2|atanh|bessel_j0|bessel_j1|bessel_jn|bessel_y0|bessel_y1|bessel_yn|bge|bgt|bit_size|ble|blt|btest|ceiling|char|cmplx|command_argument_count|conjg|cos|cosh|count|cshift|dble|digits|dim|dot_product|dprod|dshiftl|dshiftr|eoshift|epsilon|erf|erfc|erfc_scaled|exp|exponent|extends_type_of|findloc|floor|fraction|gamma|huge|hypot|iachar|iall|iand|iany|ibclr|ibits|ibset|ichar|ieor|image_index|index|int|ior|iparity|is_contiguous|is_iostat_end|is_iostat_eor|ishft|ishftc|kind|lbound|lcobound|leadz|len|len_trim|lge|lgt|lle|llt|log|log10|log_gamma|maskl|maskr|matmul|max|maxexponent|maxloc|maxval|merge|merge_bits|min|minexponent|minloc|minval|mod|modulo|nearest|new_line|nint|norm2|not|null|num_images|out_of_range|pack|parity|popcnt|poppar|precision|present|product|radix|range|rank|repeat|reshape|rrspacing|same_type_as|scale|scan|selected_char_kind|selected_int_kind|selected_logical_kind|selected_real_kind|set_exponent|shape|shifta|shiftl|shiftr|sign|sin|sinh|size|spacing|spread|sqrt|storage_size|sum|tan|tanh|this_image|tiny|trailz|transfer|transpose|trim|ubound|ucobound|unpack|verify|c_associated|c_funloc|c_loc|c_sizeof|loc|sizeof|float|sngl|dfloat|dcmplx|dconjg|dimag|dreal|cabs|dabs|zabs|iabs|dsqrt|csqrt|zsqrt|dexp|cexp|dlog|alog|clog|dlog10|alog10|dsin|dcos|dtan|dasin|dacos|datan|datan2|dsinh|dcosh|dtanh|dmax1|dmin1|amax0|amax1|amin0|amin1|max0|max1|min0|min1|dmod|amod|dsign|isign|ddim|idim|dint|dnint|idint|idnint|ifix|ieee_\\w+|mpi_\\w+|omp_\\w+)$"))

; keyword arguments: KIND=8, LEN=*, STAT=IERR
(parens
  (word) @variable.parameter
  .
  (operator) @_eq
  (#eq? @_eq "="))

(field) @property

; ------------------------------------------------- preprocessor & directives

(preproc_directive) @preproc
(preproc_identifier) @constant
((preproc_identifier) @function.builtin
  (#eq? @function.builtin "defined"))
(preproc_string) @string
(preproc_number) @number

(conditional_sentinel) @preproc
(directive_sentinel) @preproc
(directive_continuation) @preproc

; !$OMP PARALLEL DO PRIVATE(I) SCHEDULE(STATIC): directive and clause names
(directive
  [
    (word)
    "integer" "real" "double" "precision" "doubleprecision" "complex" "doublecomplex" "logical" "character" "byte" "program" "module" "submodule" "subroutine" "function" "procedure" "end" "endif" "enddo" "endselect" "endwhere" "endforall" "endassociate" "endblock" "endcritical" "endmodule" "endsubmodule" "endprogram" "endsubroutine" "endfunction" "endinterface" "endtype" "endenum" "endprocedure" "endteam" "blockdata" "endblockdata" "contains" "use" "implicit" "if" "then" "else" "elseif" "elsewhere" "do" "call" "return" "stop" "continue" "goto" "go" "where" "forall" "associate" "allocate" "deallocate" "nullify" "interface" "recursive" "pure" "elemental" "impure" "non_recursive" "include" "common" "equivalence" "namelist" "import" "generic" "deferred" "non_overridable" "nopass" "enum" "enumerator" "inquire" "rewind" "backspace" "endfile" "selectcase" "selecttype" "sync" "allocatable" "dimension" "intent" "optional" "parameter" "pointer" "public" "private" "protected" "external" "intrinsic" "contiguous" "volatile" "asynchronous" "sequence" "bind" "extends" "abstract" "non_intrinsic" "codimension" "pass" "type" "class" "select" "case"
  ] @preproc)
(directive
  (parens
    (word) @constant)
  (#match? @constant "(?i)^(static|dynamic|guided|auto|runtime|none|shared|private|firstprivate|monotonic|nonmonotonic|simd|master|primary|close|spread|in|out|inout|mutexinoutset|seq_cst|acq_rel|release|acquire|relaxed)$"))
