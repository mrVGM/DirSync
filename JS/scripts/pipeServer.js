const net = require('net');

const PIPE_NAME = "mynamedpipe";
const PIPE_PATH = "\\\\.\\pipe\\" + PIPE_NAME;

const L = console.log;
let server = net.createServer(function(stream) {
	L('Server: on connection')

	const handlers = {};

	document.pipeSend = async obj => {
		stream.write(JSON.stringify(obj));
		
		const resp = await new Promise((resolve, reject) => {			
			handlers[obj.id] = resp => {
				resolve(resp);
			};
		});
		delete handlers[obj.id];
		
		return resp;
	};

	stream.on('end', function() {
		console.log("No connection!");
		document.pipeSend = undefined;
	});

	stream.on('data', function (c) {
		L('Server: on data:', c.toString());
		
		const obj = JSON.parse(c);
		handlers[obj.id](obj);
	});
});

server.on('close',function(){
	L('Server: on close');
});

server.listen(PIPE_PATH,function(){
	L('Server: on listening');
});
