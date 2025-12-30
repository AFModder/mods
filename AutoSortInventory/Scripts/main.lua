local UEHelpers = require("UEHelpers")

local MOD_TAG = "[AutoSortInventory]"

local function log(msg)
    print(string.format("%s %s\n", MOD_TAG, msg))
end

-- =========================
-- Config
-- =========================

-- Auto-sort whenever the game reports inventory updates.
local AUTO_SORT_ENABLED = true

-- Prevent spam / potential recursive updates.
local AUTO_SORT_COOLDOWN_SECONDS = 0.35

-- Hotkeys
local SORT_NOW_KEY = Key.O
local SORT_NOW_MODS = { ModifierKey.CONTROL }

local TOGGLE_AUTO_KEY = Key.O
local TOGGLE_AUTO_MODS = { ModifierKey.CONTROL, ModifierKey.SHIFT }

-- =========================
-- Helpers
-- =========================

local function GetNowSeconds()
    local World = UEHelpers.GetWorld()
    if World and World.IsValid and World:IsValid() and World.GetTimeSeconds then
        -- UE4SS can sometimes hand us a "valid-looking" userdata that still has a nullptr instance during map loads/unloads.
        local ok, t = pcall(function()
            return World:GetTimeSeconds()
        end)
        if ok and type(t) == "number" then
            return t
        end
    end
    return os.clock()
end

---@param PC any?
---@return UObject
local function GetPlayerInventoryComponent(PC)
    if not PC or not PC.IsValid or not PC:IsValid() then
        PC = UEHelpers.GetPlayerController()
    end
    if not PC or not PC.IsValid or not PC:IsValid() then
        return CreateInvalidObject()
    end

    -- Property is present on ABeltTDPlayerController
    local Inv = PC.PlayerInventory
    if Inv and Inv.IsValid and Inv:IsValid() then
        return Inv
    end

    -- Fallback to UFunction
    if PC.GetPlayerInventory then
        Inv = PC:GetPlayerInventory()
        if Inv and Inv.IsValid and Inv:IsValid() then
            return Inv
        end
    end

    return CreateInvalidObject()
end

local function SortPlayerInventory(PC, Reason, OnDone)
    local Inv = GetPlayerInventoryComponent(PC)
    if not Inv:IsValid() then
        if Reason then
            log(string.format("Sort skipped (%s): PlayerInventory not available yet", Reason))
        end
        if OnDone then
            OnDone(false)
        end
        return
    end

    ExecuteInGameThread(function()
        local ok = false
        if Inv:IsValid() and Inv.RearrangePlayerInventory then
            Inv:RearrangePlayerInventory()
            ok = true
            if Reason then
                log(string.format("Sorted player inventory (%s)", Reason))
            else
                log("Sorted player inventory")
            end
        end
        if OnDone then
            OnDone(ok)
        end
    end)
end

local function RegisterKeybindSafe(KeyCode, Modifiers, Fn)
    if RegisterKeyBindAsync then
        if (not IsKeyBindRegistered) or (not IsKeyBindRegistered(KeyCode, Modifiers)) then
            RegisterKeyBindAsync(KeyCode, Modifiers, Fn)
        end
    elseif RegisterKeyBind then
        -- Fallback: no modifiers supported on the sync API
        RegisterKeyBind(KeyCode, Fn)
    else
        log("Keybind registration API not found (RegisterKeyBindAsync/RegisterKeyBind)")
    end
end

-- =========================
-- Hotkeys
-- =========================

RegisterKeybindSafe(SORT_NOW_KEY, SORT_NOW_MODS, function()
    SortPlayerInventory(nil, "hotkey")
end)

RegisterKeybindSafe(TOGGLE_AUTO_KEY, TOGGLE_AUTO_MODS, function()
    AUTO_SORT_ENABLED = not AUTO_SORT_ENABLED
    log(string.format("Auto-sort %s", AUTO_SORT_ENABLED and "ENABLED" or "DISABLED"))
end)

-- =========================
-- Auto-sort hook
-- =========================

local LastAutoSortAt = 0.0
local IsAutoSorting = false

PreOnInvUpdate, PostOnInvUpdate = RegisterHook(
    "/Script/BeltTD.BeltTDPlayerController:OnInventoryUpdate",
    ---@param Context RemoteUnrealParam<APlayerController>
    function(Context)
        if not AUTO_SORT_ENABLED or IsAutoSorting then
            return
        end

        local now = GetNowSeconds()
        if (now - LastAutoSortAt) < AUTO_SORT_COOLDOWN_SECONDS then
            return
        end

        local PC = Context and Context.get and Context:get() or nil
        IsAutoSorting = true
        LastAutoSortAt = now

        SortPlayerInventory(PC, "auto", function()
            IsAutoSorting = false
        end)
    end
)

log("Loaded. Ctrl+O = sort now, Ctrl+Shift+O = toggle auto-sort.")

