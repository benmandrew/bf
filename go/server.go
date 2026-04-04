package main

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"time"
)

var cache map[string][]byte
var n_requests int64
var n_cache_hits int64
var compilation_duration_sum_seconds float64

var allowed_chars = map[rune]bool{
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
	return fmt.Sprintf("%s", e.message)
}

func sanitiseInput(input string) (string, error) {
	var filteredInput bytes.Buffer
	if len(input) > 10000 {
		return "", &bfError{message: "Input too long"}
	}
	loop_stack_depth := 0
	for i, char := range input {
		if !allowed_chars[char] {
			return "", &bfError{
				message: fmt.Sprintf("Invalid character %q at location %d", char, i+1),
			}
		}
		filteredInput.WriteRune(char)
		switch {
		case char == '[':
			loop_stack_depth++
		case char == ']':
			loop_stack_depth--
		}
	}
	if loop_stack_depth != 0 {
		return "", &bfError{message: "Mismatched loops"}
	}
	return filteredInput.String(), nil
}

func runHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusOK)
		return
	}
	if r.Method != http.MethodGet {
		http.Error(w, "GET required", http.StatusMethodNotAllowed)
		return
	}
	input := []byte(r.URL.Query().Get("code"))
	str_input, err := sanitiseInput(string(input))
	if err != nil {
		http.Error(w, "Invalid input: "+err.Error(), http.StatusBadRequest)
		return
	}
	n_requests++
	w.Header().Set("Content-Type", "text/plain")
	if output, found := cache[str_input]; found {
		n_cache_hits++
		log.Printf("Cache hit, total requests: %d, cache hits: %d", n_requests, n_cache_hits+1)
		w.Write(output)
		return
	}
	programPath := os.Args[1]
	cmd := exec.Command(programPath)
	cmd.Stdin = bytes.NewReader([]byte(str_input))
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	start := time.Now()
	err = cmd.Run()
	compilation_duration_sum_seconds += time.Since(start).Seconds()
	if err != nil {
		log.Printf("Error: %v, stderr: %s", err, stderr.String())
		http.Error(w, stderr.String(), http.StatusBadRequest)
		return
	}
	w.Write(stdout.Bytes())
	cache[string(input)] = stdout.Bytes()
}

func metricsHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	compilations := n_requests - n_cache_hits
	meanCompilationDuration := 0.0
	if compilations > 0 {
		meanCompilationDuration = compilation_duration_sum_seconds / float64(compilations)
	}
	fmt.Fprintf(w, "# HELP bfc_requests_total Total number of bfc execution requests\n")
	fmt.Fprintf(w, "# TYPE bfc_requests_total counter\n")
	fmt.Fprintf(w, "bfc_requests_total %d\n", n_requests)
	fmt.Fprintf(w, "# HELP bfc_cache_hits_total Total number of bfc cache hits\n")
	fmt.Fprintf(w, "# TYPE bfc_cache_hits_total counter\n")
	fmt.Fprintf(w, "bfc_cache_hits_total %d\n", n_cache_hits)
	fmt.Fprintf(w, "# HELP bfc_compilations_total Total number of bfc compilations (cache misses)\n")
	fmt.Fprintf(w, "# TYPE bfc_compilations_total counter\n")
	fmt.Fprintf(w, "bfc_compilations_total %d\n", compilations)
	fmt.Fprintf(w, "# HELP bfc_compilation_duration_seconds_count Total number of timed bfc compilations\n")
	fmt.Fprintf(w, "# TYPE bfc_compilation_duration_seconds_count counter\n")
	fmt.Fprintf(w, "bfc_compilation_duration_seconds_count %d\n", compilations)
	fmt.Fprintf(w, "# HELP bfc_compilation_duration_seconds_mean Mean bfc compilation duration in seconds\n")
	fmt.Fprintf(w, "# TYPE bfc_compilation_duration_seconds_mean gauge\n")
	fmt.Fprintf(w, "bfc_compilation_duration_seconds_mean %f\n", meanCompilationDuration)
}

func main() {
	cache = make(map[string][]byte)
	n_requests = 0
	n_cache_hits = 0
	compilation_duration_sum_seconds = 0
	http.HandleFunc("/", runHandler)
	http.HandleFunc("/metrics", metricsHandler)
	log.Println("Listening on :8000 at /")
	log.Println("Prometheus metrics available at /metrics")
	log.Fatal(http.ListenAndServe("0.0.0.0:8000", nil))
}
