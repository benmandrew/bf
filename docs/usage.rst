Usage
=====

Live Instance
-------------

You can try the project online at:

https://benmandrew.com/articles/compiler-frontend

Run the Web Interface Locally
-----------------------------

The web demo compiles in the browser: ``bfc`` is built to WebAssembly, so it
is a static site with no backend. Build the bundle and serve it (building the
wasm module needs the Emscripten SDK):

.. code-block:: bash

	$ scripts/build-wasm.sh
	$ nix develop -c cmake -B build
	$ nix develop -c cmake --build build --target site
	$ cd build/site && nix develop -c python3 -m http.server 8080

Then open:

http://localhost:8080

See ``web/README.md`` for the pipeline and the MIME types a static host must
set.

Build the CLI Tools
-------------------

Build the project with CMake:

.. code-block:: bash

	$ cmake -B build
	$ cmake --build build

After building, both executables are available in the ``build`` directory:

- ``bfc``: bf frontend that emits LLVM IR
- ``bfi``: bf interpreter

Use ``bfc`` (compile to LLVM IR)
--------------------------------

Generate LLVM IR from a bf program and compile it with ``clang``:

.. code-block:: bash

	# Generate LLVM IR
        $ ./build/bfc test/res/helloworld.b > main.ll
        # Compile IR to binary
        $ clang main.ll -o main
        $ ./main
        Hello, World!

Use ``bfi`` (interpret directly)
--------------------------------

Run a bf program directly with the interpreter:

.. code-block:: bash

	$ ./build/bfi test/res/helloworld.b
        Hello, World!
