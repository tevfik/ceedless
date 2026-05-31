// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
//
// `ceedless fuzz` — libFuzzer harness generator.
//
// Given a header function signature like
//
//	int parse_packet(const uint8_t *buf, size_t len, packet_t *out);
//
// `ceedless fuzz parse_packet path/to/header.h` emits build/fuzz/<func>_fuzz.c
// containing a `LLVMFuzzerTestOneInput` shim that calls the target function
// with the libFuzzer-provided buffer. The harness is then compiled and linked
// with clang's `-fsanitize=fuzzer,address` against the target's sources.
//
// Only functions whose first two arguments match
//
//	(const uint8_t *, size_t)
//
// are auto-shimmed. For other signatures, use --raw to skip arg parsing and
// edit the generated stub by hand.
package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strings"
	"time"
)

func cmdFuzz(args []string) error {
	fs := flag.NewFlagSet("fuzz", flag.ContinueOnError)
	var (
		header   string
		corpus   string
		runtime0 int
		jobs     int
		raw      bool
		seed     int
		extra    string
	)
	fs.StringVar(&header, "h", "", "header file declaring the target function")
	fs.StringVar(&corpus, "corpus", "build/fuzz/corpus", "corpus directory")
	fs.IntVar(&runtime0, "max-time", 30, "fuzz duration in seconds (0=until crash)")
	fs.IntVar(&jobs, "jobs", 1, "parallel fuzzing jobs")
	fs.BoolVar(&raw, "raw", false, "emit empty harness without arg auto-detection")
	fs.IntVar(&seed, "seed", 0, "libFuzzer -seed value (0 = random)")
	fs.StringVar(&extra, "extra-cflags", "", "extra clang flags appended verbatim")
	_ = fs.Parse(args)
	rest := fs.Args()
	if len(rest) < 1 {
		return fmt.Errorf("usage: ceedless fuzz <target_function> -h header.h [opts]")
	}
	target := rest[0]
	if header == "" {
		return fmt.Errorf("-h <header> is required so the harness can declare the target")
	}

	sig, err := findSignature(header, target)
	if err != nil && !raw {
		return err
	}

	cfg, err := LoadConfig("project.yml")
	if err != nil {
		return err
	}
	home, err := homeDir()
	if err != nil {
		return err
	}
	if err := os.MkdirAll("build/fuzz", 0o755); err != nil {
		return err
	}
	if err := os.MkdirAll(corpus, 0o755); err != nil {
		return err
	}

	stub := filepath.Join("build/fuzz", target+"_fuzz.c")
	if err := writeFuzzHarness(stub, header, target, sig, raw); err != nil {
		return err
	}
	info("wrote harness %s", stub)

	cc := envOr("CC", "clang")
	if !strings.Contains(cc, "clang") {
		info("note: libFuzzer requires clang; CC=%s may not link", cc)
	}

	out := filepath.Join("build/fuzz", target)
	flags := []string{"-fsanitize=fuzzer,address,undefined", "-g", "-O1"}
	flags = append(flags, cfg.CFlags...)
	for _, d := range cfg.Defines {
		flags = append(flags, "-D"+d)
	}
	for _, p := range cfg.IncludeDirs {
		flags = append(flags, "-I"+p)
	}
	flags = append(flags, "-I"+filepath.Join(home, "include"))

	sources := []string{stub}
	sources = append(sources, cfg.ExtraFiles...)
	for _, sd := range cfg.SrcDirs {
		filepath.Walk(sd, func(p string, fi os.FileInfo, e error) error {
			if e != nil || fi.IsDir() || !strings.HasSuffix(p, ".c") {
				return nil
			}
			sources = append(sources, p)
			return nil
		})
	}

	if extra != "" {
		flags = append(flags, strings.Fields(extra)...)
	}
	args2 := append([]string{}, flags...)
	args2 = append(args2, sources...)
	args2 = append(args2, "-o", out)
	info("compiling fuzz target: %s", out)
	if err := runCmd(cc, args2...); err != nil {
		return err
	}

	runArgs := []string{corpus, "-print_final_stats=1"}
	if runtime0 > 0 {
		runArgs = append(runArgs, fmt.Sprintf("-max_total_time=%d", runtime0))
	}
	if jobs > 1 {
		runArgs = append(runArgs, fmt.Sprintf("-jobs=%d", jobs),
			fmt.Sprintf("-workers=%d", jobs))
	}
	if seed != 0 {
		runArgs = append(runArgs, fmt.Sprintf("-seed=%d", seed))
	}
	info("running fuzzer for %ds (Ctrl+C to stop early)", runtime0)
	t0 := time.Now()
	cmd := exec.Command(out, runArgs...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	err = cmd.Run()
	info("fuzz session finished in %v", time.Since(t0).Round(time.Second))
	return err
}

// findSignature extracts a function prototype from header for target.
func findSignature(header, target string) (string, error) {
	f, err := os.Open(header)
	if err != nil {
		return "", err
	}
	defer f.Close()
	scan := bufio.NewScanner(f)
	scan.Buffer(make([]byte, 65536), 1<<20)
	// match "...space target ("    over up to ~10 lines
	re := regexp.MustCompile(`\b` + regexp.QuoteMeta(target) + `\s*\(([^)]*)\)`)
	var blob strings.Builder
	for scan.Scan() {
		blob.WriteString(scan.Text())
		blob.WriteString(" ")
	}
	m := re.FindStringSubmatch(blob.String())
	if m == nil {
		return "", fmt.Errorf("could not find prototype for %s in %s", target, header)
	}
	return strings.TrimSpace(m[1]), nil
}

// writeFuzzHarness emits a minimal C file that defines LLVMFuzzerTestOneInput
// and forwards the input buffer to the target.
func writeFuzzHarness(out, header, target, sig string, raw bool) error {
	var b strings.Builder
	b.WriteString("/* AUTO-GENERATED BY ceedless fuzz — edit only if needed */\n")
	b.WriteString("#include <stdint.h>\n#include <stddef.h>\n")
	if header != "" {
		incl := filepath.Base(header)
		fmt.Fprintf(&b, "#include %q\n", incl)
	}
	b.WriteString("\nint LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {\n")
	if raw {
		b.WriteString("    /* TODO: feed `data`/`size` to your target function. */\n")
		fmt.Fprintf(&b, "    (void)%s; (void)data; (void)size;\n", target)
	} else if strings.HasPrefix(sig, "const uint8_t") || strings.Contains(sig, "uint8_t *") {
		fmt.Fprintf(&b, "    (void)%s(data, size);\n", target)
	} else {
		fmt.Fprintf(&b, "    /* unrecognized signature for %s: %s */\n", target, sig)
		fmt.Fprintf(&b, "    (void)%s; (void)data; (void)size;\n", target)
	}
	b.WriteString("    return 0;\n}\n")
	if err := os.MkdirAll(filepath.Dir(out), 0o755); err != nil {
		return err
	}
	return os.WriteFile(out, []byte(b.String()), 0o644)
}
