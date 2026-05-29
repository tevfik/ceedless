// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestScanTestFindsFunctions(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "test_foo.c")
	body := `#include "ceedless/ceedless.h"
/* TEST_SOURCE_FILE("src/extra/helper.c") */
/* TEST_SOURCE_FILE("src/extra/other.c") */
void test_alpha(void) { TEST_ASSERT_TRUE(1); }
void test_beta(void)  { TEST_ASSERT_TRUE(1); }
void not_a_test(void) {}
void test_gamma( void ) { } // odd spacing
`
	if err := os.WriteFile(src, []byte(body), 0o644); err != nil {
		t.Fatal(err)
	}
	tests, extras, err := ScanTest(src)
	if err != nil {
		t.Fatal(err)
	}
	want := []string{"test_alpha", "test_beta", "test_gamma"}
	if strings.Join(tests, ",") != strings.Join(want, ",") {
		t.Errorf("tests = %v, want %v", tests, want)
	}
	wantExtras := []string{"src/extra/helper.c", "src/extra/other.c"}
	if strings.Join(extras, ",") != strings.Join(wantExtras, ",") {
		t.Errorf("extras = %v, want %v", extras, wantExtras)
	}
}

func TestSynthRunnerProducesValidC(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "test_foo.c")
	out := filepath.Join(dir, "out", "test_foo_runner.c")
	if err := os.WriteFile(src, []byte("void test_a(void){}\nvoid test_b(void){}\n"), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := SynthRunner(src, out); err != nil {
		t.Fatal(err)
	}
	data, err := os.ReadFile(out)
	if err != nil {
		t.Fatal(err)
	}
	s := string(data)
	for _, want := range []string{
		`#include "ceedless/ceedless.h"`,
		`ceedless_begin("test_foo")`,
		`RUN_TEST(test_a)`,
		`RUN_TEST(test_b)`,
		`return ceedless_end();`,
	} {
		if !strings.Contains(s, want) {
			t.Errorf("synth runner missing %q\nfull:\n%s", want, s)
		}
	}
}

func TestConfigDefaultsWhenMissing(t *testing.T) {
	dir := t.TempDir()
	old, _ := os.Getwd()
	defer os.Chdir(old)
	os.Chdir(dir)
	cfg, err := LoadConfig("project.yml") // missing file
	if err != nil {
		t.Fatal(err)
	}
	if len(cfg.SrcDirs) == 0 || cfg.SrcDirs[0] != "src" {
		t.Errorf("default SrcDirs = %v", cfg.SrcDirs)
	}
	if len(cfg.TestDirs) == 0 || cfg.TestDirs[0] != "test" {
		t.Errorf("default TestDirs = %v", cfg.TestDirs)
	}
}

func TestConfigParsesYAMLSubset(t *testing.T) {
	dir := t.TempDir()
	yml := []byte(`project:
  name: demo
paths:
  src:     [src, lib]
  test:    [tests]
defines: ["HW=1", "CEEDLESS_TRACE_HOST"]
cflags:  ["-std=c11", "-O2"]
files:
  extra: ["src/util/crc.c"]
`)
	p := filepath.Join(dir, "project.yml")
	if err := os.WriteFile(p, yml, 0o644); err != nil {
		t.Fatal(err)
	}
	old, _ := os.Getwd()
	defer os.Chdir(old)
	os.Chdir(dir)
	cfg, err := LoadConfig("project.yml")
	if err != nil {
		t.Fatal(err)
	}
	if cfg.ProjectName != "demo" {
		t.Errorf("name = %q", cfg.ProjectName)
	}
	if strings.Join(cfg.SrcDirs, ",") != "src,lib" {
		t.Errorf("SrcDirs = %v", cfg.SrcDirs)
	}
	if strings.Join(cfg.Defines, ",") != "HW=1,CEEDLESS_TRACE_HOST" {
		t.Errorf("Defines = %v", cfg.Defines)
	}
	if strings.Join(cfg.ExtraFiles, ",") != "src/util/crc.c" {
		t.Errorf("ExtraFiles = %v", cfg.ExtraFiles)
	}
}
