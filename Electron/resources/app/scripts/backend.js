async function getSendFunc() {
    const { sendFunc } = require('./pipeServer');
    let send = await sendFunc();

    return send;
}

async function hashFiles(rootDir, fileList, tracker) {
    const send = await getSendFunc();

    const fs = require('fs');
    const path = require('path');

    const prog = [0, fileList.length];

    const tmp = fileList.map(async (x, index) => {
        const fullPath = path.join(rootDir, x.path);

        const exist = await new Promise((resolve, reject) => {
            fs.exists(fullPath, b => {
                resolve(b);
            });
        });

        if (!exist) {
            ++prog[0];
            tracker.progress(prog);

            return {
                path: x,
                hash: '',
                fileSize: 0
            };
        }

        const req = {
            op: 'hash',
            file: fullPath
        };

        const pr = send(req);
        const res = await pr;
        res.path = x.path;
        res.id = index;
        ++prog[0];
        tracker.progress(prog);

        return res;
    });

    const res = [];
    for (let i = 0; i < tmp.length; ++i) {
        const resp = await tmp[i];
        res.push(resp);
    }

    return res;
}

async function registerFiles(rootDir, fileList) {
    const send = await getSendFunc();

    let reqs = fileList.map((x, index) => {
        const path = require('path');
        const req = {
            op: 'set_file_id',
            file: path.join(rootDir, x.path),
            file_id: index,
            max_loaded_chunks: document.sysConfig.maxFileChunksLoaded,
        };

        const pr = send(req);
        return pr;
    });

    for (let i = 0; i < reqs.length; ++i) {
        await reqs[i];
    }
}

async function runUDPServer() {
    const send = await getSendFunc();
    const resp = await send({
        op: 'run_udp_server'
    });

    return resp;
}

async function checkFileProgress(fileId) {
    const send = await getSendFunc();

    const res = await send({
        op: 'get_download_progress',
        file_id: fileId,
    });

    return res;
}

async function downloadFile(fileId, serverAddr, serverPort, size, path, tracker) {
    const send = await getSendFunc();
    
    send({
        op: 'download_file',
        file_id: fileId,
        ip_addr: serverAddr,
        port: serverPort,
        file_size: size,
        path: path,
        num_workers: document.sysConfig.downloadChunks,
        download_window: document.sysConfig.maxFileChunksLoaded,
        ping_delay: document.sysConfig.pingTime
    }).then(() => {
        finished = true;
        tracker.finished();
    });

    let finished = false;
    async function checkProgress() {
        if (finished) {
            return;
        }

        let prog = await checkFileProgress(fileId);
        tracker.progress(prog);
        setTimeout(checkProgress, 100);
    }

    checkProgress();
}

async function stop(fileId) {
    const send = await getSendFunc();

    const res = send({
        op: 'stop',
        file_id: fileId,
    });

    return res;
}

async function shutdownFileServer(fileId) {
    const send = await getSendFunc();

    const res = send({
        op: 'shutdown_udp_server'
    });

    return res;
}

async function shutdownDownloaders(fileId) {
    const send = await getSendFunc();

    const res = send({
        op: 'shutdown_downloaders'
    });

    return res;
}

exports.hashFiles = hashFiles;
exports.registerFiles = registerFiles;
exports.runUDPServer = runUDPServer;
exports.downloadFile = downloadFile;
exports.stop = stop;
exports.shutdownFileServer = shutdownFileServer;
exports.shutdownDownloaders = shutdownDownloaders;
