#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json
import sys

root = Path(__file__).resolve().parents[1]
package = root / "third_party" / "boost-multiprecision-1.83.0"
manifest_path = package / "PACKAGE.json"
index_path = package / "FILES.SHA256"

if not manifest_path.is_file() or not index_path.is_file():
    print("Pinned Boost.Multiprecision package metadata is missing.")
    sys.exit(1)

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if manifest.get("name") != "boost-multiprecision-standalone" or manifest.get("version") != "1.83.0":
    print("Unexpected Boost.Multiprecision package identity.")
    sys.exit(1)
if manifest.get("licence") != "BSL-1.0" or manifest.get("linkage") != "header-only":
    print("Unexpected Boost.Multiprecision licence or linkage contract.")
    sys.exit(1)
if manifest.get("runtimeArtifacts") != [] or manifest.get("compileDefinitions") != ["BOOST_MP_STANDALONE"]:
    print("Unexpected Boost.Multiprecision runtime or compile-definition contract.")
    sys.exit(1)

index_text = index_path.read_text(encoding="utf-8")
index_hash = hashlib.sha256(index_text.encode("utf-8")).hexdigest()
if index_hash != manifest.get("contentIndexSha256"):
    print("Boost.Multiprecision content index digest does not match PACKAGE.json.")
    sys.exit(1)

entries = []
for line_number, line in enumerate(index_text.splitlines(), start=1):
    if not line:
        continue
    try:
        digest, relative = line.split("  ", 1)
    except ValueError:
        print(f"Malformed FILES.SHA256 entry at line {line_number}.")
        sys.exit(1)
    target = package / relative
    if not target.is_file():
        print(f"Pinned Boost.Multiprecision file is missing: {relative}")
        sys.exit(1)
    actual = hashlib.sha256(target.read_bytes()).hexdigest()
    if actual != digest:
        print(f"Pinned Boost.Multiprecision file digest mismatch: {relative}")
        sys.exit(1)
    entries.append(relative)

if len(entries) != manifest.get("contentFileCount"):
    print("Boost.Multiprecision package file count does not match PACKAGE.json.")
    sys.exit(1)

metadata_files = {"README.md", "PACKAGE.json", "FILES.SHA256"}
actual_files = {
    path.relative_to(package).as_posix()
    for path in package.rglob("*")
    if path.is_file()
}
expected_files = set(entries) | metadata_files
if actual_files != expected_files:
    for relative in sorted(actual_files - expected_files):
        print(f"Unreviewed file exists in pinned Boost.Multiprecision package: {relative}")
    for relative in sorted(expected_files - actual_files):
        print(f"Reviewed Boost.Multiprecision package file is missing: {relative}")
    sys.exit(1)

required = {
    "LICENSE_1_0.txt",
    "include/boost/config.hpp",
    "include/boost/version.hpp",
    "include/boost/multiprecision/cpp_bin_float.hpp",
}
if not required.issubset(entries):
    print("Boost.Multiprecision package is missing a required reviewed file.")
    sys.exit(1)

print(f"Boost.Multiprecision 1.83.0 package integrity passed ({len(entries)} files).")
