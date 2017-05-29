# n64sym
#### N64 Symbol Identification Tool
---
Usage:
```
n64sym <RAM dump path> [options] 
```
Options:
```
-l <library/object path(s)>  scan for symbols from library or object file(s)
-v                           enable verbose logging
```
Eg:
```
n64sym paper_mario.bin -l libs/libgultra_rom.a
```


This project uses [elfio](https://github.com/serge1/ELFIO).