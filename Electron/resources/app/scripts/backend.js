async function getSendFunc() {
    const { sendFunc } = require('./pipeServer');
    let send = await sendFunc();

    return send;
}

async function hashFiles(rootDir, fileList, progress) {
    const send = await getSendFunc();

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
            ++progress[0];
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

async function registerFiles(rootDir, fileList) {
    const send = await getSendFunc();

    let reqs = fileList.map((x, index) => {
        const path = require('path');
        const req = {
            op: 'set_file_id',
            file: path.join(rootDir, x.path),
            file_id: index
        };

        const pr = send(req);
        return pr;
    });

    for (let i = 0; i < reqs.length; ++i) {
        await reqs[i];
    }
}

exports.hashFiles = hashFiles;
exports.registerFiles = registerFiles;
