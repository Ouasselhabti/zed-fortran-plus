#!/usr/bin/env node
// Writes languages/<lang>/*.scm from the templates in queries/, replacing
// @TYPES@, @KEYWORDS@, @ATTRIBUTES@ and @CONTEXTUAL@ with the keyword tokens
// of ts/common/keywords.js.
const fs = require('fs');
const path = require('path');
const lists = require('../ts/common/keywords');

const root = path.join(__dirname, '..');
const quote = words => words.map(w => `"${w}"`).join(' ');

for (const lang of ['fortran', 'fortran-fixed']) {
  for (const file of fs.readdirSync(path.join(root, 'queries'))) {
    if (!file.endsWith('.scm')) continue;
    const text = fs
      .readFileSync(path.join(root, 'queries', file), 'utf8')
      .replace(/@(TYPES|KEYWORDS|ATTRIBUTES|CONTEXTUAL)@/g, (_, name) => quote(lists[name]));
    fs.writeFileSync(path.join(root, 'languages', lang, file), text);
  }
}
