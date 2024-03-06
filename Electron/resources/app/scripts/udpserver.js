const UDP = require('dgram')
const server = UDP.createSocket('udp4')
const port = 2222

function getChunkDataToSend(id, packet) {
    let startPos = 1024 * packet;
    const rawChunks = document.fileDataCache[id];

    let rawChunkIndex = 0;
    while (startPos >= rawChunks[rawChunkIndex].length) {
        startPos -= rawChunks[rawChunkIndex].length;
        ++rawChunkIndex;
    }

    const endPos = startPos + 1024;
    if (endPos <= rawChunks[rawChunkIndex].length) {
        let res = new Uint8Array(1024);
        res.set(rawChunks[rawChunkIndex].slice(startPos, endPos), 0);
        return res;
    }

    const left = rawChunks[rawChunkIndex].length - startPos;
    if (rawChunkIndex == rawChunks.length - 1) {
        let  res = new Uint8Array(left);
        res.set(rawChunks[rawChunkIndex].slice(startPos, startPos + left), 0);
        return res;
    }

    const nextChunkPart = Math.min(1024 - left, rawChunks[rawChunkIndex + 1].length);
    const buffSize = left + nextChunkPart;
    let res = new Uint8Array(buffSize);
    res = res.set(rawChunks[rawChunkIndex].slice(startPos, startPos + left));
    res = res.set(rawChunks[rawChunkIndex + 1].slice(0, nextChunkPart), startPos + left);
    return res;
}

server.on('listening', () => {
    // Server address it’s using to listen

    const address = server.address()

    console.log('Listining to ', 'Address: ', address.address, 'Port: ', address.port)
})

server.on('message', async (message, info) => {
    const messageData = JSON.parse(message.toString());

    let t = 0;
    for (let index = messageData.packetRange[0]; index <= messageData.packetRange[1]; ++index) {
        function senderFunc(id, index) {
            const buff = getChunkDataToSend(id, index);
            const res = new Uint8Array(buff.length + 8);

            function getInt32Bytes(x) {
                let bytes = [];
                let i = 4;
                do {
                    bytes[--i] = x & (255);
                    x = x >> 8;
                } while (i);

                return bytes;
            }

            {
                res.set(getInt32Bytes(messageData.fileId));
                res.set(getInt32Bytes(index), 4);
                res.set(buff, 8);
            }

            setTimeout(() => {
                server.send(res, info.port, info.address, (err) => { });
            }, 0);
        }

        senderFunc(messageData.fileId, index);
    }
})

server.bind(port)