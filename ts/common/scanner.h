/*
 * External scanner shared by the fixed-form and free-form Fortran grammars.
 * Include it after defining FIXED_FORM (0 or 1) and SCANNER(name).
 *
 * It owns everything that depends on columns or on line structure:
 *   - whole-line comments (column 1 `C`, `c`, `*`, `!` in fixed form; `!`)
 *   - continuation (column 6 in fixed form; `&` in free form), turning the
 *     newline before a continuation line into an invisible extra
 *   - statement labels, numbers (so `1.EQ.2` is not read as `1.` `EQ` `.2`)
 *     and strings (including strings continued on the next line)
 *   - OpenMP/OpenACC/compiler directives and `!$` conditional compilation
 *   - C preprocessor lines, which may sit inside a continued statement
 *
 * Only ASCII helpers are used, no libc, so it compiles to wasm as is.
 */
#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>

enum TokenType {
  END_OF_STATEMENT,
  BLANK_LINE,
  LINE_CONTINUATION,
  COMMENT,
  CONTINUATION,
  LABEL,
  NUMBER,
  STRING,
  DIRECTIVE_SENTINEL,
  DIRECTIVE_CONTINUATION,
  CONDITIONAL_SENTINEL,
  PREPROC_DIRECTIVE,
  PREPROC_IDENTIFIER,
  PREPROC_STRING,
  PREPROC_NUMBER,
  PREPROC_PUNCTUATION,
  ERROR_SENTINEL,
};

typedef struct {
  bool in_preproc;   // inside a `#` line
  bool pp_include;   // that line is an #include, so <...> is a string
  bool pending_amp;  // free form: the current line ended with `&`
  bool continuing;   // free form: on a continuation line of the statement
  bool in_directive; // free form: the current statement is a directive
} Scanner;

typedef enum { S_COMMENT, S_DIRECTIVE, S_CONDITIONAL } Sentinel;

