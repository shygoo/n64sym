.global gBuiltinSignatureFile

gBuiltinSignatureFile:
    .incbin "builtin_signatures.sig.defl"
    .long 0
