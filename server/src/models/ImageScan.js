const mongoose = require('mongoose');

const ImageScanSchema = new mongoose.Schema({
  garmentId: {
    type: String,
    required: true,
    unique: true
  },
  imageUrl: {
    type: String,
    required: [true, 'Image URL is required']
  },
  imageKey: {
    type: String,
    required: [true, 'Image key is required']
  },
  category: {
    type: String,
    required: [true, 'Category is required']
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
ImageScanSchema.index({ garmentId: 1 });
ImageScanSchema.index({ createdBy: 1 });

module.exports = mongoose.model('ImageScan', ImageScanSchema);