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
router.post('/', auth, async (req, res) => {
  try {
    const newGarment = new Garment({
      ...req.body,
      createdBy: req.user.id
    });
    const garment = await newGarment.save();
    res.json(garment);
  } catch (err) {
    res.status(500).json({ error: 'Server error' });
  }
});

module.exports = router;