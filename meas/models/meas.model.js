const mongoose = require('../../common/services/mongoose.service').mongoose;
const Schema = mongoose.Schema;

const measSchema = new Schema({
    humidity: Number,
    co2: Number,
    temp: Number,
    timestamp: Number
    
});

measSchema.virtual('id').get(function () {
    return this._id.toHexString();
});

// Ensure virtual fields are serialised.
measSchema.set('toJSON', {
    virtuals: true
});

const Meas = mongoose.model('Measurements', measSchema);


exports.newMeas = (measData) => {
    const meas = new Meas(measData);
    return meas.save();
};




