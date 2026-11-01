{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "rtxui";
  version = "@VERSION@";

  src = fetchFromGitHub {
    owner = "ArthurSonzogni";
    repo = "RTXUI";
    tag = "v${finalAttrs.version}";
    hash = "@NIX_HASH@";
  };

  __structuredAttrs = true;
  strictDeps = true;

  nativeBuildInputs = [
    cmake
    ninja
  ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_SHARED_LIBS" (!stdenv.hostPlatform.isStatic))
    (lib.cmakeBool "RTXUI_BUILD_TESTS" false)
    (lib.cmakeBool "RTXUI_BUILD_EXAMPLES" false)
  ];

  meta = {
    homepage = "https://github.com/ArthurSonzogni/RTXUI";
    changelog = "https://github.com/ArthurSonzogni/RTXUI/releases/tag/v${finalAttrs.version}";
    description = "C++ terminal user interfaces built from HTML templates, CSS and reactive state";
    license = lib.licenses.mit;
    maintainers = with lib.maintainers; [ arthursonzogni ];
    platforms = lib.platforms.unix;
  };
})
