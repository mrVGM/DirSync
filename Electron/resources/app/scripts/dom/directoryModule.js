let _panel;

const { render } = require('./renderHTML');
const { flushPrefs } = require('../files');

const fs = require('fs');

function init() {
    const panel = getPanel();
    const dirButton = render('button');

    panel.tagged.dir_button.appendChild(dirButton.element);

    let chosenDir;
    function updateDir() {
        if (!chosenDir) {
            dirButton.tagged.name.innerHTML = 'Choose Dir';
            return;
        }

        dirButton.tagged.name.innerHTML = chosenDir;
    }

    const prefs = document.prefs;

    let isValidDir = false;

    async function checkValidDir() {
        isValidDir = await new Promise(async (resolve, reject) => {

            if (!prefs.dir) {
                resolve(false);
                return;
            }

            fs.exists(prefs.dir, exists => {
                if (!exists) {
                    resolve(false);
                }

                fs.stat(prefs.dir, (err, stats) => {
                    resolve(stats.isDirectory());
                });
            });
        });

        if (isValidDir) {
            chosenDir = prefs.dir;
        }
        updateDir();
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


        let val = chosenDir;
        const dir = await new Promise((resolve, reject) => {
            selectDirCB = dir => {
                resolve(dir);
            };
            ipcRenderer.send('select_dir');
        });

        if (dir && dir.length > 0) {
            val = dir[0];
        }

        prefs.dir = val;
        await checkValidDir();

        flushPrefs(prefs);
    };

    panel.interface = {
        isValidDirChosen: () => isValidDir,
        getDir: () => chosenDir,
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