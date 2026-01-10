# Inventory Stack Size Boost

UE4SS C++ mod for **Alchemy Factory** that increases item stack sizes by patching the game’s `DT_Enemies` `UDataTable` row data (the `MaximumStack` field).

## What it does

- Finds the `DT_Enemies` `UDataTable` at runtime.
- Lets you **patch all stackable items’ `MaximumStack` to a configured value**.
- Lets you **restore the original values** (the mod stores the originals the first time it patches).

## How to use in-game

Once the mod is loaded and the table is found:

- **Shift + J**: patch all item stacks to `MAX_STACK`
- **Shift + K**: restore original stack sizes

> Note: The code currently applies the change when you press the hotkey (it does not permanently patch on startup).

## Installation (UE4SS Mods folder)

Place this mod folder under your game’s UE4SS mods directory so it looks like:

- `...\ue4ss\Mods\InventoryStackSizeBoost\mod.json`
- `...\ue4ss\Mods\InventoryStackSizeBoost\dlls\main.dll`

Then start the game with UE4SS enabled.

## Changing the stack size

Edit the constant in `src/dllmain.cpp`:

- `constexpr int32_t MAX_STACK = 1000;`

Rebuild the DLL and copy the output to:

- `dlls/main.dll`

## Building from source (CMake)

This project is meant to be built against UE4SS’s C++ mod tooling (it links against a `UE4SS` library target).

Typical build commands from the repo root:

```powershell
cmake -S .\src -B .\build
cmake --build .\build --config Release
```

Then copy the built DLL into `dlls\main.dll` (or uncomment/adjust the post-build copy path in `src/CMakeLists.txt`).

## Troubleshooting

- If the hotkeys do nothing, the mod may not have found `DT_Enemies` yet. Keep playing/loading until it’s discovered (the mod retries during updates).
- If you change `MAX_STACK`, you must rebuild and replace `dlls/main.dll` for the change to take effect.

