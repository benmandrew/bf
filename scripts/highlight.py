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

import argparse
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

# Two palettes, selected by --theme. `light` is GitHub's light syntax
# theme; `dark` is its dark counterpart. The CLI (scripts/cfg.sh) leaves
# the default at light; the web path renders dark to match the dark UI.
# The keys mirror the token names in RULES, plus the structural colours
# (plain text, node fill/border, graph background, edges).
LIGHT = {
    "colours": {
        "comment": "#6e7781",
        "string": "#0a3069",
        "boolean": "#0550ae",
        "variable": "#0550ae",
        "label": "#953800",
        "type": "#8250df",
        "keyword": "#cf222e",
        "number": "#0a3069",
        "punctuation": "#1f2328",
    },
    "plain": "#1f2328",
    "fill": "#f6f8fa",
    "border": "#d0d7de",
    "bg": "white",
    "graph_fontcolor": "#57606a",
    "edge": "#8c959f",
    "edge_true": "#1a7f37",
    "edge_false": "#cf222e",
}

DARK = {
    "colours": {
        "comment": "#8b949e",
        "string": "#a5d6ff",
        "boolean": "#79c0ff",
        "variable": "#79c0ff",
        "label": "#ffa657",
        "type": "#d2a8ff",
        "keyword": "#ff7b72",
        "number": "#79c0ff",
        "punctuation": "#c9d1d9",
    },
    "plain": "#c9d1d9",
    "fill": "#161b22",
    "border": "#30363d",
    "bg": "#0d1117",
    "graph_fontcolor": "#8b949e",
    "edge": "#6e7681",
    "edge_true": "#3fb950",
    "edge_false": "#f85149",
}

THEMES = {"light": LIGHT, "dark": DARK}


def build_preamble(theme):
    """Render the graph/node/edge default attribute block for a theme.

    Node text is Courier, not a nicer mono like DejaVu Sans Mono, because the
    browser build lays the graph out with the wasm Graphviz, which has no font
    files: it estimates text extents from its built-in PostScript AFM metrics
    and falls back to *proportional* Times for any name it does not know there.
    Courier is the one monospace font in that built-in set, so naming it keeps
    the layout monospace in the browser. Native dot (this CLI) resolves Courier
    to a real monospaced font via fontconfig, so it is unaffected. The web demo
    then renders with an embedded Courier-metric font (see web/style.css)."""
    return """\
\tgraph [bgcolor="%s", fontname="DejaVu Sans", fontsize=11,
\t       fontcolor="%s", nodesep=0.35, ranksep=0.45, pad=0.25];
\tnode [shape=plaintext, fontname="Courier", fontsize=11,
\t      fontcolor="%s"];
\tedge [color="%s", penwidth=1.1, arrowsize=0.7];""" % (
        theme["bg"],
        theme["graph_fontcolor"],
        theme["plain"],
        theme["edge"],
    )


# Active palette. apply_theme() repoints these from --theme; they default
# to light so importing the module or omitting the flag matches the CLI.
COLOURS = LIGHT["colours"]
PLAIN = LIGHT["plain"]
FILL = LIGHT["fill"]
BORDER = LIGHT["border"]
EDGE_TRUE = LIGHT["edge_true"]
EDGE_FALSE = LIGHT["edge_false"]
PREAMBLE = build_preamble(LIGHT)


def apply_theme(theme):
    """Point the module-level palette globals at the selected theme."""
    global COLOURS, PLAIN, FILL, BORDER, PREAMBLE, EDGE_TRUE, EDGE_FALSE
    COLOURS = theme["colours"]
    PLAIN = theme["plain"]
    FILL = theme["fill"]
    BORDER = theme["border"]
    EDGE_TRUE = theme["edge_true"]
    EDGE_FALSE = theme["edge_false"]
    PREAMBLE = build_preamble(theme)


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


def rejoin_wraps(lines):
    r"""Undo the line wrapping CFGPrinter applies to a record label.

    LLVM breaks a label at eighty columns by inserting a literal "\l..."
    at the last space, falling back to mid-token when there is no space
    to break on. A continuation therefore starts with exactly "..."
    followed by the text it displaced, and nothing bfc emits starts a
    line that way.

    The halves have to be put back together before tokenising. A wrap
    lands inside a quoted identifier often enough to matter -- block
    names carry Brainfuck source spans, which are long and need quoting
    -- and half of one leaves an unterminated quote, which defeats the
    string rule and gets the fragments read as ordinary code.
    """
    out = []
    for line in lines:
        if out and line.startswith("..."):
            out[-1] += line[3:]
        else:
            out.append(line)
    return out


def clean_header(field):
    """Strip the quoting opt puts round a block name in dot-cfg mode."""
    lines = [ln for ln in unescape(field).split("\n") if ln.strip()]
    text = "".join(rejoin_wraps(lines)).strip()
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
        lines = rejoin_wraps(lines)
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
    parser = argparse.ArgumentParser(
        description="Theme an LLVM dot-cfg graph, syntax-highlighting the IR."
    )
    parser.add_argument(
        "--theme",
        choices=sorted(THEMES),
        default="light",
        help="colour palette to render with (default: light)",
    )
    args = parser.parse_args()
    apply_theme(THEMES[args.theme])

    for line in sys.stdin:
        converted = convert(line)
        if converted is not None:
            sys.stdout.write(converted)
            continue
        # Colour branch edges by port: :s0 is the true successor, :s1 the
        # false one, which is what makes a loop back-edge legible.
        line = colour_edge(line, ":s0", EDGE_TRUE)
        line = colour_edge(line, ":s1", EDGE_FALSE)
        sys.stdout.write(line)
        if line.lstrip().startswith("digraph "):
            sys.stdout.write(PREAMBLE + "\n")


if __name__ == "__main__":
    main()
