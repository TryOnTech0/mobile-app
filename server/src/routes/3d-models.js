const express = require('express');
const router = express.Router();
const auth = require('../middlewares/auth');
const multer = require('multer');
const { uploadFileToS3 } = require('../utils/s3');
const ModelProcessed = require('../models/ModelProcessed');

const upload = multer({
  storage: multer.memoryStorage(),
  limits: {
    fileSize: 100 * 1024 * 1024 // 100MB for 3D models
  },
  fileFilter: (req, file, cb) => {
    // Accept common 3D model formats and preview images
    const allowedMimes = [
      'application/octet-stream', // .obj, .glb, .gltf
      'model/gltf+json',
      'model/gltf-binary',
      'text/plain', // .obj files
      'image/jpeg',
      'image/png',
      'image/jpg'
    ];
    
    if (allowedMimes.includes(file.mimetype) || file.originalname.match(/\.(obj|glb|gltf|fbx)$/i)) {
      cb(null, true);
    } else {
      cb(new Error('Only 3D model files and preview images are allowed'), false);
    }
  }
});

// GET /api/3d-models/:garmentId - Get 3D model by garmentId for authenticated user
router.get('/:garmentId', auth, async (req, res) => {
  try {
    const model = await ModelProcessed.findOne({ 
      garmentId: req.params.garmentId,
      createdBy: req.user.id 
    }).select('-__v');

    if (!model) {
      return res.status(404).json({ error: 'No 3D model found for this garment' });
    }

    res.json(model);

  } catch (err) {
    console.error('Error fetching 3D model:', err);
    res.status(500).json({ 
      error: 'Failed to fetch 3D model',
      details: err.message 
    });
  }
});

// GET /api/3d-models - Get last processed 3D model for authenticated user
router.get('/', auth, async (req, res) => {
  try {
    const lastModel = await ModelProcessed.findOne({ createdBy: req.user.id })
      .sort({ createdAt: -1 })
      .select('-__v');

    if (!lastModel) {
      return res.status(404).json({ error: 'No 3D models found' });
    }

    res.json(lastModel);

  } catch (err) {
    console.error('Error fetching last 3D model:', err);
    res.status(500).json({ 
      error: 'Failed to fetch last 3D model',
      details: err.message 
    });
  }
});

// DELETE /api/3d-models - Delete last created 3D model for authenticated user
router.delete('/', auth, async (req, res) => {
  try {
    const lastModel = await ModelProcessed.findOne({ createdBy: req.user.id })
      .sort({ createdAt: -1 });

    if (!lastModel) {
      return res.status(404).json({ error: 'No 3D models found to delete' });
    }

    await ModelProcessed.findByIdAndDelete(lastModel._id);

    res.json({ 
      message: 'Last 3D model deleted successfully',
      deletedModel: {
        id: lastModel._id,
        garmentId: lastModel.garmentId,
        createdAt: lastModel.createdAt
      }
    });

  } catch (err) {
    console.error('Error deleting last 3D model:', err);
    res.status(500).json({ 
      error: 'Failed to delete last 3D model',
      details: err.message 
    });
  }
});

// POST /api/3d-models - Upload 3D model and preview to S3
router.post('/', auth, upload.fields([
  { name: 'preview', maxCount: 1 },
  { name: 'model', maxCount: 1 }
]), async (req, res) => {
  try {
    if (!req.files || !req.files.preview || !req.files.model) {
      return res.status(400).json({ 
        error: 'Both preview image and 3D model files are required' 
      });
    }

    if (!req.body.garmentId) {
      return res.status(400).json({ 
        error: 'garmentId is required' 
      });
    }

    const previewFile = req.files.preview[0];
    const modelFile = req.files.model[0];

    // Upload preview image to S3
    const previewS3Data = await uploadFileToS3(previewFile, 'previews/');
    if (!previewS3Data?.url || !previewS3Data?.key) {
      throw new Error('Failed to upload preview image to S3');
    }

    // Upload 3D model to S3
    const modelS3Data = await uploadFileToS3(modelFile, 'models/');
    if (!modelS3Data?.url || !modelS3Data?.key) {
      throw new Error('Failed to upload 3D model to S3');
    }

    const newModel = new ModelProcessed({
      garmentId: req.body.garmentId,
      previewUrl: previewS3Data.url,
      previewKey: previewS3Data.key,
      modelUrl: modelS3Data.url,
      modelKey: modelS3Data.key,
      createdBy: req.user.id
    });
    
    const savedModel = await newModel.save();  
    res.json(savedModel);

  } catch (err) {
    console.error('3D Model upload error:', err);
    res.status(500).json({ 
      error: err.message || '3D model upload failed',
      ...(err.message?.includes('preview') && { invalidField: 'preview' }),
      ...(err.message?.includes('model') && { invalidField: 'model' })
    });
  }
});

module.exports = router;