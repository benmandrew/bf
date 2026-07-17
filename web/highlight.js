// Theme an LLVM dot-cfg graph, syntax-highlighting the IR. A port of
// scripts/highlight.py: the browser build has no Python, so the same dot
// transformation runs here before Graphviz lays the graph out. See that file
// for the why behind each step; this mirrors it rule for rule so the two
// produce identical output (scripts/wasm_parity is the guard).

// Prism's LLVM grammar, in Prism's order (priority is encoded by order). The
// one addition is `ptr` in `type`, since Prism's component predates opaque
// pointers and bfc emits them throughout.
const RULES = [
  ["comment", String.raw`;.*`],
  ["string", String.raw`"[^"]*"`],
  ["boolean", String.raw`\b(?:false|true)\b`],
  ["variable", String.raw`[%@!#](?:(?!\d)(?:[-$.\w]|\\[a-fA-F\d]{2})+|\d+)`],
  ["label", String.raw`(?!\d)(?:[-$.\w]|\\[a-fA-F\d]{2})+:`],
  ["type", String.raw`\b(?:double|float|fp128|half|i[1-9]\d*|label|metadata|ppc_fp128|ptr|token|void|x86_fp80|x86_mmx)\b`],
  ["keyword", String.raw`\b[a-z_][a-z_0-9]*\b`],
  ["number", String.raw`[+-]?\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b|\b0x[\dA-Fa-f]+\b|\b0xK[\dA-Fa-f]{20}\b|\b0x[ML][\dA-Fa-f]{32}\b|\b0xH[\dA-Fa-f]{4}\b`],
  ["punctuation", String.raw`[{}[\];(),.!*=<>]`],
];

// Alternation is tried left to right at each position, so Prism's order is
// preserved. Named groups let colourise recover which rule matched.
const SCANNER = new RegExp(
  RULES.map(([name, pat]) => `(?<${name}>${pat})`).join("|"),
  "g",
);

const LIGHT = {
  colours: {
    comment: "#6e7781",
    string: "#0a3069",
    boolean: "#0550ae",
    variable: "#0550ae",
    label: "#953800",
    type: "#8250df",
    keyword: "#cf222e",
    number: "#0a3069",
    punctuation: "#1f2328",
  },
  plain: "#1f2328",
  fill: "#f6f8fa",
  border: "#d0d7de",
  bg: "white",
  graph_fontcolor: "#57606a",
  edge: "#8c959f",
  edge_true: "#1a7f37",
  edge_false: "#cf222e",
};

const DARK = {
  colours: {
    comment: "#8b949e",
    string: "#a5d6ff",
    boolean: "#79c0ff",
    variable: "#79c0ff",
    label: "#ffa657",
    type: "#d2a8ff",
    keyword: "#ff7b72",
    number: "#79c0ff",
    punctuation: "#c9d1d9",
  },
  plain: "#c9d1d9",
  fill: "#161b22",
  border: "#30363d",
  bg: "#0d1117",
  graph_fontcolor: "#8b949e",
  edge: "#6e7681",
  edge_true: "#3fb950",
  edge_false: "#f85149",
};

const THEMES = { light: LIGHT, dark: DARK };

// Matches Python's html.escape(quote=True): order matters, & first.
function escapeHtml(s) {
  return s
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#x27;");
}

// Node text is Courier, not a nicer mono like DejaVu Sans Mono, because the
// wasm Graphviz that lays out the browser graph has no font files: it estimates
// text extents from its built-in PostScript AFM metrics and falls back to
// proportional Times for any name it does not know there, which packs the
// coloured token runs too tightly and they overlap when a real monospace font
// renders them. Courier is the one monospace font in that built-in set, so the
// layout stays monospace; style.css then renders it with an embedded
// Courier-metric font. Keep this in sync with scripts/highlight.py.
function buildPreamble(theme) {
  return (
    `\tgraph [bgcolor="${theme.bg}", fontname="DejaVu Sans", fontsize=11,\n` +
    `\t       fontcolor="${theme.graph_fontcolor}", nodesep=0.35, ranksep=0.45, pad=0.25];\n` +
    `\tnode [shape=plaintext, fontname="Courier", fontsize=11,\n` +
    `\t      fontcolor="${theme.plain}"];\n` +
    `\tedge [color="${theme.edge}", penwidth=1.1, arrowsize=0.7];`
  );
}

const NODE_RE = /^(\s*)(Node0x[0-9a-f]+)\s*\[shape=record,\s*label="(.*)"\];\s*$/;
const PORT_RE = /^<(s\d+)>([\s\S]*)$/;

// Split a record label body on unescaped, top-level '|'.
function splitTop(s) {
  const parts = [];
  let depth = 0;
  let cur = "";
  let i = 0;
  while (i < s.length) {
    const c = s[i];
    if (c === "\\" && i + 1 < s.length) {
      cur += s.slice(i, i + 2);
      i += 2;
      continue;
    }
    if (c === "{") depth++;
    else if (c === "}") depth--;
    else if (c === "|" && depth === 0) {
      parts.push(cur);
      cur = "";
      i += 1;
      continue;
    }
    cur += c;
    i += 1;
  }
  parts.push(cur);
  return parts;
}

// Undo DOT record-label escaping. \l, \n and \r become newlines.
function unescape(s) {
  let out = "";
  let i = 0;
  while (i < s.length) {
    if (s[i] === "\\" && i + 1 < s.length) {
      const nxt = s[i + 1];
      out += "lnr".includes(nxt) ? "\n" : nxt;
      i += 2;
    } else {
      out += s[i];
      i += 1;
    }
  }
  return out;
}

