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

async function initClient(address, port, onClose) {
	const client = new net.Socket();

	await new Promise((resolve, reject) => {
		client.connect(port, address, () => {
			resolve();
		});
	});

	const sendReqs = [];

	let handler;
	client.on('data', function (data) {
		handler(JSON.parse(data.toString()));
		handler = undefined;
	});

	client.on('close', function () {
		onClose();
	});

	function _send(json) {
		return new Promise((resolve, reject) => {
			handler = message => {
				resolve(message);
			};
			client.write(JSON.stringify(json));
		});
	}

	function send(json) {
		return new Promise(async (resolve, reject) => {
			if (handler) {
				sendReqs.push(async () => {
					const res = await _send(json);
					resolve(res);
				});

				return;
			}

			const res = await _send(json);
			resolve(res);

			if (sendReqs.length > 0) {
				const req = sendReqs.shift();
				req();
			}
		});
	}

	return send;
}

exports.initServer = initServer;
exports.initClient = initClient;