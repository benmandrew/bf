find_package(Doxygen QUIET)
find_package(Python3 COMPONENTS Interpreter QUIET)

set(DOCS_VENV_DIR ${CMAKE_BINARY_DIR}/docs/.venv)
if(WIN32)
    set(DOCS_VENV_PYTHON ${DOCS_VENV_DIR}/Scripts/python.exe)
else()
    set(DOCS_VENV_PYTHON ${DOCS_VENV_DIR}/bin/python3)
endif()

if(Python3_Interpreter_FOUND)
    add_custom_target(docs-setup
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/docs
        COMMAND ${Python3_EXECUTABLE} -m venv ${DOCS_VENV_DIR}
        COMMAND ${DOCS_VENV_PYTHON} -m pip install --disable-pip-version-check --quiet --upgrade pip
        COMMAND ${DOCS_VENV_PYTHON} -m pip install --disable-pip-version-check --quiet -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
        COMMENT "Creating docs virtualenv and installing Sphinx dependencies"
    )
else()
    add_custom_target(docs-setup
        COMMAND ${CMAKE_COMMAND} -E echo "docs-setup requires Python 3 with venv support."
        COMMENT "Documentation setup unavailable"
    )
endif()

if(DOXYGEN_FOUND AND Python3_Interpreter_FOUND)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs/doxygen)
    set(DOXYGEN_INPUT_DIR ${CMAKE_SOURCE_DIR}/src)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUTPUT_DIR}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E env SPHINX_DOXYGEN_XML_DIR=${DOXYGEN_OUTPUT_DIR}/xml ${DOCS_VENV_PYTHON} -m sphinx -b html ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/docs/html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building Doxygen XML and Sphinx HTML documentation"
    )
    add_dependencies(docs docs-setup)
else()
    message(WARNING "Docs target disabled: install Doxygen, Python 3, and the Sphinx/Breathe packages to build documentation.")
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo "Docs require Doxygen, Python 3, and the Sphinx/Breathe Python packages."
        COMMENT "Documentation tooling not available"
    )
    if(Python3_Interpreter_FOUND)
        add_dependencies(docs docs-setup)
    endif()
endif()