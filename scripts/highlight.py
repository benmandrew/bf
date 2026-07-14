#!/usr/bin/env python3
r"""Theme an LLVM dot-cfg graph, syntax-highlighting the IR.

Reads a .dot file produced by opt -passes=dot-cfg[-only] on stdin and
writes a themed .dot to stdout. LLVM emits each block as a record label:

    {\"entry ,\>+\>+\<\<\":\l|  %dp = alloca i32, align 4\l|{<s0>T|<s1>F}}

Record labels cannot carry per-token colour, so each is rewritten into a
Graphviz HTML-like label: a table whose body rows are coloured token by
token, and whose final row keeps the <s0>/<s1> ports the branch edges
attach to.

The token rules are a port of Prism's LLVM grammar (components/
prism-llvm.js, MIT), so that graphs highlight the same way as LLVM IR
rendered by Prism elsewhere. Rule order is significant: it encodes
priority, exactly as the ordering of keys in the Prism grammar object
does. One deliberate divergence is noted at RULES below.
"""

import html
import re
import sys

# Prism's grammar, in Prism's order. The one addition is `ptr` in `type`:
# Prism's LLVM component predates opaque pointers, so it has no rule for
# `ptr` and its catch-all `keyword` claims it, colouring it as though it
# were `store` or `align`. bfc emits opaque pointers throughout, so
# without this every `ptr` in the graph would read as a keyword.
RULES = [
    ("comment", r";.*"),
    ("string", r'"[^"]*"'),
    ("boolean", r"\b(?:false|true)\b"),
    ("variable", r"[%@!#](?:(?!\d)(?:[-$.\w]|\\[a-fA-F\d]{2})+|\d+)"),
    ("label", r"(?!\d)(?:[-$.\w]|\\[a-fA-F\d]{2})+:"),
    (
        "type",
        r"\b(?:double|float|fp128|half|i[1-9]\d*|label|metadata"
        r"|ppc_fp128|ptr|token|void|x86_fp80|x86_mmx)\b",
    ),
    ("keyword", r"\b[a-z_][a-z_0-9]*\b"),
    (
        "number",
        r"[+-]?\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b|\b0x[\dA-Fa-f]+\b"
        r"|\b0xK[\dA-Fa-f]{20}\b|\b0x[ML][\dA-Fa-f]{32}\b"
        r"|\b0xH[\dA-Fa-f]{4}\b",
    ),
    ("punctuation", r"[{}[\];(),.!*=<>]"),
]

# Alternation is tried left to right at each position, so listing the
# rules in Prism's order preserves Prism's priority. This scans leftmost
# first, whereas Prism applies each rule across the whole string before
# moving to the next; for a grammar this flat the two agree, and the IR
# bfc emits has no construct that separates them.
SCANNER = re.compile("|".join("(?P<%s>%s)" % (name, pat) for name, pat in RULES))

COLOURS = {
    "comment": "#6e7781",
    "string": "#0a3069",
    "boolean": "#0550ae",
    "variable": "#0550ae",
    "label": "#953800",
    "type": "#8250df",
    "keyword": "#cf222e",
    "number": "#0a3069",
    "punctuation": "#1f2328",
}

PLAIN = "#1f2328"
FILL = "#f6f8fa"
BORDER = "#d0d7de"

PREAMBLE = (
    """\
\tgraph [bgcolor="white", fontname="DejaVu Sans", fontsize=11,
\t       fontcolor="#57606a", nodesep=0.35, ranksep=0.45, pad=0.25];
\tnode [shape=plaintext, fontname="DejaVu Sans Mono", fontsize=11,
\t      fontcolor="%s"];
\tedge [color="#8c959f", penwidth=1.1, arrowsize=0.7];"""
    % PLAIN
)

NODE_RE = re.compile(
    r"^(\s*)(Node0x[0-9a-f]+)\s*\[shape=record,\s*"
    r'label="(.*)"\];\s*$'
)
PORT_RE = re.compile(r"^<(s\d+)>(.*)$", re.DOTALL)


def split_top(s):
    """Split a record label body on unescaped, top-level '|'."""
    parts, depth, cur, i = [], 0, [], 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            cur.append(s[i : i + 2])
            i += 2
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif c == "|" and depth == 0:
            parts.append("".join(cur))
            cur = []
            i += 1
            continue
        cur.append(c)
        i += 1
    parts.append("".join(cur))
    return parts


