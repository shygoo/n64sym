# n64sym utility

`n64sym` is a command-line utility that identifies symbols in N64 games.

## Usage

    n64sym <input path> [options] 

## Options

    -b                                     use the built-in signature file
    -l <signature/library/object path(s)>  scan for symbols from signature/object/library file(s)
    -f <output format>                     set the output format (pj64, nemu, n64split)
    -t                                     scan thoroughly

### `<input path>`

Sets the path of the file to scan. The input file may either be a RAM dump or ROM image.

If the file extension is `.z64`, `.n64`, or `.v64`, the tool will assume the file is a ROM image and attempt to identify the position of the main code segment and adjust the symbol addresses accordingly.

### `-s`

Scans against the built-in signature file. The built-in signature file currently includes symbol descriptions for the following N64 OS libraries:

|               | 2.0c | SN64 | 2.0h | 2.0i | 2.0j | 2.0k | 2.0l |
|---------------|:----:|:----:|:----:|:----:|:----:|:----:|:----:|
| `libultra`    | ✅  | ✅   | ❌  | ✅   | ✅  |   ❌ |  ✅  |
| `libgultra`   | ❌  | ❌   | ✅  | ✅   | ✅  |   ✅ |  ✅  |
| `libn_audio`  | ❌  | ❌   | ❌  | ❌   | ✅  |   ❌ |  ✅  |
| `libgn_audio` | ❌  | ❌   | ❌  | ❌   | ✅  |   ✅ |  ✅  |
| `libleo`      | ❌  | ❌   | ✅  | ✅   | ✅  |   ✅ |  ✅  |

### `-l <library/object path(s)>`

Scans against ELF libraries and objects. If a directory path is provided, `n64sym` will use all `*.a` and `*.o` files that it finds in the directory tree.

### `-f <format>`

Sets the output format. Valid formats include `pj64`, `nemu`, and `n64split`. If this option is not used, `n64sym` will use `pj64`.

### `-t`

Enables thorough scanning.

## Example

    n64sym paper_mario.bin -b > paper_mario_symbols.sym

---

# n64sig utility

`n64sig` is a command-line utility that generates `n64sym`-compatible signature files.

## Usage

    n64sig <output path> <library/object path(s)>

---

# Signature file format

An `n64sym`-compatible signature file contains a list of symbol and relocation descriptions in plain text. Signature files may be generated using the `n64sig` utility included in this repository.

## File extension

Signature files must be named with an extension of `.sig`.

## Symbol definitions

A symbol definition describes a symbol by its size and a pair CRCs representing the data.

### Syntax:

    name size crcA crcB

| Field  | Description                                                      |
|--------|------------------------------------------------------------------|
| `name` | Name of the symbol                                               |
| `size` | Byte length of the symbol's data                                 |
| `crcA` | CRC32 of the first 8 bytes or all bytes if `size` is less than 8 |
| `crcB` | CRC32 of all bytes                                               |

## Relocation definitions

A relocation definition describes a relocation within the last symbol's data.

### Syntax:

    reltype relname offsets

| Field     | Description                     |
|-----------| --------------------------------|
| `reltype` | Type of relocation              |
| `relname` | Name of the referenced symbol   |
| `offsets` | Space-separated list of offsets |

`type` may be one of the following: `.hi16`, `.lo16`, `.targ26`.

