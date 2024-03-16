const { getFileList } = require('./files');

const path = require('path');
const fs = require('fs');

async function init() {
    const htmlDir = path.join(__dirname, '../html');
    const files = await getFileList(htmlDir);

    const tmp = files.map(x => {
        const name = x.path.substring(0, x.path.length - path.extname(x.path).length);

        return new Promise((resolve, reject) => {
            fs.readFile(path.join(htmlDir, x.path), (err, data) => {
                resolve({
                    name: name,
                    contents: data.toString()
                });
            });
        });
    });

    app.html = {};

    for (let i = 0; i < tmp.length; ++i) {
        const cur = await tmp[i];
        app.html[cur.name] = cur;
    }
}

exports.init = init;