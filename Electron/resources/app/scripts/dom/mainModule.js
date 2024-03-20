let _panel;

const { render } = require('./renderHTML');

function init() {
    const panel = getPanel();

    function log(str) {
        const infoPanel = panel.tagged.info_panel;

        const div = document.createElement("div");
        div.innerHTML = str;
        infoPanel.appendChild(div);
    }

    function createBar() {
        const template = document.createElement('div');

        const src = `
<div class="progress-bar-empty" style="display: flex; width: 90%; height: 30px; margin-bottom: 5px;">
    <div style="position: relative; flex: 1; margin: 3px;">
        <div myId="bar" class="progress-bar-full" style="position: absolute; width: 20%; height: 100%;"></div>
        <div style="display: flex; flex-direction: column; position: absolute; width: 100%; height: 100%; align-items: center;">
            <div style="flex: 1;"></div>
            <div myId="label"></div>
            <div style="flex: 1;"></div>
        </div>
    </div>
</div>
`
        template.innerHTML = src;
        const res = template.children[0];

        return res;
    }

    async function initButtons() {
        const dirModule = await app.modules.dir;
        const netModule = await app.modules.net;

        panel.tagged.run_server.addEventListener('click', async () => {
            if (!dirModule.interface.isValidDirChosen()) {
                alert('Please choose a valid directory!');
                return;
            }
            
            const { startPeerServer } = require('../peers');
            await startPeerServer(netModule.interface.getPCName);
            log('Peer Server running!');
        });
    }

    initButtons();

    return panel;
}

function getPanel() {
    if (!_panel) {
        _panel = render('main_panel');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;