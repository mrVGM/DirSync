const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

function logGreen(message) {
    console.log('\x1b[32m', message,'\x1b[0m');
}

function logRed(message) {
    console.log('\x1b[31m', message,'\x1b[0m');
}

async function build() {
    const cmakePath = path.join(__dirname, 'cmake/bin/cmake.exe').normalize();
    const srcPath = path.join(__dirname, '..').normalize();
    const outPath = path.join(__dirname, 'out').normalize();

    const cmakeFound = await new Promise(resolve => {
        fs.exists(cmakePath, exists => {
            resolve(exists);
        });
    });

    if (!cmakeFound) {
        logRed(`Couldn't find "${cmakePath}"`);
        return;
    }

    await new Promise(async resolve => {
        const folderExists = await new Promise( resolve => {
            fs.exists(outPath, exists => {
                resolve(exists);
            });
        });

        if (!folderExists) {
            resolve();
        }

        fs.rm(
            outPath,
            {
                recursive: true,
                force: true
            },
            () => {
                resolve();
            }
        );
    });

    logGreen(`${outPath} cleared`);

    const cmakeCmd = `${cmakePath} -S "${srcPath}" -B "${outPath}"`;

    const generatedProjectFiles = await new Promise(resolve => {
        const tmp = exec(cmakeCmd, (err, stdout, stdin) => {
            if (err) {
                console.log(err);
                resolve(false);
                return;
            }
            resolve(true);
        });

        tmp.stdout.on('data', data => {
            console.log(data.toString());
        });
    });

    if (!generatedProjectFiles) {
        logRed('Failed generating project files!');
        return;
    }

    logGreen('Project files generated');

    const buildCmd = `${cmakePath} --build "${outPath}" --config Release`;

    const built = await new Promise(resolve => {
        const tmp = exec(buildCmd, (err, stdout, stdin) => {
            if (err) {
                console.log(err);
                resolve(false);
                return;
            }
            resolve(true);
        });

        tmp.stdout.on('data', data => {
            console.log(data.toString());
        });
    });

    if (!built) {
        logRed('Build failed');
        return;
    }

    await new Promise(resolve => {
        const mainExePath = path.join(__dirname, 'out/Main/Release/Main.exe').normalize();
        const dirsyncPath = path.join(__dirname, 'dirsync.exe').normalize();
        fs.rename(mainExePath, dirsyncPath, () => {        
            resolve();
        });
    });

    logGreen('Success');
}

build();
