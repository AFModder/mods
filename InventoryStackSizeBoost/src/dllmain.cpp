/**
 * InventoryStackSizeBoost - UE4SS C++ Mod for Alchemy Factory
 * 
 * This mod patches the EnemyConfig DataTable to increase MaximumStack for all items.
 */

#include <vector>
#include <string>
#include <unordered_map>
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include <Windows.h>
#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/Engine/UDataTable.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Core/Containers/Map.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UFunctionStructs.hpp>

using namespace RC;
using namespace RC::Unreal;

// =============================================================================
// Configuration
// =============================================================================

constexpr int32_t MAX_STACK = 1000;

// FBeltTDEnemyConfig struct layout (discovered via runtime analysis):
// MaximumStack @ offset 0x5C (size 4) - the item's max stack size
// SellCanStack @ offset 0x99 (size 1) - whether item can stack
constexpr size_t OFFSET_MAXIMUM_STACK = 0x5C;
constexpr size_t OFFSET_SELL_CAN_STACK = 0x99;

// =============================================================================
// Mod Class
// =============================================================================

class InventoryStackSizeBoost : public CppUserModBase
{
public:
    bool m_patched = false;
    int m_update_count = 0;
    bool m_hook_registered = false;
    std::pair<int, int> m_hook_ids = {-1, -1};
    UFunction* m_getItemTotalStackFunction = nullptr;
    
    // TryExchangeInventorySlot hooking
    bool m_tryExchange_hook_registered = false;
    std::pair<int, int> m_tryExchange_hook_ids = {-1, -1};
    UFunction* m_tryExchangeFunction = nullptr;
    
    UDataTable* m_enemyDataTable = nullptr;
    
    // Store original MaximumStack values for restoration
    std::unordered_map<FName, int32_t> m_originalStackValues;
    bool m_stacksArePatched = false; // Track if stacks are currently patched
    bool m_jKeyPressed = false; // Track J key state to detect press
    bool m_kKeyPressed = false; // Track K key state to detect press

    InventoryStackSizeBoost() : CppUserModBase()
    {
        ModName = STR("InventoryStackSizeBoost");
        ModVersion = STR("1.0.0");
        ModDescription = STR("Increases inventory stack sizes by patching the EnemyConfig DataTable");
        ModAuthors = STR("Local");

        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Mod constructed\n"));
    }

    ~InventoryStackSizeBoost() override = default;

    auto on_unreal_init() -> void override
    {
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] on_unreal_init called\n"));
        // Still need to find DataTable for hooks (but don't patch it)
        TryPatchDataTable();
        TryHookGetItemTotalStack();
        // TryHookTryExchangeInventorySlot();
    }

    auto on_update() -> void override
    {
        // Keep trying to find DataTable until we succeed (needed for exchange hook)
        if (!m_patched)
        {
            m_update_count++;
            // Only try every 100 updates to avoid spam
            if (m_update_count % 100 == 0)
            {
                TryPatchDataTable();
            }
        }
        
        // Try to hook GetItemTotalStack if not already hooked
        if (!m_hook_registered)
        {
            m_update_count++;
            // Only try every 100 updates to avoid spam
            if (m_update_count % 100 == 0)
            {
                TryHookGetItemTotalStack();
            }
        }
        
        // Try to hook TryExchangeInventorySlot function if not already hooked
        // if (!m_tryExchange_hook_registered)
        // {
        //     m_update_count++;
        //     // Only try every 100 updates to avoid spam
        //     if (m_update_count % 100 == 0)
        //     {
        //         TryHookTryExchangeInventorySlot();
        //     }
        // }
        
        // Check for keyboard input (J and K keys)
        if (m_enemyDataTable)
        {
            CheckKeyboardInput();
        }
    }

