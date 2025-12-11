package main

import (
	"bytes"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
)

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
	programPath := os.Args[1]
	cmd := exec.Command(programPath)
	cmd.Stdin = bytes.NewReader(input)
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	err = cmd.Run()
	if err != nil {
		http.Error(w, "bfc error:\n"+stderr.String(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "text/plain")
	w.Write(stdout.Bytes())
}

func main() {
	http.HandleFunc("/compile", runHandler)
	log.Println("Listening on :8000")
	log.Fatal(http.ListenAndServe("0.0.0.0:8000", nil))
}
