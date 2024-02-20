const net = require('net');

const PIPE_NAME_SEND = "mynamedpipe_js_to_cpp";
const PIPE_NAME_RECV = "mynamedpipe_cpp_to_js";
const PIPE_PATH_SEND = "\\\\.\\pipe\\" + PIPE_NAME_SEND;
const PIPE_PATH_RECV = "\\\\.\\pipe\\" + PIPE_NAME_RECV;

async function pipe() {
	const handlers = {};

	let sendPipe = new Promise((resolve, reject) => {
		let server = net.createServer(function (stream) {
			async function send(obj) {
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

			stream.on('data', function (c) {
				const obj = JSON.parse(c);
				handlers[obj.id](obj);
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