private:
    void TryPatchDataTable()
    {
        // DISABLED: Testing exchange hook approach instead
        // Still need to find the DataTable for the exchange hook though
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Searching for DataTable (for exchange hook)...\n"));
        
        // Approach: Find all DataTable objects and look for one with "Enemy" or "Config" in the name
        UDataTable* targetDataTable = nullptr;
        int dataTableCount = 0;
        
        // Use FindAllOf to get all DataTable objects
        std::vector<UObject*> dataTables;
        UObjectGlobals::FindAllOf(STR("DataTable"), dataTables);
        
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Found {} DataTable objects\n"), dataTables.size());
        
        for (UObject* obj : dataTables)
        {
            if (!obj) continue;
            
            dataTableCount++;
            StringType name = obj->GetName();
            StringType fullName = obj->GetFullName();
            
            Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost]   DataTable: {}\n"), fullName);
            
            // Try DT_Enemies - the actual item/enemy config table
            if (name == STR("DT_Enemies"))
            {
                targetDataTable = static_cast<UDataTable*>(obj);
                m_enemyDataTable = targetDataTable; // Store for exchange hook
                Output::send<LogLevel::Default>(STR("[InventoryStackSizeBoost] Selected DataTable: {} (stored for exchange hook)\n"), fullName);
                break; // Found it, stop searching
            }
        }
        
        if (dataTableCount == 0)
        {
            Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] No DataTables found yet, will retry...\n"));
            return; // Will retry
        }
        
        if (!targetDataTable)
        {
            Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] DT_Enemies not found among {} tables, will retry...\n"), dataTableCount);
            return; // Will retry
        }
        
        // DISABLED: Testing OnInventoryUpdate hook approach instead
        // PatchDataTableRows(targetDataTable);
        m_patched = true; // Mark as "done" so we don't keep retrying
        Output::send<LogLevel::Default>(STR("[InventoryStackSizeBoost] DataTable found and stored (patching disabled for OnInventoryUpdate hook testing)\n"));
    }

    void PatchDataTableRows(UDataTable* dataTable)
    {
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Patching DataTable: {}\n"), dataTable->GetFullName());
        
        // Get the row struct to find property offsets
        UScriptStruct* rowStruct = dataTable->GetRowStruct();
        if (rowStruct)
        {
            Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] RowStruct: {}\n"), rowStruct->GetFullName());
            
            // Enumerate properties to find MaximumStack offset
            Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Struct properties:\n"));
            for (FProperty* prop = rowStruct->GetFirstProperty(); prop; prop = prop->GetNextFieldAsProperty())
            {
                Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost]   {} @ offset 0x{:X} (size {})\n"), 
                    prop->GetName(), prop->GetOffset_Internal(), prop->GetSize());
            }
        }
        
        // GetRowMap returns TMap<FName, unsigned char*>
        // Note: GetRowMap() returns const reference, but we need to modify values
        // The actual data is mutable, so we can cast away const
        TMap<FName, unsigned char*>& rowMap = const_cast<TMap<FName, unsigned char*>&>(dataTable->GetRowMap());
        
        int32_t numRows = rowMap.Num();
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] DataTable has {} rows\n"), numRows);

        if (numRows == 0)
        {
            Output::send<LogLevel::Warning>(STR("[InventoryStackSizeBoost] DataTable is empty!\n"));
            return;
        }

        int patchedCount = 0;
        int skippedCount = 0;
        for (auto& pair : rowMap)
        {
            unsigned char* rowData = pair.Value;
            if (!rowData) continue;
            
            // MaximumStack is at offset 0x5C
            int32_t* maxStackPtr = reinterpret_cast<int32_t*>(rowData + OFFSET_MAXIMUM_STACK);
            int32_t oldMaxStack = *maxStackPtr;
            
            // Patch ALL items that have a positive stack limit less than our target
            // (removed SellCanStack check - it was filtering out most items)
            if (oldMaxStack > 0 && oldMaxStack < MAX_STACK)
            {
                *maxStackPtr = MAX_STACK;
                patchedCount++;
                
                // Log first few patches for verification
                if (patchedCount <= 10)
                {
                    Output::send<LogLevel::Verbose>(
                        STR("[InventoryStackSizeBoost] Patched '{}': MaxStack {} -> {}\n"),
                        pair.Key.ToString(), oldMaxStack, MAX_STACK);
                }
            }
            else if (oldMaxStack == 0)
            {
                skippedCount++;
            }
        }
        
        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Skipped {} items with MaxStack=0 (non-stackable)\n"), skippedCount);

        Output::send<LogLevel::Default>(
            STR("[InventoryStackSizeBoost] SUCCESS: Patched {} items to MaxStack={}\n"),
            patchedCount, MAX_STACK);
        
        m_patched = true;
    }

    void TryHookGetItemTotalStack()
    {
        if (m_hook_registered)
        {
            return;
        }

        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Attempting to hook GetItemTotalStack...\n"));

        // Skip StaticFindObject path searches as they can cause fatal errors with invalid paths
        // Instead, search all functions directly
        Output::send<LogLevel::Verbose>(
            STR("[InventoryStackSizeBoost] Searching for function by name...\n"));
        
        std::vector<UObject*> allFunctions;
        UObjectGlobals::FindAllOf(STR("Function"), allFunctions);
        
        Output::send<LogLevel::Verbose>(
            STR("[InventoryStackSizeBoost] Found {} Function objects to search\n"), allFunctions.size());
        
        for (UObject* obj : allFunctions)
        {
            if (!obj) continue;
            
            StringType name = obj->GetName();
            StringType fullName = obj->GetFullName();
            
            // Check if name contains "GetItemTotalStack" and fullName contains "BeltTDInventoryInstance"
            // StringType is std::wstring, so we can use find() directly
            if (name.find(STR("GetItemTotalStack")) != StringType::npos &&
                fullName.find(STR("BeltTDInventoryInstance")) != StringType::npos)
            {
                Output::send<LogLevel::Default>(
                    STR("[InventoryStackSizeBoost] Found matching function: {}\n"), fullName);
                m_getItemTotalStackFunction = static_cast<UFunction*>(obj);
                break;
            }
        }

        if (!m_getItemTotalStackFunction)
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] GetItemTotalStack function not found yet, will retry...\n"));
            return;
        }

        // Register post-hook to capture return value
        auto postHook = [](UnrealScriptFunctionCallableContext& Context, void* CustomData) -> void {
            InventoryStackSizeBoost* mod = static_cast<InventoryStackSizeBoost*>(CustomData);
            
            // Get the return value (assuming it's an int32)
            int32_t returnValue = *static_cast<int32_t*>(Context.RESULT_DECL);
            
            // Try to get function parameters to see what was passed in
            Output::send<LogLevel::Default>(
                STR("[InventoryStackSizeBoost] GetItemTotalStack called on {} -> ReturnValue: {}\n"),
                Context.Context->GetFullName(), returnValue);
        };

        try
        {
            m_hook_ids = UObjectGlobals::RegisterHook(
                m_getItemTotalStackFunction,
                nullptr, // Pre-hook (nullptr = no pre-hook)
                postHook,
                this // CustomData
            );
            
            Output::send<LogLevel::Default>(
                STR("[InventoryStackSizeBoost] SUCCESS: Hooked GetItemTotalStack (IDs: {}, {})\n"),
                m_hook_ids.first, m_hook_ids.second);
            
            m_hook_registered = true;
        }
        catch (const std::exception& e)
        {
            // Convert exception message to StringType (wstring)
            std::string errorStr = e.what();
            StringType errorMsg = STR("Failed to register hook: ");
            errorMsg += StringType(errorStr.begin(), errorStr.end());
            Output::send<LogLevel::Warning>(
                STR("[InventoryStackSizeBoost] {}\n"), errorMsg);
        }
    }

    void TryHookTryExchangeInventorySlot()
    {
        if (m_tryExchange_hook_registered)
        {
            return;
        }

        // Need the DataTable first
        if (!m_enemyDataTable)
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] TryExchangeInventorySlot hook: DataTable not ready yet, will retry...\n"));
            return;
        }

        Output::send<LogLevel::Verbose>(STR("[InventoryStackSizeBoost] Attempting to hook TryExchangeInventorySlot function...\n"));

        // Search for the function
        std::vector<UObject*> allFunctions;
        UObjectGlobals::FindAllOf(STR("Function"), allFunctions);
        
        Output::send<LogLevel::Verbose>(
            STR("[InventoryStackSizeBoost] Searching {} functions for TryExchangeInventorySlot...\n"), allFunctions.size());
        
        // Look for TryExchangeInventorySlot in UBeltTDInventoryComponent
        for (UObject* obj : allFunctions)
        {
            if (!obj) continue;
            
            StringType name = obj->GetName();
            StringType fullName = obj->GetFullName();
            
            // Check if this matches TryExchangeInventorySlot in BeltTDInventoryComponent
            if (name.find(STR("TryExchangeInventorySlot")) != StringType::npos &&
                fullName.find(STR("BeltTDInventoryComponent")) != StringType::npos)
            {
                Output::send<LogLevel::Default>(
                    STR("[InventoryStackSizeBoost] Found TryExchangeInventorySlot function: {}\n"), fullName);
                m_tryExchangeFunction = static_cast<UFunction*>(obj);
                break;
            }
        }

        if (!m_tryExchangeFunction)
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] TryExchangeInventorySlot function not found yet, will retry...\n"));
            return;
        }

        // Pre-hook: Temporarily patch stacks to MAX_STACK BEFORE TryExchangeInventorySlot runs
        auto preHook = [](UnrealScriptFunctionCallableContext& Context, void* CustomData) -> void {
            InventoryStackSizeBoost* mod = static_cast<InventoryStackSizeBoost*>(CustomData);
            
            if (!mod->m_enemyDataTable)
            {
                return;
            }

            // Don't patch if manual patching is active (user pressed J)
            if (mod->m_stacksArePatched)
            {
                Output::send<LogLevel::Verbose>(
                    STR("[InventoryStackSizeBoost] TryExchangeInventorySlot pre-hook: Skipping (manual patch active)\n"));
                return;
            }

            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] TryExchangeInventorySlot pre-hook: Patching stacks to MAX_STACK...\n"));

            // Use the shared patching function, but don't set m_stacksArePatched flag
            mod->PatchAllStacksToMaxInternal(false);
        };

        // Post-hook: Restore original MaximumStack values AFTER TryExchangeInventorySlot completes
        auto postHook = [](UnrealScriptFunctionCallableContext& Context, void* CustomData) -> void {
            InventoryStackSizeBoost* mod = static_cast<InventoryStackSizeBoost*>(CustomData);
            
            if (!mod->m_enemyDataTable)
            {
                return;
            }

            // Don't restore if manual patching is active (user pressed J)
            if (mod->m_stacksArePatched)
            {
                Output::send<LogLevel::Verbose>(
                    STR("[InventoryStackSizeBoost] TryExchangeInventorySlot post-hook: Skipping restore (manual patch active)\n"));
                return;
            }

            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] TryExchangeInventorySlot post-hook: Restoring stacks to default...\n"));

            // Use the shared restore function, but don't set m_stacksArePatched flag
            mod->RestoreAllStacksToDefaultInternal(false);
        };

        try
        {
            m_tryExchange_hook_ids = UObjectGlobals::RegisterHook(
                m_tryExchangeFunction,
                preHook,
                postHook,
                this // CustomData
            );
            
            Output::send<LogLevel::Default>(
                STR("[InventoryStackSizeBoost] SUCCESS: Hooked TryExchangeInventorySlot function (IDs: {}, {})\n"),
                m_tryExchange_hook_ids.first, m_tryExchange_hook_ids.second);
            
            m_tryExchange_hook_registered = true;
        }
        catch (const std::exception& e)
        {
            std::string errorStr = e.what();
            StringType errorMsg = STR("Failed to register TryExchangeInventorySlot hook: ");
            errorMsg += StringType(errorStr.begin(), errorStr.end());
            Output::send<LogLevel::Warning>(
                STR("[InventoryStackSizeBoost] {}\n"), errorMsg);
        }
    }

    void CheckKeyboardInput()
    {
        // Check Shift key state
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        
        // Check Shift+J key (patch to max)
        bool jKeyDown = (GetAsyncKeyState('J') & 0x8000) != 0;
        if (shiftPressed && jKeyDown && !m_jKeyPressed)
        {
            m_jKeyPressed = true;
            PatchAllStacksToMax();
        }
        else if (!shiftPressed || !jKeyDown)
        {
            m_jKeyPressed = false;
        }
        
        // Check Shift+K key (restore to default)
        bool kKeyDown = (GetAsyncKeyState('K') & 0x8000) != 0;
        if (shiftPressed && kKeyDown && !m_kKeyPressed)
        {
            m_kKeyPressed = true;
            RestoreAllStacksToDefault();
        }
        else if (!shiftPressed || !kKeyDown)
        {
            m_kKeyPressed = false;
        }
    }

    void PatchAllStacksToMax()
    {
        if (!m_enemyDataTable)
        {
            Output::send<LogLevel::Warning>(
                STR("[InventoryStackSizeBoost] Cannot patch stacks: DataTable not available!\n"));
            return;
        }

        // If stacks are already patched, don't do anything
        if (m_stacksArePatched)
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] Stacks are already patched to MAX_STACK\n"));
            return;
        }

        Output::send<LogLevel::Default>(
            STR("[InventoryStackSizeBoost] Patching all stacks to MAX_STACK (pressed J)...\n"));

        // Use the shared function and set the flag
        PatchAllStacksToMaxInternal(true);
    }

    void PatchAllStacksToMaxInternal(bool setPatchedFlag)
    {
        if (!m_enemyDataTable)
        {
            return;
        }

        // Get the DataTable row map (need non-const reference to modify)
        TMap<FName, unsigned char*>& rowMap = const_cast<TMap<FName, unsigned char*>&>(m_enemyDataTable->GetRowMap());
        
        // Only store original values if we don't have them yet (first time patching)
        bool isFirstPatch = m_originalStackValues.empty();
        
        // Set all MaximumStack values to MAX_STACK
        int modifiedCount = 0;
        int alreadyAtMaxCount = 0;
        for (auto& pair : rowMap)
        {
            unsigned char* rowData = pair.Value;
            if (!rowData) continue;
            
            int32_t* maxStackPtr = reinterpret_cast<int32_t*>(rowData + OFFSET_MAXIMUM_STACK);
            int32_t currentValue = *maxStackPtr;
            
            // Store original value ONLY on first patch (never overwrite)
            if (isFirstPatch)
            {
                m_originalStackValues[pair.Key] = currentValue;
            }
            
            // Set to max if it's a valid stackable item
            if (currentValue > 0 && currentValue < MAX_STACK)
            {
                *maxStackPtr = MAX_STACK;
                modifiedCount++;
            }
            else if (currentValue >= MAX_STACK)
            {
                alreadyAtMaxCount++;
            }
        }
        
        if (setPatchedFlag)
        {
            m_stacksArePatched = true;
            if (isFirstPatch)
            {
                Output::send<LogLevel::Default>(
                    STR("[InventoryStackSizeBoost] Patched {} items to MAX_STACK={} (stored {} original values)\n"), 
                    modifiedCount, MAX_STACK, m_originalStackValues.size());
            }
            else
            {
                Output::send<LogLevel::Default>(
                    STR("[InventoryStackSizeBoost] Patched {} items to MAX_STACK={} (using stored original values)\n"), 
                    modifiedCount, MAX_STACK);
            }
        }
        else
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] Patched {} items to MAX_STACK\n"), 
                modifiedCount);
        }
    }

    void RestoreAllStacksToDefault()
    {
        if (!m_enemyDataTable)
        {
            Output::send<LogLevel::Warning>(
                STR("[InventoryStackSizeBoost] Cannot restore stacks: DataTable not available!\n"));
            return;
        }

        if (!m_stacksArePatched)
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] Stacks are already at default values\n"));
            return;
        }

        if (m_originalStackValues.empty())
        {
            Output::send<LogLevel::Warning>(
                STR("[InventoryStackSizeBoost] No original values stored to restore!\n"));
            return;
        }

        Output::send<LogLevel::Default>(
            STR("[InventoryStackSizeBoost] Restoring all stacks to default (pressed K)...\n"));

        // Use the shared function and set the flag
        RestoreAllStacksToDefaultInternal(true);
    }

    void RestoreAllStacksToDefaultInternal(bool setPatchedFlag)
    {
        if (!m_enemyDataTable || m_originalStackValues.empty())
        {
            return;
        }

        // Restore original values (need non-const reference to modify)
        TMap<FName, unsigned char*>& rowMap = const_cast<TMap<FName, unsigned char*>&>(m_enemyDataTable->GetRowMap());
        
        int restoredCount = 0;
        int unchangedCount = 0;
        int notFoundCount = 0;
        
        // Log a few examples before/after for debugging (only for manual restore)
        int logCount = 0;
        const int maxLogExamples = setPatchedFlag ? 5 : 0;
        
        for (const auto& [itemName, originalValue] : m_originalStackValues)
        {
            auto* rowDataPtr = rowMap.Find(itemName);
            if (rowDataPtr && *rowDataPtr)
            {
                unsigned char* rowData = *rowDataPtr;
                int32_t* maxStackPtr = reinterpret_cast<int32_t*>(rowData + OFFSET_MAXIMUM_STACK);
                int32_t currentValue = *maxStackPtr;
                
                // Log first few examples (only for manual restore)
                if (logCount < maxLogExamples)
                {
                    Output::send<LogLevel::Verbose>(
                        STR("[InventoryStackSizeBoost] Restoring '{}': {} -> {}\n"),
                        itemName.ToString(), currentValue, originalValue);
                    logCount++;
                }
                
                // Restore the original value
                *maxStackPtr = originalValue;
                
                // Verify the restore worked
                int32_t verifyValue = *maxStackPtr;
                if (verifyValue == originalValue)
                {
                    if (currentValue != originalValue)
                    {
                        restoredCount++;
                    }
                    else
                    {
                        unchangedCount++;
                    }
                }
                else
                {
                    Output::send<LogLevel::Warning>(
                        STR("[InventoryStackSizeBoost] WARNING: Restore failed for '{}': expected {}, got {}\n"),
                        itemName.ToString(), originalValue, verifyValue);
                }
            }
            else
            {
                notFoundCount++;
            }
        }
        
        if (setPatchedFlag)
        {
            m_stacksArePatched = false;
            // Keep m_originalStackValues for potential re-patching
            Output::send<LogLevel::Default>(
                STR("[InventoryStackSizeBoost] Restored {} items to default values ({} unchanged, {} not found)\n"), 
                restoredCount, unchangedCount, notFoundCount);
        }
        else
        {
            Output::send<LogLevel::Verbose>(
                STR("[InventoryStackSizeBoost] Restored {} items to default\n"), 
                restoredCount);
        }
    }
};

// =============================================================================
// Mod Entry Points
// =============================================================================

#define MOD_API __declspec(dllexport)

extern "C"
{
    MOD_API CppUserModBase* start_mod()
    {
        return new InventoryStackSizeBoost();
    }

    MOD_API void uninstall_mod(CppUserModBase* mod)
    {
        delete mod;
    }
}
