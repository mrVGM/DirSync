const UDP = require('dgram');
const port = 2222;

async function initPeerServer(onMessage) {
    const server = UDP.createSocket('udp4');

    server.on('message', onMessage);
    server.bind(port);

    await new Promise((resolve, reject) => {
        server.on('listening', () => {
            // Server address it�s using to listen

            const address = server.address();
            console.log('Listining to ', 'Address: ', address.address, 'Port: ', address.port);
            resolve();
        });
    });

    return server;
}

function initPeerClient(onMessage) {
    const client = UDP.createSocket('udp4');

    client.on('message', onMessage);

    function send(data, address) {
        client.send(data, port, address, (err, num) => { });
    };

    return {
        send,
        client
    };

}

exports.initPeerServer = initPeerServer;
exports.initPeerClient = initPeerClient;