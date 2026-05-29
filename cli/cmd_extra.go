// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"bufio"
	"crypto/sha256"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

// snapshot returns a content hash of every relevant file under cfg.{Src,Test,Include}Dirs.
func snapshot(cfg *Config) string {
	h := sha256.New()
	roots := append([]string{}, cfg.SrcDirs...)
	roots = append(roots, cfg.TestDirs...)
	roots = append(roots, cfg.IncludeDirs...)
	for _, r := range roots {
		filepath.Walk(r, func(p string, fi os.FileInfo, err error) error {
			if err != nil || fi.IsDir() {
				return nil
			}
			if !strings.HasSuffix(p, ".c") && !strings.HasSuffix(p, ".h") {
				return nil
			}
			fmt.Fprintf(h, "%s %d %d\n", p, fi.Size(), fi.ModTime().UnixNano())
			return nil
		})
	}
	return fmt.Sprintf("%x", h.Sum(nil))
}

func cmdWatch(args []string) error {
	info("watching src/, test/, include/ — Ctrl+C to stop")
	cfg, _ := LoadConfig("project.yml")
	last := ""
	for {
		now := snapshot(cfg)
		if now != last {
			if last != "" {
				fmt.Printf("\n%s── change detected ──%s\n", cBlu(), cRst())
			}
			last = now
			_ = cmdTest(args)
		}
		time.Sleep(500 * time.Millisecond)
	}
}

func cmdBench(args []string) error {
	// run with --timing and sort by duration after completion via report.tap
	args = append([]string{"-timing", "-format", "tap"}, args...)
	err := cmdTest(args)
	// post-process the TAP file
	f, ferr := os.Open("build/report.tap")
	if ferr != nil {
		return err
	}
	defer f.Close()
	type row struct {
		name string
		dur  time.Duration
		ok   bool
	}
	var rows []row
	s := bufio.NewScanner(f)
	for s.Scan() {
		line := s.Text()
		if !strings.HasPrefix(line, "ok") && !strings.HasPrefix(line, "not ok") {
			continue
		}
		ok := strings.HasPrefix(line, "ok")
		i := strings.LastIndex(line, "#")
		if i < 0 {
			continue
		}
		head := strings.Fields(line[:i])
		if len(head) < 3 {
			continue
		}
		name := head[len(head)-1]
		ds := strings.TrimSpace(line[i+1:])
		d, perr := time.ParseDuration(ds)
		if perr != nil {
			continue
		}
		rows = append(rows, row{name, d, ok})
	}
	sort.Slice(rows, func(i, j int) bool { return rows[i].dur > rows[j].dur })
	fmt.Printf("\n%sceedless:%s slowest tests:\n", cBlu(), cRst())
	for i, r := range rows {
		if i >= 10 {
			break
		}
		mark := cGrn() + "✓" + cRst()
		if !r.ok {
			mark = cRed() + "✗" + cRst()
		}
		fmt.Printf("  %s %10v  %s\n", mark, r.dur.Round(time.Microsecond), r.name)
	}
	return err
}

func cmdCov(args []string) error {
	// gcov + simple HTML summary in build/coverage.html
	if err := cmdTest(append([]string{"-gcov"}, args...)); err != nil {
		return err
	}
	out := "build/coverage.html"
	f, err := os.Create(out)
	if err != nil {
		return err
	}
	defer f.Close()
	fmt.Fprintln(f, "<!doctype html><meta charset=utf-8><title>ceedless coverage</title>")
	fmt.Fprintln(f, "<style>body{font:14px sans-serif;margin:2em}table{border-collapse:collapse}td,th{padding:.3em .8em;border:1px solid #ccc}tr:nth-child(2n){background:#f7f7f7}</style>")
	fmt.Fprintln(f, "<h1>ceedless coverage</h1><table><tr><th>file</th><th>lines</th><th>%</th></tr>")
	filepath.Walk("build", func(p string, fi os.FileInfo, err error) error {
		if err != nil || fi.IsDir() || !strings.HasSuffix(p, ".gcov") {
			return nil
		}
		total, hit := 0, 0
		gf, ferr := os.Open(p)
		if ferr != nil {
			return nil
		}
		defer gf.Close()
		s := bufio.NewScanner(gf)
		for s.Scan() {
			line := s.Text()
			// gcov format: "<count>:<lineno>:<source>"
			i := strings.Index(line, ":")
			if i < 0 {
				continue
			}
			count := strings.TrimSpace(line[:i])
			if count == "-" {
				continue // non-code line
			}
			total++
			if count != "#####" {
				hit++
			}
		}
		if total == 0 {
			return nil
		}
		pct := 100 * float64(hit) / float64(total)
		fmt.Fprintf(f, "<tr><td>%s</td><td>%d / %d</td><td>%.1f%%</td></tr>\n",
			filepath.Base(p), hit, total, pct)
		return nil
	})
	fmt.Fprintln(f, "</table>")
	info("wrote %s", out)
	return nil
}

