cmake_minimum_required(VERSION 3.28)
project(vexspoke C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(APPLE)
    enable_language(OBJC)
    set(CMAKE_OBJC_STANDARD 23)
    set(CMAKE_OBJC_STANDARD_REQUIRED ON)
endif()

# Release: dead-code stripping so unused symbols cost zero bytes in artifacts.
# (Duplicated per repo for standalone builds; harmless when built in-tree.)
if(APPLE)
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,-dead_strip>)
elseif(UNIX)
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections>)
endif()

if(NOT TARGET vexspoke)
# Core VexSpoke (formerly Anti) Runtime Library
add_library(vexspoke STATIC
    src/bit/bit.c
    src/deferred/dispatch.c
    src/nio/mem.c
    src/c23/equals.c
    src/c23/free.c
    src/oop/type.c
    src/oop/class.c
    src/oop/field.c
    src/oop/stride.c
    src/oop/struct.c
    src/cli/command.c
    src/cli/commandparser.c
    src/cli/commandregistry.c
    src/cli/console.c
    src/cli/logcommands.c
    src/cli/scanner.c
    src/primitive/string.c
    src/primitive/int.c
    src/primitive/long.c
    src/primitive/float.c
    src/primitive/double.c
    src/primitive/byte.c
    src/primitive/short.c
    src/primitive/bool.c
    src/primitive/brain.c
    src/primitive/fixed32.c
    src/primitive/fixed64.c
    src/primitive/int_float.c
    src/primitive/int_double.c
    src/primitive/long_float.c
    src/primitive/long_double.c
    src/primitive/pack.c
    src/engine/loop.c
    src/spoke/bespoke.c
    src/math/coord_frame.c
    src/math/strict_math.c
    src/math/fast_math.c
    src/math/math.c
    src/lang/vec2.c
    src/lang/vec2/vec2.c
    src/lang/vec2/vec2d.c
    src/lang/vec3.c
    src/lang/vec3/vec3.c
    src/lang/vec3/vec3d.c
    src/lang/vec3/vec3_int_float.c
    src/lang/vec3/vec3_long_double.c
    src/lang/vec4.c
    src/lang/vec4/vec4.c
    src/lang/vec4/vec4d.c
    src/lang/point/point.c
    src/lang/rect/rectangle.c
    src/lang/mat4.c
    src/lang/fastmath.c
    src/lang/str.c
    src/search/calc.c
    src/search/find.c
    src/search/spotlight.c
    src/algo/draft_sort.c
    src/algo/radix_sort.c
    src/algo/dijkstra.c
    src/algo/kd_tree.c
    src/algo/bvh.c
    src/algo/tree_sit.c
    src/struct/array.c
    src/struct/circle_array.c
    src/struct/sphere_array.c
    src/struct/octree.c
    src/struct/collection.c
    src/struct/chunked_list.c
    src/struct/deque.c
    src/struct/list.c
    src/struct/map.c
    src/struct/minheap.c
    src/struct/queue.c
    src/struct/set.c
    src/struct/sparseset.c
    src/struct/stack.c
    src/security/crypto.c
    src/exception/exception.c
    src/io/vexhome.c
    src/io/cache.c
    src/io/file.c
    src/io/filewriter.c
    src/io/ws_client.c
    src/io/process_spawn.c
    src/io/log.c
    src/io/logparser.c
    src/io/hot_file.c
    src/input/focus.c
    src/input/key.c
    src/input/key_map.c
    src/input/mouse.c
    src/input/touch.c
    src/input/piano_key.c
    src/input/turntable.c
    src/input/gesture.c
    src/input/gamepad.c
    src/input/hardware_event.c
    src/net/http.c
    src/net/download.c
    src/net/json.c
    src/net/netfacade.c
    src/net/url.c
    src/objects/choice.c
    src/objects/future.c
    src/objects/global.c
    src/objects/local.c
    src/objects/passive.c
    src/objects/probable.c
    src/objects/probable_objects.c
    src/objects/reactive.c
    src/relational/variable.c
    src/relational/variable_pool.c
    src/relational/variable_slot.c
    src/relational/variable_hash_map.c
    src/relational/variable_mini_map.c
    src/relational/cell.c
    src/relational/shelf.c
    src/algo/segment_index.c
    src/reflection/reflect_class.c
    src/reflection/reflect_field.c
    src/reflection/reflect_variable.c
    src/reflection/reflect_method.c
    src/reflection/reflect_generic.c
    src/relational/relational.c
    src/atomic/ring.c
    src/atomic/registry.c
    src/atomic/spin.c
    src/thread/thread.c
    src/thread/networking.c
    src/thread/event.c
    src/thread/draw.c
    src/thread/scripting.c
    src/thread/ui.c
    src/time/calendar.c
    src/time/clock.c
    src/time/datetime.c
    src/time/nanotime.c
    src/util/arrays.c
    src/util/hash.c
    src/util/random.c
    src/system/system.c
    src/system/display_monitor.c
    src/system/display_info.c
    src/system/hardware_info.c
    src/system/graphics_info.c
    src/system/app_detect.c
    src/system/process_probe.c
    src/system/capture_tool.c
)

