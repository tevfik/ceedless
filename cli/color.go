// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"os"
)

var (
	colorOn = false
)

func init() {
	if os.Getenv("CEEDLESS_NO_COLOR") != "" || os.Getenv("NO_COLOR") != "" {
		return
	}
	fi, err := os.Stdout.Stat()
	if err == nil && (fi.Mode()&os.ModeCharDevice) != 0 {
		colorOn = true
	}
}

func cRed() string {
	if colorOn {
		return "\033[1;31m"
	}
	return ""
}
func cGrn() string {
	if colorOn {
		return "\033[1;32m"
	}
	return ""
}
func cYlw() string {
	if colorOn {
		return "\033[1;33m"
	}
	return ""
}
func cBlu() string {
	if colorOn {
		return "\033[1;34m"
	}
	return ""
}
func cDim() string {
	if colorOn {
		return "\033[2m"
	}
	return ""
}
func cRst() string {
	if colorOn {
		return "\033[0m"
	}
	return ""
}
