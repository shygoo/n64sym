const CRC32Table = (function()
{
    var table = [];

    for(var i = 0; i < 256; i++)
    {
        var crc = i;

        for(var j = 0; j < 8; j++)
        {
            crc = (crc & 1) ? (crc >>> 1) ^ 0xEDB88320 : (crc >>> 1)
        }

        table.push(crc >>> 0);
    }

    return table;
})();

function crc32(arr, offs, size)
{
    var crc = 0xFFFFFFFF;

    for(var i = 0; i < size; i++)
    {
        crc = (CRC32Table[(crc & 0xFF) ^ arr[offs + i]] ^ (crc >>> 8)) >>> 0;
    }

    return (~crc) >>> 0;
}