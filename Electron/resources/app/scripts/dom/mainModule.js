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

    async function initButtons() {
        const dirModule = await app.modules.dir;
        const netModule = await app.modules.net;

        let runServerButtonActive = true;
        panel.tagged.run_server.addEventListener('click', async () => {
            if (!runServerButtonActive) {
                return;
            }

            if (!dirModule.interface.isValidDirChosen()) {
                alert('Please choose a valid directory!');
                return;
            }

            runServerButtonActive = false;

            const { hashFiles, registerFiles } = require('../backend');
            const { getFileList } = require('../files');

            const dir = dirModule.interface.getDir();
            const fileList = await getFileList(dir);
            
            const progress = [0, 1];
            const hashed = await hashFiles(dir, fileList, progress);
            
            await registerFiles(dir, fileList);

            const { startPeerServer, getTCPServer } = require('../peers');
            await startPeerServer(netModule.interface.getPCName);
            log('Peer Server running!');

            const { setHandler, send } = await getTCPServer();

            setHandler('records', json => {
                send(hashed);
            });
        });

        let downloadFilesButtonActive = true;
        panel.tagged.download_files.addEventListener('click', async () => {
            if (!downloadFilesButtonActive) {
                return;
            }

            const peer = netModule.interface.getPeer();
            if (!peer) {
                alert('Find a peer, please!');
                return;
            }

            if (!dirModule.interface.isValidDirChosen()) {
                alert('Please choose a valid directory!');
                return;
            }

            downloadFilesButtonActive = false;

            const peerAddr = peer.addr;
            const { initClient } = require('../tcpServer');
            const tcpClient = await initClient(peerAddr.ip, peerAddr.port);

            let fileList = await tcpClient({ req: 'records' });

            let numSlots = 5;

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

            const path = require('path');

            const { downloadFile, stop } = require('../backend');
            const { writeFile, createFolders } = require('../files');

            const rootDir = path.join(dirModule.interface.getDir(), '../dst');

            const downloads = fileList.map(async f => {
                const filePath = path.join(rootDir, f.path);
                await createFolders(rootDir, f.path);

                if (f.fileSize === 0) {
                    await writeFile(rootDir, f.path, []);
                    return;
                }

                await takeSlot();
                await new Promise((resolve, reject) => {
                    const bar = render('bar');
                    panel.tagged.bar_space.appendChild(bar.element);

                    const tracker = {
                        finished: () => {
                            stop(f.id),
                            releaseSlot();
                            bar.element.remove();
                            resolve();
                        },
                        progress: p => {
                            bar.tagged.bar.style.width = `${100 * p.progress[0] / p.progress[1]}%`;
                            bar.tagged.label.innerHTML = `${f.path} [${p.progress[0]}/${p.progress[1]}]`;
                        }
                    };

                    downloadFile(
                        f.id,
                        peerAddr.ip,
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