// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use std::{env, fs, io, path, process};

fn main() -> io::Result<()> {
    let proj_dir = path::PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());

    let nearby_dir = proj_dir.join("nearby-root");
    let nearby_dir_str = nearby_dir.to_str().expect("nearby path not UTF-8");
    println!("cargo::rerun-if-changed={}", nearby_dir_str);
    let out_dir = path::PathBuf::from(env::var_os("OUT_DIR").unwrap());

    // generate bindings for nc.h
    let bindings = bindgen::Builder::default()
        .header(
            nearby_dir
                .join("connections")
                .join("c")
                .join("nc.h")
                .to_str()
                .expect("header path not UTF-8"),
        )
        // search path for other NC headers pulled in from nc.h
        .clang_arg(format!("-I{}", nearby_dir_str))
        // it's actually a C++ header despite being a .h
        .clang_arg("-x")
        .clang_arg("c++")
        // In case `rerun-if-changed` above isn't broad enough, also have bindgen tell cargo
        // about everything it touches
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // don't pull in all of C++
        .allowlist_function("Nc.*")
        .generate()
        .unwrap();

    bindings.write_to_file(out_dir.join("bindings.rs")).unwrap();

    let bazel_user_root_dir = out_dir.join("nearby-bazel");
    fs::create_dir_all(&bazel_user_root_dir).unwrap();
    // Bazel's output base dir is allegedly the MD5 of the workspace root path, but that doesn't
    // seem to be true in practice, so we force it to something predictable
    let bazel_output_base_dir = bazel_user_root_dir.join("output-base");

    // build NC
    let status = process::Command::new("bazel")
        .args([
            &format!(
                "--output_user_root={}",
                bazel_user_root_dir.to_str().unwrap()
            ),
            &format!("--output_base={}", bazel_output_base_dir.to_str().unwrap()),
            "build",
            "//connections/c:nc",
        ])
        .current_dir(&nearby_dir)
        .status()
        .unwrap();
    if !status.success() {
        panic!("Build did not complete successfully");
    }

    eprintln!("nearby: {:?}", nearby_dir);
    eprintln!("bazel user root: {:?}", bazel_user_root_dir);
    eprintln!("bazel output base: {:?}", bazel_output_base_dir);

    // In the verify step of `cargo package`, `nearby-root` is a copy of what passed the Cargo.toml
    // `include` patterns, so it will not have `bazel-bin` populated.
    // Instead, we navigate through bazel's output to find libcore.a.

    // The desired path in the output base will look something like this:
    // ./execroot/_main/bazel-out/darwin_arm64-fastbuild/bin/connections/c/libnc.a
    // Search for `bin/connections` so we can use that as the lib search path, starting in
    // `/execroot` to avoid a bunch of results in `/sandbox`.
    let mut matches = walkdir::WalkDir::new(bazel_output_base_dir.join("execroot"))
        .into_iter()
        .map(|r| r.unwrap())
        .filter(|e| {
            e.file_name() == "connections"
                && e.file_type().is_dir()
                && e.path().parent().unwrap().file_name().unwrap() == "bin"
        })
        .collect::<Vec<_>>();

    if matches.len() != 1 {
        panic!("Needed exactly 1 output path match, got {:?}", matches);
    }
    let connections_lib_dir = matches.swap_remove(0).into_path();
    let nc_lib_dir = connections_lib_dir.join("c");

    // println!(
    //     "cargo::rustc-link-search=native={}",
    //     connections_lib_dir.to_str().expect("lib path not UTF-8")
    // );
    // println!("cargo::rustc-link-lib=static=core");

    println!(
        "cargo::rustc-link-search=native={}",
        nc_lib_dir.to_str().expect("lib path not UTF-8")
    );
    // TODO absolute path to libnc.a
    println!("cargo::rustc-link-lib=static=nc");

    Ok(())
}
