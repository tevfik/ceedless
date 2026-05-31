// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
//
// `ceedless report` — aggregate JUnit XML files into a single HTML report.
//
// Usage:
//
//	ceedless test --report-dir reports/
//	ceedless report reports/
//
// Produces reports/index.html with per-suite tables and totals.
package main

import (
	"encoding/xml"
	"flag"
	"fmt"
	"html"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

type jUnitCase struct {
	Name      string `xml:"name,attr"`
	ClassName string `xml:"classname,attr"`
	Failure   *struct {
		Message string `xml:"message,attr"`
	} `xml:"failure"`
	Skipped *struct {
		Message string `xml:"message,attr"`
	} `xml:"skipped"`
}

type jUnitSuite struct {
	XMLName  xml.Name    `xml:"testsuite"`
	Name     string      `xml:"name,attr"`
	Tests    int         `xml:"tests,attr"`
	Failures int         `xml:"failures,attr"`
	Skipped  int         `xml:"skipped,attr"`
	Cases    []jUnitCase `xml:"testcase"`
}

func cmdReport(args []string) error {
	fs := flag.NewFlagSet("report", flag.ContinueOnError)
	out := fs.String("o", "", "output HTML file (default: <dir>/index.html)")
	_ = fs.Parse(args)
	rest := fs.Args()
	dir := "reports"
	if len(rest) > 0 {
		dir = rest[0]
	}
	if *out == "" {
		*out = filepath.Join(dir, "index.html")
	}

	var files []string
	filepath.Walk(dir, func(p string, fi os.FileInfo, e error) error {
		if e != nil || fi.IsDir() || !strings.HasSuffix(p, ".xml") {
			return nil
		}
		files = append(files, p)
		return nil
	})
	if len(files) == 0 {
		return fmt.Errorf("no JUnit .xml files found under %s", dir)
	}
	sort.Strings(files)

	suites := make([]jUnitSuite, 0, len(files))
	totalTests, totalFails, totalSkip := 0, 0, 0
	for _, p := range files {
		data, err := os.ReadFile(p)
		if err != nil {
			info("skip %s: %v", p, err)
			continue
		}
		var s jUnitSuite
		if err := xml.Unmarshal(data, &s); err != nil {
			info("skip %s: %v", p, err)
			continue
		}
		if s.Name == "" {
			s.Name = strings.TrimSuffix(filepath.Base(p), ".xml")
		}
		suites = append(suites, s)
		totalTests += s.Tests
		totalFails += s.Failures
		totalSkip += s.Skipped
	}

	f, err := os.Create(*out)
	if err != nil {
		return err
	}
	defer f.Close()

	fmt.Fprintln(f, `<!doctype html><meta charset="utf-8"><title>ceedless report</title>`)
	fmt.Fprintln(f, `<style>
body{font:14px/1.5 system-ui,sans-serif;margin:2em;color:#222}
h1{margin:0 0 .2em}
.totals{margin:1em 0 2em;padding:1em;background:#f5f7fa;border-radius:6px;display:flex;gap:2em}
.totals div{font-size:1.4em}
.pass{color:#1a7f37}.fail{color:#cf222e}.skip{color:#9a6700}
table{border-collapse:collapse;width:100%;margin:.6em 0 1.4em}
th,td{padding:.4em .7em;border:1px solid #d0d7de;text-align:left;font-size:.95em}
th{background:#f6f8fa}
tr.f td{background:#ffe8e8}tr.s td{background:#fff8cc}
.msg{font-family:ui-monospace,monospace;font-size:.85em;color:#57606a;white-space:pre-wrap}
</style>`)
	fmt.Fprintf(f, "<h1>ceedless test report</h1>\n")
	fmt.Fprintf(f, `<div class="totals"><div class="pass">passed %d</div><div class="fail">failed %d</div><div class="skip">skipped %d</div><div>total %d</div></div>`,
		totalTests-totalFails-totalSkip, totalFails, totalSkip, totalTests)

	for _, s := range suites {
		fmt.Fprintf(f, `<h2>%s <small>(%d tests, %d failures, %d skipped)</small></h2>`,
			html.EscapeString(s.Name), s.Tests, s.Failures, s.Skipped)
		fmt.Fprintln(f, "<table><tr><th style=width:8em>status</th><th>test</th><th>message</th></tr>")
		for _, c := range s.Cases {
			st, cls, msg := "pass", "", ""
			if c.Failure != nil {
				st, cls, msg = "FAIL", "f", c.Failure.Message
			} else if c.Skipped != nil {
				st, cls, msg = "skip", "s", c.Skipped.Message
			}
			fmt.Fprintf(f, `<tr class="%s"><td>%s</td><td>%s</td><td class="msg">%s</td></tr>`,
				cls, st, html.EscapeString(c.Name), html.EscapeString(msg))
		}
		fmt.Fprintln(f, "</table>")
	}
	info("wrote %s", *out)
	return nil
}
