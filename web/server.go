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
var nRequests int64
var nCacheHits int64
var compilationDurationSumSeconds float64

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
	strInput, err := sanitiseInput(string(input))
	if err != nil {
		http.Error(w, "Invalid input: "+err.Error(), http.StatusBadRequest)
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

	programPath := os.Args[1]
	cmd := exec.Command(programPath)
	cmd.Stdin = bytes.NewReader([]byte(strInput))

	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	start := time.Now()
	err = cmd.Run()
	compilationDurationSumSeconds += time.Since(start).Seconds()
	if err != nil {
		log.Printf("Error: %v, stderr: %s", err, stderr.String())
		http.Error(w, stderr.String(), http.StatusBadRequest)
		return
	}

	w.Write(stdout.Bytes())
	cache[strInput] = stdout.Bytes()
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
}

func main() {
	cache = make(map[string][]byte)
	nRequests = 0
	nCacheHits = 0
	compilationDurationSumSeconds = 0

	http.HandleFunc("/", runHandler)
	http.HandleFunc("/metrics", metricsHandler)
	log.Println("Listening on :8000 at /")
	log.Println("Prometheus metrics available at /metrics")
	log.Fatal(http.ListenAndServe("0.0.0.0:8000", nil))
}
