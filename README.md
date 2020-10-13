# n64sym

`n64sym` is a command-line utility that identifies symbols in N64 games.

## Usage

    n64sym <input path> [options] 

#### `<input path>`

Sets the path of the file to scan. The input file may either be a RAM dump or ROM image.

If the file extension is `.z64`, `.n64`, or `.v64`, the tool will assume the file is a ROM image and attempt to identify the position of the main code segment and adjust the symbol addresses accordingly. Note that scanning a ROM image may yield inaccurate results; using a RAM dump for the input file is recommended.

## Options

    -s                        scan for symbols from the built-in signature file
    -l <sig/lib/obj path(s)>  scan for symbols from signature/object/library file(s)
    -f <output format>        set the output format (pj64, nemu, armips, n64split, default)
    -o <output path>          set the output path
    -h <headersize>           set the header size  (default: 0x80000000)
    -t                        scan thoroughly
    -v                        enable verbose logging

#### `-s`

Scans against the built-in signature file. See [Included Libraries](included-libs.md) for a list of currently included libraries.

#### `-l <sig/lib/obj path(s)>`

Scans against signature files and ELF libraries/objects. If a directory path is provided, `n64sym` will use all `*.sig`, `*.a` and `*.o` files that it finds in the directory tree. This option may be used multiple times.

#### `-f <format>`

Sets the output format. Valid formats include `pj64`, `nemu`, `armips`, `n64split`, and `default`.

| Format     | Description                             |
|------------|-----------------------------------------|
| `pj64`     | Project64 debugger symbols (*.sym)      |
| `nemu`     | Nemu64 bookmarks (*.nbm)                |
| `armips`   | armips labels (*.asm)                   |
| `n64split` | n64split config labels (*.yaml)         |
| `default`  | Space-separated address and symbol name |

#### `-o <output path>`

Sets the output path. If this option is not used, `n64sym` will use the standard output.

#### `-h <headersize>`

Overrides the header size (displacement of memory address against absolute file address). By default this value is either `0x80000000` or the entry point if the input file is a ROM image.

#### `-t`

Enables thorough scanning. When this option is enabled, the scanner will check every byte of the input file instead of only checking spots that look like functions.

#### `-v`

Enables verbose output.

## Examples
```
n64sym paper_mario_ram.bin -s -f "pj64" -o "C:/Project64/Save/PAPER MARIO.sym"
```

```
n64sym "Super Mario 64 (U).z64" -s -f "n64split" -o "sm64config.yaml"
```

```
n64sym "Ronaldinho Soccer 64.z64" -l "./libultra" | grep "osPiStartDma"
```
---

# n64sig

`n64sig` is a command-line utility that generates `n64sym`-compatible signature files.

## Usage

    n64sig [options] > output_path

## Options

    -l <lib/obj path(s)>  generate signatures from object/library file(s)
    -f <format>           set the output format (json, default)
---

# Building

## Utilities

Run `make` to build `n64sym` and `n64sig`.
 
## Built-in signatures

Create a directory in the project root named `oslibs` and drop the desired library/object files in it. Then run `make rebuild_sigs` to rebuild `src/builtin_signatures.sig` and `web/signatures.json`.
