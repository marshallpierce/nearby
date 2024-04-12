# Overview

Rust bindings for the simplified Nearby Connections C++ API in `c/nc.h`. Despite the .h suffix, it is C++, just sort of C-like C++.

# Requirements

- C++ toolchain
- `bazel` -- typically via installing [Bazelisk](https://github.com/bazelbuild/bazelisk) so the proper version of Bazel can be automatically downloaded if needed

# Notes

`nearby-root` is a symlink to the root of the repo. This makes it convenient to have logic in `build.rs` that works both for `cargo build` and `cargo package` with suitable `include` patterns in `Cargo.toml`.
