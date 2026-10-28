# Fortran Plus for Zed

Fortran support for Zed, highlighting the way VS Code and Sublime do: it reads
tokens and context and never needs to understand the whole program. Preprocessor
macros (`__MACRO__ SAFE_DEALLOC(X)`), `#if` blocks inside continued
statements, and half-written code therefore don't break highlighting for the rest
of the file.

| Language             | Files                                       | Grammar                 |
| -------------------- | ------------------------------------------- | ----------------------- |
| `Fortran`            | `.f90 .f95 .f03 .f08 .f18` (and upper case) | `ts/fortran_free`       |
| `Fortran Fixed Form` | `.f .F .for .f77 .ftn .fpp`                 | `ts/fortran_fixed`      |

Both languages handle:

- **Comments**: column-1 `C`/`c`/`*`/`!` lines in fixed form, `!` comments, and
  `!>`/`!!` doc comments
- **Continuation**: column 6 in fixed form, `&` in free form, including strings
  continued onto the next line
- **Labels**: statement labels, `GOTO 100`, `DO 10 ...`
- **Directives**: `#if/#ifdef/#include/#define` lines anywhere, `!$OMP`/`!$OMP&`,
  `!$` conditional compilation, `!DIR$`
- **Keywords by context**: `IN`/`OUT` only inside `INTENT(...)`, `VALUE`/`TARGET`
  only as attributes, `READ`/`EXIT`/`CYCLE` only where they start a statement
- **Named things**: type, module and procedure names; intrinsics; MPI/OpenMP
  calls and constants; keyword arguments; `%` components
- **Outline**: an outline (`ctrl-shift-o`) of modules, procedures, types and
  interfaces
- **Language server**: fortls (from `PATH`, or `lsp.fortls.binary.path`)

## Install

```sh
git clone https://github.com/Ouasselhabti/zed-fortran-plus ~/code/zed-extensions/fortran-plus
```

1. Uninstall the `Fortran` extension (it registers the same language name).
2. Run `zed: install dev extension` and pick the clone. Zed fetches the
   grammars from GitHub at the `rev` in `extension.toml`, and needs Rust
   (rustup) to build the fortls glue.

## Develop

- `ts/common/define-grammar.js`, `ts/common/scanner.h`: the grammar (both forms)
- `ts/common/keywords.js`: words that become keyword tokens
- `queries/*.scm`: query templates; `make queries` fills in the keyword lists,
  writes `languages/*/` and checks that every query compiles

```sh
make generate              # after grammar changes; needs tree-sitter (npm i -g tree-sitter-cli)
make queries               # after query or keyword changes
git commit -am "..." && git push   # Zed builds grammars from a pushed commit
make bump && git commit -am "Pin grammar revision" && git push
```

Then click "Rebuild" on the extension in Zed's extensions page.

Highlighting runs on every redraw, so keep queries cheap: start patterns from
a keyword token rather than a plain `(word)`, and anchor sibling steps with `.`.
