var readStats = function() {
  return $.ajax({
    url: "/api/read_stats",
    type: "GET",
  });
};

var readEvents = function(statIds, start, length) {
  return $.ajax({
    url: "/api/read_events",
    data: {
      "stat_id": statIds.join(','),
      "start": start,
      "length": length,
    },
    type: "GET",
  });
};

var defineStat = function(displayName) {
  return $.ajax({
    url: "/api/define_stat",
    data: {
      "display_name": displayName,
    },
    type: "GET",
  });
};

var deleteStat = function(statId) {
  return $.ajax({
    url: "/api/delete_stat",
    data: {
      "stat_id": statId,
    },
    type: "GET",
  });
};

var recordEventNow = function(statId) {
  return $.ajax({
    url: "/api/record_event",
    data: {
      "stat_id": statId,
    },
    type: "GET",
  });
};

var recordEvent = function(statId, start, length) {
  return $.ajax({
    url: "/api/record_event",
    data: {
      "stat_id": statId,
      "start": start,
      "length": length,
    },
    type: "GET",
  });
};

var deleteEvent = function(statId, eventId) {
  return $.ajax({
    url: "/api/delete_event",
    data: {
      "stat_id": statId,
      "event_id": eventId,
    },
    type: "GET",
  });
};

