const net = require('net');

const PIPE_NAME_SEND = "mynamedpipe_js_to_cpp";
const PIPE_NAME_RECV = "mynamedpipe_cpp_to_js";
const PIPE_PATH_SEND = "\\\\.\\pipe\\" + PIPE_NAME_SEND;
const PIPE_PATH_RECV = "\\\\.\\pipe\\" + PIPE_NAME_RECV;

async function pipe() {
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

			stream.on('end', function () { });
			resolve(send);
		});
		server.on('close', function () { });
		server.listen(PIPE_PATH_SEND, function () { });
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
		server.listen(PIPE_PATH_RECV, function () { });
	});

	await recvPipe;
	return await sendPipe;
}

exports.getSendFunc = pipe;