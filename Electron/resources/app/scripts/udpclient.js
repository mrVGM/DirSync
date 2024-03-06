const UDP = require('dgram')

const client = UDP.createSocket('udp4')
const port = 2222;
const hostname = 'localhost';

const handlers = {};

function intFromBytes(x) {
    let val = 0;
    for (let i = 0; i < x.length; ++i) {
        val += x[i];
        if (i < x.length - 1) {
            val = val << 8;
        }
    }
    return val;
}

client.on('message', (message, info) => {
    const file = intFromBytes(message.slice(0, 4));
    if (handlers[file]) {
        handlers[file](message);
    }
});

function requestMissingChunks(chunks) {
    function getSubRanges(range) {
        let left = range[0];
        let right = range[1];

        while (chunks[left]) {
            ++left;
            if (left > right) {
                left = right;
                break;
            }
        }

        while (chunks[right]) {
            --right;
            if (right < left) {
                right = left;
                break;
            }
        }

        function split(left, right) {
            const mid = Math.floor((left + right) / 2);
            const leftRange = [left, mid - 1];
            const rightRange = [mid, right];

            const res = getSubRanges(leftRange).concat(getSubRanges(rightRange));
            return res;
        }

        if (right - left + 1 > 64) {
            return split(left, right);
        }

        if (left === right) {
            if (chunks[left]) {
                return [];
            }
            else {
                return [[left, right]];
            }
        }

        let received = 0;
        for (let i = left; i <= right; ++i) {
            if (chunks[i]) {
                ++received;
            }
        }

        if (received < 5) {
            return [[left, right]];
        }

        return split(left, right);
    }

    const rangesToRequest = getSubRanges([0, chunks.length - 1]);
    return rangesToRequest;
}

exports.fetchFile = async (id) => {
    const { getRecs } = require('./httpserver');
    const { writeFile } = require('./files');
    const recs = await getRecs();
    const file = recs[id];

    document.curFile = file;
    await writeFile(document.prefs.dstDir, file.path, []);

    if (file.numPackets === 0) {
        return;
    }
    console.log(`Downloading file ${file.path} with ${file.numPackets} packets`);

    progress1 = [0, file.numPackets];
    let chunkReceived  = false;

    function* chunkGroups() {
        let start = 0;

        while (start < file.numPackets) {
            const end = Math.min(start + 5, file.numPackets - 1);
            yield [start, end];
            start = end + 1;
        }
    }

    const it = chunkGroups();
    let curChunkGroup = it.next();

    const path = require('path');
    const fs = require('fs');

    const writer = fs.createWriteStream(path.join(document.prefs.dstDir, file.path));
    while (!curChunkGroup.done) {
        const chunkGroup = curChunkGroup.value;
        curChunkGroup = it.next();
        const chunks = {};

        function areAllChunksHere() {
            for (let i = chunkGroup[0]; i <= chunkGroup[1]; ++i) {
                if (!chunks[i]) {
                    return false;
                }
            }

            return true;
        }
        handlers[id] = function (data) {
            const chunkId = intFromBytes(data.slice(4, 8));
            if (chunkGroup[0] <= chunkId && chunkId <= chunkGroup[1]) {
                if (!chunks[chunkId]) {
                    ++progress1[0];
                }
                chunks[chunkId] = data.slice(8);
            }
            chunkReceived = true;
        };

        async function requestChunks() {
            const message = {
                fileId: id,
                packetRange: chunkGroup
            };

            const packet = Buffer.from(JSON.stringify(message));
            client.send(packet, port, hostname, (err, num) => { });

            await new Promise((resolve, reject) => {
                setTimeout(() => {
                    resolve();
                }, 1);
            });
        }

        await new Promise(async (resolve, reject) => {
            while (!areAllChunksHere()) {
                await requestChunks();
            }

            for (let i = chunkGroup[0]; i <= chunkGroup[1]; ++i) {
                await new Promise((resolve, reject) => {
                    writer.write(chunks[i], err => {
                        resolve();
                    });
                });
            }

            resolve();
        });
    }
    writer.close();
};

