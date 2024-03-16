const net = require('net');

async function initServer(onConnection) {
	const res = await new Promise((resolve, reject) => {
		const server = net.createServer(onConnection);
		server.listen(0, () => {
			resolve(server);
		});
	});
	return res;
}

async function initClient(address, port) {
	const client = new net.Socket();

	await new Promise((resolve, reject) => {
		client.connect(port, address, () => {
			resolve();
		});
	});

	return client;

	client.on('data', function (data) {
		console.log('Received: ' + data);
	});

	client.on('close', function () {
		console.log('Connection closed');
	});
}

exports.initServer = initServer;
exports.initClient = initClient;