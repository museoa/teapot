cmake_minimum_required(VERSION 2.6)
project(Teapot)

set(Teapot_VERSION_MAJOR 2)
set(Teapot_VERSION_MINOR 1)
set(Teapot_VERSION_PATCH 0)

option(ENABLE_HELP "Enable built-in documentation" ON)

add_library(teapotlib complete.c context.c csv.c eval.c func.c html.c latex.c main.c misc.c parser.c sc.c scanner.c sheet.c wk1.c xdr.c)

find_library(LIB_PORTABLEXDR portablexdr)
if (NOT LIB_PORTABLEXDR)
	set(LIB_PORTABLEXDR "")
endif ()

find_package(Curses)
if (CURSES_FOUND)
	include_directories("${CURSES_INCLUDE_DIR}")
	add_executable(teapot display.c)
	target_link_libraries(teapot teapotlib m ${CURSES_CURSES_LIBRARY} ${LIB_PORTABLEXDR})
	install(TARGETS teapot DESTINATION bin)
	set(teapot_DEB_DEPENDS ", libncurses5 (>= 5.6+20071006-3)") 
endif ()

find_package(FLTK NO_MODULE)
if (FLTK_FOUND)
	include(${FLTK_USE_FILE})

	fltk_wrap_ui(fteapot fteapot.fl)
	add_executable(fteapot WIN32 ${fteapot_FLTK_UI_SRCS})
	set(fteapot_DEB_DEPENDS ", libstdc++6 (>= 4.1.1), libx11-6, libxinerama1") 
	if (FLTK_XFT_FOUND)
		set(fteapot_DEB_DEPENDS "${fteapot_DEB_DEPENDS}, libfontconfig1 (>= 2.4.0), libxft2 (>> 2.1.1)")
	endif ()
	if (ENABLE_HELP)
		set(fteapot_DEB_DEPENDS "${fteapot_DEB_DEPENDS}, libjpeg62, libpng12-0 (>= 1.2.13-4)") 
		target_link_libraries(fteapot teapotlib fltk fltk_images ${LIB_PORTABLEXDR})
	else ()
		target_link_libraries(fteapot teapotlib fltk ${LIB_PORTABLEXDR})
	endif ()
	install(TARGETS fteapot DESTINATION bin)
endif ()

if (ENABLE_HELP)
	add_custom_command(
		OUTPUT teapot.tex teapot.lyx
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/doc/teapot.lyx
		VERBATIM
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/doc/teapot.lyx teapot.lyx
		COMMAND lyx -e pdflatex -f main teapot.lyx
	)
	add_custom_command(
		OUTPUT teapot.pdf teapot.aux teapot.log teapot.toc
		MAIN_DEPENDENCY teapot.tex
		VERBATIM
		COMMAND pdflatex teapot.tex
		COMMAND pdflatex teapot.tex
	)
	add_custom_command(OUTPUT teapot.pdf teapot.out MAIN_DEPENDENCY teapot.tex teapot.aux VERBATIM COMMAND pdflatex teapot.tex)
	add_custom_command(
		OUTPUT html/ html/index.html .latex2html-init
		MAIN_DEPENDENCY teapot.tex
		DEPENDS teapot.tex teapot.pdf ${CMAKE_CURRENT_SOURCE_DIR}/doc/.latex2html-init
		VERBATIM
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/doc/.latex2html-init .latex2html-init
		COMMAND ${CMAKE_COMMAND} -E make_directory html
		COMMAND latex2html teapot.tex
	)
	add_custom_target(pdf DEPENDS teapot.pdf)
	add_custom_target(html DEPENDS html/index.html)
	add_custom_target(doc ALL DEPENDS teapot.pdf html/index.html)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc/teapot FILES_MATCHING PATTERN *.html PATTERN *.png)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/teapot.pdf DESTINATION share/doc/teapot)
	set(HELPFILE "${CMAKE_INSTALL_PREFIX}/share/doc/teapot/html/index.html")
endif ()

install(FILES COPYING README DESTINATION share/doc/teapot)

add_custom_target(dist
	COMMAND ${CMAKE_COMMAND} -E remove_directory teapot-${Teapot_VERSION_MAJOR}.${Teapot_VERSION_MINOR}.${Teapot_VERSION_PATCH}
	COMMAND ${CMAKE_COMMAND} -E make_directory teapot-${Teapot_VERSION_MAJOR}.${Teapot_VERSION_MINOR}.${Teapot_VERSION_PATCH}
	COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/doc ${CMAKE_CURRENT_SOURCE_DIR}/examples ${CMAKE_CURRENT_SOURCE_DIR}/[A-Z]* ${CMAKE_CURRENT_SOURCE_DIR}/*.*[a-z] teapot-${Teapot_VERSION_MAJOR}.${Teapot_VERSION_MINOR}.${Teapot_VERSION_PATCH}
	COMMAND tar czf teapot-${Teapot_VERSION_MAJOR}.${Teapot_VERSION_MINOR}.${Teapot_VERSION_PATCH}.tar.gz teapot-${Teapot_VERSION_MAJOR}.${Teapot_VERSION_MINOR}.${Teapot_VERSION_PATCH}
)

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Table Editor And Planner, Or: Teapot!")
set(CPACK_PACKAGE_DESCRIPTION "A three-dimensional spreadsheet specialized in advanced calculations. Comes in GUI (FLTK) and console (curses) flavours.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VENDOR "Jörg Walter, http://www.syntax-k.de/projekte/teapot")

set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "${Teapot_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Teapot_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${Teapot_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "Jörg Walter <info@syntax-k.de>")
set(CPACK_PACKAGE_EXECUTABLES "fteapot" "Teapot")
set(CPACK_STRIP_FILES TRUE)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.11), libgcc1 (>= 1:4.1.1)${teapot_DEB_DEPENDS}${fteapot_DEB_DEPENDS}")
include(InstallRequiredSystemLibraries)
include(CPack)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()
