const { app, BrowserWindow, ipcMain } = require('electron')

ipcMain.on('get_argv', (event, data) => {
    event.reply('argv', process.argv);
});

function createWindow() {
    const win = new BrowserWindow({
        width: 800,
        height: 600,
        webPreferences: {
            contextIsolation: false,
            nodeIntegration: true
        }
    });


    win.loadFile('index.html');
    win.webContents.openDevTools();

    win.on('ready', () => {
        console.log('ready');
    });

    win.on("close", () => {
        console.log("window closed!");
    });
}

app.whenReady().then(() => {
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
})

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