static inline bool is_alpha(int32_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline bool is_digit(int32_t c) { return c >= '0' && c <= '9'; }
static inline bool is_word(int32_t c) {
  return is_alpha(c) || is_digit(c) || c == '_' || c == '$';
}
static inline bool is_hspace(int32_t c) {
  return c == ' ' || c == '\t' || c == '\f' || c == '\r';
}
static inline bool is_exponent(int32_t c) {
  return c == 'e' || c == 'E' || c == 'd' || c == 'D' || c == 'q' || c == 'Q';
}
static inline int32_t lower(int32_t c) {
  return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}
static inline void adv(TSLexer *l) { l->advance(l, false); }
static inline void skip(TSLexer *l) { l->advance(l, true); }
static inline bool at_eol(TSLexer *l) { return l->lookahead == '\n' || l->eof(l); }
static inline void to_eol(TSLexer *l) {
  while (!at_eol(l)) adv(l);
}

static bool word_is(const char *w, int n, const char *lit) {
  int i = 0;
  for (; i < n; i++) {
    if (lit[i] != w[i]) return false;
  }
  return lit[i] == '\0';
}

// Up to `max` lowercased letters into `w`; returns how many were read.
static int read_letters(TSLexer *l, char *w, int max) {
  int n = 0;
  while (n < max && is_alpha(l->lookahead)) {
    w[n++] = (char)lower(l->lookahead);
    adv(l);
  }
  return n;
}

/*
 * Called just after the first character of a `!` (or fixed-form `C`, `c`,
 * `*`) line. Consumes the directive name when there is one: `$OMP`, `$ACC`,
 * `DIR$`, `DEC$`, `GCC$`, or the bare `$` of conditional compilation.
 */
static Sentinel read_sentinel(TSLexer *l) {
  char w[3];
  int n;
  if (l->lookahead == '$') {
    adv(l);
    n = read_letters(l, w, 3);
    if (n == 3 && (word_is(w, n, "omp") || word_is(w, n, "acc"))) return S_DIRECTIVE;
    if (n == 0) return S_CONDITIONAL;
    return S_COMMENT;
  }
  n = read_letters(l, w, 3);
  if (n == 3 && l->lookahead == '$' &&
      (word_is(w, n, "dir") || word_is(w, n, "dec") || word_is(w, n, "gcc"))) {
    adv(l);
    return S_DIRECTIVE;
  }
  return S_COMMENT;
}

// `_8`, `_dp`, `_int64` after a literal.
static void kind_suffix(TSLexer *l) {
  if (l->lookahead != '_') return;
  adv(l);
  if (!is_word(l->lookahead)) return;
  while (is_word(l->lookahead)) adv(l);
  l->mark_end(l);
}

// After the mantissa, positioned at a possible exponent letter.
static void exponent(TSLexer *l) {
  if (!is_exponent(l->lookahead)) {
    kind_suffix(l);
    return;
  }
  adv(l);
  if (l->lookahead == '+' || l->lookahead == '-') adv(l);
  if (!is_digit(l->lookahead)) return; // `1E` + something else: stop at the mantissa
  while (is_digit(l->lookahead)) adv(l);
  l->mark_end(l);
  kind_suffix(l);
}

// Digits after `.`, and the exponent/kind that may follow.
static void fraction(TSLexer *l) {
  while (is_digit(l->lookahead)) adv(l);
  l->mark_end(l);
  exponent(l);
}

static bool scan_number(TSLexer *l) {
  while (is_digit(l->lookahead)) adv(l);
  l->mark_end(l);
  if (l->lookahead != '.') {
    exponent(l);
    return true;
  }
  adv(l);
  if (is_digit(l->lookahead)) {
    fraction(l);
    return true;
  }
  if (!is_alpha(l->lookahead)) {
    l->mark_end(l); // `1.`
    exponent(l);
    return true;
  }
  // `1.E5`, `1.D0` (exponent) versus `1.EQ.2`, `0.AND.` (the number is `1`,
  // and the `.` belongs to the operator)
  if (is_exponent(l->lookahead)) {
    adv(l);
    if (l->lookahead == '+' || l->lookahead == '-') adv(l);
    if (is_digit(l->lookahead)) {
      while (is_digit(l->lookahead)) adv(l);
      l->mark_end(l);
      kind_suffix(l);
    }
  }
  return true;
}

#if FIXED_FORM

static void skip_preproc_line(TSLexer *l) {
  while (!l->eof(l)) {
    if (l->lookahead == '\\') {
      adv(l);
      if (l->lookahead == '\r') adv(l);
      if (l->lookahead == '\n') adv(l);
      continue;
    }
    if (l->lookahead == '\n') return;
    adv(l);
  }
}

/*
 * After a sentinel or at the start of the statement field: walks blanks (and
 * label digits) up to column 6 and reports whether it holds a continuation
 * character, consuming it if so.
 */
static bool fixed_continuation_field(TSLexer *l) {
  while (l->get_column(l) < 5 && (l->lookahead == ' ' || is_digit(l->lookahead))) adv(l);
  if (l->get_column(l) != 5) return false;
  int32_t c = l->lookahead;
  if (c == ' ' || c == '0' || c == '\t' || c == '\r' || at_eol(l)) return false;
  adv(l);
  return true;
}

/*
 * Lookahead from the start of a line: is the next line that is not a comment,
 * blank or preprocessor line a continuation line?
 */
static bool fixed_next_line_continues(TSLexer *l) {
  for (;;) {
    if (l->eof(l)) return false;
    int32_t c = l->lookahead;
    if (c == '\n' || c == '\r') {
      adv(l);
      continue;
    }
    if (c == 'C' || c == 'c' || c == '*' || c == '!') {
      adv(l);
      if (read_sentinel(l) == S_COMMENT) {
        to_eol(l);
        continue;
      }
      return fixed_continuation_field(l);
    }
    if (c == '#') {
      skip_preproc_line(l);
      continue;
    }
    bool blank = true, comment = false;
    while (l->get_column(l) < 5) {
      c = l->lookahead;
      if (c == ' ' || is_digit(c)) {
        if (c != ' ') blank = false;
        adv(l);
      } else if (c == '\t') {
        adv(l);
        return l->lookahead >= '1' && l->lookahead <= '9';
      } else if (c == '!') {
        comment = true;
        break;
      } else if (c == '\r') {
        adv(l);
      } else if (at_eol(l)) {
        break;
      } else {
        return false; // code in the label field: a new (sloppy) statement
      }
    }
    if (comment) {
      to_eol(l);
      continue;
    }
    c = l->lookahead;
    if (at_eol(l) || c == '\r') {
      if (!blank) return false;
      to_eol(l);
      continue;
    }
    return c != ' ' && c != '0' && c != '\t';
  }
}

// From the start of a line, for a string left open: is this line a direct
// continuation line? Consumes up to and including the continuation character.
static bool fixed_line_continues_string(TSLexer *l) {
  while (l->get_column(l) < 5 && l->lookahead == ' ') adv(l);
  return fixed_continuation_field(l);
}

#else

// Lookahead from the start of a line: does it hold code (as opposed to being
// blank, a comment or a preprocessor line)?
static bool free_line_has_code(TSLexer *l) {
  while (is_hspace(l->lookahead)) adv(l);
  if (at_eol(l) || l->lookahead == '#') return false;
  if (l->lookahead == '!') {
    adv(l);
    return l->lookahead == '$';
  }
  return true;
}

#endif

static void end_statement(Scanner *s) {
  s->pending_amp = false;
  s->continuing = false;
  s->in_directive = false;
}

static bool scan_newline(Scanner *s, TSLexer *l, const bool *valid) {
  if (l->eof(l)) {
    // Close a statement that has no trailing newline (zero-width token).
    if (valid[END_OF_STATEMENT] && !valid[ERROR_SENTINEL]) {
      end_statement(s);
      l->mark_end(l);
      l->result_symbol = END_OF_STATEMENT;
      return true;
    }
    return false;
  }
  adv(l); // '\n'
  l->mark_end(l);

  bool soft;
#if FIXED_FORM
  soft = fixed_next_line_continues(l);
#else
  soft = s->pending_amp;
  if (soft) {
    s->continuing = true;
    if (free_line_has_code(l)) s->pending_amp = false;
  }
#endif
  if (soft) {
    l->result_symbol = LINE_CONTINUATION;
    return true;
  }
  end_statement(s);
  if (valid[END_OF_STATEMENT]) {
    l->result_symbol = END_OF_STATEMENT;
  } else if (valid[BLANK_LINE]) {
    l->result_symbol = BLANK_LINE;
  } else {
    l->result_symbol = LINE_CONTINUATION;
  }
  return true;
}

static bool scan_string(TSLexer *l) {
  int32_t quote = l->lookahead;
  adv(l);
#if !FIXED_FORM
  bool amp = false; // last non-blank character was `&`
#endif
  for (;;) {
    int32_t c = l->lookahead;
    if (c == quote) {
      adv(l);
      if (l->lookahead == quote) { // doubled quote
        adv(l);
        continue;
      }
      l->mark_end(l);
      return true;
    }
    if (at_eol(l)) {
      l->mark_end(l);
      if (l->eof(l)) return true;
      adv(l); // '\n'
#if FIXED_FORM
      if (fixed_line_continues_string(l)) continue;
#else
      if (amp) {
        while (is_hspace(l->lookahead)) adv(l);
        if (l->lookahead == '&') adv(l);
        amp = false;
        continue;
      }
#endif
      return true; // unterminated: ends with the line
    }
#if !FIXED_FORM
    if (c == '&') {
      amp = true;
    } else if (!is_hspace(c)) {
      amp = false;
    }
#endif
    adv(l);
  }
}

static bool scan_preproc_directive(Scanner *s, TSLexer *l) {
  adv(l); // '#'
  while (l->lookahead == ' ' || l->lookahead == '\t') adv(l);
  char w[8];
  int n = 0;
  while (is_alpha(l->lookahead)) {
    if (n < 8) w[n++] = (char)lower(l->lookahead);
    adv(l);
  }
  l->mark_end(l);
  s->in_preproc = true;
  s->pp_include = word_is(w, n, "include");
  l->result_symbol = PREPROC_DIRECTIVE;
  return true;
}

static bool scan_preproc_body(Scanner *s, TSLexer *l, const bool *valid) {
  for (;;) {
    while (is_hspace(l->lookahead)) skip(l);
    int32_t c = l->lookahead;
    if (c == '\\') { // line splice
      skip(l);
      if (l->lookahead == '\r') skip(l);
      if (l->lookahead == '\n') skip(l);
      continue;
    }
    if (at_eol(l)) {
      s->in_preproc = false;
      s->pp_include = false;
      return scan_newline(s, l, valid);
    }
    if (c == '"' || (c == '<' && s->pp_include)) {
      int32_t close = c == '"' ? '"' : '>';
      adv(l);
      while (l->lookahead != close && !at_eol(l)) adv(l);
      if (l->lookahead == close) adv(l);
      l->mark_end(l);
      l->result_symbol = PREPROC_STRING;
      return true;
    }
    if (is_alpha(c) || c == '_') {
      while (is_word(l->lookahead)) adv(l);
      l->mark_end(l);
      l->result_symbol = PREPROC_IDENTIFIER;
      return true;
    }
    if (is_digit(c)) {
      while (is_word(l->lookahead)) adv(l);
      l->mark_end(l);
      l->result_symbol = PREPROC_NUMBER;
      return true;
    }
    if (c == '/') { // C comments are allowed on cpp lines
      adv(l);
      if (l->lookahead == '/') {
        to_eol(l);
        l->mark_end(l);
        l->result_symbol = COMMENT;
        return true;
      }
      if (l->lookahead == '*') {
        adv(l);
        while (!l->eof(l)) {
          if (l->lookahead == '*') {
            adv(l);
            if (l->lookahead == '/') {
              adv(l);
              break;
            }
          } else {
            adv(l);
          }
        }
        l->mark_end(l);
        l->result_symbol = COMMENT;
        return true;
      }
      l->mark_end(l);
      l->result_symbol = PREPROC_PUNCTUATION;
      return true;
    }
    skip(l); // other punctuation is left unhighlighted
  }
}

// A `!` (or fixed-form column-1 `C`/`c`/`*`) has just been consumed.
static bool scan_bang(Scanner *s, TSLexer *l, const bool *valid, bool may_be_sentinel) {
  if (may_be_sentinel) {
    switch (read_sentinel(l)) {
    case S_DIRECTIVE: {
#if FIXED_FORM
      l->mark_end(l);
      bool continued = fixed_continuation_field(l);
      if (continued) l->mark_end(l);
#else
      if (l->lookahead == '&') adv(l); // `!$omp&`
      l->mark_end(l);
      bool continued = s->in_directive && s->continuing;
#endif
      if (continued || !valid[DIRECTIVE_SENTINEL]) {
        l->result_symbol = DIRECTIVE_CONTINUATION;
      } else {
        s->in_directive = true;
        l->result_symbol = DIRECTIVE_SENTINEL;
      }
      return true;
    }
    case S_CONDITIONAL:
      l->mark_end(l);
#if FIXED_FORM
      if (fixed_continuation_field(l)) l->mark_end(l);
#endif
      l->result_symbol = CONDITIONAL_SENTINEL;
      return true;
    case S_COMMENT:
      break;
    }
  }
  to_eol(l);
  l->mark_end(l);
  l->result_symbol = COMMENT;
  return true;
}

static bool scan_body(Scanner *s, TSLexer *l, const bool *valid, bool line_start) {
  while (is_hspace(l->lookahead)) skip(l);
  int32_t c = l->lookahead;

  if (at_eol(l)) return scan_newline(s, l, valid);

  if (c == '!') {
    adv(l);
    return scan_bang(s, l, valid, !FIXED_FORM && line_start);
  }
  if (c == '#' && line_start) return scan_preproc_directive(s, l);
  if (c == '\'' || c == '"') {
    if (!valid[STRING]) return false;
    l->result_symbol = STRING;
    return scan_string(l);
  }
  if (is_digit(c)) {
#if !FIXED_FORM
    if (line_start && !s->continuing) {
      while (is_digit(l->lookahead)) adv(l);
      l->mark_end(l);
      l->result_symbol = LABEL;
      return true;
    }
#endif
    if (!valid[NUMBER]) return false;
    l->result_symbol = NUMBER;
    return scan_number(l);
  }
  if (c == '.') {
    adv(l);
    if (!is_digit(l->lookahead) || !valid[NUMBER]) return false;
    l->result_symbol = NUMBER;
    fraction(l);
    return true;
  }
#if !FIXED_FORM
  if (c == '&') {
    adv(l);
    l->mark_end(l);
    while (is_hspace(l->lookahead)) adv(l);
    if (at_eol(l) || l->lookahead == '!') s->pending_amp = true;
    l->result_symbol = CONTINUATION;
    return true;
  }
#endif
  return false;
}

#if FIXED_FORM
// At column 1 of a fixed-form line.
static bool scan_fixed_line_start(Scanner *s, TSLexer *l, const bool *valid) {
  int32_t c = l->lookahead;
  if (c == 'C' || c == 'c' || c == '*' || c == '!') {
    adv(l);
    return scan_bang(s, l, valid, true);
  }
  if (c == '#') return scan_preproc_directive(s, l);
  if (c != ' ' && c != '\t' && !is_digit(c)) return scan_body(s, l, valid, true);

  // label field, columns 1-5
  while (l->lookahead == ' ' && l->get_column(l) < 5) skip(l);
  c = l->lookahead;
  if (l->get_column(l) < 5) {
    if (is_digit(c)) {
      while (is_digit(l->lookahead) && l->get_column(l) < 5) adv(l);
      l->mark_end(l);
      l->result_symbol = LABEL;
      return true;
    }
    if (c == '!') {
      adv(l);
      return scan_bang(s, l, valid, false);
    }
  }
  if (c == '\t' && l->get_column(l) < 6) { // tab format: TAB then 1-9 continues
    skip(l);
    if (l->lookahead >= '1' && l->lookahead <= '9') {
      adv(l);
      l->mark_end(l);
      l->result_symbol = CONTINUATION;
      return true;
    }
    return scan_body(s, l, valid, true);
  }
  if (l->get_column(l) == 5) {
    c = l->lookahead;
    if (c == '0') {
      skip(l);
    } else if (c != ' ' && c != '\r' && !at_eol(l)) {
      adv(l);
      l->mark_end(l);
      l->result_symbol = CONTINUATION;
      return true;
    }
  }
  return scan_body(s, l, valid, true);
}
#endif

void *SCANNER(create)(void) { return ts_calloc(1, sizeof(Scanner)); }

void SCANNER(destroy)(void *payload) { ts_free(payload); }

unsigned SCANNER(serialize)(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  buffer[0] = (char)((s->in_preproc ? 1 : 0) | (s->pp_include ? 2 : 0) | (s->pending_amp ? 4 : 0) |
                     (s->continuing ? 8 : 0) | (s->in_directive ? 16 : 0));
  return 1;
}

void SCANNER(deserialize)(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  uint8_t b = length > 0 ? (uint8_t)buffer[0] : 0;
  s->in_preproc = b & 1;
  s->pp_include = b & 2;
  s->pending_amp = b & 4;
  s->continuing = b & 8;
  s->in_directive = b & 16;
}

bool SCANNER(scan)(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;
  if (s->in_preproc) return scan_preproc_body(s, lexer, valid_symbols);
  bool line_start = lexer->get_column(lexer) == 0;
#if FIXED_FORM
  if (line_start) return scan_fixed_line_start(s, lexer, valid_symbols);
#endif
  return scan_body(s, lexer, valid_symbols, line_start);
}
