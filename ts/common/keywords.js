// Words the lexer turns into keyword tokens (case-insensitive). They are safe
// to treat as keywords everywhere: Fortran code seldom uses them as names.
// Keywords that are also common names (READ, DATA, VALUE, IN, ...) stay
// plain `word`s and are recognised from context in the queries.
//
// scripts/gen-queries.js substitutes these lists into the queries, so the
// queries never mention a keyword that the grammar does not have.

const TYPES = [
  'integer', 'real', 'double', 'precision', 'doubleprecision', 'complex',
  'doublecomplex', 'logical', 'character', 'byte',
];

const KEYWORDS = [
  'program', 'module', 'submodule', 'subroutine', 'function', 'procedure',
  'end', 'endif', 'enddo', 'endselect', 'endwhere', 'endforall',
  'endassociate', 'endblock', 'endcritical', 'endmodule', 'endsubmodule',
  'endprogram', 'endsubroutine', 'endfunction', 'endinterface', 'endtype',
  'endenum', 'endprocedure', 'endteam', 'blockdata', 'endblockdata',
  'contains', 'use', 'implicit', 'if', 'then', 'else', 'elseif', 'elsewhere',
  'do', 'call', 'return', 'stop', 'continue', 'goto', 'go', 'where', 'forall',
  'associate', 'allocate', 'deallocate', 'nullify', 'interface', 'recursive',
  'pure', 'elemental', 'impure', 'non_recursive', 'include', 'common',
  'equivalence', 'namelist', 'import', 'generic', 'deferred',
  'non_overridable', 'nopass', 'enum', 'enumerator', 'inquire', 'rewind',
  'backspace', 'endfile', 'selectcase', 'selecttype', 'sync',
];

const ATTRIBUTES = [
  'allocatable', 'dimension', 'intent', 'optional', 'parameter', 'pointer',
  'public', 'private', 'protected', 'external', 'intrinsic', 'contiguous',
  'volatile', 'asynchronous', 'sequence', 'bind', 'extends', 'abstract',
  'non_intrinsic', 'codimension', 'pass',
];

// Tokens too, but only highlighted where they act as keywords.
const CONTEXTUAL = ['type', 'class', 'select', 'case'];

module.exports = { TYPES, KEYWORDS, ATTRIBUTES, CONTEXTUAL };
