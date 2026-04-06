Usage
=====

Live Instance
-------------

You can try the project online at:

https://benmandrew.com/articles/compiler-frontend

Run Locally with Docker Compose
--------------------------------

To run the web interface locally:

.. code-block:: bash

	$ docker compose up

Then open:

http://localhost:8080

Build the CLI Tools
-------------------

Build the project with CMake:

.. code-block:: bash

	$ cmake -B build
	$ cmake --build build

After building, both executables are available in the ``build`` directory:

- ``bfc``: Brainfuck frontend that emits LLVM IR
- ``bfi``: Brainfuck interpreter

Use ``bfc`` (compile to LLVM IR)
--------------------------------

Generate LLVM IR from a Brainfuck program and compile it with ``clang``:

.. code-block:: bash

	# Generate LLVM IR
        $ ./build/bfc test/res/helloworld.b > main.ll
        # Compile IR to binary
        $ clang main.ll -o main
        $ ./main
        Hello, World!

Use ``bfi`` (interpret directly)
--------------------------------

Run a Brainfuck program directly with the interpreter:

.. code-block:: bash

	$ ./build/bfi test/res/helloworld.b
        Hello, World!
