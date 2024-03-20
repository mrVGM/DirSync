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


	let handler;
	client.on('data', function (data) {
		handler(JSON.parse(data.toString()));
	});

	client.on('close', function () {
		console.log('Connection closed');
	});

	return json => new Promise((resolve, reject) => {
		handler = message => {
			handler = undefined;
			resolve(message);
		};
		client.write(JSON.stringify(json));
	});
}

exports.initServer = initServer;
exports.initClient = initClient;