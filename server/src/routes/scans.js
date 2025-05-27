const express = require('express');
const router = express.Router();
const auth = require('../middlewares/auth');
const multer = require('multer');
const { uploadFileToS3 } = require('../utils/s3');
const ImageScan = require('../models/ImageScan');

const upload = multer({
  storage: multer.memoryStorage(),
  limits: {
    fileSize: 50 * 1024 * 1024 // 50MB
  },
  fileFilter: (req, file, cb) => {
    if (file.mimetype.startsWith('image/')) {
      cb(null, true);
    } else {
      cb(new Error('Only image files are allowed'), false);
    }
  }
});

// POST /api/scans - Upload scan to S3
router.post('/', auth, upload.single('image'), async (req, res) => {
    try {
      if (!req.file) {
        return res.status(400).json({ error: 'Image file required' });
      }
  
      const s3Data = await uploadFileToS3(req.file);
  
      // Check if S3 returned valid data
      if (!s3Data?.url || !s3Data?.key) {
        throw new Error('Failed to retrieve image URL or key from S3');
      }
  
      const newScan = new ImageScan({
        imageId: `SCN-${Date.now()}`,
        imageUrl: s3Data.url,
        imageKey: s3Data.key,
        category: req.body.category,
        createdBy: req.user.id
      });
      
      const scan = await newScan.save();  
      res.json(scan);
  
    } catch (err) {
      console.error(err);
      res.status(500).json({ 
        error: err.message || 'Scan upload failed',
        ...(err.message?.includes('image') && { invalidField: 'image' })
      });
    }
  });

module.exports = router;