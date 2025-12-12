package main

import (
	"bytes"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
)

var cache map[string][]byte
var n_requests int64
var n_cache_hits int64

func runHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "POST, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusOK)
		return
	}
	if r.Method != http.MethodPost {
		http.Error(w, "POST required", http.StatusMethodNotAllowed)
		return
	}
	input, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Failed reading input", http.StatusBadRequest)
		return
	}
	n_requests++
	w.Header().Set("Content-Type", "text/plain")
	if output, found := cache[string(input)]; found {
		n_cache_hits++
		log.Printf("Cache hit, total requests: %d, cache hits: %d", n_requests, n_cache_hits+1)
		w.Write(output)
		return
	}
	programPath := os.Args[1]
	cmd := exec.Command(programPath)
	cmd.Stdin = bytes.NewReader(input)
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	err = cmd.Run()
	if err != nil {
		log.Printf("Error: %v, stderr: %s", err, stderr.String())
		http.Error(w, stderr.String(), http.StatusBadRequest)
		return
	}
	w.Write(stdout.Bytes())
	cache[string(input)] = stdout.Bytes()
}

func main() {
	cache = make(map[string][]byte)
	n_requests = 0
	n_cache_hits = 0
	http.HandleFunc("/compile", runHandler)
	log.Println("Listening on :8000")
	log.Fatal(http.ListenAndServe("0.0.0.0:8000", nil))
}
