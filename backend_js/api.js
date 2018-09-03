var grpc = require('grpc');
var protoLoader = require('@grpc/proto-loader');

var rpcResultToResponseBody = function(response, next) {
  return (err, result) => {
    if (err)
      next(err);
    else
      response.send(result);
  };
};

var readStats = function(service, request, response, next) {
  service.readStats(
      {user_id: request.user.googleId},
      rpcResultToResponseBody(response, next));
};

var readEvents = function(service, request, response, next) {
  var start = parseInt(request.query.start) || 0;
  var length = parseInt(request.query.length) || Number.MAX_SAFE_INTEGER;
  service.readEvents(
      {
        user_id: request.user.googleId,
        stat_id: request.query.stat_id,
        start_time: {seconds: start},
        duration: {seconds: length},
      },
      rpcResultToResponseBody(response, next));
};

var defineStat = function(service, request, response, next) {
  service.defineStat(
      {
        user_id: request.user.googleId,
        stat: {display_name: request.query.display_name},
      },
      rpcResultToResponseBody(response, next));
};

var deleteStat = function(service, request, response, next) {
  service.deleteStat(
      {
        user_id: request.user.googleId,
        stat_id: request.query.stat_id,
      },
      rpcResultToResponseBody(response, next));
};

var recordEvent = function(service, request, response, next) {
  var start = parseInt(request.query.start) || Date.now() / 1000;
  var length = parseInt(request.query.length) || 0;
  service.recordEvent(
      {
        user_id: request.user.googleId,
        event: {
          stat_id: request.query.stat_id,
          start_time: {seconds: start},
          duration: {seconds: length},
        },
      },
      rpcResultToResponseBody(response, next));
};

var deleteEvent = function(service, request, response, next) {
  service.deleteEvent(
      {
        user_id: request.user.googleId,
        stat_id: request.query.stat_id,
        event_id: request.query.event_id,
      },
      rpcResultToResponseBody(response, next));
};

exports.registerStatServiceHttpHandlers = function(
    app, options) {
  var urlPrefix = options.urlPrefix;
  var serviceProtoPath = options.serviceProtoPath;
  var serverHostPort = options.serverHostPort;

  var packageDefinition = protoLoader.loadSync(serviceProtoPath, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
  });
  var protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
  var statTracker = protoDescriptor.stat_tracker;
  var statService = new statTracker.StatService(
      serverHostPort, grpc.credentials.createInsecure());

  app.get(urlPrefix + 'read_stats', (request, response, next) => {
    readStats(statService, request, response, next); });
  app.get(urlPrefix + 'read_events', (request, response, next) => {
    readEvents(statService, request, response, next); });
  app.get(urlPrefix + 'define_stat', (request, response, next) => {
    defineStat(statService, request, response, next); });
  app.get(urlPrefix + 'delete_stat', (request, response, next) => {
    deleteStat(statService, request, response, next); });
  app.get(urlPrefix + 'record_event', (request, response, next) => {
    recordEvent(statService, request, response, next); });
  app.get(urlPrefix + 'delete_event', (request, response, next) => {
    deleteEvent(statService, request, response, next); });
};
