const mongoose = require('mongoose');

const GarmentSchema = new mongoose.Schema({
  name: {
    type: String,
    required: true
  },
  description: String,
  previewUrl: {
    type: String,
    required: true
  },
  modelUrl: {
    type: String,
    required: true
  },
  category: {
    type: String,
    enum: ['shirt', 'pants', 'dress'],
    default: 'shirt'
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