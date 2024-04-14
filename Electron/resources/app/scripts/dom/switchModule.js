let _panel;

const { render } = require('./renderHTML');

function init() {
    const panel = getPanel();

    panel.tagged.input.checked = true;

    async function updateMode() {
        let checked = panel.tagged.input.checked;
        panel.tagged.mode.innerHTML = checked ? 'Server Mode' : 'Client Mode';

        const mainMod = await app.modules.main;
        const netMod = await app.modules.net;
        const dirMod = await app.modules.dir;

        mainMod.tagged.run_server.style.display = checked ? '' : 'none';
        mainMod.tagged.download_files.style.display = checked ? 'none' : '';

        netMod.interface.changeMode(checked);
        dirMod.interface.changeMode(checked);
    }

    updateMode();

    panel.tagged.input.addEventListener('change', async () => {
        const unlockModal = await app.locks.modal();
        if (!unlockModal) {
            panel.tagged.input.checked = !panel.tagged.input.checked;
            return;
        }
        unlockModal();

        const unlockRunning = await app.locks.running();
        if (!unlockRunning) {
            panel.tagged.input.checked = !panel.tagged.input.checked;

            const notification = await app.modules.notification;
            notification.interface.show('File transfer has already started!');
            return;
        }
        unlockRunning();

        updateMode();
    });

    panel.interface = {
        isServer: () => panel.tagged.input.checked
    };

    return panel;
}

function getPanel() {
    if (!_panel) {
        _panel = render('switch');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;