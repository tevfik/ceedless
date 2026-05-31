// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

// coreSources returns the absolute paths of all framework C sources.
func coreSources(home string) []string {
	rel := []string{
		"src/runner.c",
		"src/exception.c",
		"src/mock.c",
		"src/virtual_peripheral.c",
		"src/trace/trace_port.c",
		"src/trace/trace_host.c",
		"src/trace/trace_uart.c",
		"src/trace/trace_rtt.c",
		"src/trace/trace_itm.c",
		"src/trace/trace_buffer.c",
		"src/peripherals/vp_spi.c",
		"src/peripherals/vp_uart.c",
		"src/peripherals/vp_gpio.c",
		"src/peripherals/vp_adc.c",
	}
	out := make([]string, 0, len(rel))
	for _, r := range rel {
		p := filepath.Join(home, r)
		if exists(p) {
			out = append(out, p)
		}
	}
	return out
}

// DiscoverTests returns all test_*.c files under cfg.TestDirs, sorted.
func DiscoverTests(cfg *Config, filter string) ([]string, error) {
	var tests []string
	for _, d := range cfg.TestDirs {
		if !exists(d) {
			continue
		}
		err := filepath.Walk(d, func(p string, fi os.FileInfo, err error) error {
			if err != nil || fi.IsDir() {
				return nil
			}
			b := filepath.Base(p)
			if !strings.HasPrefix(b, "test_") || !strings.HasSuffix(b, ".c") {
				return nil
			}
			if filter != "" && !strings.Contains(b, filter) {
				return nil
			}
			tests = append(tests, p)
			return nil
		})
		if err != nil {
			return nil, err
		}
	}
	sort.Strings(tests)
	return tests, nil
}

var (
	reTestFn   = regexp.MustCompile(`^\s*void\s+(test_[A-Za-z0-9_]+)\s*\(\s*void\s*\)`)
	reTestFnP  = regexp.MustCompile(`^\s*(?:static\s+)?void\s+(test_[A-Za-z0-9_]+)\s*\(([^)]+)\)`)
	reSrcAnnot = regexp.MustCompile(`TEST_SOURCE_FILE\s*\(\s*"([^"]+)"\s*\)`)
	reTestCase = regexp.MustCompile(`^\s*TEST_CASE\s*\((.+)\)\s*$`)
)

// testEntry describes one runnable test discovered in a source file.
type testEntry struct {
	name   string // C function name
	label  string // printable label ("" → defaults to function name)
	args   string // raw argument expression for RUN_TEST_CASE ("" → use RUN_TEST)
	params string // captured C parameter list, used for forward declarations
}

// ScanTest returns the list of test_* function names and TEST_SOURCE_FILE
// annotations found in a single test source file.
//
// Legacy callers receive bare function names. ScanTestEntries below returns
// the richer parametric-case data for the auto-generated runner.
func ScanTest(path string) (tests, extras []string, err error) {
	entries, ex, err := ScanTestEntries(path)
	if err != nil {
		return nil, nil, err
	}
	seen := map[string]bool{}
	for _, e := range entries {
		if !seen[e.name] {
			seen[e.name] = true
			tests = append(tests, e.name)
		}
	}
	return tests, ex, nil
}

// ScanTestEntries returns one entry per test invocation. A bare
// `void test_x(void) {…}` becomes a single RUN_TEST entry. Each preceding
// `TEST_CASE(args…)` line attaches a parametric RUN_TEST_CASE invocation
// to the next parametric-signature test function.
func ScanTestEntries(path string) (entries []testEntry, extras []string, err error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, nil, err
	}
	defer f.Close()
	scan := bufio.NewScanner(f)
	scan.Buffer(make([]byte, 65536), 1<<20)

	var pending []string // queued TEST_CASE(...) args, applied to next test fn
	caseCounter := 0
	for scan.Scan() {
		line := scan.Text()
		if m := reTestCase.FindStringSubmatch(line); m != nil {
			pending = append(pending, strings.TrimSpace(m[1]))
			continue
		}
		// parametric form: `void test_x(int a, int b, int e)`
		if m := reTestFnP.FindStringSubmatch(line); m != nil && len(pending) > 0 {
			name := m[1]
			params := strings.TrimSpace(m[2])
			for _, args := range pending {
				caseCounter++
				label := fmt.Sprintf("%s_case_%d", name, caseCounter)
				entries = append(entries, testEntry{name: name, label: label, args: args, params: params})
			}
			pending = nil
			continue
		}
		// no-arg form: `void test_x(void)`
		if m := reTestFn.FindStringSubmatch(line); m != nil {
			entries = append(entries, testEntry{name: m[1]})
			pending = nil
			continue
		}
		for _, m := range reSrcAnnot.FindAllStringSubmatch(line, -1) {
			extras = append(extras, m[1])
		}
	}
	return entries, extras, scan.Err()
}

// SynthRunner writes a generated _runner.c containing main() and RUN_TEST
// / RUN_TEST_CASE calls for every test_* / TEST_CASE discovered in srcTest.
//
// The test source is #include'd directly so that `static` parametric test
// functions are visible from the runner without forward declarations.
func SynthRunner(srcTest, outPath string) error {
	entries, _, err := ScanTestEntries(srcTest)
	if err != nil {
		return err
	}
	stem := strings.TrimSuffix(filepath.Base(srcTest), ".c")
	// Compute relative path from runner location to the test source so the
	// #include works regardless of where the runner is dropped.
	absTest, _ := filepath.Abs(srcTest)
	absRunnerDir, _ := filepath.Abs(filepath.Dir(outPath))
	rel, err := filepath.Rel(absRunnerDir, absTest)
	if err != nil {
		rel = absTest
	}
	var b strings.Builder
	b.WriteString("/* AUTO-GENERATED BY ceedless — DO NOT EDIT */\n")
	b.WriteString("#include \"ceedless/ceedless.h\"\n")
	b.WriteString("#include <stdlib.h>\n")
	fmt.Fprintf(&b, "#include %q\n\n", rel)

	b.WriteString("int main(void) {\n")
	fmt.Fprintf(&b, "    ceedless_begin(%q);\n", stem)
	b.WriteString("    const char *jx = getenv(\"CEEDLESS_JUNIT\");\n")
	b.WriteString("    if (jx) ceedless_set_junit_path(jx);\n")
	for _, e := range entries {
		if e.args == "" {
			fmt.Fprintf(&b, "    RUN_TEST(%s);\n", e.name)
		} else {
			fmt.Fprintf(&b, "    RUN_TEST_CASE(%s, %q, %s);\n", e.name, e.label, e.args)
		}
	}
	b.WriteString("    return ceedless_end();\n}\n")
	if err := os.MkdirAll(filepath.Dir(outPath), 0o755); err != nil {
		return err
	}
	return os.WriteFile(outPath, []byte(b.String()), 0o644)
}
