const { ALL } = require('dns');
const { resolve } = require('dns/promises');
const fs = require('fs');
const path = require('path');

async function enumFiles(dir) {
    const pr = new Promise((resolve, reject) => {
        fs.readdir(dir, (err, files) => {
            resolve(files);
        });
    });

    let files = await pr;

    files = files.map(file => {
        let fullPath = path.join(dir, file);

        return new Promise((resolve, reject) => {
            fs.lstat(fullPath, (err, stats) => {
                resolve({
                    path: fullPath,
                    stats
                });
            });
        });
    });

    for (let i = 0; i < files.length; ++i) {
        const tmp = await files[i];
        files[i] = {
            path: tmp.path,
            isDirectory: tmp.stats.isDirectory()
        };
    }
    
    return files;
}

async function getFiles(dir) {
    async function getFilesInDir(dir) {
        const allFiles = [];
        const files = await enumFiles(dir);

        const dirs = [];

        files.forEach(file => {
            if (file.isDirectory) {
                dirs.push(file.path);
            }
            else {
                allFiles.push(file.path);
            }
        });
        
        const subFiles = dirs.map(d => getFilesInDir(d));
        for (let i = 0; i < subFiles.length; ++i) {
            const tmp = await subFiles[i];
            tmp.forEach(file => {
                allFiles.push(file);
            });
        }

        return allFiles;
    }

    const fileList = await getFilesInDir(dir);

    const res = [];
    
    fileList.forEach(file => {
        res.push({
            path: path.relative(dir, file)
        });
    });

    return res;
}

async function readSingleFile(file) {
    const pr = new Promise((resolve, reject) => {
        const fullData = [];

        const reader = fs.createReadStream(file);
        reader.on('data', buff => {
            fullData.push(buff);
        });

        reader.on('end', () => {
            resolve(fullData);
        });
    });

    return pr;
}
async function hashSingleFile(file, fileId, dataCacheObject) {
    const { createHash } = require('crypto');

    const fileExist = await new Promise((resolve, reject) => {
        fs.exists(file, b => {
            resolve(b);
        });
    });

    if (!fileExist) {
        return {
            hash: '',
            numPackets: 0
        };
    }

    const data = await readSingleFile(file);
    if (dataCacheObject) {
        dataCacheObject[fileId] = data;
    }

    let fullSize = 0;
    data.forEach(x => {
        fullSize += x.length;
    });

    const hash = createHash("sha256");

    for (let i = 0; i < data.length; ++i) {
        hash.update(data[i]);
    }
    const str = hash.digest("hex");

    return {
        hash: str,
        numPackets: Math.ceil(fullSize / 1024)
    };
}

async function hashFiles(rootDir, fileRecords, dataCacheObject) {
    const { createHash } = require('crypto');

    const filesTmp = [];

    for (let i = 0; i < fileRecords.length; ++i) {
        const record = fileRecords[i];
        filesTmp.push({
            record: record,
            settings: hashSingleFile(path.join(rootDir, record.path), i, dataCacheObject),
        });
    }

    for (let i = 0; i < filesTmp.length; ++i) {
        const tmp = await filesTmp[i].settings;
        filesTmp[i].record.hash = tmp.hash;
        filesTmp[i].record.numPackets = tmp.numPackets;
    }

    return fileRecords;
}

async function getRecords(dir) {
    console.log(`reading records of ${dir}`);
    let logTime = false;
    if (!document.cachedRecords) {
        logTime = true;
        document.cachedRecords = new Promise(async (resolve, reject) => {
            const tmp = await getFiles(dir);
            document.fileDataCache = {};
            resolve(await hashFiles(dir, tmp, document.fileDataCache));
        });
    }

    const preRead = Date.now();
    const res = await document.cachedRecords;
    const postRead = Date.now();

    if (logTime) {
        console.log(`records read in ${(postRead - preRead) / 1000}`);
    }
    return res;
}

async function readPrefs() {
    const prefsPath = path.join(__dirname, '../configs/prefs.json');

    const pr = new Promise((resolve, reject) => {
        fs.readFile(prefsPath, (err, data) => {
            resolve(data);
        });
    });

    const str = (await pr).toString();
    const config = JSON.parse(str);

    return config;
}

function flushPrefs(prefs) {
    const prefsPath = path.join(__dirname, '../configs/prefs.json');
    const dataToWrite = JSON.stringify(prefs);
    fs.writeFile(prefsPath, dataToWrite, err => {
        console.log('Flushed');
    });
}

async function writeFile(rootDir, relativePath, chunks) {
    const filePath = path.join(rootDir, relativePath);

    function* getNames() {
        let dirName = relativePath;
        let baseName = path.basename(relativePath);
        yield baseName;

        while (dirName.length > baseName.length) {
            dirName = path.dirname(dirName);
            baseName = path.basename(dirName);
            yield baseName;
        }
    }

    const folders = [];
    {
        const it = getNames();
        let cur = it.next();
        cur = it.next();

        while (!cur.done) {
            folders.unshift(cur.value);
            cur = it.next();
        }
    }

    async function createFolders() {
        let curPath = rootDir;
        for (let i = 0; i < folders.length; ++i) {
            curPath = path.join(curPath, folders[i]);

            const exists = await new Promise((resolve, reject) => {
                fs.exists(curPath, b => {
                    resolve(b);
                });
            });

            if (!exists) {
                await new Promise((resolve, reject) => {
                    fs.mkdir(curPath, () => {
                        resolve();
                    });
                });
            }
        }
    }

    await createFolders();

    const writer = fs.createWriteStream(filePath);
    for (let i = 0; i < chunks.length; ++i) {
        await new Promise((resolve, reject) => {
            writer.write(chunks[i], err => {
                resolve();
            });
        });
    }
    writer.close();
}

exports.getFileList = getFiles;
exports.hashFiles = hashFiles;
exports.getRecords = getRecords;
exports.readPrefs = readPrefs;
exports.flushPrefs = flushPrefs;
exports.writeFile = writeFile;