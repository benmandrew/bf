# Tests

To run all tests:

```bash
cmake --build build --target test
```

## Unit tests

Requires `check`.

```bash
cmake --build build --target unittest
```

## Expect tests

Requires `expect`.

```bash
cmake --build build --target expecttest
```

## LLVM FileCheck tests

Requires `FileCheck` from the LLVM tools.

```bash
cmake --build build --target filecheck
```
