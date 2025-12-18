cmake_minimum_required(VERSION 3.16)
project(FreeRTOS_PC_Scheduler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# FreeRTOS dizinleri
set(FREERTOS_DIR ${CMAKE_SOURCE_DIR}/FreeRTOS)
set(FREERTOS_SRC ${FREERTOS_DIR}/source)
set(FREERTOS_INC ${FREERTOS_DIR}/include)

# Platform tespiti
if(WIN32)
    set(FREERTOS_PORT ${FREERTOS_DIR}/portable/MSVC-MingW)
    add_definitions(-DWIN32 -D_WIN32)
else()
    set(FREERTOS_PORT ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix)
    add_definitions(-D_POSIX_C_SOURCE=200809L)
endif()

set(FREERTOS_MEMMANG ${FREERTOS_DIR}/portable/MemMang)

# Include dizinleri
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${FREERTOS_INC}
    ${FREERTOS_PORT}
)

# POSIX port için ek include
if(NOT WIN32)
    include_directories(${FREERTOS_PORT}/utils)
endif()

# FreeRTOS kaynak dosyalari
set(FREERTOS_SOURCES
    ${FREERTOS_SRC}/tasks.c
    ${FREERTOS_SRC}/queue.c
    ${FREERTOS_SRC}/list.c
    ${FREERTOS_SRC}/timers.c
    ${FREERTOS_SRC}/event_groups.c
    ${FREERTOS_PORT}/port.c
    ${FREERTOS_MEMMANG}/heap_4.c
)

# POSIX port için ek dosyalar
if(NOT WIN32)
    list(APPEND FREERTOS_SOURCES ${FREERTOS_PORT}/utils/wait_for_event.c)
endif()

# Uygulama kaynak dosyalari
set(APP_SOURCES
    src/main_freertos.c
)

# Executable
add_executable(freertos_sim ${APP_SOURCES} ${FREERTOS_SOURCES})

# Platform bagimliliklari
if(WIN32)
    target_link_libraries(freertos_sim winmm ws2_32)
else()
    target_link_libraries(freertos_sim pthread rt)
endif()

# Derleme flaglari
if(MSVC)
    target_compile_options(freertos_sim PRIVATE /W4)
else()
    target_compile_options(freertos_sim PRIVATE -Wall -Wextra -g)
endif()

# Calistirma komutlari
add_custom_target(run
    COMMAND freertos_sim ${CMAKE_SOURCE_DIR}/giris.txt
    DEPENDS freertos_sim
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Simulasyon baslatiliyor..."
)
