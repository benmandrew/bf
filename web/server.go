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
	http.HandleFunc("/run", runHandler)
	log.Println("Listening on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
