// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
//
// Minimal YAML subset parser sufficient for project.yml. We deliberately
// avoid the upstream YAML package to keep the binary dependency-free.
//
// Supported:
//
//	key: value                        (scalar)
//	key: [a, b, "c"]                  (inline list)
//	group:
//	  key: value
//	  list: [x, y]
package main

import (
	"bufio"
	"os"
	"strings"
)

type Config struct {
	ProjectName string
	SrcDirs     []string
	IncludeDirs []string
	TestDirs    []string
	Defines     []string
	CFlags      []string
	LDFlags     []string
	ExtraFiles  []string
}

func LoadConfig(path string) (*Config, error) {
	c := &Config{}
	f, err := os.Open(path)
	if err != nil {
		// missing project.yml is fine; defaults apply
		c.applyDefaults()
		return c, nil
	}
	defer f.Close()

	scan := bufio.NewScanner(f)
	scan.Buffer(make([]byte, 65536), 1<<20)
	group := ""
	for scan.Scan() {
		raw := scan.Text()
		line := raw
		if i := strings.Index(line, "#"); i >= 0 {
			line = line[:i]
		}
		trim := strings.TrimRight(line, " \t\r")
		if trim == "" {
			continue
		}
		// top-level key:
		if !strings.HasPrefix(line, " ") && !strings.HasPrefix(line, "\t") {
			k, v := splitKV(strings.TrimSpace(trim))
			if v == "" {
				group = k
				continue
			}
			group = ""
			c.assign(k, parseValue(v))
			continue
		}
		// nested:
		k, v := splitKV(strings.TrimSpace(trim))
		if v == "" {
			continue
		}
		full := group + "." + k
		c.assignNested(full, parseValue(v))
	}
	c.applyDefaults()
	return c, nil
}

func splitKV(s string) (string, string) {
	i := strings.Index(s, ":")
	if i < 0 {
		return s, ""
	}
	return strings.TrimSpace(s[:i]), strings.TrimSpace(s[i+1:])
}

func parseValue(v string) []string {
	if strings.HasPrefix(v, "[") && strings.HasSuffix(v, "]") {
		inner := v[1 : len(v)-1]
		parts := splitCSV(inner)
		out := make([]string, 0, len(parts))
		for _, p := range parts {
			p = strings.TrimSpace(p)
			p = unquote(p)
			if p != "" {
				out = append(out, p)
			}
		}
		return out
	}
	return []string{unquote(v)}
}

func splitCSV(s string) []string {
	var out []string
	cur := ""
	inq := byte(0)
	for i := 0; i < len(s); i++ {
		c := s[i]
		if inq != 0 {
			if c == inq {
				inq = 0
			}
			cur += string(c)
			continue
		}
		if c == '"' || c == '\'' {
			inq = c
			cur += string(c)
			continue
		}
		if c == ',' {
			out = append(out, cur)
			cur = ""
			continue
		}
		cur += string(c)
	}
	if cur != "" {
		out = append(out, cur)
	}
	return out
}

func unquote(s string) string {
	if len(s) >= 2 {
		if (s[0] == '"' && s[len(s)-1] == '"') || (s[0] == '\'' && s[len(s)-1] == '\'') {
			return s[1 : len(s)-1]
		}
	}
	return s
}

func (c *Config) assign(key string, vals []string) {
	switch key {
	case "defines":
		c.Defines = vals
	case "cflags":
		c.CFlags = vals
	case "ldflags":
		c.LDFlags = vals
	}
}

func (c *Config) assignNested(full string, vals []string) {
	switch full {
	case "project.name":
		if len(vals) > 0 {
			c.ProjectName = vals[0]
		}
	case "paths.src":
		c.SrcDirs = vals
	case "paths.include":
		c.IncludeDirs = vals
	case "paths.test":
		c.TestDirs = vals
	case "files.extra":
		c.ExtraFiles = vals
	}
}

func (c *Config) applyDefaults() {
	if len(c.SrcDirs) == 0 {
		c.SrcDirs = []string{"src"}
	}
	if len(c.IncludeDirs) == 0 {
		c.IncludeDirs = []string{"include", "src"}
	}
	if len(c.TestDirs) == 0 {
		c.TestDirs = []string{"test"}
	}
	if len(c.CFlags) == 0 {
		c.CFlags = []string{"-std=c11", "-Wall", "-Wextra", "-O0", "-g"}
	}
}
