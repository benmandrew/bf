# Formatting and linting for the shell and Python sources under scripts/
# and verification/. The C sources are covered by formatting.cmake and
# linting.cmake, which attach these targets to fmt, fmt-ci and lint via
# if(TARGET ...) guards. Those guards are evaluated at configure time, so
# this file must be included before both of them.
#
# These targets used to live in verification/CMakeLists.txt, covering
# only that directory. That file was reached through add_subdirectory,
# which testing.cmake gates on LLVM being available, so formatting a
# shell script depended on having a compiler toolchain. Owning them here
# drops that coupling and extends them over scripts/ as well.
#
# The tools are invoked by name rather than located with find_program:
# shfmt, shellcheck and ruff are all pinned by the Nix devShell, which is
# the source of truth for them. A build outside 'nix develop' that lacks
# one will fail on the target that needs it rather than skip it quietly.

file(GLOB SHELL_FILES
    "${CMAKE_SOURCE_DIR}/scripts/*.sh"
    "${CMAKE_SOURCE_DIR}/verification/*.sh"
)
file(GLOB PYTHON_FILES "${CMAKE_SOURCE_DIR}/scripts/*.py")

# The repository has no .editorconfig and shfmt defaults to tabs, so the
# house style has to be spelled out: four spaces, and -ci to indent
# switch cases.
set(SHFMT_STYLE -i 4 -ci)

add_custom_target(fmt-scripts
    COMMAND shfmt ${SHFMT_STYLE} -w ${SHELL_FILES}
    COMMAND ruff format --quiet ${PYTHON_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting shell and Python scripts"
)

# Both checks exit non-zero on a diff, which is what fmt-ci needs.
add_custom_target(fmt-scripts-ci
    COMMAND shfmt ${SHFMT_STYLE} -d ${SHELL_FILES}
    COMMAND ruff format --quiet --diff ${PYTHON_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking shell and Python script formatting"
)

add_custom_target(lint-scripts
    COMMAND ${CMAKE_COMMAND} -E echo "--- Running shellcheck ---"
    COMMAND shellcheck ${SHELL_FILES}
    COMMAND ${CMAKE_COMMAND} -E echo "--- Running ruff ---"
    COMMAND ruff check ${PYTHON_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Linting shell and Python scripts"
)
