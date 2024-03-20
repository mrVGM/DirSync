let _panel;

const { render } = require('./renderHTML');
const { choose } = require('./modal');
const { flushPrefs } = require('../files');

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

function getPCName() {
    const { hostname } = require('os');
    const name = hostname();
    return name;
}

function init() {
    const panel = getPanel();

    const pairButton = render('button');
    const interfaceButton = render('button');

    panel.tagged.pair_button.appendChild(pairButton.element);
    panel.tagged.interface_button.appendChild(interfaceButton.element);


    pairButton.tagged.name.innerHTML = 'Find Peer';
    
    const prefs = document.prefs;
    const nets = getNets();

    let pcName = getPCName();
    {
        if (prefs.pcName) {
            pcName = prefs.pcName;
        }

        const pcNameEl = panel.tagged.pc_name;
        pcNameEl.value = pcName;

        let curName = pcName;

        pcNameEl.addEventListener('change', event => {
            let tmp = pcNameEl.value.trim();
            if (tmp.length === 0) {
                pcNameEl.value = curName;
                return;
            }

            curName = tmp;
            pcNameEl.value = curName;
            prefs.pcName = curName;
            flushPrefs(prefs);
        });
    }

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

    let peerChosen;

    pairButton.element.addEventListener('click', async () => {
        if (!netOfChoice) {
            alert('Please choose a network interface!');
            return;
        }

        const net = nets[netOfChoice];

        const peer = await new Promise(async (resolve, reject) => {

            const { findPeers } = require('../peers');
            const { req, peers, destroySocket } = findPeers(net);

            let stopReqs = false;
            const lookup = new Promise((resolve, reject) => {
                function lookupReq() {
                    if (stopReqs) {
                        resolve();
                        return;
                    }
                    req();

                    setTimeout(lookupReq, 1000);
                }

                lookupReq();
            });

            let choosePeer = new Promise((resolve, reject) => {
                let finish = false;
                function updatePeers() {
                    if (finish) {
                        return;
                    }

                    const options = [];
                    const p = peers();
                    for (let k in p) {
                        const cur = p[k];
                        options.push({
                            name: `${cur.pcName} - ${k}`,
                            addr: cur
                        });
                    }
                    choose(options).then(x => {
                        finish = true;
                        keepChecking = false;
                        resolve(x);
                    });

                    setTimeout(updatePeers, 2000);
                }

                updatePeers();
            });

            const choice = await choosePeer;
            stopReqs = true;
            await lookup;

            destroySocket();

            resolve(choice);
        });

        peerChosen = peer;
        console.log(peer);

        pairButton.tagged.name.innerHTML = peer.name;

        return;
    });

    const { initPeerClient } = require('../udpserver');

    let onPeerFound = undefined;
    const client = initPeerClient(async (message, info) => {
        if (!onPeerFound) {
            return;
        }

        const mess = JSON.parse(message.toString());
        onPeerFound({
            ip: info.address,
            port: mess.port,
            pcName: mess.pcName
        });

        return;
    });

    panel.interface = {
        getPeer: () => peerChosen,
        getPCName: () => pcName,
    };

    return panel;
}

function getPanel() {
    if (!_panel) {
        _panel = render('network_interface_panel');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;