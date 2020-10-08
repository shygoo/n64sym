importScripts("crc32.js");

const SYM_NAME = 0;
const SYM_SIZE = 1;
const SYM_CRCA = 2;
const SYM_CRCB = 3;
const SYM_RELOCS = 4;

const REL_TYPE = 0;
const REL_NAME = 1;
const REL_OFFSETS = 3;

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


function testSymbol(binary, offset, symbol)
{
    if(symbol.relocs.length == 0)
    {
        return (symbol.crcA == crc32(binary, offset, Math.min(symbol.size, 8))) &&
               (symbol.crcB == crc32(binary, offset, symbol.size));
    }

    return false;
}
