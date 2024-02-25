const { rejects } = require('assert');
const { resolve } = require('dns');
const http = require('http');
const path = require('path');

async function handleRequest(req, res) {
    console.log(req.url);
    if (req.url === '/records') {
        const fileList = await myFileList;
        res.write(JSON.stringify(fileList));
        res.end();
        return;
    }
}

async function checkLocalFiles() {
    const { hashFiles, writeFile } = require('./files');

    let remoteRecs = await getRecs();
    const rootDir = document.prefs.dstDir;

    const localRecs = remoteRecs.map(x => {
        return {
            path: x.path
        };
    });

    await hashFiles(rootDir, localRecs);

    let toUpdate = [];
    for (let i = 0; i < remoteRecs.length; ++i) {
        if (remoteRecs[i].hash !== localRecs[i].hash) {
            toUpdate.push(i);
        }
    }
    progress2[1] = toUpdate.length;

    const { fetchFile } = require('./udpclient');

    for (let i = 0; i < toUpdate.length; ++i) {
        const x = toUpdate[i];

        await fetchFile(x);
        ++progress2[0];

        console.log(`file ${x} downloaded`);
    }

    console.log('Done');
}

exports.startServer = async () => {
    return new Promise((resolve, reject) => {
        http.createServer(handleRequest).listen(8080, () => {
            console.log('Server listening at port 8080');
            resolve();
        }); //the server object listens on port 8080
    });
};

async function getRecs(){
    let recs = await new Promise((resolve, reject) => {
        http.get(`http://${document.prefs.serverIP}:8080/records`, res => {
            resolve(res);
        });
    });
    
    recs = await new Promise((resolve, reject) => {
        let data = '';
        recs.on('data', chunk => {
            data += chunk;
        });
        recs.on('end', () => {
            resolve(data);
        });
    });

    recs = JSON.parse(recs);

    return recs;
};

exports.getRecs = getRecs;
exports.checkLocalFiles = checkLocalFiles;