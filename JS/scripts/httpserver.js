const { rejects } = require('assert');
const { resolve } = require('dns');
const http = require('http');
const path = require('path');

async function handleRequest(req, res) {
    console.log(req.url);
    if (req.url === '/records') {
        const { getRecords } = require('./files');
        const recs = await getRecords(document.prefs.srcDir);
        res.write(JSON.stringify(recs));
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
    document.overalProgress = [0, toUpdate.length];

    const { fetchFile } = require('./udpclient');

    for (let i = 0; i < toUpdate.length; ++i) {
        const x = toUpdate[i];

        await fetchFile(x);
        document.curFile = undefined;
        ++document.overalProgress[0];

        console.log(`file ${x} downloaded`);
    }

    console.log('Done');
}

exports.startServer = () => {
    http.createServer(handleRequest).listen(8080, () => {
        console.log('Server listening at port 8080');
    }); //the server object listens on port 8080
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