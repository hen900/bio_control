const MeasController = require('./controllers/meas.controller');
const config = require('../common/config/env.config');


exports.routesConfig = function (app) {
    app.post('/meas', [
        MeasController.insert
    ]);
};
