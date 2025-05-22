const express = require('express');
const router = express.Router();
const auth = require('../middlewares/auth'); // Correct import
const Garment = require('../models/Garment');

// Get all garments
router.get('/', auth, async (req, res) => {
  try {
    const garments = await Garment.find().populate('createdBy', 'username');
    res.json(garments);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

// Get single garment
router.get('/:id', auth, async (req, res) => {
  try {
    const garment = await Garment.findById(req.params.id);
    if (!garment) return res.status(404).json({ error: 'Garment not found' });
    res.json(garment);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

// Add new garment
const multer = require('multer');
const path = require('path');

const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    const folder = file.fieldname === 'preview' ? 'previews' : 'models';
    cb(null, path.join(__dirname, '../../uploads/', folder));
  },
  filename: (req, file, cb) => {
    cb(null, `${Date.now()}-${file.originalname}`);
  }
});

const upload = multer({ storage });

router.post('/', auth, upload.fields([
  { name: 'preview', maxCount: 1 },
  { name: 'model', maxCount: 1 }
]), async (req, res) => {
  try {
    const newGarment = new Garment({
      name: req.body.name,
      previewUrl: `/uploads/previews/${req.files.preview[0].filename}`,
      modelUrl: `/uploads/models/${req.files.model[0].filename}`,
      createdBy: req.user.id
    });
    
    const garment = await newGarment.save();
    res.json(garment);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

//'/uploads/models/default.glb'
module.exports = router;