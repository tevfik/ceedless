// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"time"
)

type testOpts struct {
	filter    string
	jobs      int
	verbose   bool
	gcov      bool
	junitDir  string
	target    string
	timing    bool
	format    string
	reportDir string
}

type buildResult struct {
	test string
	bin  string
	err  error
}

type runResult struct {
	stem string
	ok   bool
	dur  time.Duration
}

func parseTestFlags(args []string, o *testOpts) []string {
	fs := flag.NewFlagSet("test", flag.ContinueOnError)
	fs.StringVar(&o.filter, "p", "", "substring filter on test filename")
	fs.StringVar(&o.filter, "pattern", "", "substring filter (alias)")
	fs.IntVar(&o.jobs, "j", runtime.NumCPU(), "parallel build jobs")
	fs.IntVar(&o.jobs, "jobs", runtime.NumCPU(), "parallel build jobs (alias)")
	fs.BoolVar(&o.verbose, "v", false, "verbose: echo compiler command lines")
	fs.BoolVar(&o.gcov, "gcov", false, "build with --coverage")
	fs.StringVar(&o.junitDir, "junit-dir", "", "write JUnit XML per test program here")
	fs.StringVar(&o.target, "target", "", "value passed via -DCEEDLESS_TARGET=…")
	fs.BoolVar(&o.timing, "timing", false, "show per-test program wall time")
	fs.StringVar(&o.format, "format", "text", "report format: text|tap")
	fs.StringVar(&o.reportDir, "report-dir", "", "collect all reports under DIR")
	_ = fs.Parse(args)
	return fs.Args()
}

func cmdTest(args []string) error {
	o := &testOpts{}
	rest := parseTestFlags(args, o)
	if len(rest) > 0 && o.filter == "" {
		o.filter = rest[0]
	}
	if o.reportDir != "" {
		if err := os.MkdirAll(o.reportDir, 0o755); err != nil {
			return err
		}
		if o.junitDir == "" {
			o.junitDir = o.reportDir
		}
	}
	return runTests(o)
}

func cmdGcov(args []string) error {
	return cmdTest(append([]string{"-gcov"}, args...))
}

func runTests(o *testOpts) error {
	home, err := homeDir()
	if err != nil {
		return err
	}
	cfg, err := LoadConfig("project.yml")
	if err != nil {
		return err
	}
	tests, err := DiscoverTests(cfg, o.filter)
	if err != nil {
		return err
	}
	if len(tests) == 0 {
		return fmt.Errorf("no tests found")
	}

	if err := os.MkdirAll("build", 0o755); err != nil {
		return err
	}
	if o.junitDir != "" {
		if err := os.MkdirAll(o.junitDir, 0o755); err != nil {
			return err
		}
	}

	cc := os.Getenv("CC")
	if cc == "" {
		cc = "gcc"
	}
	core := coreSources(home)
	baseFlags := append([]string{}, cfg.CFlags...)
	if o.gcov {
		baseFlags = append(baseFlags, "--coverage", "-O0")
	}
	defines := []string{"-DCEEDLESS_TRACE_HOST"}
	for _, d := range cfg.Defines {
		defines = append(defines, "-D"+d)
	}
	if o.target != "" {
		defines = append(defines, "-DCEEDLESS_TARGET="+o.target)
	}
	includes := []string{"-I" + filepath.Join(home, "include")}
	for _, p := range cfg.IncludeDirs {
		includes = append(includes, "-I"+p)
	}

	results := make(chan buildResult, len(tests))
	jobs := o.jobs
	if jobs < 1 {
		jobs = 1
	}
	sem := make(chan struct{}, jobs)
	var wg sync.WaitGroup
	for _, t := range tests {
		t := t
		sem <- struct{}{}
		wg.Add(1)
		go func() {
			defer wg.Done()
			defer func() { <-sem }()
			results <- buildOne(t, cc, baseFlags, defines, includes, core, cfg, o)
		}()
	}
	wg.Wait()
	close(results)

	built := map[string]string{}
	fails := 0
	for r := range results {
		if r.err != nil {
			fmt.Fprintf(os.Stderr, "%sCC FAIL%s %s: %v\n", cRed(), cRst(), r.test, r.err)
			fails++
			continue
		}
		built[r.test] = r.bin
	}
	if fails > 0 {
		return fmt.Errorf("%d test program(s) failed to compile", fails)
	}

	var runs []runResult
	for _, t := range tests {
		stem := strings.TrimSuffix(filepath.Base(t), ".c")
		bin := built[t]
		fmt.Printf("\n%s=== %s ===%s\n", cBlu(), stem, cRst())
		c := exec.Command(bin)
		c.Stdout = os.Stdout
		c.Stderr = os.Stderr
		c.Env = os.Environ()
		if o.junitDir != "" {
			c.Env = append(c.Env, "CEEDLESS_JUNIT="+filepath.Join(o.junitDir, stem+".xml"))
		}
		t0 := time.Now()
		err := c.Run()
		dur := time.Since(t0)
		runs = append(runs, runResult{stem, err == nil, dur})
		if o.timing {
			fmt.Printf("%s[time]%s %s %v\n", cDim(), cRst(), stem, dur)
		}
	}

	if o.gcov {
		runGcov(cfg)
	}

	pass := 0
	for _, r := range runs {
		if r.ok {
			pass++
		}
	}
	col := cGrn()
	if pass != len(runs) {
		col = cRed()
	}
	fmt.Printf("\n%sceedless:%s %d/%d test programs passed\n", col, cRst(), pass, len(runs))
	if o.format == "tap" {
		writeTAP(runs)
	}
	if pass != len(runs) {
		return fmt.Errorf("test failures")
	}
	return nil
}

