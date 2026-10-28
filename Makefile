TS ?= tree-sitter
GRAMMARS := fortran_fixed fortran_free
LANGS := fortran fortran-fixed

.PHONY: all generate queries bump

all: generate queries

# Regenerate ts/*/src/parser.c after editing ts/common/*.js.
generate:
	for g in $(GRAMMARS); do (cd ts/$$g && $(TS) generate --abi 14) || exit 1; done

# queries/ holds templates; write each language's copy, then check that every
# query compiles against both grammars (a broken query disables highlighting).
queries:
	node scripts/gen-queries.js
	echo > /tmp/fortran-plus-empty.f
	for g in $(GRAMMARS); do for q in languages/fortran/*.scm; do \
	  (cd ts/$$g && $(TS) query -q ../../$$q /tmp/fortran-plus-empty.f >/dev/null 2>&1) || \
	  { echo "$$q fails on $$g:"; cd ts/$$g && $(TS) query -q ../../$$q /tmp/fortran-plus-empty.f; exit 1; }; \
	done; done

# Zed builds the grammars from a commit: commit grammar changes, then run
# this to point extension.toml at HEAD, then rebuild the dev extension.
# Zed only recompiles a grammar when src/parser.c or src/scanner.c is newer
# than its .wasm, which a scanner.h-only change is not, so drop the .wasm.
bump:
	sed -i "s/^rev = .*/rev = \"$$(git rev-parse HEAD)\"/" extension.toml
	rm -f grammars/*.wasm
