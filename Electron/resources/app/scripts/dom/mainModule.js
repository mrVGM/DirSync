let _panel;

const { render } = require('./renderHTML');

async function init() {
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

    const { getRecs, stopBucket } = require('../httpserver');
    const { getRecords, readPrefs, flushPrefs, getFileList } = require('../files');


    async function initButtons() {
        const { ipcRenderer } = require('electron');
        const argv = await new Promise((resolve, reject) => {
            ipcRenderer.on('argv', (event, data) => {
                resolve(data);
            });
            ipcRenderer.send('get_argv');
        });

        const path = require('path');
        const { getSendFunc } = await require('../pipeserver');

        const { exec } = require('child_process');

        log('Waiting for backend...');
        const send = await getSendFunc(argv[1], argv[2]);
        sendToCpp = send;

        log('Backend running!');

        async function calculateFileHashes(rootDir, fileList, progress) {
            const fs = require('fs');
            const path = require('path');

            const tmp = fileList.map(async x => {
                const fullPath = path.join(rootDir, x.path);

                const exist = await new Promise((resolve, reject) => {
                    fs.exists(fullPath, b => {
                        resolve(b);
                    });
                });

                if (!exist) {
                    return new Promise((resolve, reject) => {
                        resolve({
                            path: x,
                            hash: '',
                            fileSize: 0
                        });
                    });
                    ++progress;
                }

                const req = {
                    op: 'hash',
                    file: fullPath
                };

                const pr = send(req);
                const res = await pr;
                res.path = x.path;
                ++progress[0];

                return res;
            });

            const res = [];
            for (let i = 0; i < tmp.length; ++i) {
                const resp = await tmp[i];
                res.push(resp);
            }

            return res;
        }


        panel.tagged.download_files.addEventListener('click', async event => {
            const interfaceModule = await app.modules.interface();
            if (!interfaceModule.interface.getPeer()) {
                alert('Please choose a Peer!');
                return;
            }

            let serverList = await getRecs();
            let fileList = serverList;
            log('File list acquired.')

            progress2[1] = fileList.length;
            progress2[0] = 0;

            log('Calculating file hashes');

            let myHashes = await calculateFileHashes(document.prefs.dstDir, fileList, progress2);
            console.log(myHashes);

            fileList = fileList.filter((x, index) => x.hash !== myHashes[index].hash);

            progress2[1] = fileList.length;
            progress2[0] = 0;

            const { writeFile, createFolders } = require('./scripts/files');

            function* filesIt() {
                for (let i = 0; i < fileList.length; ++i) {
                    yield {
                        fileEntry: fileList[i],
                        index: i
                    };
                }
            }

            const it = filesIt();

            let currentlyDownloading = 0;
            let downloaded = 0;

            const maxDownloadsInParallel = 5;

            await new Promise((resolve, reject) => {

                function startDownloading() {
                    if (currentlyDownloading >= maxDownloadsInParallel) {
                        return;
                    }

                    let cur = it.next();
                    if (!cur.value) {
                        return;
                    }

                    const { fileEntry, index } = cur.value;

                    log(`Downloading ${fileEntry.path} (${fileEntry.fileSize / 1000} KB)...`);

                    ++currentlyDownloading;

                    const pr = new Promise(async (resolve, reject) => {
                        const fileId = index;
                        let checkProgress = true;
                        document.curFile = fileEntry;

                        if (fileEntry.fileSize === 0) {
                            await writeFile(document.prefs.dstDir, fileEntry.path, []);
                            resolve();
                            return;
                        }

                        await createFolders(document.prefs.dstDir, fileEntry.path);

                        send({
                            op: 'download_file',
                            file_id: fileId,
                            ip_addr: document.prefs.serverIP,
                            file_size: fileEntry.fileSize,
                            path: path.join(document.prefs.dstDir, fileEntry.path)
                        }).then(() => {

                            checkProgress = false;
                            if (bar) {
                                bar.parentNode.removeChild(bar);
                            }
                            stopBucket(fileId).then(x => { console.log(x); });
                            resolve();
                        });

                        let bar;
                        let prBar;
                        let label;

                        async function reqProgress() {
                            if (!checkProgress) {
                                return;
                            }

                            const prog = await send({
                                op: 'get_download_progress',
                                file_id: fileId,
                            });

                            if (!checkProgress) {
                                return;
                            }

                            if (prog.progress[0] > 0) {
                                if (!bar) {
                                    bar = createBar();
                                    prBar = bar.querySelector('div[myId="bar"]');
                                    label = bar.querySelector('div[myId="label"]');

                                    panel.tagged.bar_space.appendChild(bar);
                                }

                                let progress = 0;
                                if (prog.progress[1] > 0) {
                                    progress = prog.progress[0] / prog.progress[1];
                                }
                                prBar.style.width = `${100 * progress}%`;
                                label.innerHTML = `${fileEntry.path} [${prog.progress[0]}/${prog.progress[1]}]`;
                            }

                            setTimeout(reqProgress, 10);
                        }

                        reqProgress();
                    });

                    pr.then(() => {
                        ++downloaded;
                        ++progress2[0];
                        if (downloaded >= fileList.length) {
                            resolve();
                        }
                        --currentlyDownloading;
                        startDownloading();
                    });

                    startDownloading();
                }

                startDownloading();
            });

            log('Download Complete!');
        });


        document.getElementById("run_server").addEventListener('click', async event => {
            const dirModule = await app.modules.dir();

            if (!dirModule.interface.isValidDirChosen()) {
                alert('Plase choose a valid Directory!');
                return;
            }

            const myFileList = new Promise(async (resolve, reject) => {

                let fileList = await getFileList(document.prefs.srcDir);
                progress2[1] = fileList.length;

                log('Calculating file hashes...');
                fileList = await calculateFileHashes(document.prefs.srcDir, fileList, progress2);

                {
                    let reqs = fileList.map((x, index) => {
                        const path = require('path');
                        const req = {
                            op: 'set_file_id',
                            file: path.join(document.prefs.srcDir, x.path),
                            file_id: index
                        };

                        const pr = send(req);
                        return pr;
                    });

                    for (let i = 0; i < reqs.length; ++i) {
                        await reqs[i];
                    }
                }

                resolve(fileList);
            });

            const fileList = await myFileList;

            console.log(fileList);

            await send({
                op: 'run_udp_server'
            });
            log('UDP server running.');

            const interfaceModule = await app.modules.interface();
            await interfaceModule.interface.startPeerServer();

            log('Peer UDP server running.');

            return;
        });

        function updateProgressBar(progressValue, bar, label) {
            let progress = 0;
            if (progressValue[1] !== 0) {
                progress = progressValue[0] / progressValue[1];
            }

            bar.style.width = `${100 * progress}%`;
            label.innerHTML = `[${progressValue[0]}/${progressValue[1]}]`;

            setTimeout(() => {
                updateProgressBar(progressValue, bar, label);
            }, 10);
        };

        {
            const bar2 = document.getElementById('overal_progress_bar');
            const label2 = document.getElementById('progress_2_label');

            updateProgressBar(progress2, bar2, label2);
        }
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