func buildOne(test, cc string, baseFlags, defines, includes, core []string, cfg *Config, o *testOpts) buildResult {
	br := buildResult{test: test}
	stem := strings.TrimSuffix(filepath.Base(test), ".c")
	mod := strings.TrimPrefix(stem, "test_")
	runner := filepath.Join("build", stem+"_runner.c")
	if err := SynthRunner(test, runner); err != nil {
		br.err = err
		return br
	}

	sources := []string{test, runner}
	sources = append(sources, cfg.ExtraFiles...)
	sources = append(sources, core...)
	for _, sd := range cfg.SrcDirs {
		cand := filepath.Join(sd, mod+".c")
		if exists(cand) {
			sources = append(sources, cand)
			break
		}
	}
	_, extras, _ := ScanTest(test)
	for _, e := range extras {
		if exists(e) {
			sources = append(sources, e)
		}
	}

	out := filepath.Join("build", stem)
	args := append([]string{}, baseFlags...)
	args = append(args, includes...)
	args = append(args, defines...)
	args = append(args, sources...)
	args = append(args, "-lm", "-o", out)

	if o.verbose {
		fmt.Fprintf(os.Stderr, "  CC %s %s\n", cc, strings.Join(args, " "))
	} else {
		fmt.Printf("  %sCC%s %s\n", cDim(), cRst(), test)
	}
	cmd := exec.Command(cc, args...)
	cmd.Stdout = os.Stderr
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		br.err = err
		return br
	}
	br.bin = out
	return br
}

func writeTAP(runs []runResult) {
	f, err := os.Create("build/report.tap")
	if err != nil {
		return
	}
	defer f.Close()
	fmt.Fprintf(f, "TAP version 13\n1..%d\n", len(runs))
	for i, r := range runs {
		st := "ok"
		if !r.ok {
			st = "not ok"
		}
		fmt.Fprintf(f, "%s %d %s # %v\n", st, i+1, r.stem, r.dur)
	}
}

func runGcov(cfg *Config) {
	info("gcov reports:")
	// gcov resolves source paths relative to its CWD, so we run it from
	// the project root and then move the produced .gcov files into build/.
	cwd, _ := os.Getwd()
	var gcdas []string
	filepath.Walk("build", func(p string, fi os.FileInfo, err error) error {
		if err != nil || fi.IsDir() || !strings.HasSuffix(p, ".gcda") {
			return nil
		}
		abs, _ := filepath.Abs(p)
		gcdas = append(gcdas, abs)
		return nil
	})
	for _, g := range gcdas {
		c := exec.Command("gcov", "-b", g)
		c.Dir = cwd
		c.Run()
	}
	matches, _ := filepath.Glob("*.gcov")
	for _, m := range matches {
		os.Rename(m, filepath.Join("build", m))
	}
	_ = cfg
}
