// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
package main

import (
	"fmt"
	"os"
)

const version = "0.5.0"

type command struct {
	name string
	help string
	run  func(args []string) error
}

var commands []command

func register(c command) { commands = append(commands, c) }

func usage() {
	fmt.Fprintf(os.Stderr, "ceedless %s — embedded C unit test framework\n\n", version)
	fmt.Fprintln(os.Stderr, "usage: ceedless <command> [args]")
	fmt.Fprintln(os.Stderr)
	fmt.Fprintln(os.Stderr, "commands:")
	for _, c := range commands {
		fmt.Fprintf(os.Stderr, "  %-10s  %s\n", c.name, c.help)
	}
	fmt.Fprintln(os.Stderr, "\nrun 'ceedless <command> --help' for command-specific options.")
}

func main() {
	register(command{"new", "scaffold a new project", cmdNew})
	register(command{"init", "interactive project wizard", cmdInit})
	register(command{"module", "generate src/<name>.{c,h} + test/test_<name>.c", cmdModule})
	register(command{"test", "discover, build, run every test_*.c", cmdTest})
	register(command{"gcov", "alias for: test --gcov", cmdGcov})
	register(command{"cov", "build HTML coverage report (post-test)", cmdCov})
	register(command{"bench", "run tests and report timing per test", cmdBench})
	register(command{"watch", "re-run tests on file changes", cmdWatch})
	register(command{"mock", "generate MOCK_DEFINE boilerplate from a header", cmdMock})
	register(command{"fuzz", "build & run a libFuzzer harness for a function", cmdFuzz})
	register(command{"report", "aggregate JUnit XMLs into an HTML report", cmdReport})
	register(command{"doctor", "diagnose toolchain availability", cmdDoctor})
	register(command{"clean", "remove build/", cmdClean})
	register(command{"docker", "run any subcommand inside the container", cmdDocker})
	register(command{"version", "print version", cmdVersion})

	if len(os.Args) < 2 {
		cmdVersion(nil)
		return
	}
	sub := os.Args[1]
	switch sub {
	case "-h", "--help", "help":
		usage()
		return
	case "-v", "--version":
		cmdVersion(nil)
		return
	}
	for _, c := range commands {
		if c.name == sub {
			if err := c.run(os.Args[2:]); err != nil {
				fmt.Fprintf(os.Stderr, "%sceedless%s: %v\n", cRed(), cRst(), err)
				os.Exit(1)
			}
			return
		}
	}
	fmt.Fprintf(os.Stderr, "ceedless: unknown command: %s (try 'ceedless help')\n", sub)
	os.Exit(2)
}

func cmdVersion(_ []string) error {
	fmt.Printf("ceedless %s\n", version)
	return nil
}
