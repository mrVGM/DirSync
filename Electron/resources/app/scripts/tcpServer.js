const net = require('net');

async function initServer(handler, onEnd, onConnection) {
    let connection;

    const server = net.createServer(socket => {
        if (connection) {
            setTimeout(() => {
                socket.destroy();
            }, 0);

            return;
        }
        connection = socket;
        if (onConnection) {
            onConnection();
        }

        let cnt = 0;
        let buffer = ''

        async function handle(message) {
            const { id, data } = message;
            const resp = await handler(data);
            const tmp = {
                id: id,
                data: resp
            };
            socket.write(JSON.stringify(tmp));
        }

        socket.on('data', data => {
            data = data.toString();
            for (let i = 0; i < data.length; ++i) {
                const cur = data[i];
                buffer += cur;
                if (cur === '{') {
                    ++cnt;
                }
                if (cur === '}') {
                    --cnt;
                }

                if (cnt === 0) {
                    const message = JSON.parse(buffer);
                    handle(message);
                    buffer = '';
                }
            }
        });

        socket.on('end', () => {
            onEnd();
            server.close();
        });
    });

    await new Promise(resolve => {
        server.listen(0, '0.0.0.0', () => {
            console.log(`listening on port ${server.address().port}`);
            resolve();
        });
    });

    return {
        port: server.address().port,
        close: () => {
            if (connection) {
                connection.destroy();
            }
            server.close();
            onEnd();
        }
    }
}


async function initClient(address, port, onEnd) {
    let socket = await new Promise(resolve => {
        const s = net.createConnection(port, address, () => {
            resolve(s);
        });
    });

    let cnt = 0;
    let buffer = ''

    let handlers = {};

    socket.on('data', data => {
        data = data.toString();
        for (let i = 0; i < data.length; ++i) {
            const cur = data[i];
            buffer += cur;
            if (cur === '{') {
                ++cnt;
            }
            if (cur === '}') {
                --cnt;
            }

            if (cnt === 0) {
                const message = JSON.parse(buffer);
                const { id, data } = message;

                const handler = handlers[id];
                handler(data);
                delete handlers[id];

                buffer = '';
            }
        }
    });

    socket.on('end', () => {
        onEnd();
    });

    let id = 0;
    return {
        send: async obj => {
            const req = {
                id: id++,
                data: obj
            };

            const res = await new Promise(resolve => {
                handlers[req.id] = data => {
                    resolve(data);
                };

                const message = JSON.stringify(req);
                socket.write(message);
            });

            return res;
        },
        close: () => {
            socket.destroy();
            onEnd();
        },
    };
}

exports.initServer = initServer;
exports.initClient = initClient;