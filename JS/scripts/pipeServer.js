const net = require('net');

const PIPE_NAME = "mynamedpipe";
const PIPE_PATH = "\\\\.\\pipe\\" + PIPE_NAME;

const L = console.log;
let server = net.createServer(function(stream) {
    L('Server: on connection')

    stream.on('end', function() {
        console.log("No connection!");
    });

    stream.on('data', function (c) {
        L('Server: on data:', c.toString());
    });


    const req = {
        id: 0,
        op: "noop",
        message: "Hello"
    }
    stream.write(JSON.stringify(req));
    setTimeout(() => {
        stream.write("shutdown");
    }, 5000);
});

server.on('close',function(){
    L('Server: on close');
})

server.listen(PIPE_PATH,function(){
    L('Server: on listening');
})
