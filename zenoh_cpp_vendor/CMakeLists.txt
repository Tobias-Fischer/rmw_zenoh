cmake_minimum_required(VERSION 3.16)
project(zenoh_cpp_vendor)

# Note: this package has no compiled targets of its own; it only drives the
# vendored zenoh-c / zenoh-cpp external projects, which set their own C++
# standard. There is therefore no local target to apply a C++ standard to.

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_vendor_package REQUIRED)

option(USE_SYSTEM_ZENOH "Use non-vendored zenoh from the system" OFF)

if(USE_SYSTEM_ZENOH)
  find_package(zenohc REQUIRED)
  find_package(zenohcxx REQUIRED)
  message(STATUS "Using system zenohc version: ${zenohc_VERSION}")
  message(STATUS "Using system zenohcxx version: ${zenohcxx_VERSION}")
else()
  # Add a set of usefull features that are not enabled by default in zenoh-c.
  # For a complete list of features see: https://github.com/eclipse-zenoh/zenoh/blob/main/zenoh/Cargo.toml
  # Note: the list of features are separated with whitespaces.
  # However, if you want to add other cargo build flags, use "$<SEMICOLON>" as separator and not ";" as the
  # latter is a list separater in cmake and hence the string will be split into two when expanded.
  # shared-memory needs real OS shared memory (POSIX shm/mmap), which doesn't
  # exist -- or mean anything -- across a browser/WASM boundary, so it's
  # dropped for the wasm32 experiment below. transport_serial similarly
  # doesn't apply (no serial ports in a browser).
  #
  # zenoh-c's own `default` feature set (see its Cargo.toml) unconditionally
  # pulls in transport_quic/transport_tls/transport_quic_datagram, which
  # depend on `ring` for crypto -- ring hand-writes per-target assembly/C and
  # doesn't support wasm32-unknown-emscripten (confirmed: cc-rs invoked emcc
  # with a native `--target=arm64-apple-macosx` flag while cross-compiling
  # ring's C sources, which emcc rejects outright). None of TLS/QUIC are
  # needed for this experiment (a plain TCP connection to a native zenohd),
  # so wasm32 disables zenoh-c's crate-level defaults entirely and opts back
  # into only transport_tcp.
  if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(ZENOHC_CARGO_FLAGS "--no-default-features --features=zenoh/transport_tcp")
  else()
    set(ZENOHC_CARGO_FLAGS "--features=shared-memory zenoh/transport_serial")
  endif()

  include(get_cargo_version.cmake)
  get_cargo_version()

  # By default, use commit id from branch ROS/zenoh-2687c5135.
  # It corresponding to zenoh-c 1.8.0 plus few fixes, including removal of a error log at closure causing #951
  # See https://github.com/eclipse-zenoh/zenoh-c/blob/05bd370343b5161ca9269649b9a914c9c2dc4170/Cargo.lock#L4207
  # pointing to zenoh https://github.com/eclipse-zenoh/zenoh/commit/2687c51352121f006e3a603ce07925a8ad0b295c
  set(zenoh_c_commit 05bd370343b5161ca9269649b9a914c9c2dc4170)

  # If Rust version below Zenoh MSRV (currently 1.88), use equivalent commit from branch ROS/rust-1.75-zenoh-2687c5135
  # See https://github.com/eclipse-zenoh/zenoh-c/blob/b31348fa7f94f44f1f7b049c111a710e970a2725/Cargo.lock#L4237
  # pointing to zenoh https://github.com/eclipse-zenoh/zenoh/commit/2687c51352121f006e3a603ce07925a8ad0b295c
  if(CARGO_VERSION VERSION_GREATER_EQUAL "1.75" AND CARGO_VERSION VERSION_LESS "1.88")
    set(zenoh_c_commit b31348fa7f94f44f1f7b049c111a710e970a2725)
  endif()

  # wasm32-unknown-emscripten experiment (not upstream): build zenoh-c/zenoh-cpp
  # from Tobias-Fischer's forks (branch wasm32-nonblocking-open on each), which
  # add the non-blocking Runtime/SessionOpenTask API this target needs instead
  # of zenoh-c's normal block_in_place-based synchronous API. zenoh-c's own
  # Cargo.toml on that branch already repoints its zenoh/zenoh-ext/
  # zenoh-runtime/zenoh-util deps at Tobias-Fischer/zenoh's wasm32-full branch
  # (which itself carries the mio/tokio wasm32 patches), so nothing further is
  # needed here for that part of the chain. See eclipse-zenoh/zenoh-c#1338 and
  # eclipse-zenoh/zenoh-cpp#810 (both draft) for the corresponding upstream PRs.
  if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    ament_vendor(zenoh_c_vendor
      VCS_URL https://github.com/Tobias-Fischer/zenoh-c.git
      VCS_VERSION wasm32-nonblocking-open
      CMAKE_ARGS
        "-DZENOHC_CARGO_FLAGS=${ZENOHC_CARGO_FLAGS}"
        "-DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE"
        "-DZENOHC_CUSTOM_TARGET=${ZENOHC_CUSTOM_TARGET}"
    )

    ament_vendor(zenoh_cpp_vendor
      VCS_URL https://github.com/Tobias-Fischer/zenoh-cpp.git
      VCS_VERSION wasm32-nonblocking-open
      CMAKE_ARGS
        -DZENOHCXX_ZENOHC=OFF
    )
  else()
    ament_vendor(zenoh_c_vendor
      VCS_URL https://github.com/eclipse-zenoh/zenoh-c.git
      VCS_VERSION ${zenoh_c_commit}
      PATCHES
        silence-build-script-warnings.patch
      CMAKE_ARGS
        "-DZENOHC_CARGO_FLAGS=${ZENOHC_CARGO_FLAGS}"
        "-DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE"
        "-DZENOHC_CUSTOM_TARGET=${ZENOHC_CUSTOM_TARGET}"
    )

    ament_vendor(zenoh_cpp_vendor
      VCS_URL https://github.com/eclipse-zenoh/zenoh-cpp
      VCS_VERSION 481b71bf974f684ea75afb4243b0313646d545f9
      CMAKE_ARGS
        -DZENOHCXX_ZENOHC=OFF
    )
  endif()

  externalproject_add_stepdependencies(zenoh_cpp_vendor configure zenoh_c_vendor)
endif()

ament_export_dependencies(zenohc)
ament_export_dependencies(zenohcxx)

ament_package()
