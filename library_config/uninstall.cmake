cmake_minimum_required(VERSION 2.8.5)

set(MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

if(NOT EXISTS ${MANIFEST})
    message(FATAL_ERROR "Cannot find install mainfest: ${MANIFEST}")
endif()

file(STRINGS ${MANIFEST} files)
foreach(file ${files})
    if(EXISTS ${file} OR IS_SYMLINK ${file})
        message(STATUS "Removing: ${file}")

	execute_process(COMMAND rm -f ${file}
            RESULT_VARIABLE result
            OUTPUT_QUIET
            ERROR_VARIABLE stderr
            ERROR_STRIP_TRAILING_WHITESPACE
        )

        if(NOT ${result} EQUAL 0)
            message(FATAL_ERROR "${stderr}")
        endif()
    else()
        message(STATUS "Does-not-exist: ${file}")
    endif()
endforeach(file)
