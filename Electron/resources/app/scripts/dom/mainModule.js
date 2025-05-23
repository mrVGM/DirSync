let _panel;

const { render } = require('./renderHTML');

function init() {
    const panel = getPanel();

    const overalProgress = render('bar');
    overalProgress.element.style.width = '100%';
    const simpleLabel = render('simple_bar_label');
    overalProgress.tagged.label.appendChild(simpleLabel.element);

    panel.tagged.overal_progress.appendChild(overalProgress.element);

    overalProgress.tagged.bar.style.width = `0%`;
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

    function createProgressBar() {
        const bar = render('bar');
        panel.tagged.bar_space.appendChild(bar.element);

        const label = render('download_file_label');
        bar.tagged.label.appendChild(label.element);

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

        panel.tagged.run_server.addEventListener('click', async () => {
            const unlockModal = await app.locks.modal();
            if (!unlockModal) {
                return;
            }
            unlockModal();

            const isValidDir = await dirModule.interface.isValidDirChosen();

            if (!isValidDir) {
                const notification = await app.modules.notification;
                notification.interface.show('Please choose a valid directory!');
                return;
            }

            const initialDisplay = panel.tagged.run_server.style.display;
            panel.tagged.run_server.style.display = 'none';

            const unlockDir = await app.locks.dir();
            const unlockRunning = await app.locks.running();

            const { hashFiles, registerFiles, stop, shutdownFileServer } = require('../backend');
            const { getFileList } = require('../files');

            const dir = await dirModule.interface.getDir();
            const fileList = await getFileList(dir);
            
            const tracker = {
                progress: p => {
                    updateOveralProgress(p)
                },
                finished: () => {}
            };
            const hashed = await hashFiles(dir, fileList, tracker);
            console.log(hashed);
            
            await registerFiles(dir, fileList);

            const { startPeerServer } = require('../peers');
            const { fileServer, tcpServer } = await startPeerServer(
                netModule.interface.getPCName,
                async json => {
                    const { req } = json;

                    if (req === 'records') {
                        if (!json.slice) {
                            return { numFiles: hashed.length };
                        }

                        return hashed.slice(json.slice[0], json.slice[1]);
                    }

                    if (req === 'fileServer') {
                        return fileServer;
                    }

                    if (req === 'stop') {
                        await stop(json.fileId);
                        return { res: 'bucket_stopped' };
                    };

                    if (req === 'progress') {
                        updateOveralProgress(json.overal);

                        if (json.overal[0] >= json.overal[1]) {
                            for (let k in bars) {
                                bars[k].remove();
                                delete bars[k];
                            }

                            return ({ res: 'stop' });
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

                        return { res: 'more_progress' };
                    }
                },
                async () => {
                    await shutdownFileServer();
                    unlockDir();
                    unlockRunning();
                    panel.tagged.bar_space.innerHTML = '';
                    log('Server shut down!');

                    panel.tagged.run_server.style.display = initialDisplay;
                    panel.tagged.cancel.style.display = 'none';
                }
            );
            const bars = {};

            const { close } = tcpServer;
            panel.tagged.cancel.style.display = '';

            function cancel() {
                panel.tagged.cancel.removeEventListener('click', cancel);

                close();
            }
            panel.tagged.cancel.addEventListener('click', cancel);

            log('Server running!');
        });

        panel.tagged.download_files.addEventListener('click', async () => {
            const unlockModal = await app.locks.modal();
            if (!unlockModal) {
                return;
            }
            unlockModal();

            const peer = netModule.interface.getPeer();
            if (!peer) {
                const notification = await app.modules.notification;
                notification.interface.show('Find a server, please!');
                return;
            }

            const isValidDir = await dirModule.interface.isValidDirChosen();
            if (!isValidDir) {
                const notification = await app.modules.notification;
                notification.interface.show('Please choose a valid directory!');
                return;
            }

            let initialDisplay = panel.tagged.download_files.style.display;
            panel.tagged.download_files.style.display = 'none';

            const unlockDir = await app.locks.dir();
            const unlockRunning = await app.locks.running();

            const { hashFiles, downloadFile, shutdownDownloaders } = require('../backend');

            downloadFilesButtonActive = false;

            const peerAddr = peer.addr;
            const { initClient } = require('../tcpServer');

            let tcpConnectionActive = true;
            const { send: tcpClient, close } = await initClient(peerAddr.ip, peerAddr.port, async () => {
                const netModule = await app.modules.net;
                netModule.interface.reset();

                tcpConnectionActive = false;

                for (let id in slotRequests) {
                    const cur = slotRequests[id];
                    if (!cur) {
                        continue;
                    }

                    cur();
                }

                await shutdownDownloaders();

                log('Downloaders stopped!');

                panel.tagged.bar_space.innerHTML = '';
                unlockDir();
                unlockRunning();

                panel.tagged.download_files.style.display = initialDisplay;
                panel.tagged.cancel.style.display = 'none';
            });

            panel.tagged.cancel.style.display = '';

            function cancel() {
                panel.tagged.cancel.removeEventListener('click', cancel);

                close();
            }
            panel.tagged.cancel.addEventListener('click', cancel);

            const fileServer = await tcpClient({ req: 'fileServer' });

            async function fetchFileList() {
                const res = await tcpClient({ req: 'records' });
                let numFiles = res.numFiles;

                let fileList = [];

                const step = 10;
                for (let i = 0; i < numFiles; i += step) {
                    const tmp = await tcpClient({
                        req: 'records',
                        slice: [i, Math.min(i + step, numFiles)]
                    });

                    fileList = fileList.concat(tmp);
                }

                return fileList;
            }

            let fileList = await fetchFileList();
            console.log(fileList);


            const path = require('path');
            const rootDir = await dirModule.interface.getDir();

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
                        if (!tcpConnectionActive) {
                            reject();
                        }

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

                try {
                    await takeSlot();
                }
                catch {
                    return;
                }

                log(`Starting to download ${f.path}`);
                await new Promise((resolve, reject) => {
                    const bar = createProgressBar();

                    const maxSamples = 10;
                    let samples = [];

                    const startTime = Date.now();

                    const tracker = {
                        finished: async () => {
                            const totalTime = (Date.now() - startTime) / 1000;
                            await tcpClient({ req: 'stop', fileId: f.id });
                            releaseSlot();
                            bar.remove();
                            delete filesProgress[f.id];

                            ++prog[0];
                            updateOveralProgress(prog);


                            log(`Finished downloading ${f.path} with ${formatBytes(f.fileSize / totalTime)}/s`);
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