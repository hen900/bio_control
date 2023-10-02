const MeasModel = require('../models/meas.model');

exports.insert = (req, res) => {
    MeasModel.newMeas(req.body)
        .then((result) => {
            res.status(201).send({id: result._id});
        });
};

