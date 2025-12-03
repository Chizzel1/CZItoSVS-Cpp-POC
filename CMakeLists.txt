cmake_minimum_required(VERSION 3.15) 
project(CZIConvert LANGUAGES CXX)

# Set C++ standard (use 17 or later)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(CZIConvert src/main.cpp src/stb_impl.cpp)


#" -DCMAKE_TOOLCHAIN_FILE=C:/Projects/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
find_package(fmt CONFIG REQUIRED)      # fmt::fmt
find_package(JPEG REQUIRED)            # JPEG::JPEG (libjpeg-turbo exposes this)
find_package(TIFF REQUIRED)            # TIFF::TIFF
find_package(TBB CONFIG REQUIRED)      # TBB::tbb
find_package(spdlog CONFIG REQUIRED)   # spdlog::spdlog or spdlog::spdlog_header_only
find_package(Catch2 CONFIG REQUIRED)   # for tests: Catch2::Catch2



target_include_directories(CZIConvert PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libczi/Src/libCZI
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libczi/Src/libCZIAPI/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libczi/build/vendor/eigen3/src/eigen_ext
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb-master
)


set(LIBCZI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libczi/build/Src/libCZI")

# Import library (.lib) — different for Debug/Release
set(LIBCZI_LIB
    $<$<CONFIG:Debug>:"${LIBCZI_ROOT}/Debug/libCZId">
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:"${LIBCZI_ROOT}/Release/libCZI">
)


target_link_libraries(CZIConvert PRIVATE
    ${LIBCZI_LIB}
    fmt::fmt
    JPEG::JPEG
    TIFF::TIFF
    TBB::tbb
    spdlog::spdlog




)

add_custom_command(TARGET CZIConvert POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:"${LIBCZI_ROOT}/Debug/libCZId.dll">
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:"${LIBCZI_ROOT}/Release/libCZI.dll">
        $<TARGET_FILE_DIR:CZIConvert>

#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#       $<$<CONFIG:Debug>:"${LIBCZI_ROOT}/Debug/zstd.dll">
#        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:"${LIBCZI_ROOT}/Release/zstd.dll">
#        $<TARGET_FILE_DIR:CZIConvert>
)