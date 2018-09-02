var flags = require('flags');
flags.defineInteger('port', 8080, 'listening port');
flags.defineString(
    'stat_service', 'localhost:8081', 'host serving StatService GRPC service.');
flags.defineString(
    'service_proto', 'service.proto',
    'path to proto file containing StatService GRPC service definition.');
flags.parse();

var express = require('express');
var app = express();

app.use((request, response, next) => {
  console.log(request.method + ': ' + request.originalUrl);
  console.log('headers: ' + JSON.stringify(request.headers));
  next();
});

var api = require('./api.js');
api.registerStatServiceHttpHandlers(
    app, '/api/', flags.get('service_proto'), flags.get('stat_service'));

app.listen(flags.get('port'), (err) => {
  if (err) console.log('error', err);
  else console.log('listening on port ' + flags.get('port'));
});
