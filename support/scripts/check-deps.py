#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
#
# check for necessary software and their versions before invoking the
# main build system.  uses topological sort  to model
# tool dependencies as a DAG, detect cycles, and check tools in correct
# dependency order so that a missing parent tool cascades clearly to its
# dependents.
#
# Usage:
#   python3 check-deps.py [--cross-compile=PREFIX] [--quiet] [--no-color]
#
# Exit codes:
#   0  All required tools are present and meet minimum versions
#   1  One or more required tools are missing // too old // cycle detected

import argparse
import re
import shutil
import subprocess
import sys
from collections import defaultdict, deque
from enum import Enum

IS_DARWIN = sys.platform == "darwin"

# Severity levels


class Severity(Enum):
    REQUIRED = "required"  # Build will fail without this
    RECOMMENDED = "recommended"  # Some features may not work
    OPTIONAL = "optional"  # Nice to have


class Tool:
    """Describes a single build dependency"""

    def __init__(
        self,
        name,
        *,
        severity=Severity.REQUIRED,
        min_version=None,
        version_cmd=None,
        version_re=None,
        depends_on=None,
        alts=None,
        cross_prefixed=False,
    ):
        self.name = name
        self.severity = severity
        self.min_version = min_version
        self.version_cmd = version_cmd
        self.version_re = version_re
        self.depends_on = depends_on if depends_on is not None else []
        self.alts = alts if alts is not None else []
        self.cross_prefixed = cross_prefixed


#  regex that grabs the first version-looking string (X.Y or X.Y.Z)
_DEFAULT_VER_RE = r"(?P<ver>\d+\.\d+(?:\.\d+)?)"