func cmdDoctor(_ []string) error {
	checks := []struct {
		name string
		cmd  string
		args []string
	}{
		{"cc", "cc", []string{"--version"}},
		{"gcc", "gcc", []string{"--version"}},
		{"gcov", "gcov", []string{"--version"}},
		{"gcovr", "gcovr", []string{"--version"}},
		{"docker", "docker", []string{"--version"}},
		{"qemu-system-arm", "qemu-system-arm", []string{"--version"}},
		{"arm-none-eabi-gcc", "arm-none-eabi-gcc", []string{"--version"}},
	}
	for _, c := range checks {
		mark := cRed() + "✗" + cRst()
		ver := "not found"
		if path, err := lookPath(c.cmd); err == nil {
			mark = cGrn() + "✓" + cRst()
			ver = path
			if out, oerr := captureOutput(c.cmd, c.args...); oerr == nil {
				ver = path + "  (" + firstLine(out) + ")"
			}
		}
		fmt.Printf("  %s %-22s %s\n", mark, c.name, ver)
	}
	if home, err := homeDir(); err == nil {
		fmt.Printf("\n  %sceedless framework:%s %s\n", cBlu(), cRst(), home)
	} else {
		fmt.Printf("\n  %sceedless framework:%s %v\n", cRed(), cRst(), err)
	}
	return nil
}

func cmdInit(_ []string) error {
	r := bufio.NewReader(os.Stdin)
	ask := func(q, def string) string {
		fmt.Printf("%s [%s]: ", q, def)
		s, _ := r.ReadString('\n')
		s = strings.TrimSpace(s)
		if s == "" {
			return def
		}
		return s
	}
	name := ask("project name", filepath.Base(mustGetwd()))
	trace := ask("trace backend (host|uart|rtt|itm|buffer)", "host")
	std := ask("C standard", "c11")
	if err := cmdNew([]string{name}); err != nil {
		return err
	}
	cfg := fmt.Sprintf(`# ceedless project file
project:
  name: %s
paths:
  src:     [src]
  include: [include, src]
  test:    [test]
defines: ["CEEDLESS_TRACE_%s"]
cflags:  ["-std=%s", "-Wall", "-Wextra", "-O0", "-g"]
files:
  extra: []
`, name, strings.ToUpper(trace), std)
	_ = os.WriteFile(filepath.Join(name, "project.yml"), []byte(cfg), 0o644)
	info("wrote %s/project.yml with %s trace backend", name, trace)
	return nil
}

func cmdDocker(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("usage: ceedless docker <subcommand> [args]")
	}
	img := "ceedless:latest"
	if err := runCmd("docker", "image", "inspect", img); err != nil {
		info("building docker image (one-time)…")
		home, _ := homeDir()
		if home == "" {
			return fmt.Errorf("cannot find ceedless framework to build image")
		}
		if err := runCmd("docker", "build", "-t", img, home); err != nil {
			return err
		}
	}
	cwd, _ := os.Getwd()
	full := append([]string{"run", "--rm", "-t",
		"-v", cwd + ":/work", "-w", "/work",
		"-e", "CC=" + envOr("CC", "gcc"),
		img, "ceedless"}, args...)
	return runCmd("docker", full...)
}

func mustGetwd() string { d, _ := os.Getwd(); return d }
func envOr(k, d string) string {
	if v := os.Getenv(k); v != "" {
		return v
	}
	return d
}
func firstLine(s string) string {
	if i := strings.IndexByte(s, '\n'); i >= 0 {
		return s[:i]
	}
	return s
}
