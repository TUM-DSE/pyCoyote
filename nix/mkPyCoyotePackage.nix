{
  pkgs,
  coyoteRoot,
  pycoyoteSrc,
  python ? pkgs.python312,
  pname ? "pycoyote",
  version ? "0.1.0",
  enableGpu ? false,
}:

python.pkgs.buildPythonPackage {
  inherit pname version;
  pyproject = true;
  src = pycoyoteSrc;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    patchelf
    python.pkgs.scikit-build-core
  ];

  # CMake is needed by scikit-build-core, but the generic Nix CMake setup hook
  # must not run a separate configure/build in ./build before the Python backend.
  dontUseCmakeConfigure = true;

  buildInputs =
    (with pkgs; [
      boost
    ])
    ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [
      pkgs.numactl
    ];

  postPatch = ''
    rm -rf deps/Coyote
    ln -s ${coyoteRoot} deps/Coyote
  '';

  cmakeFlags = pkgs.lib.optionals (!pkgs.stdenv.hostPlatform.isx86_64) [
    "-DEN_AVX=0"
  ];

  PYCOYOTE_EN_GPU = if enableGpu then "1" else "0";

  postFixup = ''
    lib_path="$out/${python.sitePackages}/pycoyote:${
      pkgs.lib.makeLibraryPath (
        [
          pkgs.boost
          pkgs.stdenv.cc.cc
        ]
        ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [
          pkgs.numactl
        ]
      )
    }"

    for elf in $out/${python.sitePackages}/pycoyote/*.so*; do
      [ -e "$elf" ] || continue
      if [ -f "$elf" ]; then
        patchelf --set-rpath "$lib_path" "$elf" || true
      fi
    done
  '';

  meta = {
    description = "Python bindings for Coyote's software stack";
    homepage = "https://github.com/fpgasystems/pyCoyote";
    license = pkgs.lib.licenses.mit;
    platforms = pkgs.lib.platforms.linux;
  };
}
