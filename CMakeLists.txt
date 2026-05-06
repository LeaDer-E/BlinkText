cmake_minimum_required(VERSION 3.20)
project(BeefyTextCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(BeefyTextCpp WIN32
    src/main.cpp
    src/AppWindow.cpp
    src/SimpleJson.cpp
    src/BlinkText.rc
)

target_include_directories(BeefyTextCpp PRIVATE src)
target_compile_definitions(BeefyTextCpp PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(BeefyTextCpp PRIVATE user32 gdi32 comdlg32 shell32 comctl32 uxtheme dwmapi gdiplus imm32 ole32)

install(TARGETS BeefyTextCpp RUNTIME DESTINATION .)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/app.ico")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/app.ico" DESTINATION .)
endif()
