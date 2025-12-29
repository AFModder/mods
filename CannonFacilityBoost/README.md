# Cannon Facility Boost

A simple **UE4SS Lua** mod for **Alchemy Factory** that buffs the cannon/catapult facility by patching its default values at runtime.

## What it does

When the game creates a `CannonFacilityComponent`, this mod finds the cannon blueprint’s `GEN_VARIABLE` and applies these tweaks:

- **Max distance**: `99999.0`
- **Min distance**: `0.1`
- **Spline points**: `6`
- **Obstacle traces**: `0` (disables obstacle tracing)

It also prints a log message when the patch is applied.

## Requirements

- **UE4SS** installed for Alchemy Factory
- This mod installed under:
  - `.../AlchemyFactory/Binaries/Win64/ue4ss/Mods/CannonFacilityBoost/`

## Install

1. Download/copy this folder into your UE4SS Mods directory:

   `CannonFacilityBoost/`

2. Ensure the script exists at:

   `CannonFacilityBoost/Scripts/main.lua`

3. Enable the mod:
   - If using the loader app: click **Install**, then **Enable**
   - Or manually: create an empty file named `enabled.txt` inside the mod folder

## Notes / Behavior

- The patch is applied **once per session** (the first time the relevant object is seen).
- Changes take effect immediately after the patch runs; you don’t need to restart the game once it has triggered.

## Troubleshooting

- **Nothing changes**: make sure UE4SS is running and the mod is enabled (`enabled.txt` exists).
- **No log output**: UE4SS logging/console output may depend on your UE4SS setup. Look for lines prefixed with:

  `[CannonFacilityBoost]`

## Source

The mod logic lives in `Scripts/main.lua` and hooks `CannonFacilityComponent` creation to apply the patch.