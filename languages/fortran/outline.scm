; Template, see highlights.scm. Statements are flat, so the outline is a
; flat list of definitions. Keyword tokens are anonymous nodes, which `.`
; skips, so these also match prefixed forms (RECURSIVE SUBROUTINE, DOUBLE
; PRECISION FUNCTION, ...) and END statements, which are filtered out.

; MODULE M, PROGRAM P, SUBROUTINE S, FUNCTION F, INTERFACE G
((statement
  .
  [
    "program"
    "module"
    "submodule"
    "subroutine"
    "function"
    "interface"
    "blockdata"
  ] @context
  .
  (word) @name) @item
  (#not-match? @item "(?i)^(end|module\\s+procedure)"))

; TYPE T, TYPE, EXTENDS(B), PUBLIC :: T
((statement
  .
  "type" @context
  .
  (word) @name
  (#not-match? @name "(?i)^is$")) @item
  (#not-match? @item "(?i)^(end|module\\s+procedure)"))
(statement
  .
  "type" @context
  .
  (comma)
  (double_colon)
  .
  (word) @name) @item

; INTEGER(8) FUNCTION F, TYPE(T) FUNCTION F, REAL*8 FUNCTION F
(statement
  .
  [
    "integer" "real" "double" "precision" "doubleprecision" "complex" "doublecomplex" "logical" "character" "byte"
    "type"
    "class"
  ]
  .
  (parens)
  .
  [
    "subroutine"
    "function"
  ] @context
  .
  (word) @name) @item
(statement
  .
  ["integer" "real" "double" "precision" "doubleprecision" "complex" "doublecomplex" "logical" "character" "byte"]
  .
  (operator)
  .
  (number)
  .
  "function" @context
  .
  (word) @name) @item
