const mongoose = require('mongoose');

const GarmentSchema = new mongoose.Schema({
  name: {
    type: String,
    required: true
  },
  previewFileId: {
    type: mongoose.Schema.Types.ObjectId,
    required: true
  },
  modelFileId: {
    type: mongoose.Schema.Types.ObjectId,
    required: true
  },
  createdBy: {
    type: mongoose.Schema.Types.ObjectId,
    ref: 'User'
  },
  createdAt: {
    type: Date,
    default: Date.now
  }
});

module.exports = mongoose.model('Garment', GarmentSchema);