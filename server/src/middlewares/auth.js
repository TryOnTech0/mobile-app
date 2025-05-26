const jwt = require('jsonwebtoken');
const User = require('../models/User');

module.exports = async (req, res, next) => {
  try {
    // console.log('Auth middleware triggered');
    console.log('Authorization header:', req.header('Authorization'));
    
    // Get token from header
    const authHeader = req.header('Authorization');
    
    if (!authHeader) {
      // console.log('No Authorization header found');
      return res.status(401).json({ error: 'Authentication required' });
    }
    
    if (!authHeader.startsWith('Bearer ')) {
      console.log('Authorization header does not start with Bearer');
      return res.status(401).json({ error: 'Authentication required' });
    }
    
    const token = authHeader.replace('Bearer ', '');
    console.log('Token extracted:', token.substring(0, 20) + '...');
    // console.log('JWT_SECRET exists:', !!process.env.JWT_SECRET);
    
    // Verify token
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    // console.log('Token decoded successfully, user ID:', decoded.id);
    
    // Find user and attach to request
    const user = await User.findOne({ _id: decoded.id });
    // console.log('User found in database:', !!user);
    
    if (!user) {
      console.log('User not found for ID:', decoded.id);
      return res.status(401).json({ error: 'Authentication required' });
    }
    
    req.user = user;
    // console.log('User attached to request:', user.username);
    next();
  } catch (err) {
    console.error('Auth middleware error:', err.name, err.message);
    res.status(401).json({ error: 'Authentication required' });
  }
};