target_include_directories(vexspoke PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(vexspoke PUBLIC $<$<CONFIG:Debug>:DEBUG_BORROW_CHECK=1>)

target_compile_options(vexspoke PRIVATE -Wall -Wextra -Werror -mcpu=native)
target_link_libraries(vexspoke PUBLIC pthread)

if(APPLE)
    target_sources(vexspoke PRIVATE
        src/objc/touchid_cocoa.m
        src/objc/tls_apple.m
        src/objc/audio_cocoa.m
        src/objc/discovery.m
        src/system/image_mac.m
        src/objc/clipboard_mac.m
        src/objc/audio_hal_mac.m
    )
    set_source_files_properties(
        src/objc/touchid_cocoa.m
        src/objc/tls_apple.m
        src/objc/audio_cocoa.m
        src/objc/discovery.m
        src/system/image_mac.m
        src/objc/clipboard_mac.m
        src/objc/audio_hal_mac.m
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(vexspoke PUBLIC
        "-framework Foundation"
        "-framework LocalAuthentication"
        "-framework Network"
        "-framework Security"
        "-framework AVFoundation"
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreGraphics"
        "-framework Metal"
        "-framework IOKit"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
else()
    target_sources(vexspoke PRIVATE
        src/security/touchid.c
        src/net/tls_curl.c
        src/audio/audio_stub.c
        src/io/clipboard_stub.c
        src/audio/audio_hal_stub.c
    )
    find_package(CURL REQUIRED)
    target_link_libraries(vexspoke PUBLIC CURL::libcurl)
endif()
endif() # NOT TARGET vexspoke

# Headless demo target
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main/main.c" AND NOT TARGET vexspoke_demo)
    add_executable(vexspoke_demo
        src/main/main.c
    )
    target_link_libraries(vexspoke_demo PRIVATE vexspoke)
endif()

set(VEXSPOKE_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/vexspoke")
if(NOT EXISTS "${VEXSPOKE_TEST_DIR}")
    set(VEXSPOKE_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

# Deferred dispatch queue: MPSC post, one-consumer drain, steal-the-batch
# semantics, FIFO order, growable slab. Headless — runs everywhere.
if(EXISTS "${VEXSPOKE_TEST_DIR}/dispatch_test.c" AND NOT TARGET dispatch_test)
    add_executable(dispatch_test
        ${VEXSPOKE_TEST_DIR}/dispatch_test.c
    )
    target_compile_options(dispatch_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(dispatch_test PRIVATE vexspoke pthread)
endif()

# System Discovery & Hardware / Display probe target
if(EXISTS "${VEXSPOKE_TEST_DIR}/system_test.c" AND NOT TARGET system_test)
    add_executable(system_test
        ${VEXSPOKE_TEST_DIR}/system_test.c
    )
    target_link_libraries(system_test PRIVATE vexspoke)
endif()

# Reactive per-variable event emitter: fan-out, onSet/onChanged, mid-fire remove,
# onRemove teardown notify.
if(EXISTS "${VEXSPOKE_TEST_DIR}/reactive_test.c" AND NOT TARGET reactive_test)
    add_executable(reactive_test
        ${VEXSPOKE_TEST_DIR}/reactive_test.c
    )
    target_compile_options(reactive_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(reactive_test PRIVATE vexspoke)
endif()

# App + capture registry / live-probe targets (AppDetect, CaptureTool)
if(EXISTS "${VEXSPOKE_TEST_DIR}/app_detect_test.c" AND NOT TARGET app_detect_test)
    add_executable(app_detect_test
        ${VEXSPOKE_TEST_DIR}/app_detect_test.c
    )
    target_link_libraries(app_detect_test PRIVATE vexspoke)
endif()

if(EXISTS "${VEXSPOKE_TEST_DIR}/capture_tool_test.c" AND NOT TARGET capture_tool_test)
    add_executable(capture_tool_test
        ${VEXSPOKE_TEST_DIR}/capture_tool_test.c
    )
    target_link_libraries(capture_tool_test PRIVATE vexspoke)
endif()

# TouchID Demo Target
if(EXISTS "${VEXSPOKE_TEST_DIR}/touchid_demo.c" AND NOT TARGET touchid_demo)
    add_executable(touchid_demo
        ${VEXSPOKE_TEST_DIR}/touchid_demo.c
    )
    target_link_libraries(touchid_demo PRIVATE vexspoke)
endif()

# Headless net package test
if(EXISTS "${VEXSPOKE_TEST_DIR}/net_test.c" AND NOT TARGET net_test)
    add_executable(net_test
        ${VEXSPOKE_TEST_DIR}/net_test.c
    )
    target_link_libraries(net_test PRIVATE vexspoke)
endif()

# Headless threading package test
if(EXISTS "${VEXSPOKE_TEST_DIR}/thread_test.c" AND NOT TARGET thread_test)
    add_executable(thread_test
        ${VEXSPOKE_TEST_DIR}/thread_test.c
    )
    target_link_libraries(thread_test PRIVATE vexspoke)
endif()

# Headless time package test
if(EXISTS "${VEXSPOKE_TEST_DIR}/time_test.c" AND NOT TARGET time_test)
    add_executable(time_test
        ${VEXSPOKE_TEST_DIR}/time_test.c
    )
    target_link_libraries(time_test PRIVATE vexspoke)
endif()

# Headless input pipeline test
if(EXISTS "${VEXSPOKE_TEST_DIR}/input_test.c" AND NOT TARGET input_test)
    add_executable(input_test
        ${VEXSPOKE_TEST_DIR}/input_test.c
    )
    target_link_libraries(input_test PRIVATE vexspoke)
endif()

# Headless key map binding registry test
if(EXISTS "${VEXSPOKE_TEST_DIR}/key_map_test.c" AND NOT TARGET key_map_test)
    add_executable(key_map_test
        ${VEXSPOKE_TEST_DIR}/key_map_test.c
    )
    target_link_libraries(key_map_test PRIVATE vexspoke)
endif()

# VexHome cache factory test
if(EXISTS "${VEXSPOKE_TEST_DIR}/vexhome_test.c" AND NOT TARGET vexhome_test)
    add_executable(vexhome_test
        ${VEXSPOKE_TEST_DIR}/vexhome_test.c
    )
    target_link_libraries(vexhome_test PRIVATE vexspoke)
endif()

# Class relational test
if(EXISTS "${VEXSPOKE_TEST_DIR}/class_relational_test.c" AND NOT TARGET class_relational_test)
    add_executable(class_relational_test
        ${VEXSPOKE_TEST_DIR}/class_relational_test.c
    )
    target_link_libraries(class_relational_test PRIVATE vexspoke)
endif()

# Chunked list (never-moved rows) test
if(EXISTS "${VEXSPOKE_TEST_DIR}/chunked_list_test.c" AND NOT TARGET chunked_list_test)
    add_executable(chunked_list_test
        ${VEXSPOKE_TEST_DIR}/chunked_list_test.c
    )
    target_link_libraries(chunked_list_test PRIVATE vexspoke)
endif()

# Chunked list radix/paged form test
if(EXISTS "${VEXSPOKE_TEST_DIR}/chunked_paged_test.c" AND NOT TARGET chunked_paged_test)
    add_executable(chunked_paged_test
        ${VEXSPOKE_TEST_DIR}/chunked_paged_test.c
    )
    target_compile_options(chunked_paged_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(chunked_paged_test PRIVATE vexspoke)
endif()

# Dynamic Lifetime Verifier Debug-only test
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND EXISTS "${VEXSPOKE_TEST_DIR}/transient_lifetime_test.c" AND NOT TARGET transient_lifetime_test)
    add_executable(transient_lifetime_test
        ${VEXSPOKE_TEST_DIR}/transient_lifetime_test.c
    )
    target_link_libraries(transient_lifetime_test PRIVATE vexspoke)
endif()

# Header veto and variable tests
if(EXISTS "${VEXSPOKE_TEST_DIR}/header_veto_test.c" AND NOT TARGET header_veto_test)
    add_executable(header_veto_test
        ${VEXSPOKE_TEST_DIR}/header_veto_test.c
    )
    target_link_libraries(header_veto_test PRIVATE vexspoke)
endif()

if(EXISTS "${VEXSPOKE_TEST_DIR}/variable_pool_test.c" AND NOT TARGET variable_pool_test)
    add_executable(variable_pool_test
        ${VEXSPOKE_TEST_DIR}/variable_pool_test.c
    )
    target_link_libraries(variable_pool_test PRIVATE vexspoke)
endif()

# Name box (32-byte variable slot) reflection atom test
if(EXISTS "${VEXSPOKE_TEST_DIR}/variable_slot_test.c" AND NOT TARGET variable_slot_test)
    add_executable(variable_slot_test
        ${VEXSPOKE_TEST_DIR}/variable_slot_test.c
    )
    target_compile_options(variable_slot_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(variable_slot_test PRIVATE vexspoke)
endif()

# Relational name => pointer hash map (hash system B) test
if(EXISTS "${VEXSPOKE_TEST_DIR}/variable_hash_map_test.c" AND NOT TARGET variable_hash_map_test)
    add_executable(variable_hash_map_test
        ${VEXSPOKE_TEST_DIR}/variable_hash_map_test.c
    )
    target_compile_options(variable_hash_map_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(variable_hash_map_test PRIVATE vexspoke)
endif()

# Identity cell + global shelf test
if(EXISTS "${VEXSPOKE_TEST_DIR}/shelf_test.c" AND NOT TARGET shelf_test)
    add_executable(shelf_test
        ${VEXSPOKE_TEST_DIR}/shelf_test.c
    )
    target_compile_options(shelf_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(shelf_test PRIVATE vexspoke)
endif()

# Cold path resolver test
if(EXISTS "${VEXSPOKE_TEST_DIR}/segment_index_test.c" AND NOT TARGET segment_index_test)
    add_executable(segment_index_test
        ${VEXSPOKE_TEST_DIR}/segment_index_test.c
    )
    target_compile_options(segment_index_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(segment_index_test PRIVATE vexspoke)
endif()

# Scoped name => pointer mini map test
if(EXISTS "${VEXSPOKE_TEST_DIR}/variable_mini_map_test.c" AND NOT TARGET variable_mini_map_test)
    add_executable(variable_mini_map_test
        ${VEXSPOKE_TEST_DIR}/variable_mini_map_test.c
    )
    target_compile_options(variable_mini_map_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(variable_mini_map_test PRIVATE vexspoke)
endif()

# Reflection kinds test
if(EXISTS "${VEXSPOKE_TEST_DIR}/reflection_test.c" AND NOT TARGET reflection_test)
    add_executable(reflection_test
        ${VEXSPOKE_TEST_DIR}/reflection_test.c
    )
    target_compile_options(reflection_test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(reflection_test PRIVATE vexspoke)
endif()

if(EXISTS "${VEXSPOKE_TEST_DIR}/variable_strict_test.c" AND NOT TARGET variable_strict_test)
    add_executable(variable_strict_test
        ${VEXSPOKE_TEST_DIR}/variable_strict_test.c
    )
    target_link_libraries(variable_strict_test PRIVATE vexspoke)
endif()

# 12 CoordFrames & Math Precision Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/math_coord_test.c" AND NOT TARGET math_coord_test)
    add_executable(math_coord_test
        ${VEXSPOKE_TEST_DIR}/math_coord_test.c
    )
    target_link_libraries(math_coord_test PRIVATE vexspoke)
endif()

# String Calc & IDE Search Flags Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/search_calc_test.c" AND NOT TARGET search_calc_test)
    add_executable(search_calc_test
        ${VEXSPOKE_TEST_DIR}/search_calc_test.c
    )
    target_link_libraries(search_calc_test PRIVATE vexspoke)
endif()

# Spatial Data Structures (CircleArray, SphereArray, Octree) Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/spatial_struct_test.c" AND NOT TARGET spatial_struct_test)
    add_executable(spatial_struct_test
        ${VEXSPOKE_TEST_DIR}/spatial_struct_test.c
    )
    target_link_libraries(spatial_struct_test PRIVATE vexspoke)
endif()

# Security & Cryptography Engine Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/crypto_security_test.c" AND NOT TARGET crypto_security_test)
    add_executable(crypto_security_test
        ${VEXSPOKE_TEST_DIR}/crypto_security_test.c
    )
    target_link_libraries(crypto_security_test PRIVATE vexspoke)
endif()

# Hardware Input & Multiplexer Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/hardware_input_test.c" AND NOT TARGET hardware_input_test)
    add_executable(hardware_input_test
        ${VEXSPOKE_TEST_DIR}/hardware_input_test.c
    )
    target_link_libraries(hardware_input_test PRIVATE vexspoke)
endif()

# Algorithm Suite (Radix, Dijkstra, KD-Tree, BVH, TreeSit) Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/algo_suite_test.c" AND NOT TARGET algo_suite_test)
    add_executable(algo_suite_test
        ${VEXSPOKE_TEST_DIR}/algo_suite_test.c
    )
    target_link_libraries(algo_suite_test PRIVATE vexspoke)
endif()

# Clipboard, Cache & Destructor Dispatch Test
if(EXISTS "${VEXSPOKE_TEST_DIR}/clipboard_test.c" AND NOT TARGET clipboard_test)
    add_executable(clipboard_test
        ${VEXSPOKE_TEST_DIR}/clipboard_test.c
    )
    target_link_libraries(clipboard_test PRIVATE vexspoke)
endif()



