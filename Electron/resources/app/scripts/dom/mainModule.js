let _panel;

const { render } = require('./renderHTML');

function init() {
    const panel = getPanel();

    const overalProgress = render('bar');
    overalProgress.element.style.width = '100%';
    const simpleLabel = render('simple_bar_label');
    overalProgress.tagged.label.appendChild(simpleLabel.element);

    panel.tagged.overal_progress.appendChild(overalProgress.element);

    function updateOveralProgress(progress) {
        overalProgress.tagged.bar.style.width = `${100 * progress[0] / progress[1]}%`;
        simpleLabel.tagged.label.innerHTML = `[${progress[0]}/${progress[1]}]`;
    }

    function log(str) {
        const infoPanel = panel.tagged.info_panel;

        const div = document.createElement("div");
        div.innerHTML = str;
        infoPanel.appendChild(div);
    }

    function createProgressBar() {
        const bar = render('bar');
        panel.tagged.bar_space.appendChild(bar.element);

        const label = render('download_file_label');
        bar.tagged.label.appendChild(label.element);

        function formatBytes(cnt) {
            let suf = ['B', 'KB', 'MB', 'GB'];

            let index = 0;
            for (let i = 0; i < suf.length; ++i) {
                index = i;
                if (cnt < 1024) {
                    break;
                }
                cnt /= 1024;
            }

            return `${Math.round(cnt * 100) / 100}${suf[index]}`;
        }

        function setProgress(name, progress, speed) {
            bar.tagged.bar.style.width = `${100 * progress[0] / progress[1]}%`;

            label.tagged.name.innerHTML = name;
            label.tagged.file_size.innerHTML = `[${formatBytes(progress[0])}/${formatBytes(progress[1])}]`;
            label.tagged.speed.innerHTML = `${formatBytes(speed)}/s`;
        }

        function remove() {
            bar.element.remove();
        }

        return {
            setProgress,
            remove
        }
    }

    async function initButtons() {
        const dirModule = await app.modules.dir;
        const netModule = await app.modules.net;

        let runServerButtonActive = true;
        panel.tagged.run_server.addEventListener('click', async () => {
            const unlockModal = await app.locks.modal();
            if (!unlockModal) {
                return;
            }
            unlockModal();


            if (!runServerButtonActive) {
                return;
            }

            if (!dirModule.interface.isValidDirChosen()) {
                const notification = await app.modules.notification;
                notification.interface.show('Please choose a valid directory!');
                return;
            }

            panel.tagged.run_server.style.display = 'none';

            await app.locks.dir();
            await app.locks.running();

            runServerButtonActive = false;

            const { hashFiles, registerFiles, stop } = require('../backend');
            const { getFileList } = require('../files');

            const dir = dirModule.interface.getDir();
            const fileList = await getFileList(dir);
            
            const tracker = {
                progress: p => {
                    updateOveralProgress(p)
                },
                finished: () => {}
            };
            const hashed = await hashFiles(dir, fileList, tracker);
            
            await registerFiles(dir, fileList);

            const { startPeerServer, getTCPServer } = require('../peers');
            const fileServer = await startPeerServer(netModule.interface.getPCName);
            
            log('Server running!');

            const { setHandler, send } = await getTCPServer();

            setHandler('records', json => {
                send(hashed);
            });

            setHandler('fileServer', json => {
                send(fileServer);
            });

            setHandler('stop', async json => {
                await stop(json.fileId);
                send({ res: 'bucket_stopped'});
            });

            const bars = {};

            setHandler('progress', async json => {
                updateOveralProgress(json.overal);

                if (json.overal[0] >= json.overal[1]) {
                    send({ res: 'stop' });
                    for (let k in bars) {
                        bars[k].remove();
                        delete bars[k];
                    }
                    return;
                }

                const toDelete = [];
                for (let k in bars) {
                    if (!json.individual[k]) {
                        toDelete.push(k);
                    }
                }
                toDelete.forEach(k => {
                    bars[k].remove();
                    delete bars[k];
                });

                for (let k in json.individual) {
                    let bar = bars[k];
                    if (!bar) {
                        bar = createProgressBar();
                        bars[k] = bar;
                    }

                    const p = json.individual[k];
                    bar.setProgress(fileList[p.id].path, p.progress, p.speed);
                }

                send({ res: 'more_progress'});
            });
        });

        let downloadFilesButtonActive = true;
        panel.tagged.download_files.addEventListener('click', async () => {
            const unlockModal = await app.locks.modal();
            if (!unlockModal) {
                return;
            }
            unlockModal();


            if (!downloadFilesButtonActive) {
                return;
            }

            const peer = netModule.interface.getPeer();
            if (!peer) {
                const notification = await app.modules.notification;
                notification.interface.show('Find a server, please!');
                return;
            }

            if (!dirModule.interface.isValidDirChosen()) {
                const notification = await app.modules.notification;
                notification.interface.show('Please choose a valid directory!');
                return;
            }

            panel.tagged.download_files.style.display = 'none';

            await app.locks.dir();
            await app.locks.running();

            const { hashFiles, downloadFile, stop } = require('../backend');

            downloadFilesButtonActive = false;

            const peerAddr = peer.addr;
            const { initClient } = require('../tcpServer');
            const tcpClient = await initClient(peerAddr.ip, peerAddr.port);

            const fileServer = await tcpClient({ req: 'fileServer' });
            
            let fileList = await tcpClient({ req: 'records' });

            const path = require('path');
            const rootDir = dirModule.interface.getDir();

            const tracker = {
                progress: p => {
                    updateOveralProgress(p)
                },
                finished: () => { }
            };
            const myFiles = await hashFiles(rootDir, fileList, tracker);

            console.log(fileList);
            console.log(myFiles);

            fileList = fileList.filter((f, index) => {
                return myFiles[index].hash !== f.hash;
            });

            let numSlots = 4;

            let slotRequests = [];
            function releaseSlot() {
                numSlots++;

                for (let id in slotRequests) {
                    if (numSlots === 0) {
                        return;
                    }
                    const cur = slotRequests[id];
                    if (!cur) {
                        continue;
                    }

                    cur();
                }
            }

            let reqId = 0;
            function takeSlot() {
                const pr = new Promise((resolve, reject) => {
                    const id = reqId++;
                    function tryTakeSlot() {
                        if (numSlots > 0) {
                            --numSlots;
                            slotRequests[id] = undefined;
                            resolve();
                            return;
                        }

                        slotRequests[id] = tryTakeSlot;
                    }

                    tryTakeSlot();
                });
                return pr;
            }


            const { writeFile, createFolders } = require('../files');

            const prog = [0, fileList.length];

            const filesProgress = {};
            async function updatingServerWithProgress() {
                const resp = await tcpClient({
                    req: 'progress',
                    overal: prog,
                    individual: filesProgress
                });

                if (resp.res === 'more_progress') {
                    setTimeout(updatingServerWithProgress, 100);
                }
            }
            updatingServerWithProgress();

            updateOveralProgress(prog);
            const downloads = fileList.map(async f => {
                const filePath = path.join(rootDir, f.path);
                await createFolders(rootDir, f.path);

                if (f.fileSize === 0) {
                    await writeFile(rootDir, f.path, []);
                    return;
                }

                await takeSlot();

                log(`Starting to download ${f.path}`);
                await new Promise((resolve, reject) => {
                    const bar = createProgressBar();

                    const maxSamples = 10;
                    let samples = [];

                    const tracker = {
                        finished: async () => {
                            await tcpClient({ req: 'stop', fileId: f.id });
                            releaseSlot();
                            bar.remove();
                            delete filesProgress[f.id];

                            ++prog[0];
                            updateOveralProgress(prog);

                            log(`Finished downloading ${f.path}`);
                            resolve();
                        },
                        progress: p => {
                            let speed = 0;
                            const now = Date.now();
                            samples.push({
                                bytes: p.progress[0],
                                time: now
                            });

                            while (samples.length > maxSamples) {
                                samples.shift();
                            }
                            if (samples.length >= 2) {
                                const first = samples[0];
                                const last = samples[samples.length - 1];
                                speed = 1000 * (last.bytes - first.bytes) / (last.time - first.time);
                            }

                            bar.setProgress(f.path, p.progress, speed);
                            filesProgress[f.id] = {
                                id: f.id,
                                progress: p.progress,
                                speed: speed
                            };
                        }
                    };

                    downloadFile(
                        f.id,
                        peerAddr.ip,
                        fileServer.port,
                        f.fileSize,
                        filePath,
                        tracker
                    );
                });
            });

            for (let i = 0; i < downloads.length; ++i) {
                await downloads[i];
            }

            log('Download Done');
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