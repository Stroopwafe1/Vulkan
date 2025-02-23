{ pkgs ? (import <nixpkgs> {}) }:
with pkgs;
mkShell {
  buildInputs = [
    libGL
    glm
    glfw
    mesa
    gnumake
    gcc
    clang
    
    glslang
    shaderc
    vulkan-headers
    vulkan-loader
    vulkan-validation-layers
    vulkan-tools
    vulkan-tools-lunarg
    assimp
    valgrind
  ];

  # If it doesnt get picked up through nix magic
  VULKAN_SDK = "${vulkan-headers}";
  VK_LAYER_PATH = "${vulkan-validation-layers}/share/vulkan/explicit_layer.d";

  shellHook = ''
    kdevelop . &>/dev/null &
    qrenderdoc &>/dev/null &
  '';
}
