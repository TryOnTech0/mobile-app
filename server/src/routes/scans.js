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

// GET /api/scans - Get last uploaded image for authenticated user
router.get('/', auth, async (req, res) => {
  try {
    const lastScan = await ImageScan.findOne({ createdBy: req.user.id })
      .sort({ createdAt: -1 })
      .select('-__v');

    if (!lastScan) {
      return res.status(404).json({ error: 'No scans found' });
    }

    res.json(lastScan);

  } catch (err) {
    console.error('Error fetching last scan:', err);
    res.status(500).json({ 
      error: 'Failed to fetch last scan',
      details: err.message 
    });
  }
});

// DELETE /api/scans - Delete last created entry for authenticated user
router.delete('/', auth, async (req, res) => {
  try {
    const lastScan = await ImageScan.findOne({ createdBy: req.user.id })
      .sort({ createdAt: -1 });

    if (!lastScan) {
      return res.status(404).json({ error: 'No scans found to delete' });
    }

    await ImageScan.findByIdAndDelete(lastScan._id);

    res.json({ 
      message: 'Last scan deleted successfully',
      deletedScan: {
        id: lastScan._id,
        garmentId: lastScan.garmentId,
        category: lastScan.category,
        createdAt: lastScan.createdAt
      }
    });

  } catch (err) {
    console.error('Error deleting last scan:', err);
    res.status(500).json({ 
      error: 'Failed to delete last scan',
      details: err.message 
    });
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
        garmentId: req.body.garmentId,
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