// Tokenise a line of LLVM IR into coloured HTML-like spans.
function colourise(line, theme) {
  let out = "";
  let pos = 0;
  for (const m of line.matchAll(SCANNER)) {
    if (m.index > pos) out += escapeHtml(line.slice(pos, m.index));
    const name = RULES.find(([n]) => m.groups[n] !== undefined)[0];
    const colour = theme.colours[name] ?? theme.plain;
    out += `<FONT COLOR="${colour}">${escapeHtml(m[0])}</FONT>`;
    pos = m.index + m[0].length;
  }
  if (pos < line.length) out += escapeHtml(line.slice(pos));
  return out;
}

// Undo the eighty-column wrapping CFGPrinter applies to a record label: a
// continuation starts with "..." and nothing bfc emits begins a line that way.
function rejoinWraps(lines) {
  const out = [];
  for (const line of lines) {
    if (out.length && line.startsWith("...")) {
      out[out.length - 1] += line.slice(3);
    } else {
      out.push(line);
    }
  }
  return out;
}

// Strip the quoting opt puts round a block name in dot-cfg mode.
function cleanHeader(field) {
  const lines = unescape(field)
    .split("\n")
    .filter((ln) => ln.trim());
  let text = rejoinWraps(lines).join("").trim();
  if (text.endsWith(":")) text = text.slice(0, -1);
  if (text.length > 1 && text.startsWith('"') && text.endsWith('"')) {
    text = text.slice(1, -1);
  }
  return text;
}

// Parse a '{<s0>T|<s1>F}' field into [[port, text], ...] or null.
function parsePorts(field) {
  const text = field.trim();
  if (!(text.startsWith("{") && text.endsWith("}"))) return null;
  const cells = [];
  for (const cell of splitTop(text.slice(1, -1))) {
    const m = PORT_RE.exec(cell);
    if (!m) return null;
    cells.push([m[1], unescape(m[2]).trim()]);
  }
  return cells.length ? cells : null;
}

// Render parsed record fields as an HTML-like table label.
function buildLabel(fields, theme) {
  const ports = fields.length > 1 ? parsePorts(fields[fields.length - 1]) : null;
  const body = ports ? fields.slice(1, -1) : fields.slice(1);
  const span = ports ? ports.length : 1;

  const rows = [
    `<TR><TD ALIGN="LEFT" COLSPAN="${span}">${escapeHtml(cleanHeader(fields[0]))}</TD></TR>`,
  ];

  for (const field of body) {
    let lines = unescape(field)
      .split("\n")
      .filter((ln) => ln.trim());
    lines = rejoinWraps(lines);
    if (!lines.length) continue;
    rows.push("<HR/>");
    const cell = lines.map((ln) => colourise(ln, theme)).join("<BR/>");
    rows.push(
      `<TR><TD ALIGN="LEFT" BALIGN="LEFT" COLSPAN="${span}">${cell}<BR/></TD></TR>`,
    );
  }

  if (ports) {
    rows.push("<HR/>");
    const cells = ports
      .map(([port, text]) => `<TD PORT="${port}">${escapeHtml(text)}</TD>`)
      .join("<VR/>");
    rows.push(`<TR>${cells}</TR>`);
  }

  const table =
    `<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" ` +
    `CELLPADDING="4" BGCOLOR="${theme.fill}" COLOR="${theme.border}" STYLE="ROUNDED">` +
    `${rows.join("")}</TABLE>`;
  return `<${table}>`;
}

function convert(line, theme) {
  const m = NODE_RE.exec(line);
  if (!m) return null;
  const [, indent, name, label] = m;
  const bodyText = label.trim();
  if (!(bodyText.startsWith("{") && bodyText.endsWith("}"))) return null;
  const fields = splitTop(bodyText.slice(1, -1));
  return `${indent}${name} [label=${buildLabel(fields, theme)}];\n`;
}

// Colour a `Node:sN -> Node` branch edge, merging into any existing
// attribute list.
function colourEdge(line, port, colour) {
  const escPort = port.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const pattern = new RegExp(
    `(${escPort} -> Node0x[0-9a-f]+)\\s*(\\[[^\\]]*\\])?;`,
    "g",
  );
  return line.replace(pattern, (_match, edge, attrs) => {
    if (attrs) return `${edge} [color="${colour}", ${attrs.slice(1, -1).trim()}];`;
    return `${edge} [color="${colour}"];`;
  });
}

// Iterate lines the way Python's `for line in stdin` does: each keeps its
// trailing newline, and a trailing newline in the input yields no empty line.
function iterLines(text) {
  const raw = text.split("\n");
  const lines = [];
  for (let i = 0; i < raw.length; i++) {
    if (i < raw.length - 1) lines.push(raw[i] + "\n");
    else if (raw[i] !== "") lines.push(raw[i]);
  }
  return lines;
}

/// Theme a dot-cfg graph. Takes the raw dot from bfc --emit-cfg-dot and a
/// theme name ("light" or "dark", default dark), returns themed dot.
export function highlightDot(dot, themeName = "dark") {
  const theme = THEMES[themeName] || DARK;
  const preamble = buildPreamble(theme);
  let out = "";
  for (let line of iterLines(dot)) {
    const converted = convert(line, theme);
    if (converted !== null) {
      out += converted;
      continue;
    }
    line = colourEdge(line, ":s0", theme.edge_true);
    line = colourEdge(line, ":s1", theme.edge_false);
    out += line;
    if (line.replace(/^\s+/, "").startsWith("digraph ")) {
      out += preamble + "\n";
    }
  }
  return out;
}
