# Boost.Multiprecision standalone source package

This directory is the project-pinned source package for the independent high-precision backend.

- Upstream project: Boost.Multiprecision
- Pinned upstream version: 1.83.0
- Imported source provenance: Debian `libboost1.83-dev` package version `1.83.0-4.2`
- Upstream release date: 2023-08-11
- Upstream archive SHA-256: `6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e` (`boost_1_83_0.tar.bz2`)
- Licence: Boost Software License 1.0 (`LICENSE_1_0.txt`)
- Packaging: source headers only; no compiled library, DLL, package manager, installer component or network fetch
- Included modules: `boost/multiprecision`, `boost/config`, `boost/config.hpp`, `boost/version.hpp`
- Compile mode: `BOOST_MP_STANDALONE`

The package is intentionally limited to the standalone headers required by `cpp_bin_float`. Do not add unrelated Boost libraries. Changes require an updated package manifest, file-hash inventory, licence review, native MSVC build evidence and numerical regression evidence.
