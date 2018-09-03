var passport = require('passport');
var GoogleStrategy = require('passport-google-oauth20').Strategy;

exports.registerAuthHttpHandlers = function(
    app, options) {
  var clientID = options.clientID;
  var clientSecret = options.clientSecret;
  var callbackDomain = options.callbackDomain;
  var urlPrefix = options.urlPrefix;
  var sessionSecret = options.sessionSecret;
  var cookieMaxAge = options.cookieMaxAge || 1000 * 60 * 10;

  app.use(require('cookie-parser')());
  app.use(require('body-parser').urlencoded({extended: true}));
  app.use(require('express-session')({
    secret: sessionSecret,
    resave: true,
    saveUninitialized: true,
    cookie: {maxAge: cookieMaxAge},
  }));
  app.use(passport.initialize());
  app.use(passport.session());

  passport.serializeUser(function(user, done) {
      done(null, user);
  });
  passport.deserializeUser(function(user, done) {
      done(null, user);
  });

  var fullCallbackUrl = callbackDomain + urlPrefix + 'callback';
  passport.use(new GoogleStrategy(
        {
          clientID: clientID,
          clientSecret: clientSecret,
          callbackURL: fullCallbackUrl,
        },
        function (accessToken, refreshToken, profile, cb) {
          cb(null, {googleId: profile.id});
        }));

  var callbackUrl = urlPrefix + 'callback';
  var failureUrl = urlPrefix + 'failure';
  var successUrl = urlPrefix + 'success';
  app.get(callbackUrl, passport.authenticate('google', {
    successReturnToOrRedirect: successUrl,
    failureRedirect: failureUrl
  }));
  app.get(failureUrl, (request, response) => {
    console.log('user failed to authenticate');
    response.send('failed to authenticate');
  });
  app.get(successUrl, (request, response) => {
    console.log('successfully authenticated');
    response.send('successfully authenticated');
  });

  app.get('/auth/google/login', passport.authenticate('google', {
    scope: ['https://www.googleapis.com/auth/plus.login'],
  }));

  app.get('/auth/google/logout', (request, response) => {
    request.logout();
    response.send('logged out');
  });
};

var ensureLoggedIn = require('connect-ensure-login').ensureLoggedIn;
exports.requireLogin = function(app, url) {
  app.use(url, ensureLoggedIn('/auth/google/login'));
};
