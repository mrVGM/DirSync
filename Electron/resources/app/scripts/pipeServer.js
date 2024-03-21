const { resolve } = require('dns/promises');
const net = require('net');
const { send } = require('process');

let _resolve;
const pipePr = new Promise((resolve, reject) => {
	_resolve = resolve;
});

async function sendFunc() {
	return await pipePr;
}

async function init(sendPipeName, receivePipeName) {
	const handlers = {};

	let sendPipe = new Promise((resolve, reject) => {
		let server = net.createServer(function (stream) {
			let reqIndex = 0;
			async function send(obj) {
				obj.id = reqIndex++;
				stream.write(JSON.stringify(obj));

				const resp = await new Promise((resolve, reject) => {
				handlers[obj.id] = resp => {
						resolve(resp);
					};
				});
				delete handlers[obj.id];

				return resp;
			};

			stream.on('end', function () {
				const { ipcRenderer } = require('electron');
				ipcRenderer.send('quit');
			});
			resolve(send);
		});
		server.on('close', function () {
			const { ipcRenderer } = require('electron');
			ipcRenderer.send('quit');
		});
		server.listen(sendPipeName, function () { });
	});

	let recvPipe = new Promise((resolve, reject) => {
		let server = net.createServer(function (stream) {
			stream.on('end', function () { });

			let data = "";
			let cb = 0;
			stream.on('data', function (c) {
				data += c;

				let processed = false;
				while (!processed) {

					processed = true;
					for (let i = 0; i < data.length; ++i) {
						if (data[i] === '{') {
							++cb;
						}
						if (data[i] === '}') {
							--cb;
						}

						if (cb === 0) {
							message = data.substring(0, i + 1);
							data = data.substring(i + 1);

							const obj = JSON.parse(message);
							handlers[obj.id](obj);
							processed = false;
							break;
						}
					}
				}
			});

			resolve();
		});
		server.on('close', function () { });
		server.listen(receivePipeName, function () { });
	});

	await recvPipe;
	_resolve(await sendPipe);
}

exports.init = init;
exports.sendFunc = sendFunc;