def unescape(s):
    """Undo DOT record-label escaping. \\l, \\n and \\r become newlines."""
    out, i = [], 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            out.append("\n" if nxt in "lnr" else nxt)
            i += 2
        else:
            out.append(s[i])
            i += 1
    return "".join(out)


def colourise(line):
    """Tokenise a line of LLVM IR into coloured HTML-like spans."""
    out, pos = [], 0
    for m in SCANNER.finditer(line):
        if m.start() > pos:
            out.append(html.escape(line[pos : m.start()]))
        colour = COLOURS.get(m.lastgroup or "", PLAIN)
        out.append('<FONT COLOR="%s">%s</FONT>' % (colour, html.escape(m.group())))
        pos = m.end()
    if pos < len(line):
        out.append(html.escape(line[pos:]))
    return "".join(out)


def clean_header(field):
    """Strip the quoting opt puts round a block name in dot-cfg mode."""
    text = unescape(field).strip()
    if text.endswith(":"):
        text = text[:-1]
    if len(text) > 1 and text.startswith('"') and text.endswith('"'):
        text = text[1:-1]
    return text


def parse_ports(field):
    """Parse a '{<s0>T|<s1>F}' field into [(port, text), ...] or None."""
    text = field.strip()
    if not (text.startswith("{") and text.endswith("}")):
        return None
    cells = []
    for cell in split_top(text[1:-1]):
        m = PORT_RE.match(cell)
        if not m:
            return None
        cells.append((m.group(1), unescape(m.group(2)).strip()))
    return cells or None


def build_label(fields):
    """Render parsed record fields as an HTML-like table label."""
    ports = parse_ports(fields[-1]) if len(fields) > 1 else None
    body = fields[1:-1] if ports else fields[1:]
    span = len(ports) if ports else 1

    rows = [
        '<TR><TD ALIGN="LEFT" COLSPAN="%d">%s</TD></TR>'
        % (span, html.escape(clean_header(fields[0])))
    ]

    for field in body:
        lines = [ln for ln in unescape(field).split("\n") if ln.strip()]
        if not lines:
            continue
        rows.append("<HR/>")
        cell = "<BR/>".join(colourise(ln) for ln in lines)
        rows.append(
            '<TR><TD ALIGN="LEFT" BALIGN="LEFT" COLSPAN="%d">'
            "%s<BR/></TD></TR>" % (span, cell)
        )

    if ports:
        rows.append("<HR/>")
        cells = "<VR/>".join(
            '<TD PORT="%s">%s</TD>' % (port, html.escape(text)) for port, text in ports
        )
        rows.append("<TR>%s</TR>" % cells)

    table = (
        '<TABLE BORDER="1" CELLBORDER="0" CELLSPACING="0" '
        'CELLPADDING="4" BGCOLOR="%s" COLOR="%s" STYLE="ROUNDED">'
        "%s</TABLE>" % (FILL, BORDER, "".join(rows))
    )
    return "<%s>" % table


def convert(line):
    m = NODE_RE.match(line)
    if not m:
        return None
    indent, name, label = m.groups()
    body = label.strip()
    if not (body.startswith("{") and body.endswith("}")):
        return None
    fields = split_top(body[1:-1])
    return "%s%s [label=%s];\n" % (indent, name, build_label(fields))


def colour_edge(line, port, colour):
    """Colour a `Node:sN -> Node` branch edge, merging into any existing
    attribute list. Some LLVM versions attach edge attributes (e.g. a
    probability tooltip), so the colour cannot assume a bare edge."""

    def repl(m):
        edge, attrs = m.group(1), m.group(2)
        if attrs:
            return '%s [color="%s", %s];' % (edge, colour, attrs[1:-1].strip())
        return '%s [color="%s"];' % (edge, colour)

    pattern = r"(%s -> Node0x[0-9a-f]+)\s*(\[[^\]]*\])?;" % re.escape(port)
    return re.sub(pattern, repl, line)


def main():
    for line in sys.stdin:
        converted = convert(line)
        if converted is not None:
            sys.stdout.write(converted)
            continue
        # Colour branch edges by port: :s0 is the true successor, :s1 the
        # false one, which is what makes a loop back-edge legible.
        line = colour_edge(line, ":s0", "#1a7f37")
        line = colour_edge(line, ":s1", "#cf222e")
        sys.stdout.write(line)
        if line.lstrip().startswith("digraph "):
            sys.stdout.write(PREAMBLE + "\n")


if __name__ == "__main__":
    main()
