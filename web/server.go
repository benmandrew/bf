package main

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"regexp"
	"time"
)

var cache map[string][]byte
var cfgCache map[string][]byte
var nRequests int64
var nCacheHits int64
var nCfgRequests int64
var nCfgCacheHits int64
var compilationDurationSumSeconds float64

// Set from the command line in main: the bfc binary and highlight.py.
var bfcPath string
var highlightPath string

var allowedChars = map[rune]bool{
	'>':  true,
	'<':  true,
	'+':  true,
	'-':  true,
	'.':  true,
	',':  true,
	'[':  true,
	']':  true,
	'\n': true,
	' ':  true,
}

type bfError struct {
	message string
}

func (e *bfError) Error() string {
	return e.message
}

func sanitiseInput(input string) (string, error) {
	var filteredInput bytes.Buffer
	if len(input) > 10000 {
		return "", &bfError{message: "Input too long"}
	}

	loopStackDepth := 0
	for i, char := range input {
		if !allowedChars[char] {
			return "", &bfError{
				message: fmt.Sprintf("Invalid character %q at location %d", char, i+1),
			}
		}
		filteredInput.WriteRune(char)
		switch char {
		case '[':
			loopStackDepth++
		case ']':
			loopStackDepth--
		}
	}

	if loopStackDepth != 0 {
		return "", &bfError{message: "Mismatched loops"}
	}
	return filteredInput.String(), nil
}

// corsAndSanitise applies the shared CORS headers, handles the OPTIONS
// preflight and method check, and returns the sanitised program. The bool
// is false when the request has already been fully answered (preflight or
// error) and the handler should return.
func corsAndSanitise(w http.ResponseWriter, r *http.Request) (string, bool) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusOK)
		return "", false
	}
	if r.Method != http.MethodGet {
		http.Error(w, "GET required", http.StatusMethodNotAllowed)
		return "", false
	}
	strInput, err := sanitiseInput(r.URL.Query().Get("code"))
	if err != nil {
		http.Error(w, "Invalid input: "+err.Error(), http.StatusBadRequest)
		return "", false
	}
	return strInput, true
}

func runHandler(w http.ResponseWriter, r *http.Request) {
	strInput, ok := corsAndSanitise(w, r)
	if !ok {
		return
	}

	nRequests++
	w.Header().Set("Content-Type", "text/plain")
	if output, found := cache[strInput]; found {
		nCacheHits++
		log.Printf("Cache hit, total requests: %d, cache hits: %d", nRequests, nCacheHits)
		w.Write(output)
		return
	}

	cmd := exec.Command(bfcPath)
	cmd.Stdin = bytes.NewReader([]byte(strInput))

	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	start := time.Now()
	err := cmd.Run()
	compilationDurationSumSeconds += time.Since(start).Seconds()
	if err != nil {
		log.Printf("Error: %v, stderr: %s", err, stderr.String())
		http.Error(w, stderr.String(), http.StatusBadRequest)
		return
	}

	w.Write(stdout.Bytes())
	cache[strInput] = stdout.Bytes()
}

// runStage feeds stdin to cmd and returns its stdout, or an error carrying
// the command's stderr for logging.
func runStage(cmd *exec.Cmd, stdin []byte) ([]byte, error) {
	cmd.Stdin = bytes.NewReader(stdin)
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	if err := cmd.Run(); err != nil {
		return nil, fmt.Errorf("%s: %w: %s", cmd.Path, err, stderr.String())
	}
	return stdout.Bytes(), nil
}

// Defence in depth for inlining the SVG into the page: Graphviz does not
// emit any of these for a dot-cfg graph, and the program is already
// restricted to Brainfuck characters with its labels HTML-escaped by
// highlight.py, but strip active content anyway.
var svgScriptRe = regexp.MustCompile(`(?is)<script.*?</script\s*>`)
var svgForeignRe = regexp.MustCompile(`(?is)<foreignObject.*?</foreignObject\s*>`)
var svgHandlerRe = regexp.MustCompile(`(?i)\son[a-z]+\s*=\s*("[^"]*"|'[^']*')`)

func sanitiseSVG(svg []byte) []byte {
	svg = svgScriptRe.ReplaceAll(svg, nil)
	svg = svgForeignRe.ReplaceAll(svg, nil)
	svg = svgHandlerRe.ReplaceAll(svg, nil)
	return svg
}

// cfgTheme picks the highlight.py palette from the ?theme= query. The web
// demo omits it and defaults to dark, to match the dark UI; the light site
// embed passes theme=light. Anything else falls back to dark.
func cfgTheme(r *http.Request) string {
	if r.URL.Query().Get("theme") == "light" {
		return "light"
	}
	return "dark"
}