TOOLS = [
    #  build tooling
    Tool(
        "make",
        min_version="4.1",
        version_cmd=["make", "--version"],
        version_re=rf"GNU Make {_DEFAULT_VER_RE}",
    ),
    # compilers support gcc or clang
    Tool(
        "gcc/clang",
        alts=["gcc", "clang"],
        min_version="7.0",
        version_cmd=["$exe", "--version"],
        version_re=r"(?:gcc|clang)(?: version)?(?: \([^)]+\))?[- ](?P<ver>\d+\.\d+(?:\.\d+)?)",
        cross_prefixed=True,
    ),
    # tools that depend on compiler
    Tool(
        "gcc-ar/llvm-ar",
        depends_on=["gcc/clang"],
        alts=["gcc-ar", "llvm-ar"],
        cross_prefixed=True,
    ),
    Tool(
        "gcc-nm/llvm-nm",
        depends_on=["gcc/clang"],
        alts=["gcc-nm", "llvm-nm"],
        cross_prefixed=True,
    ),
    Tool(
        "as/llvm-as",
        depends_on=["gcc/clang"],
        alts=["as", "llvm-as"],
        cross_prefixed=True,
    ),
    Tool("objcopy", cross_prefixed=True),
    Tool("objdump", cross_prefixed=True),
    Tool("strip", cross_prefixed=True),
    Tool("readelf", cross_prefixed=True),
    Tool("nm", cross_prefixed=True),
    Tool("ar"),
    # coreutils
    Tool(
        "sed",
        alts=["gsed"] if IS_DARWIN else ["sed"],
    ),
    Tool("awk"),
    Tool("cat"),
    Tool(
        "grep",
        alts=["ggrep"] if IS_DARWIN else ["grep"],
    ),
    Tool("m4"),
    # prepare tools
    Tool(
        "wget",
        severity=Severity.RECOMMENDED,
        min_version="1.19",
        version_cmd=["wget", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    Tool("tar", severity=Severity.RECOMMENDED),
    Tool("gzip", severity=Severity.RECOMMENDED),
    Tool("unzip", severity=Severity.RECOMMENDED),
    Tool(
        "git",
        severity=Severity.RECOMMENDED,
        min_version="2.0",
        version_cmd=["git", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    Tool("patch", severity=Severity.RECOMMENDED),
    #  Feature-specific
    Tool(
        "bison",
        severity=Severity.RECOMMENDED,
        min_version="3.0",
        version_cmd=["bison", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    Tool(
        "flex",
        severity=Severity.RECOMMENDED,
        min_version="2.6",
        version_cmd=["flex", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    Tool("dtc", severity=Severity.RECOMMENDED),
    Tool(
        "python3",
        severity=Severity.RECOMMENDED,
        min_version="3.6",
        version_cmd=["python3", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    #  Checksums
    Tool("sha1sum", severity=Severity.RECOMMENDED),
    Tool("sha256sum", severity=Severity.RECOMMENDED),
    Tool("sha512sum", severity=Severity.RECOMMENDED),
    Tool("md5sum", severity=Severity.RECOMMENDED),
    # opt-ins
    Tool(
        "rustc",
        severity=Severity.OPTIONAL,
        min_version="1.65",
        version_cmd=["rustc", "--version"],
        version_re=_DEFAULT_VER_RE,
    ),
    Tool("gccgo", severity=Severity.OPTIONAL, cross_prefixed=True),
    Tool(
        "socat",
        severity=Severity.OPTIONAL,
        min_version="1.7.3.4",
        version_cmd=["socat", "-V"],
        version_re=r"socat version (?P<ver>\d+\.\d+\.\d+(?:\.\d+)?)",
    ),
    Tool("liftoff", severity=Severity.OPTIONAL),
]


def kahns_toposort(tools_by_name):
    """
    run kahn's algorithm on the dependency graph.

    returns:
        (sorted_order, cycle_members)
        sorted_order : list[str]  - tool names in dependency-safe order
        cycle_members: list[str]  - tool names involved in a cycle (empty if DAG)
    """

    adj = defaultdict(list)  # parent -> [children]
    in_degree = defaultdict(int)

    for name, tool in tools_by_name.items():
        if name not in in_degree:
            in_degree[name] = 0
        for dep in tool.depends_on:
            if dep not in tools_by_name:
                raise KeyError(f"Unknown dependency '{dep}' declared by tool '{name}'")
            adj[dep].append(name)
            in_degree[name] += 1

    # pushing into  queue with nodes that have zero in-degree
    queue = deque(n for n, d in in_degree.items() if d == 0)
    sorted_order = []

    while queue:
        node = queue.popleft()
        sorted_order.append(node)
        for child in adj[node]:
            in_degree[child] -= 1
            if in_degree[child] == 0:
                queue.append(child)

    # not all nodes present → cycle exists
    sorted_set = set(sorted_order)
    cycle_members = [n for n in in_degree if n not in sorted_set]
    return sorted_order, cycle_members


def parse_version(ver_str):
    """Parse '1.5.0' into a tuple (1, 5, 0) for comparison"""
    parts = re.split(r"[.\-]", ver_str)
    result = []
    for p in parts:
        try:
            result.append(int(p))
        except ValueError:
            break
    return tuple(result) if result else (0,)


def get_tool_version(tool, executable):
    """Extract the version string"""
    if not tool.version_cmd or not tool.version_re:
        return None

    cmd = list(tool.version_cmd)
    # Replace any explicit '$exe' placeholders with the resolved executable.
    # If no placeholder is present, assume the first argument is the executable.
    replaced = False
    for i, arg in enumerate(cmd):
        if arg == "$exe":
            cmd[i] = executable
            replaced = True
    if not replaced and cmd:
        cmd[0] = executable

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        output = result.stdout + result.stderr
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return None

    match = re.search(tool.version_re, output)
    return match.group("ver") if match else None


class Colors:
    def __init__(self, enabled=True):
        self.enabled = enabled

    def _wrap(self, code, text):
        return f"\033[{code}m{text}\033[0m" if self.enabled else text

    def green(self, t):
        return self._wrap("32", t)

    def red(self, t):
        return self._wrap("31", t)

    def yellow(self, t):
        return self._wrap("33", t)

    def cyan(self, t):
        return self._wrap("36", t)

    def bold(self, t):
        return self._wrap("1", t)

    def dim(self, t):
        return self._wrap("2", t)


# Status tracking


class Status(Enum):
    OK = "ok"
    VERSION_LOW = "version_low"
    MISSING = "missing"
    SKIPPED = "skipped"


def _check_tools(sorted_order, tools_by_name, cross_compile):
    results = {}
    failed = set()
    has_error = False

    for name in sorted_order:
        tool = tools_by_name[name]

        # skip if a parent dependency is missing
        parent_missing = False
        for dep in tool.depends_on:
            if dep in failed:
                parent_missing = True
                break

        if parent_missing:
            results[name] = (Status.SKIPPED, None)
            failed.add(name)
            continue

        # Identify the correct executable name based on fallbacks
        executables_to_check = tool.alts if tool.alts else [tool.name]
        executable = None

        for exe in executables_to_check:
            # prepend cross string if necessary
            full_exe = (
                f"{cross_compile}{exe}"
                if tool.cross_prefixed and cross_compile
                else exe
            )
            if shutil.which(full_exe):
                executable = full_exe
                break

        if not executable:
            results[name] = (Status.MISSING, None)
            failed.add(name)
            if tool.severity == Severity.REQUIRED:
                has_error = True
            continue

        # Check version if applicable
        if tool.min_version:
            found_ver = get_tool_version(tool, executable)
            if found_ver:
                if parse_version(found_ver) < parse_version(tool.min_version):
                    results[name] = (Status.VERSION_LOW, found_ver)
                    failed.add(name)
                    if tool.severity == Severity.REQUIRED:
                        has_error = True
                    continue
                else:
                    results[name] = (Status.OK, found_ver)
            else:
                # could'nt determine version; assume all good (maths fact:: p->q, if p false statement true )
                results[name] = (Status.OK, None)
        else:
            results[name] = (Status.OK, None)

    return results, has_error


def _print_summary(sorted_order, tools_by_name, results, has_error, quiet, C):
    if not quiet:
        hdr = f"  {'':4} {'Tool':<16} {'Status':<10} {'Found':<12} {'Required':<12} {'Severity'}"
        print(C.bold(hdr))
        encoding = sys.stdout.encoding or ""
        sep_char = "─" if "UTF" in encoding.upper() else "-"
        print("  " + sep_char * 68)

    for name in sorted_order:
        tool = tools_by_name[name]
        status, found_ver = results[name]

        if quiet and status == Status.OK:
            continue

        ver_str = found_ver or ""
        req_str = f"≥{tool.min_version}" if tool.min_version else ""
        sev_str = tool.severity.value

        if status == Status.OK:
            sym = C.green("[+]")
            stat = C.green("ok")
        elif status == Status.SKIPPED:
            sym = C.yellow("[~]")
            stat = C.yellow("skipped")
            dep_names = ", ".join(tool.depends_on)
            ver_str = f"(needs {dep_names})"
        elif status == Status.MISSING:
            if tool.severity == Severity.REQUIRED:
                sym = C.red("[!]")
                stat = C.red("MISSING")
            elif tool.severity == Severity.RECOMMENDED:
                sym = C.yellow("[-]")
                stat = C.yellow("missing")
            else:
                sym = C.dim("[ ]")
                stat = C.dim("absent")
        elif status == Status.VERSION_LOW:
            sym = C.red("[-]")
            stat = C.red("too old")
        else:
            raise RuntimeError(f"Unhandled status: {status!r}")
        # todo:: replace this with switch case when python >=3.10 as min version is updated
        line = f"  {sym} {name:<14} {stat:<19} {ver_str:<12} {req_str:<12} {sev_str}"
        print(line)

    #  Summary
    n_ok = sum(1 for s, _ in results.values() if s == Status.OK)
    n_fail = sum(
        1 for s, _ in results.values() if s in (Status.MISSING, Status.VERSION_LOW)
    )
    n_skip = sum(1 for s, _ in results.values() if s == Status.SKIPPED)

    if has_error or n_fail > 0 or not quiet:
        print()

    if has_error:
        print(
            C.red(
                C.bold(
                    f"FAILED: {n_fail} tool(s) missing or too old, "
                    f"{n_skip} skipped, {n_ok}/{len(results)} ok."
                )
            )
        )
        print(C.red("Install the missing REQUIRED tools before building."))
        return 1
    elif n_fail > 0 and not quiet:
        print(
            C.yellow(
                f"WARNING: {n_fail} recommended/optional tool(s) missing. "
                f"{n_ok}/{len(results)} ok."
            )
        )
        print(C.dim("Some build features (fetch, prepare, languages) may not work."))
        return 0
    else:
        if not quiet:
            print(C.green(C.bold(f"ALL OK: {n_ok}/{len(results)} tools found.")))
        return 0


def check_dependencies(cross_compile="", quiet=False, use_color=True):

    C = Colors(enabled=use_color)

    #  lookup map
    tools_by_name = {t.name: t for t in TOOLS}

    # kahn's topological sort
    sorted_order, cycle_members = kahns_toposort(tools_by_name)

    if cycle_members:
        print(C.red(C.bold("ERROR: Circular dependency detected!")))
        print(C.red(f"  Involved tools: {', '.join(cycle_members)}"))
        print(C.red("  This is a  bug in check-deps.py."))
        return 1

    if not quiet:
        print(C.bold(" Build Dependency Check"))
        if cross_compile:
            print(C.dim(f"  Cross-compile prefix: {cross_compile}"))
        print()

    results, has_error = _check_tools(sorted_order, tools_by_name, cross_compile)
    return _print_summary(sorted_order, tools_by_name, results, has_error, quiet, C)


def main():
    parser = argparse.ArgumentParser(description="Check  build dependencies ")
    parser.add_argument(
        "--cross-compile",
        default="",
        help="Cross-compile prefix, e.g. aarch64-linux-gnu-",
    )
    parser.add_argument(
        "--quiet", "-q", action="store_true", help="Only show errors and warnings"
    )
    parser.add_argument(
        "--no-color", action="store_true", help="Disable colored output"
    )
    args = parser.parse_args()

    use_color = not args.no_color and sys.stdout.isatty()
    sys.exit(
        check_dependencies(
            cross_compile=args.cross_compile, quiet=args.quiet, use_color=use_color
        )
    )


if __name__ == "__main__":
    main()
