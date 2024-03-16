let _panel;

const { render } = require('./renderHTML');
const { choose } = require('./modal');
const { flushPrefs } = require('../files');
const { log } = require('console');

function getNets() {
    const { networkInterfaces } = require('os');

    const nets = networkInterfaces();
    const res = {};

    for (const name of Object.keys(nets)) {
        for (const net of nets[name]) {
            // Skip over non-IPv4 and internal (i.e. 127.0.0.1) addresses
            // 'IPv4' is in Node <= 17, from 18 it's a number 4 or 6
            const familyV4Value = typeof net.family === 'string' ? 'IPv4' : 4
            if (net.family === familyV4Value && !net.internal) {
                res[name] = net;
                break;
            }
        }
    }

    return res;
}

async function init() {
    const panel = getPanel();

    const pairButton = render('button');
    const interfaceButton = render('button');

    panel.tagged.pair_button.appendChild(pairButton.element);
    panel.tagged.interface_button.appendChild(interfaceButton.element);


    pairButton.tagged.name.innerHTML = 'Find Peer';
    
    const prefs = document.prefs;
    const nets = getNets();

    let netOfChoice = undefined;
    
    function updateNetOfChoice() {
        if (!netOfChoice) {
            interfaceButton.tagged.name.innerHTML = 'Choose Net Interface';
            return;
        }

        interfaceButton.tagged.name.innerHTML = `${netOfChoice} - ${nets[netOfChoice].address}`;
    }

    for (let k in nets) {
        if (k === prefs.netInterface) {
            netOfChoice = k;
        }
    }

    updateNetOfChoice();

    async function chooseNet() {
        const netArray = [];
        for (let k in nets) {
            netArray.push({
                name: k
            });
        }
        const choice = await choose(netArray);

        netOfChoice = choice.name;

        prefs.netInterface = choice.name;
        flushPrefs(prefs);

        updateNetOfChoice();
    }

    interfaceButton.element.addEventListener('click', chooseNet);
    pairButton.element.addEventListener('click', async () => {
        if (!netOfChoice) {
            await chooseNet();
        }

        const net = nets[netOfChoice];
        console.log(net);

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
            res = 0;
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

        onPeerFound = addr => {
            console.log(addr);
        };
        client(JSON.stringify({ req: 'TCPPort?' }), broadcastStr);
    });

    const { initServer, initClient } = require('../tcpServer');
    const tcpServer = await initServer(socket => {
        socket.on('data', data => {
            console.log(data.toString());
        });

        socket.on('close', () => {
            console.log('closed');
        });
    });

    const { initPeerServer, initPeerClient } = require('../udpserver');
    function peerServer(message, info) {
        const messageData = JSON.parse(message.toString());

        if (messageData.req === 'TCPPort?') {
            const res = JSON.stringify({
                port: tcpServer.address().port
            });
            server.send(res, info.port, info.address, (err) => { });
        }
    }
    const server = await initPeerServer(peerServer);


    let onPeerFound = undefined;
    const client = initPeerClient(async (message, info) => {
        if (!onPeerFound) {
            return;
        }

        const mess = JSON.parse(message.toString());
        onPeerFound({
            ip: info.address,
            port: mess.port
        });

        return;


        const client = await initClient(info.address, mess.port);
        client.write('fwefewf');
        client.destroy();
    });

    
}

function getPanel() {
    if (!_panel) {
        _panel = render('network_interface_panel');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;