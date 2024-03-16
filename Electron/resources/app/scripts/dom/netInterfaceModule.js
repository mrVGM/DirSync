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

function init() {
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

    interfaceButton.element.addEventListener('click', async () => {
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