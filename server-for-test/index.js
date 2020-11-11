'use strict';

const http = require('http');

const server = http.createServer((req, res) => {
  console.log(req.url);
  res.end('ok\n');
});

server.listen(7001);
