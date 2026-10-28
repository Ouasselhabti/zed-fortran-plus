/**
 * Token-level Fortran grammar, in the spirit of the TextMate grammars used by
 * VS Code and Sublime: it never tries to understand the whole program, so
 * preprocessor macros, `#if` blocks inside continued statements and partial
 * code cannot derail it.
 *
 * Column and line sensitive tokens (comments, continuation, labels, numbers,
 * strings, directives, preprocessor lines) come from the external scanner in
 * ../common/scanner.h. Fortran keywords are not reserved: the words listed in
 * ./keywords.js become (case-insensitive) keyword tokens, any other word is a
 * `word` that the highlight queries classify from its context.
 */
const { TYPES, KEYWORDS, ATTRIBUTES, CONTEXTUAL } = require('./keywords');

// `call` -> /[cC][aA][lL][lL]/, shown in the tree as the anonymous node "call"
const keyword = w => alias(new RegExp(w.replace(/[a-z]/g, c => `[${c}${c.toUpperCase()}]`)), w);

module.exports = function defineGrammar(form) {
  return grammar({
    name: `fortran_${form}`,

    // Keyword tokens are only produced where a keyword can appear, so after
    // `%` (a component name) `TYPE` stays a word.
    word: $ => $.word,

    externals: $ => [
      $._end_of_statement,
      $._blank_line,
      $._line_continuation,
      $.comment,
      $.continuation,
      $.label,
      $.number,
      $.string,
      $.directive_sentinel,
      $.directive_continuation,
      $.conditional_sentinel,
      $.preproc_directive,
      $.preproc_identifier,
      $.preproc_string,
      $.preproc_number,
      $._preproc_punctuation,
      $._error_sentinel,
    ],

    extras: $ => [
      /[ \t\f\r]/,
      $.comment,
      $.continuation,
      $.label,
      $._line_continuation,
      $.directive_continuation,
      $.conditional_sentinel,
      $.preproc_directive,
      $.preproc_identifier,
      $.preproc_string,
      $.preproc_number,
      $._preproc_punctuation,
    ],

    rules: {
      source_file: $ => repeat(choice($._statements, $.directive, $._blank_line)),

      _statements: $ => seq(
        $.statement,
        repeat(seq(';', optional($.statement))),
        $._end_of_statement,
      ),

      statement: $ => repeat1($._item),

      // !$OMP / !$ACC / !DIR$ lines
      directive: $ => seq($.directive_sentinel, repeat($._item), $._end_of_statement),

      _item: $ => choice(
        $.word,
        $.macro_identifier,
        ...[...TYPES, ...KEYWORDS, ...ATTRIBUTES, ...CONTEXTUAL].map(keyword),
        $.number,
        $.string,
        $.boz,
        $.dot_operator,
        $.operator,
        $.parens,
        $.brackets,
        $.member,
        $.comma,
        $.colon,
        $.double_colon,
        $._other,
      ),

      // The closing bracket is optional so that an unbalanced line only
      // affects itself instead of the rest of the file.
      parens: $ => prec.right(seq('(', repeat($._item), optional(')'))),
      brackets: $ => prec.right(seq('[', repeat($._item), optional(']'))),

      member: $ => seq('%', alias($.word, $.field)),

      word: _ => /[A-Za-z_][A-Za-z0-9_$]*/,

      // cpp-style marker such as __MACRO__
      macro_identifier: _ => token(prec(1, /__[A-Za-z0-9_]*__/)),

      boz: _ => token(choice(/[BbOoZz]'[0-9A-Fa-f]*'/, /[BbOoZz]"[0-9A-Fa-f]*"/)),

      dot_operator: _ => /\.[A-Za-z]+\./,

      operator: _ => token(choice(
        '**', '*', '//', '/', '+', '-',
        '==', '/=', '<=', '>=', '<', '>', '=>', '=',
      )),

      comma: _ => ',',
      colon: _ => ':',
      double_colon: _ => '::',

      // Anything else (stray `)`, `$`, `\`, ...) instead of a parse error.
      _other: _ => token(prec(-1, /[^\s]/)),
    },
  });
};
