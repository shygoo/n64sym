# n64sym utility

`n64sym` is a command-line utility that identifies symbols in N64 games.

## Usage

    n64sym <input path> [options] 

## Options

    -s                        scan for symbols from the built-in signature file
    -l <sig/lib/obj path(s)>  scan for symbols from signature/object/library file(s)
    -f <output format>        set the output format (pj64, nemu, armips, n64split, default)
    -h <headersize>           set the header size  (default: 0x80000000)
    -t                        scan thoroughly
    -v                        enable verbose logging

### `<input path>`

Sets the path of the file to scan. The input file may either be a RAM dump or ROM image.

If the file extension is `.z64`, `.n64`, or `.v64`, the tool will assume the file is a ROM image and attempt to identify the position of the main code segment and adjust the symbol addresses accordingly. Note that scanning a ROM image may yield inaccurate results; using a RAM dump for the input file is recommended.

### `-s`

Scans against the built-in signature file. See [Included Libraries](included-libs.md) for a list of currently included libraries.

### `-l <library/object path(s)>`

Scans against ELF libraries and objects. If a directory path is provided, `n64sym` will use all `*.a` and `*.o` files that it finds in the directory tree.

### `-f <format>`

Sets the output format. Valid formats include `pj64`, `nemu`, `armips`, `n64split`, and `default`.

| Format     | Description                             |
|------------|-----------------------------------------|
| `pj64`     | Project64 debugger symbols (*.sym)      |
| `nemu`     | Nemu64 bookmarks (*.nbm)                |
| `armips`   | armips labels (*.asm)                   |
| `n64split` | n64split config labels (*.yaml)         |
| `default`  | Space-separated address and symbol name |

### `-h <headersize>`

Overrides the header size (the value to add to symbol addresses). By default this value is either `0x80000000` or a value determined by the entry point field and bootcode if the input file is a ROM image.

### `-t`

Scan thoroughly. When this option is enabled, the scanner will check every byte of the input file instead of only checking spots that look like functions.

### `-v`

Enable verbose logging.

## Example

    n64sym paper_mario.bin -b > paper_mario_symbols.sym

---

# n64sig utility

`n64sig` is a command-line utility that generates `n64sym`-compatible signature files.

## Usage

    n64sig <output path> <library/object path(s)>


