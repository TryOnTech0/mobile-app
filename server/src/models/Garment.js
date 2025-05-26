const mongoose = require('mongoose');

const GarmentSchema = new mongoose.Schema({
  garmentId: {
    type: String,
    required: true,
    unique: true
  },
  name: {
    type: String,
    required: true,
    trim: true
  },
  previewUrl: {
    type: String,
    required: [true, 'Preview URL is required']
  },
  previewKey: {
    type: String,
    required: [true, 'Preview key is required']
  },
  modelUrl: {
    type: String,
    required: [true, 'Model URL is required']
  },
  modelKey: {
    type: String,
    required: [true, 'Model key is required']
  },
  createdBy: {
    type: mongoose.Schema.Types.ObjectId,
    ref: 'User',
    required: true
  },
  createdAt: {
    type: Date,
    default: Date.now
  }
});

// Add index for efficient querying
GarmentSchema.index({ garmentId: 1 });
GarmentSchema.index({ createdBy: 1 });

module.exports = mongoose.model('Garment', GarmentSchema);