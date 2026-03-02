cmake_minimum_required(VERSION 3.17)
project(EHR_CDSS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Find system libraries
# ---------------------------------------------------------------------------
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Optional: enable real CLIPS engine
# Build with: cmake -DUSE_CLIPS=ON ..
# ---------------------------------------------------------------------------
option(USE_CLIPS "Link against the real CLIPS library" OFF)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    main.cpp
    models/Patient.cpp
    models/Encounter.cpp
    config_manager/ConfigManager.cpp
    database/DatabaseManager.cpp
    cdss/CDSSManager.cpp
    ehr/EHRSystemController.cpp
)

add_executable(ehr_cdss ${SOURCES})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(ehr_cdss PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/clips
    ${SQLite3_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(ehr_cdss PRIVATE
    ${SQLite3_LIBRARIES}
    Threads::Threads
)

if(USE_CLIPS)
    target_compile_definitions(ehr_cdss PRIVATE CLIPS_AVAILABLE)
    # Adjust the path below to where you compiled the CLIPS library:
    target_link_libraries(ehr_cdss PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/clips/libclips.a
    )
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(ehr_cdss PRIVATE /W4)
else()
    target_compile_options(ehr_cdss PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Copy config directory to build tree so the binary can find config.json
# ---------------------------------------------------------------------------
add_custom_command(TARGET ehr_cdss POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:ehr_cdss>/config
    COMMENT "Copying config/ to build directory"
)

add_custom_command(TARGET ehr_cdss POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/cdss/rules
        $<TARGET_FILE_DIR:ehr_cdss>/cdss/rules
    COMMENT "Copying CLIPS rule files to build directory"
)
