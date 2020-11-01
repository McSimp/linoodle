# linoodle

A Linux wrapper for the [Oodle Data Compression](http://www.radgametools.com/oodlecompressors.htm) library.

It essentially works like this:

1. Parse the Windows Oodle DLL in the current directory (`oo2core_8_win64.dll`) using the `pe-parse` library
2. Map the PE into memory
3. Perform relocations
4. Resolve imports to point to our own implementation of the minimum set of required Windows API functions
5. Setup the `gs` register to point to a fake `TIB` structure
6. Execute the DLL's entry point
7. Execute whatever Oodle exports you want (currently only `OodleLZ_Decompress`)

## Usage

When you clone this repository, make sure you use `--recurse-submodules` to get the `pe-parse` submodule.
Otherwise you can run `git submodule update --init` after you've cloned it.

After that, just use cmake to build the project and you'll get a `liblinoodle.so` file created in the `build`
directory. You can link to this or use `dlopen`, then call the `OodleLZ_Decompress` export.

```
mkdir build
cd build
cmake ..
cd ..
cmake --build build/
```

This will also build an executable which tests that the library works. Run it with `./build/testlinoodle`.
