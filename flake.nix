{
  description = "RPi Pico (ARM & RISC-V) C/C++ Development Shell";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      packages = with pkgs; [
        gcc-arm-embedded                             # arm-none-eabi-gcc
        pkgsCross.riscv32-embedded.buildPackages.gcc # riscv32-none-elf-gcc
        picotool                                     # USB flashing & inspection tool
        pico-sdk                                     # C/C++ SDK (install for 
        cmake
      ];

      # Expose SDK location to CMake
      shellHook = ''
        export PICO_SDK_PATH="${pkgs.pico-sdk}/lib/pico-sdk"
      '';
    };
  };
}
