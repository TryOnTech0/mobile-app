const express = require('express');
const router = express.Router();
const auth = require('../middlewares/auth');
const Garment = require('../models/Garment');
const mongoose = require('mongoose');
const { GridFSBucket } = require('mongodb');
const multer = require('multer');

// Initialize GridFS bucket
let gfs;
const conn = mongoose.connection;
conn.once('open', () => {
  gfs = new GridFSBucket(conn.db, { bucketName: 'garmentFiles' });
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

// Add new garment with type-specific handling
router.post('/', auth, upload.fields([
  { name: 'preview', maxCount: 1 },
  { name: 'model', maxCount: 1 }
]), async (req, res) => {
  try {
    const uploadedFiles = {
      preview: req.files.preview[0],
      model: req.files.model[0]
    };

    const uploadFileToGridFS = (file) => new Promise((resolve, reject) => {
      const filename = `${Date.now()}-${file.originalname}`;
      const options = {
        metadata: {
          field: file.fieldname,
          createdBy: req.user.id,
          // For previews, add image-specific metadata
          ...(file.fieldname === 'preview' && {
            dimensions: '1024x1024', // Optional: add actual dimensions if needed
            type: 'preview_image'
          })
        },
        contentType: file.mimetype // Store MIME type
      };

      const uploadStream = gfs.openUploadStream(filename, options);
      
      uploadStream.end(file.buffer);
      
      uploadStream
        .on('finish', () => resolve(uploadStream.id))
        .on('error', reject);
    });

    // Upload files with different handling
    const [previewFileId, modelFileId] = await Promise.all([
      uploadFileToGridFS(uploadedFiles.preview),
      uploadFileToGridFS(uploadedFiles.model)
    ]);

    const newGarment = new Garment({
      name: req.body.name,
      previewFileId,
      modelFileId,
      createdBy: req.user.id
    });

    const garment = await newGarment.save();
    res.json(garment);
    
  } catch (err) {
    console.error(err);
    res.status(500).json({ 
      error: err.message || 'Server error',
      ...(err.message?.includes('Preview') && { invalidField: 'preview' })
    });
  }
});

// Enhanced file download endpoint for images
router.get('/preview/:fileId', auth, (req, res) => {
  const fileId = new mongoose.Types.ObjectId(req.params.fileId);
  const downloadStream = gfs.openDownloadStream(fileId);

  downloadStream.on('file', (file) => {
    // Set cache headers for images
    res.set({
      'Content-Type': file.contentType,
      'Cache-Control': 'public, max-age=31536000', // 1 year cache
      'Content-Disposition': `inline; filename="${file.filename}"` // Display in browser
    });
  });

  downloadStream.on('error', () => {
    res.status(404).json({ error: 'Preview image not found' });
  });

  downloadStream.pipe(res);
});

// Model file download remains generic
router.get('/model/:fileId', auth, (req, res) => {
  const fileId = new mongoose.Types.ObjectId(req.params.fileId);
  const downloadStream = gfs.openDownloadStream(fileId);

  downloadStream.on('file', (file) => {
    res.set({
      'Content-Type': file.contentType,
      'Content-Disposition': `attachment; filename="${file.filename}"`
    });
  });

  downloadStream.on('error', () => {
    res.status(404).json({ error: 'Model file not found' });
  });

  downloadStream.pipe(res);
});

module.exports = router;