const { initPeerServer, initPeerClient } = require('./udpServer');
const { runUDPServer } = require('./backend')

let _started = false;
let _resolve;

const _tcpServer = new Promise((resolve, reject) => {
    _resolve = resolve;
});

async function startPeerServer(getPCName) {
    if (_started) {
        return;
    }
    _started = true;

    const { initServer } = require('./tcpServer');
    const tcpServer = await initServer(socket => {
        const handlers = { };

        _resolve({
            send: data => {
                socket.write(JSON.stringify(data));
            },
            setHandler: (req, handler) => {
                handlers[req] = handler;
            }
        });


        socket.on('data', data => {
            const json = JSON.parse(data.toString());
            const req = json.req;

            const handler = handlers[req];
            if (handler) {
                handler(json);
                return;
            }

            socket.write(JSON.stringify(json));
        });

        socket.on('close', () => {
            console.log('closed');
        });
    });



    const fileServer = await runUDPServer();

    function peerServer(message, info) {
        const messageData = JSON.parse(message.toString());

        if (messageData.req === 'TCPPort?') {
            const res = JSON.stringify({
                port: tcpServer.address().port,
                pcName: getPCName()
            });
            server.send(res, info.port, info.address, (err) => { });
        }
    }

    const server = await initPeerServer(peerServer);

    return fileServer;
}

function findPeers(net) {
    let peersFound = {};

    const { send, client } = initPeerClient(async (message, info) => {
        const mess = JSON.parse(message.toString());
        peersFound[info.address] = {
            ip: info.address,
            port: mess.port,
            pcName: mess.pcName
        };

        return;
    });

    function toBin(n) {
        const res = [];
        while (n > 0) {
            res.unshift(n % 2);
            n = Math.floor(n / 2);
        }

        while (res.length < 8) {
            res.unshift(0);
        }
        return res;
    }

    function fromBin(bin) {
        let res = 0;
        for (let i = 0; i < 8; ++i) {
            res *= 2;
            res += bin[i]
        }
        return res;
    }

    const ip = net.address;

    let ipNum = ip.split('.');
    ipNum = ipNum.map(x => parseInt(x));
    ipNum = ipNum.map(x => toBin(x));

    const mask = net.netmask;
    let numMask = mask.split('.');
    numMask = numMask.map(x => parseInt(x));
    numMask = numMask.map(x => toBin(x));

    let broadcastAddr = [];
    for (let i = 0; i < ipNum.length; ++i) {
        const cur = [];
        broadcastAddr.push(cur);

        const curIp = ipNum[i];
        const curMask = numMask[i];
        for (let j = 0; j < 8; ++j) {
            if (curMask[j]) {
                cur.push(curIp[j]);
            }
            else {
                cur.push(1);
            }
        }
    }

    broadcastAddr = broadcastAddr.map(x => fromBin(x));
    broadcastAddr = broadcastAddr.map(x => x.toString());
    let broadcastStr = `${broadcastAddr[0]}.${broadcastAddr[1]}.${broadcastAddr[2]}.${broadcastAddr[3]}`;

    const res = {
        req: () => {
            send(JSON.stringify({ req: 'TCPPort?' }), broadcastStr);
        },
        peers: () => peersFound,
        destroySocket: () => {
            client.close();
        }
    };

    return res;
}

exports.startPeerServer = startPeerServer;
exports.findPeers = findPeers;
exports.getTCPServer = () => _tcpServer;