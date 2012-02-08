# Ищет библиотеку FTGL
#
# Модуль определяет следующие переменные:
# FTGL_FOUND - TRUE, если библиотека найдена
# FTGL_INCLUDE_DIRS - пути к заголовочным файлам
# FTGL_LIBRARIES - имена подключаемых библиотек

# проверяем опции QUIET и REQUIRED
if(FTGL_FIND_QUIETLY)
	set(FTGL_QUIET_ARG QUIET)
endif()
if(FTGL_FIND_REQUIRED)
	set(FTGL_REQUIRED_ARG REQUIRED)
endif()

# ищем зависимости - OpenGL и FreeType
find_package(OpenGL ${FTGL_QUIET_ARG} ${FTGL_REQUIRED_ARG})
find_package(Freetype ${FTGL_QUIET_ARG} ${FTGL_REQUIRED_ARG})

# если нашли зависимости, то ищем саму библиотеку FTGL
if(OPENGL_FOUND AND OPENGL_GLU_FOUND AND FREETYPE_FOUND)
	# ищем заголовочный файл
	find_path(FTGL_INCLUDE_DIR "FTGL/ftgl.h")

	# ищем библиотеку
	find_library(FTGL_LIBRARY "ftgl")

	# нашли FTGL - заполняем выходные переменные FTGL_INCLUDE_DIRS и FTGL_LIBRARIES
	if(FTGL_INCLUDE_DIR AND FTGL_LIBRARY)
		set(FTGL_INCLUDE_DIRS ${FTGL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
		set(FTGL_LIBRARIES ${FTGL_LIBRARY} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES})
		list(REMOVE_DUPLICATES FTGL_INCLUDE_DIRS)
	endif()
endif()

# обрабатываем аргументы QUIET и REQUIRED и устанавливаем переменную FTGL_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FTGL DEFAULT_MSG FTGL_INCLUDE_DIR FTGL_LIBRARY)

# помечаем переменные FTGL_INCLUDE_DIR и FTGL_LIBRARY как продвинутые
mark_as_advanced(FTGL_INCLUDE_DIR FTGL_LIBRARY)
