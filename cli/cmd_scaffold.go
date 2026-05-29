// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

const projectYml = `# ceedless project file
project:
  name: %s
paths:
  src:     [src]
  include: [include, src]
  test:    [test]
defines: []
cflags:  ["-std=c11", "-Wall", "-Wextra", "-O0", "-g"]
files:
  extra: []
`

const gitignore = `build/
*.o
*.gcda
*.gcno
*.junit.xml
`

func cmdNew(args []string) error {
	if len(args) < 1 {
		return fmt.Errorf("usage: ceedless new <path>")
	}
	target := args[0]
	if fi, err := os.Stat(target); err == nil && fi.IsDir() {
		entries, _ := os.ReadDir(target)
		if len(entries) > 0 {
			return fmt.Errorf("%s is not empty", target)
		}
	}
	for _, d := range []string{"src", "include", "test", "build"} {
		if err := os.MkdirAll(filepath.Join(target, d), 0o755); err != nil {
			return err
		}
	}
	name := filepath.Base(target)
	if err := os.WriteFile(filepath.Join(target, "project.yml"),
		[]byte(fmt.Sprintf(projectYml, name)), 0o644); err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(target, "README.md"),
		[]byte(fmt.Sprintf("# %s\n\nGenerated with `ceedless new`. Run `ceedless test` to execute tests.\n", name)),
		0o644); err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(target, ".gitignore"), []byte(gitignore), 0o644); err != nil {
		return err
	}
	info("created project %s at %s", name, target)
	info("next: cd %s && ceedless module hello && ceedless test", filepath.Base(target))
	return nil
}

func cmdModule(args []string) error {
	if len(args) < 1 {
		return fmt.Errorf("usage: ceedless module <name>")
	}
	name := args[0]
	cfg, _ := LoadConfig("project.yml")
	src := cfg.SrcDirs[0]
	inc := cfg.IncludeDirs[0]
	tst := cfg.TestDirs[0]
	for _, d := range []string{src, inc, tst} {
		_ = os.MkdirAll(d, 0o755)
	}
	U := strings.ToUpper(name)

	hdr := filepath.Join(inc, name+".h")
	if !exists(hdr) {
		body := fmt.Sprintf("/* SPDX-License-Identifier: MIT */\n#ifndef %s_H\n#define %s_H\n\nint %s_add(int a, int b);\n\n#endif\n", U, U, name)
		_ = os.WriteFile(hdr, []byte(body), 0o644)
		info("created %s", hdr)
	}
	srcF := filepath.Join(src, name+".c")
	if !exists(srcF) {
		body := fmt.Sprintf("/* SPDX-License-Identifier: MIT */\n#include \"%s.h\"\n\nint %s_add(int a, int b) { return a + b; }\n", name, name)
		_ = os.WriteFile(srcF, []byte(body), 0o644)
		info("created %s", srcF)
	}
	tstF := filepath.Join(tst, "test_"+name+".c")
	if !exists(tstF) {
		body := fmt.Sprintf(`/* SPDX-License-Identifier: MIT */
#include "ceedless/ceedless.h"
#include "%s.h"

void setUp(void)    { }
void tearDown(void) { }

void test_%s_add_basic(void) {
    TEST_ASSERT_EQUAL_INT(5, %s_add(2, 3));
}

void test_%s_add_negative(void) {
    TEST_ASSERT_EQUAL_INT(-1, %s_add(2, -3));
}
`, name, name, name, name, name)
		_ = os.WriteFile(tstF, []byte(body), 0o644)
		info("created %s", tstF)
	}
	return nil
}

func cmdClean(_ []string) error {
	if err := os.RemoveAll("build"); err != nil {
		return err
	}
	info("removed build/")
	return nil
}
