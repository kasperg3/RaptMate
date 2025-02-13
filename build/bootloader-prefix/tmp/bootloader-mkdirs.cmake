# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/kang/esp/esp-idf/components/bootloader/subproject"
  "/home/kang/workspace/RaptPillMate/build/bootloader"
  "/home/kang/workspace/RaptPillMate/build/bootloader-prefix"
  "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/tmp"
  "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/src/bootloader-stamp"
  "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/src"
  "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/kang/workspace/RaptPillMate/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
