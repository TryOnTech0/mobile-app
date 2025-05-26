const express = require('express');
const router = express.Router();
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const User = require('../models/User');
const auth = require('../middlewares/auth'); 

// Verify token endpoint
router.get('/verify', auth, (req, res) => {
  res.json({ 
    valid: true,
    user: {
      id: req.user._id,
      username: req.user.username
    }
  });
});

// User registration
router.post('/register', async (req, res) => {
  try {
    const { username, email, password } = req.body;
    
    console.log('Registration attempt:', { username, email, passwordLength: password?.length });
    
    // Validate input
    if (!username || !email || !password) {
      return res.status(400).json({ error: 'All fields are required' });
    }
    
    // Check if user exists
    let user = await User.findOne({ email });
    if (user) {
      console.log('User already exists with email:', email);
      return res.status(400).json({ error: 'User already exists' });
    }
    
    // Check username uniqueness
    let existingUsername = await User.findOne({ username });
    if (existingUsername) {
      console.log('Username already exists:', username);
      return res.status(400).json({ error: 'Username already exists' });
    }
    
    // Create new user
    user = new User({ username, email, password });
    await user.save();
    
    console.log('User created successfully:', user._id);
    
    // Generate JWT
    const token = jwt.sign({ id: user._id }, process.env.JWT_SECRET, {
      expiresIn: '1h'
    });
    
    res.json({ 
      token,
      user: {
        id: user._id,
        username: user.username,
        email: user.email
      }
    });
  } catch (err) {
    console.error('Registration error:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

// User login
router.post('/login', async (req, res) => {
  try {
    const { email, password } = req.body;
    
    console.log('Login attempt for email:', email);
    console.log('Password provided:', !!password);
    console.log('JWT_SECRET exists:', !!process.env.JWT_SECRET);
    
    // Validate input
    if (!email || !password) {
      return res.status(400).json({ error: 'Email and password are required' });
    }
    
    // Find user
    const user = await User.findOne({ email });
    console.log('User found in database:', !!user);
    
    if (!user) {
      return res.status(400).json({ error: 'Invalid credentials' });
    }
    
    console.log('User data:', {
      id: user._id,
      username: user.username,
      email: user.email,
      hasPassword: !!user.password,
      passwordLength: user.password?.length
    });
    
    // Check password
    console.log('Comparing passwords...');
    const isMatch = await bcrypt.compare(password, user.password);
    console.log('Password match result:', isMatch);
    
    if (!isMatch) {
      return res.status(400).json({ error: 'Invalid credentials' });
    }
    
    // Generate JWT
    const token = jwt.sign({ id: user._id }, process.env.JWT_SECRET, {
      expiresIn: '1h'
    });
    
    console.log('Login successful, token generated');
    
    res.json({ 
      token,
      user: {
        id: user._id,
        username: user.username,
        email: user.email
      }
    });
  } catch (err) {
    console.error('Login error:', err);
    res.status(500).json({ error: 'Server error' });
  }
});

module.exports = router;