# Shell script formatting and linting

file(GLOB SHELL_SCRIPTS "*.sh")

# shfmt for shell script formatting
find_program(SHFMT shfmt PATHS /usr/bin /usr/local/bin /opt/homebrew/bin)
if(SHFMT AND SHELL_SCRIPTS)
    add_custom_target(fmt-scripts
        COMMAND ${SHFMT} -w -i 4 -ci ${SHELL_SCRIPTS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting shell scripts with shfmt"
    )
else()
    if(NOT SHFMT)
        message(WARNING "shfmt not found, shell script formatting will not be available")
    endif()
    add_custom_target(fmt-scripts
        COMMAND echo "Shell script formatting not available - shfmt not found or no scripts"
        COMMENT "Shell script formatting disabled"
    )
endif()

# shellcheck for shell script linting
find_program(SHELLCHECK shellcheck PATHS /usr/bin /usr/local/bin /opt/homebrew/bin)
if(SHELLCHECK AND SHELL_SCRIPTS)
    add_custom_target(lint-scripts
        COMMAND ${SHELLCHECK} ${SHELL_SCRIPTS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Linting shell scripts with shellcheck"
    )
else()
    if(NOT SHELLCHECK)
        message(WARNING "shellcheck not found, shell script linting will not be available")
    endif()
    add_custom_target(lint-scripts
        COMMAND echo "Shell script linting not available - shellcheck not found or no scripts"
        COMMENT "Shell script linting disabled"
    )
endif()