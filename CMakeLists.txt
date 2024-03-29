cmake_minimum_required(VERSION 3.17)   

project("BatchCompress")
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")

if(MSVC)
  add_definitions(/MP)
  add_definitions(/Qpar)
endif()

file(GLOB source "source/*.cpp" "source/*.hpp" "source/*.h")
if(APPLE)
    set(macfiles "source/wxmac.icns;source/objc_bridge.h;source/objc_bridge.mm" CACHE INERNAL "")
elseif(WIN32)
	set(winfiles "source/windows.rc" CACHE INTERNAL "")
endif()
add_executable("${PROJECT_NAME}" WIN32 ${source} ${macfiles} ${winfiles})


#wxwidgets
set(wxBUILD_SHARED OFF CACHE INTERNAL "")
add_subdirectory("wxWidgets" EXCLUDE_FROM_ALL)
add_subdirectory("zopfli" EXCLUDE_FROM_ALL)
add_subdirectory("fmt" EXCLUDE_FROM_ALL)

target_link_libraries("${PROJECT_NAME}"
    PUBLIC 
    wx::base
    wx::core
	libzopflipng
	fmt
)


# mac app
set_target_properties("${PROJECT_NAME}" PROPERTIES 
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/source/Info.plist"
)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/source/wxmac.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
if(APPLE)
INSTALL(CODE 
		"include(BundleUtilities)
		fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION>/${PROJECT_NAME}.app\" \"\" \"\")
		" 
		COMPONENT Runtime
	)
elseif(LINUX)
INSTALL(CODE 
	"include(${CMAKE_CURRENT_SOURCE_DIR}/appimage.cmake)
	make_appimage(
		EXE \"${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION>/${PROJECT_NAME}\"
		NAME \"${PROJECT_NAME}\"
		ICON \"${CMAKE_CURRENT_SOURCE_DIR}/source/wxlin.xpm\"
		OUTPUT_NAME \"${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION>/${PROJECT_NAME}.AppImage\"
		ASSETS \"\"
	)
	" 
	COMPONENT Runtime
	)
endif()
# windows app
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
