const express = require('express');
const router = express.Router();
const auth = require('../middlewares/auth');
const Garment = require('../models/Garment');
const mongoose = require('mongoose');
const multer = require('multer');
const { S3Client, PutObjectCommand } = require('@aws-sdk/client-s3');

// Initialize S3 client
const s3Client = new S3Client({
  region: process.env.AWS_REGION,
  credentials: {
    accessKeyId: process.env.AWS_ACCESS_KEY_ID,
    secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY
  }
});

// Configure Multer with file filtering
const storage = multer.memoryStorage();
const upload = multer({
  storage,
  fileFilter: (req, file, cb) => {
    // Validate preview file type
    if (file.fieldname === 'preview') {
      if (file.mimetype === 'image/jpeg' || file.mimetype === 'image/png') {
        cb(null, true);
      } else {
        cb(new Error('Preview must be JPG/PNG image'), false);
      }
    } else {
      cb(null, true);
    }
  },
  limits: {
    fileSize: 10 * 1024 * 1024 // 10MB limit for preview
  }
});

// Helper function to generate unique garmentId
const generateGarmentId = async () => {
  const timestamp = Date.now().toString(36);
  const random = Math.random().toString(36).substr(2, 5);
  const garmentId = `GRM-${timestamp}-${random}`.toUpperCase();
  
  // Check if ID already exists (very unlikely but good practice)
  const existing = await Garment.findOne({ garmentId });
  if (existing) {
    return generateGarmentId(); // Recursively generate new ID
  }
  
  return garmentId;
};

// Helper function to upload file to S3
const uploadFileToS3 = async (file) => {
  const bucketName = process.env.AWS_S3_BUCKET_NAME;
  const key = `${Date.now()}-${file.originalname}`;

  const uploadParams = {
    Bucket: bucketName,
    Key: key,
    Body: file.buffer,
    ContentType: file.mimetype,
  };

  try {
    const command = new PutObjectCommand(uploadParams);
    await s3Client.send(command);
    return {
      url: `https://${bucketName}.s3.${process.env.AWS_REGION}.amazonaws.com/${key}`,
      key: key
    };
  } catch (error) {
    console.error('Error uploading to S3:', error);
    throw error;
  }
};

// Add new garment with type-specific handling
router.post('/', auth, upload.fields([
  { name: 'preview', maxCount: 1 },
  { name: 'model', maxCount: 1 }
]), async (req, res) => {
  try {
    if (!req.files || !req.files.preview || !req.files.model) {
      return res.status(400).json({ error: 'Both preview and model files are required' });
    }

    const uploadedFiles = {
      preview: req.files.preview[0],
      model: req.files.model[0]
    };

    // Upload files with different handling
    const [previewData, modelData] = await Promise.all([
      uploadFileToS3(uploadedFiles.preview),
      uploadFileToS3(uploadedFiles.model)
    ]);

    // Generate unique garmentId
    const garmentId = await generateGarmentId();

    const newGarment = new Garment({
      garmentId,
      name: req.body.name,
      previewUrl: previewData.url,
      previewKey: previewData.key,
      modelUrl: modelData.url,
      modelKey: modelData.key,
      createdBy: req.user.id
    });

    const garment = await newGarment.save();
    res.json(garment);
    
  } catch (err) {
    console.error(err);
    if (err.code === 11000 && err.keyPattern?.garmentId) {
      return res.status(400).json({ error: 'Garment ID already exists. Please try again.' });
    }
    res.status(500).json({ 
      error: err.message || 'Server error',
      ...(err.message?.includes('Preview') && { invalidField: 'preview' })
    });
  }
});

// Get all garments for the authenticated user
router.get('/', auth, async (req, res) => {
  try {
    const garments = await Garment.find({ createdBy: req.user.id })
      .select('-__v')
      .sort({ createdAt: -1 });
    res.json(garments);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Server error' });
  }
});

// Delete garment by garmentId
router.delete('/:garmentId', auth, async (req, res) => {
  try {
    const garment = await Garment.findOne({
      garmentId: req.params.garmentId,
      createdBy: req.user.id
    });
    
    if (!garment) {
      return res.status(404).json({ error: 'Garment not found' });
    }

    // Delete associated files from S3
    try {
      const deleteParams = {
        Bucket: process.env.AWS_S3_BUCKET_NAME,
        Keys: [garment.previewKey, garment.modelKey]
      };
      
      const deleteCommands = deleteParams.Keys.map(key => {
        return s3Client.send(new DeleteObjectCommand({
          Bucket: deleteParams.Bucket,
          Key: key
        }));
      });
      
      await Promise.all(deleteCommands);
    } catch (fileErr) {
      console.warn('Error deleting files from S3:', fileErr);
    }

    // Delete the garment document
    await Garment.deleteOne({ _id: garment._id });
    
    res.json({ message: 'Garment deleted successfully' });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Server error' });
  }
});
router.get('/files/:fileId', auth, (req, res) => {
  try {
    const fileId = new mongoose.Types.ObjectId(req.params.fileId);
    const downloadStream = gfs.openDownloadStream(fileId);
    
    downloadStream.on('error', () => {
      res.status(404).json({ error: 'File not found' });
    });

    downloadStream.pipe(res);
  } catch (err) {
    res.status(400).json({ error: 'Invalid file ID' });
  }
});






module.exports = router;