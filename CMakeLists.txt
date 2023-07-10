# This file is part of Persona 4 Golden HD Patch
# Copyright 2020 浅倉麗子

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
  endif()
endif()

include("$ENV{DOLCESDK}/share/dolce.cmake" REQUIRED)

project(p4goldenhd LANGUAGES C)

# build font
set(FONT_BASE "terminus-font-4.48")
set(FONT_FILE "ter-u28b.bdf")
set(SFN_FILE "font.sfn")
set(SFN_C "${SFN_FILE}.c")
set(SFN_OBJ "font_sfn")

file(DOWNLOAD
  "https://sourceforge.net/projects/terminus-font/files/terminus-font-4.48/terminus-font-4.48.tar.gz/download"
  "${FONT_BASE}.tar.gz"
  EXPECTED_HASH SHA256=34799c8dd5cec7db8016b4a615820dfb43b395575afbb24fc17ee19c869c94af
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SFN_FILE}"
  COMMAND tar xzf "${FONT_BASE}.tar.gz" --strip-components=1 "${FONT_BASE}/${FONT_FILE}"
  COMMAND bit2sfn
    -r "Basic Latin" "${FONT_FILE}"
    "${CMAKE_CURRENT_BINARY_DIR}/${SFN_FILE}"
)

add_custom_command(OUTPUT "${SFN_C}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${SFN_FILE}"
  COMMAND xxd -i "${SFN_FILE}" "${SFN_C}"
  COMMENT "Converting ${SFN_FILE} to ${SFN_C}"
)

add_library("${SFN_OBJ}" STATIC
  "${SFN_C}"
)

# build module
set(ELF "${PROJECT_NAME}.elf")
set(SELF "${PROJECT_NAME}.suprx")

add_executable("${ELF}"
  main.c
)

target_compile_options("${ELF}"
  PRIVATE -O3 -std=c99
  PRIVATE -Wall -Wextra -Wdouble-promotion -Wshadow -Wundef
  PRIVATE -fsingle-precision-constant -fno-common
)

if(LOG_PRINTF)
  target_compile_definitions("${ELF}" PRIVATE LOG_PRINTF)
endif()

if(FB_FHD)
  target_compile_definitions("${ELF}" PRIVATE FB_FHD)
  message("Framebuffer set to 1920x1080")
else()
  message("Framebuffer set to 1280x720")
endif()

target_link_options("${ELF}"
  PRIVATE -nostdlib
)

target_link_libraries("${ELF}"
  "${SFN_OBJ}"
  fnblit
  gcc
  SceKernelModulemgr_stub
  SceLibKernel_stub
  taihen_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG exports.yml
)
