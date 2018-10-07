var flags = require('flags');
flags.defineInteger('port', 8080, 'listening port');

flags.defineString(
    'stat_service', 'localhost:8081', 'host serving StatService GRPC service.');
flags.defineString(
    'service_proto', 'service.proto',
    'path to proto file containing StatService GRPC service definition.');

flags.defineString(
    'domain', 'http://wnste.in', 'serving domain for auth callback');
flags.defineString('gapps_client_id', '', '');
flags.defineString('gapps_client_secret', '', '');

flags.defineString('session_secret', '', '');

flags.parse();

var express = require('express');
var app = express();

app.use((request, response, next) => {
  console.log(request.method + ': ' + request.originalUrl);
  console.log('headers: ' + JSON.stringify(request.headers));
  next();
});

var auth = require('./auth.js');
auth.registerAuthHttpHandlers(app, {
  clientID: flags.get('gapps_client_id'),
  clientSecret: flags.get('gapps_client_secret'),
  callbackDomain: flags.get('domain') + ':' + flags.get('port'),
  urlPrefix: '/auth/google/',
  sessionSecret: flags.get('session_secret'),
});
auth.requireLogin(app, '/');

var api = require('./api.js');
var statService = api.newStatService({
  serviceProtoPath: flags.get('service_proto'),
  serverHostPort: flags.get('stat_service'),
});
api.registerStatServiceHttpHandlers(app, {
  urlPrefix: '/api/',
  statService: statService,
});

var path = require('path');
app.use(express.static(path.join(__dirname, 'public')))

app.listen(flags.get('port'), (err) => {
  if (err) console.log('error', err);
  else console.log('listening on port ' + flags.get('port'));
});
