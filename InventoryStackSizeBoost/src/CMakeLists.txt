cmake_minimum_required(VERSION 3.22)

set(TARGET InventoryStackSizeBoost)

add_library(${TARGET} SHARED
    dllmain.cpp
)

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS)

# Copy the DLL to the game's mod directory after building (optional)
# Uncomment and adjust the path as needed:
# set(GAME_MODS_PATH "F:/SteamLibrary/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/ue4ss/Mods/InventoryStackSizeBoost/dlls")
# add_custom_command(TARGET ${TARGET} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "${GAME_MODS_PATH}/main.dll"
# )
