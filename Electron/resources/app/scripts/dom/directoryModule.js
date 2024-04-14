let _panel;

const { render } = require('./renderHTML');
const { flushPrefs } = require('../files');

const fs = require('fs');

function init() {
    const prefs = document.prefs;

    const panel = getPanel();
    const dirButton = render('button');

    panel.tagged.dir_button.appendChild(dirButton.element);

    async function getChosenDir() {
        const switchModule = await app.modules.modeSwitch;

        const isServer = switchModule.interface.isServer();
        let dirProp = isServer ? 'srcDir' : 'dstDir';

        const res = prefs[dirProp];
        return res;
    }

    async function setChosenDir(dir) {
        const switchModule = await app.modules.modeSwitch;

        const isServer = switchModule.interface.isServer();
        let dirProp = isServer ? 'srcDir' : 'dstDir';

        prefs[dirProp] = dir;
    }

    async function updateDir() {
        const chosenDir = await getChosenDir();

        if (!chosenDir) {
            dirButton.tagged.name.innerHTML = 'Choose Dir';
            return;
        }

        dirButton.tagged.name.innerHTML = chosenDir;
    }

    async function isValidDirChosen() {
        const res = await checkValidDir();
        return res;
    }

    async function checkValidDir() {
        const isValidDir = await new Promise(async (resolve, reject) => {

            const switchModule = await app.modules.modeSwitch;

            const isServer = switchModule.interface.isServer();
            let dirProp = isServer ? 'srcDir' : 'dstDir';

            if (!prefs[dirProp]) {
                resolve(false);
                return;
            }

            fs.exists(prefs[dirProp], exists => {
                if (!exists) {
                    resolve(false);
                }

                fs.stat(prefs[dirProp], (err, stats) => {
                    resolve(stats.isDirectory());
                });
            });
        });

        await updateDir();
        return isValidDir;
    }
    checkValidDir();

    const { ipcRenderer } = require('electron');
    let selectDirCB = undefined;
    ipcRenderer.on('path', (event, data) => {
        selectDirCB(data);
        selectDirCB = undefined;
    });

    dirButton.element.addEventListener('click', async () => {
        const unlockModal = await app.locks.modal();
        if (!unlockModal) {
            return;
        }

        await selectDir();

        unlockModal();
    });

    async function selectDir() {

        const unlockDir = await app.locks.dir();
        if (!unlockDir) {
            const notification = await app.modules.notification;
            notification.interface.show('Directory already selected!');
            return;
        }
        unlockDir();


        let val = await getChosenDir();
        const dir = await new Promise((resolve, reject) => {
            selectDirCB = dir => {
                resolve(dir);
            };
            ipcRenderer.send('select_dir');
        });

        if (dir && dir.length > 0) {
            val = dir[0];
        }

        await setChosenDir(val);
        await checkValidDir();

        flushPrefs(prefs);
    };

    panel.interface = {
        isValidDirChosen: isValidDirChosen,
        getDir: getChosenDir,
        changeMode: updateDir
    };

    return panel;
}

function getPanel() {
    if (!_panel) {
        _panel = render('directory_panel');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;