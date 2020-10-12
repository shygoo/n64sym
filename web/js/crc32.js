function CRC32()
{
    this.crc = 0xFFFFFFFF;
    this.result = 0;
}

function crc32(arr, offs, size)
{
    var crc = new CRC32();
    crc.read(arr, offs, size);
    return crc.result;
}

CRC32.TABLE = (function()
{
    var table = [];

    for(var i = 0; i < 256; i++)
    {
        var crc = i;

        for(var j = 0; j < 8; j++)
        {
            crc = (crc & 1) ? (crc >>> 1) ^ 0xEDB88320 : (crc >>> 1);
        }

        table.push(crc >>> 0);
    }

    return table;
})();

CRC32.prototype.reset = function()
{
    this.crc = 0xFFFFFFFF;
}

CRC32.prototype.read = function(arr, offs, length)
{
    for(var i = 0; i < length; i++)
    {
        this.crc = (CRC32.TABLE[(this.crc & 0xFF) ^ arr[offs + i]] ^ (this.crc >>> 8)) >>> 0;
    }

    this.result = (~this.crc) >>> 0;
}
