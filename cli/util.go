// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
)

// homeDir returns the directory holding include/ and src/ of the framework.
// Resolution order:
//  1. $CEEDLESS_HOME if set and contains include/ceedless/
//  2. ../<parent of exe> if it contains include/ceedless/   (./bin/ceedless)
//  3. /opt/ceedless                                          (docker default)
//  4. error
func homeDir() (string, error) {
	if h := os.Getenv("CEEDLESS_HOME"); h != "" {
		if exists(filepath.Join(h, "include", "ceedless")) {
			return h, nil
		}
	}
	if exe, err := os.Executable(); err == nil {
		if real, err := filepath.EvalSymlinks(exe); err == nil {
			cand := filepath.Dir(filepath.Dir(real))
			if exists(filepath.Join(cand, "include", "ceedless")) {
				return cand, nil
			}
		}
	}
	if exists("/opt/ceedless/include/ceedless") {
		return "/opt/ceedless", nil
	}
	return "", fmt.Errorf("cannot locate ceedless framework (set CEEDLESS_HOME)")
}

func exists(p string) bool { _, err := os.Stat(p); return err == nil }

func runCmd(name string, args ...string) error {
	c := exec.Command(name, args...)
	c.Stdout = os.Stdout
	c.Stderr = os.Stderr
	return c.Run()
}

func info(format string, a ...any) {
	fmt.Printf("%sceedless%s: %s\n", cBlu(), cRst(), fmt.Sprintf(format, a...))
}

func warn(format string, a ...any) {
	fmt.Fprintf(os.Stderr, "%sceedless%s: %s\n", cYlw(), cRst(), fmt.Sprintf(format, a...))
}
