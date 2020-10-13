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
| `type`    | Type of relocation              |
| `name`    | Name of the referenced symbol   |
| `offsets` | Space-separated list of offsets |

`type` may be one of the following: `.hi16`, `.lo16`, `.targ26`.