let _panel;

const { render } = require('./renderHTML');
const { choose } = require('./modal');
const { flushPrefs } = require('../files');

const fs = require('fs');
const path = require('path');

async function init() {
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

    const isValidDir = await new Promise(async (resolve, reject) => {

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


    const { ipcRenderer } = require('electron');
    let selectDirCB = undefined;
    ipcRenderer.on('path', (event, data) => {
        selectDirCB(data);
        selectDirCB = undefined;
    });

    dirButton.element.addEventListener('click',  async () => {
        if (selectDirCB) {
            return;
        }
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
        chosenDir = val;
        updateDir();
        flushPrefs(prefs);
    });
}

function getPanel() {
    if (!_panel) {
        _panel = render('directory_panel');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;