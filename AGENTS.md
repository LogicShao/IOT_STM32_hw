# Repository Guidelines

## Project Structure & Module Organization
This repository is an STM32F070 CubeMX project built with CMake. Active application code lives in `Core/Src`, with public headers in `Core/Inc`. Treat `Drivers/` as third-party vendor code (`CMSIS` and `STM32F0xx_HAL_Driver`) and avoid manual edits there unless a vendor patch is required. Build logic lives in `CMakeLists.txt`, `CMakePresets.json`, and `cmake/`. The hardware definition source of truth is `BlankProject-STM32F070CBT6.ioc`. Root-level `main_Elevator.c` and `main_TrafficLight.c` are reference files; the default target builds `Core/Src/main.c`.

## Build, Test, and Development Commands
Prerequisite: `arm-none-eabi-*` tools and `ninja` must be in `PATH`.

- `cmake --preset Debug`: configure a debug build in `build/Debug`.
- `cmake --build --preset Debug`: build the ELF image with debug symbols.
- `cmake --preset Release`: configure an optimized release build.
- `cmake --build --preset Release`: verify the release profile also compiles cleanly.

If you change clocks, pins, or peripherals, regenerate from the `.ioc` file and review the diff before committing.

## Coding Style & Naming Conventions
Use C11 and follow the existing STM32/HAL style: 4-space indentation, braces on the same line, and short single-purpose functions. Keep module files lowercase, paired as `Core/Src/<module>.c` and `Core/Inc/<module>.h`. Preserve established API naming such as `TEST_LED_On`, `MX_GPIO_Init`, and `Error_Handler`. Comments should stay in English to match the current codebase. No formatter or linter config is committed, so keep `-Wall` builds warning-free.

**File naming**: Use short English abbreviations, not long descriptive names. Prefer `lab_req.md` over `experiment_requirements_summary.md`. Keep doc filenames concise and lowercase.

## Testing Guidelines
No unit test framework or `tests/` directory is configured yet. Every change must pass a clean `Debug` and `Release` build. For hardware-facing changes, run a board smoke test covering boot, GPIO, and any touched peripherals such as UART, I2C, or timers. Document the exact validation steps in your PR.

## Commit & Pull Request Guidelines
**AI 仅拥有 Git 只读权限，禁止执行任何 Git 写操作（commit、push、rebase、tag 等）。** Commit 由用户自行完成。
This repository currently has no Git history, so there is no inherited commit convention to copy. Use concise imperative Conventional Commit messages such as `feat: add rs485 transmit helper` or `fix: correct timer prescaler`. Keep PRs small, describe the hardware impact, list regenerated CubeMX files, and include proof of validation such as serial logs, logic-analyzer captures, or board photos when behavior is visible.

## STM32-Specific Notes
Prefer edits inside `USER CODE BEGIN/END` regions for CubeMX-managed files. Avoid hand-editing generated initialization unless regeneration cannot express the change. Keep linker script, startup file, and `.ioc` updates synchronized.
