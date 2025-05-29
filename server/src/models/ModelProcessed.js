const mongoose = require('mongoose');

const ModelProcessedSchema = new mongoose.Schema({
  garmentId: {
    type: String,
    required: true,
    unique: true
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
  }
}, {
  timestamps: true
});

// Index for better query performance
ModelProcessedSchema.index({ createdBy: 1, createdAt: -1 });
ModelProcessedSchema.index({ modelId: 1 });

module.exports = mongoose.model('ModelProcessed', ModelProcessedSchema);