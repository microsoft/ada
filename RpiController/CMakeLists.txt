#
# cmake file for RpiController project
#
cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

# define project
project(RpiController)
set(tool_name RpiController)

# Set C++ version
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apply -fPIC where applicable to the platform
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

IF(UNIX)
    
ELSE()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0600 /GS /W4 /wd4100 /wd4505 /wd4820 /wd4464 /wd4514 /wd4710 /wd4571 /Zc:wchar_t /ZI /Zc:inline /fp:precise /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /D_UNICODE /DUNICODE /WX- /Zc:forScope /Gd /EHsc /D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NXCOMPAT /DYNAMICBASE /INCREMENTAL:NO ")
ENDIF()

# Find the platform-specific way of working with threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)

set(src
    Ports/Port.cpp
    Ports/SerialPort.cpp
    Ports/SocketInit.cpp
    Ports/TcpClientPort.cpp
    Ports/UdpClientPort.cpp
    Utils/crc32.cpp
    main.cpp
)

set(include
    Controller/Controller.h
    Controller/TeensyPixelBuffer.h
    Controller/Commands.h
    Controller/Sensei.h
    Ports/Port.h
    Ports/SerialPort.h
    Ports/SocketInit.h
    Ports/TcpClientPort.h
    Ports/UdpClientPort.h
    Utils/Color.h
    Utils/Utils.h
    Utils/Timer.h
    Utils/Json.h
    Utils/Settings.h
    Utils/FileSystem.h    
    Utils/StreamWriter.h
    Utils/crc32.h
)

IF(UNIX)
    LIST(APPEND src "Ports/linux/LinuxFindSerialPorts.cpp")
    set(EXTRA_LIBS Threads::Threads)
ELSE()
    LIST(APPEND src "Ports/windows/WindowsFindSerialPorts.cpp")
    set(EXTRA_LIBS Threads::Threads Setupapi.lib Cfgmgr32.lib)
ENDIF()

source_group("src" FILES ${src})
source_group("include" FILES ${include})

# create executable in build\bin
set(GLOBAL_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${GLOBAL_BIN_DIR})
add_executable(${tool_name} ${src} ${include})
target_link_libraries(${tool_name} ${EXTRA_LIBS})
target_include_directories(${tool_name} PRIVATE Controller Utils Ports)
