# Auto Sort Inventory

A **UE4SS Lua** mod for **Alchemy Factory** that automatically sorts the player inventory by calling the game's built-in `RearrangePlayerInventory()`.

## Default Hotkeys

- **Sort now**: `Ctrl + O`
- **Toggle auto-sort**: `Ctrl + Shift + O`

## Requirements

- **UE4SS** installed for Alchemy Factory
- This mod installed under:
  - `.../AlchemyFactory/Binaries/Win64/ue4ss/Mods/AutoSortInventory/`

## Install

1. Download/copy this folder into your UE4SS Mods directory:

   `AutoSortInventory/`

2. Ensure the script exists at:

   `AutoSortInventory/Scripts/main.lua`

3. Enable the mod:
   - If using the loader app: click **Install**, then **Enable**
   - Or manually: create an empty file named `enabled.txt` inside the mod folder

## Notes

- Auto-sort triggers when the game fires `ABeltTDPlayerController:OnInventoryUpdate()`.
- A small cooldown prevents infinite loops / excessive sorting.