// renderCFG runs the control-flow-graph pipeline for a program:
// bfc --emit-cfg-dot -> highlight.py -> dot -Tsvg. --cfg-instructions puts
// each block's LLVM IR in its node, which highlight.py syntax-highlights
// with the given theme (light or dark).
func renderCFG(code, theme string) ([]byte, error) {
	dot, err := runStage(
		exec.Command(bfcPath, "--emit-cfg-dot", "--cfg-instructions",
			"--label-blocks"),
		[]byte(code))
	if err != nil {
		return nil, err
	}
	themed, err := runStage(
		exec.Command("python3", highlightPath, "--theme", theme), dot)
	if err != nil {
		return nil, err
	}
	svg, err := runStage(exec.Command("dot", "-Tsvg"), themed)
	if err != nil {
		return nil, err
	}
	return sanitiseSVG(svg), nil
}

func cfgHandler(w http.ResponseWriter, r *http.Request) {
	strInput, ok := corsAndSanitise(w, r)
	if !ok {
		return
	}

	nCfgRequests++
	w.Header().Set("Content-Type", "image/svg+xml")
	// Key the cache by theme too: the same program renders differently
	// under the light and dark palettes.
	theme := cfgTheme(r)
	cacheKey := theme + "\x00" + strInput
	if svg, found := cfgCache[cacheKey]; found {
		nCfgCacheHits++
		w.Write(svg)
		return
	}

	svg, err := renderCFG(strInput, theme)
	if err != nil {
		log.Printf("CFG error: %v", err)
		http.Error(w, "CFG generation failed", http.StatusBadRequest)
		return
	}

	w.Write(svg)
	cfgCache[cacheKey] = svg
}

func metricsHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	compilations := nRequests - nCacheHits
	meanCompilationDuration := 0.0
	if compilations > 0 {
		meanCompilationDuration = compilationDurationSumSeconds / float64(compilations)
	}

	fmt.Fprintf(w, "# HELP bfc_requests_total Total number of bfc execution requests\n")
	fmt.Fprintf(w, "# TYPE bfc_requests_total counter\n")
	fmt.Fprintf(w, "bfc_requests_total %d\n", nRequests)
	fmt.Fprintf(w, "# HELP bfc_cache_hits_total Total number of bfc cache hits\n")
	fmt.Fprintf(w, "# TYPE bfc_cache_hits_total counter\n")
	fmt.Fprintf(w, "bfc_cache_hits_total %d\n", nCacheHits)
	fmt.Fprintf(w, "# HELP bfc_compilations_total Total number of bfc compilations (cache misses)\n")
	fmt.Fprintf(w, "# TYPE bfc_compilations_total counter\n")
	fmt.Fprintf(w, "bfc_compilations_total %d\n", compilations)
	fmt.Fprintf(w, "# HELP bfc_compilation_duration_seconds_count Total number of timed bfc compilations\n")
	fmt.Fprintf(w, "# TYPE bfc_compilation_duration_seconds_count counter\n")
	fmt.Fprintf(w, "bfc_compilation_duration_seconds_count %d\n", compilations)
	fmt.Fprintf(w, "# HELP bfc_compilation_duration_seconds_mean Mean bfc compilation duration in seconds\n")
	fmt.Fprintf(w, "# TYPE bfc_compilation_duration_seconds_mean gauge\n")
	fmt.Fprintf(w, "bfc_compilation_duration_seconds_mean %f\n", meanCompilationDuration)
	fmt.Fprintf(w, "# HELP bfc_cfg_requests_total Total number of CFG render requests\n")
	fmt.Fprintf(w, "# TYPE bfc_cfg_requests_total counter\n")
	fmt.Fprintf(w, "bfc_cfg_requests_total %d\n", nCfgRequests)
	fmt.Fprintf(w, "# HELP bfc_cfg_cache_hits_total Total number of CFG cache hits\n")
	fmt.Fprintf(w, "# TYPE bfc_cfg_cache_hits_total counter\n")
	fmt.Fprintf(w, "bfc_cfg_cache_hits_total %d\n", nCfgCacheHits)
}

func main() {
	if len(os.Args) < 3 {
		log.Fatalf("usage: %s <bfc-path> <highlight.py-path>", os.Args[0])
	}
	bfcPath = os.Args[1]
	highlightPath = os.Args[2]

	cache = make(map[string][]byte)
	cfgCache = make(map[string][]byte)

	http.HandleFunc("/", runHandler)
	http.HandleFunc("/cfg", cfgHandler)
	http.HandleFunc("/metrics", metricsHandler)
	log.Println("Listening on :8000 at / (IR) and /cfg (control-flow graph)")
	log.Println("Prometheus metrics available at /metrics")
	log.Fatal(http.ListenAndServe("0.0.0.0:8000", nil))
}
