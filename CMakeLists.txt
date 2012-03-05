# -DCMAKE_BUILD_TYPE=Debug (Debug/Release/RelWithDebInfo/MinSizeRel)

# задаем минимальную версию CMake
cmake_minimum_required(VERSION 2.6)

# создаем проект
set(PROJECT "gui-creator")
project(${PROJECT})

# задаем имя прекомпилированного заголовка
set(PRECOMPILED_SOURCE src/pch.cpp)
set(PRECOMPILED_HEADER include/pch.h)

# задаем списки файлов проекта
set(SOURCES
	src/base_layer.cpp
	src/editor_window.cpp
	src/font_browser.cpp
	src/font_manager.cpp
	src/game_object.cpp
	src/label.cpp
	src/layer.cpp
	src/layer_group.cpp
	src/layers_tree_widget.cpp
	src/layers_window.cpp
	src/location.cpp
	src/lua_script.cpp
	src/main.cpp
	src/main_window.cpp
	src/options.cpp
	src/options_dialog.cpp
	src/property_window.cpp
	src/sprite.cpp
	src/sprite_browser.cpp
	src/texture.cpp
	src/texture_manager.cpp
	src/utils.cpp)

set(HEADERS
	${PRECOMPILED_HEADER}
	include/base_layer.h
	include/editor_window.h
	include/font_browser.h
	include/font_manager.h
	include/game_object.h
	include/label.h
	include/layer.h
	include/layer_group.h
	include/layers_tree_widget.h
	include/layers_window.h
	include/location.h
	include/lua_script.h
	include/main_window.h
	include/options.h
	include/options_dialog.h
	include/property_window.h
	include/singleton.h
	include/sprite.h
	include/sprite_browser.h
	include/texture.h
	include/texture_loader.h
	include/texture_manager.h
	include/thumbnail_loader.h
	include/utils.h)

set(FORMS
	ui/font_browser.ui
	ui/layers_window.ui
	ui/main_window.ui
	ui/options_dialog.ui
	ui/property_window.ui
	ui/sprite_browser.ui)

set(RESOURCES
	res/resources.qrc)

# задаем путь для поиска дополнительных модулей CMake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# включаем каталог с заголовочными файлами и каталог сборки CMake, куда помещаются сгенерированные moc файлы
include_directories("include" ${CMAKE_BINARY_DIR})

# задаем пути для поиска библиотек
set(DEPS_DIRS "${CMAKE_BINARY_DIR}/deps" "${CMAKE_BINARY_DIR}/../deps" "${CMAKE_SOURCE_DIR}/deps" "${CMAKE_SOURCE_DIR}/../deps")
foreach(DEPS_DIR ${DEPS_DIRS})
	if(EXISTS ${DEPS_DIR})
		get_filename_component(DEPS_PATH ${DEPS_DIR} ABSOLUTE)
		list(APPEND CMAKE_FIND_ROOT_PATH ${DEPS_PATH})
	endif()
endforeach()

# ищем Qt
find_package(Qt4 4.8 REQUIRED QtCore QtGui QtOpenGL)
include(${QT_USE_FILE})
qt4_wrap_cpp(MOC_SOURCES ${HEADERS} OPTIONS -nw)
qt4_wrap_ui(UIC_SOURCES ${FORMS})
qt4_add_resources(QRC_SOURCES ${RESOURCES})

# ищем FTGL
find_package(FTGL REQUIRED)
include_directories(${FTGL_INCLUDE_DIRS})

# ищем Lua 5.1
find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# включаем вывод предупреждений компилятора
if(MSVC)
	add_definitions("/W3")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	add_definitions("-Wall -Winvalid-pch")
endif()

# определяем макрос для статической линковки FTGL
if(MSVC)
	add_definitions("/D FTGL_LIBRARY_STATIC")
endif()

# подключаем прекомпилированный заголовок
if(MSVC)
	# получаем имя pch-файла
	set(PCH_NAME "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT}.pch")
	get_filename_component(PCH_HEADER ${PRECOMPILED_HEADER} NAME)

	# устанавливаем свойства файлов для создания/использования pch-файла
	add_definitions("/Yu${PCH_HEADER} /Fp${PCH_NAME}")
	set_source_files_properties(${PRECOMPILED_SOURCE} PROPERTIES COMPILE_FLAGS "/Yc${PCH_HEADER} /Fp${PCH_NAME}" OBJECT_OUTPUTS ${PCH_NAME})
	set_source_files_properties(${SOURCES} ${MOC_SOURCES} ${QRC_SOURCES} PROPERTIES COMPILE_FLAGS "/Yu${PCH_HEADER} /Fp${PCH_NAME}" OBJECT_DEPENDS ${PCH_NAME})

	# добавляем cpp-файл для прекомпилированного заголовка в проект
	list(APPEND SOURCES ${PRECOMPILED_SOURCE})

	# добавляем опции командной строки для подключения pch-файла
	add_definitions("/FI${PCH_HEADER}")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	# получаем имя gch-файла и добавляем его в проект
	set(GCH_NAME "${CMAKE_BINARY_DIR}/${PROJECT}.gch")
	list(APPEND HEADERS ${GCH_NAME})

	# формируем аргументы командной строки
	set(GCH_ARGS ${CMAKE_CXX_FLAGS})

	string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" VAR_NAME)
	list(APPEND GCH_ARGS ${${VAR_NAME}})

	get_directory_property(PROPERTY COMPILE_DEFINITIONS)
	foreach(ITEM ${PROPERTY})
		list(APPEND GCH_ARGS "-D${ITEM}")
	endforeach()

	string(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" VAR_NAME)
	get_directory_property(PROPERTY ${VAR_NAME})
	foreach(ITEM ${PROPERTY})
		list(APPEND GCH_ARGS "-D${ITEM}")
	endforeach()

	get_directory_property(PROPERTY DEFINITIONS)
	foreach(ITEM ${PROPERTY})
		list(APPEND GCH_ARGS ${ITEM})
	endforeach()

	get_directory_property(PROPERTY INCLUDE_DIRECTORIES)
	foreach(ITEM ${PROPERTY})
		list(APPEND GCH_ARGS "-I${ITEM}")
	endforeach()

	list(APPEND GCH_ARGS "-o ${GCH_NAME} -c ${CMAKE_SOURCE_DIR}/${PRECOMPILED_HEADER}")

	separate_arguments(GCH_ARGS)
	list(REMOVE_DUPLICATES GCH_ARGS)

	# добавляем правило для сборки прекомпилированного заголовка
	add_custom_command(OUTPUT ${GCH_NAME} COMMAND ${CMAKE_CXX_COMPILER} ${GCH_ARGS} DEPENDS "${CMAKE_SOURCE_DIR}/${PRECOMPILED_HEADER}")

	# добавляем опции командной строки для подключения gch-файла
	add_definitions("-include ${PROJECT}")
endif()

# указываем группы для файлов проекта
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})
source_group("Forms" FILES ${FORMS})
source_group("Resources" FILES ${RESOURCES})
source_group("Generated Files" FILES ${MOC_SOURCES} ${UIC_SOURCES} ${QRC_SOURCES})

# добавляем исполняемый файл в проект
add_executable(${PROJECT} WIN32 ${SOURCES} ${HEADERS} ${MOC_SOURCES} ${UIC_SOURCES} ${QRC_SOURCES})
target_link_libraries(${PROJECT} ${QT_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${FTGL_LIBRARIES} ${LUA_LIBRARIES})
