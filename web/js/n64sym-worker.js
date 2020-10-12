importScripts("crc32.js");

const SYM_NAME = 0;
const SYM_SIZE = 1;
const SYM_CRCA = 2;
const SYM_CRCB = 3;
const SYM_RELOCS = 4;

const REL_TYPE = 0;
const REL_NAME = 1;
const REL_OFFSETS = 2;

function Symbol(entry)
{
    this.name = entry[SYM_NAME];
    this.size = entry[SYM_SIZE];
    this.crcA = entry[SYM_CRCA];
    this.crcB = entry[SYM_CRCB];
    this.relocs = entry[SYM_RELOCS];
}

self.onmessage = function(e)
{
    var binary = e.data.binary;
    var signatures = e.data.signatures;
    var sigIndex = e.data.index;
    var sigCount = e.data.count;
    var offsets = e.data.offsets;
    var thorough = e.data.thorough;

    for(var i = 0; i < sigCount; i++)
    {
        var symbol = new Symbol(signatures[sigIndex + i]);

        if(!thorough)
        {
            for(var j = 0; j < offsets.length; j++)
            {
                if(testSymbol(binary, offsets[j], symbol))
                {
                    self.postMessage({ 'status': 'result', 'name': symbol.name, 'offset': offsets[j] });
                    break;
                }
            }
        }
        else
        {
            for(var offset = 0; offset < binary.byteLength - symbol.size; offset += 4)
            {
                if(testSymbol(binary, offset, symbol))
                {
                    self.postMessage({ 'status': 'result', 'name': symbol.name, 'offset': offset});
                    break;
                }
            }
        }

        self.postMessage({ 'status': 'progress' });
    }

    self.postMessage({ 'status': 'done' });
}

function readStrippedOpcode(binary, offset, relocType)
{
    var opcode = binary.slice(offset, offset + 4);
    
    switch(relocType)
    {
    case 'targ26':
        opcode[0] &= 0xFC;
        opcode[1] = 0x00;
        opcode[2] = 0x00;
        opcode[3] = 0x00;
        break;
    case 'hi16':
    case 'lo16':
        opcode[2] = 0x00;
        opcode[3] = 0x00;
        break;
    }

    return opcode;
}

function testSymbol(binary, offset, symbol)
{
    var crcA = new CRC32();
    var crcB = new CRC32();

    if(symbol.relocs.length == 0)
    {
        crcA.read(binary, offset, Math.min(symbol.size, 8));
        crcB.read(binary, offset, symbol.size);
        return (crcB.result == symbol.crcB);
    }

    var relocs = []; // flattened relocs array
    symbol.relocs.forEach(reloc => {
        reloc[REL_OFFSETS].forEach(offset => {
            relocs.push({ type: reloc[REL_TYPE], name: reloc[REL_NAME], offset: offset });
        });
    });
    relocs.sort((a, b) => a.offset - b.offset);

    var nReloc = 0;
    var fnOffset = 0;
    var crcA_limit = Math.min(symbol.size, 8);

    while(fnOffset < crcA_limit && nReloc < relocs.length)
    {
        if(fnOffset < relocs[nReloc].offset)
        {
            // read up to relocated op or crcA limit
            var start = offset + fnOffset;
            var length = Math.min(relocs[nReloc].offset, crcA_limit) - fnOffset;
            crcA.read(binary, start, length);
            crcB.read(binary, start, length);
            fnOffset += length;
        }
        else if(fnOffset == relocs[nReloc].offset)
        {
            // read stripped relocated op
            var opcode = readStrippedOpcode(binary, offset + fnOffset, relocs[nReloc].type);
            crcA.read(opcode, 0, 4);
            crcB.read(opcode, 0, 4);
            fnOffset += 4;
            nReloc++;
        }
    }

    if(fnOffset < crcA_limit)
    {
        crcA.read(binary, offset + fnOffset, crcA_limit - fnOffset);
        crcB.read(binary, offset + fnOffset, crcA_limit - fnOffset);
        fnOffset = crcA_limit;
    }

    if(crcA.result != symbol.crcA)
    {
        return false;
    }

    while(fnOffset < symbol.size && nReloc < relocs.length)
    {
        if(fnOffset < relocs[nReloc].offset)
        {
            // read up to relocated op
            crcB.read(binary, offset + fnOffset, relocs[nReloc].offset - fnOffset);
            fnOffset = relocs[nReloc.offset];
        }
        else if(fnOffset == relocs[nReloc].offset) 
        {
            // strip and read relocated op
            var opcode = readStrippedOpcode(binary, offset + fnOffset, relocs[nReloc].type);
            crcB.read(opcode, 0, 4);
            fnOffset += 4;
            nReloc++;
        }
    }

    if(fnOffset < symbol.size)
    {
        crcB.read(binary, offset + fnOffset, symbol.size - fnOffset);
        fnOffset = symbol.size;
    }

    return (crcB.result == symbol.crcB);
}
