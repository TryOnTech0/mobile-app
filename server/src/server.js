require('dotenv').config();
const express = require('express');
const path = require('path'); // ADD THIS LINE - Missing import
const connectDB = require('./config/db');
const authRoutes = require('./routes/auth');
const garmentRoutes = require('./routes/garments');
const userRoutes = require('./routes/users');
const cors = require('cors');

const app = express();

// Connect Database
connectDB();

// Middleware
app.use(express.json());
app.use(cors({
    origin: '*', // Allow all origins for development
    methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
    allowedHeaders: ['Content-Type', 'Authorization']
}));

// Serve static files (for uploaded images and models)
app.use('/uploads', express.static(path.join(__dirname, 'uploads')));

// Add a simple status endpoint for testing connectivity
app.get('/api/status', (req, res) => {
    res.json({ 
        online: true, 
        version: '1.0.0',
        timestamp: new Date().toISOString(),
        server: 'TryOn App Server'
    });
});

// Routes
app.use('/api/auth', authRoutes);
app.use('/api/garments', garmentRoutes);
app.use('/api/users', userRoutes);

// Database connection test endpoint
app.get('/api/database/connect', (req, res) => {
    // This can be used by your Qt app to test database connectivity
    res.json({ 
        success: true, 
        message: 'Database connected successfully',
        database: process.env.DB_NAME || 'tryonDB'
    });
});

// Error Handling
app.use((err, req, res, next) => {
    console.error('Error:', err.stack);
    res.status(500).json({ 
        error: 'Internal Server Error',
        message: err.message 
    });
});

// 404 handler
app.use('*', (req, res) => {
    res.status(404).json({ 
        error: 'Route not found',
        path: req.originalUrl 
    });
});

const PORT = process.env.PORT || 5000;

// IMPORTANT: Bind to 0.0.0.0 to accept connections from Android devices
app.listen(PORT, '0.0.0.0', () => {
    console.log(`Server started on port ${PORT}`);
    console.log(`Server running on http://0.0.0.0:${PORT}`);
    console.log(`For Android development, use your laptop's IP address`);
    console.log(`Example: http://192.168.1.XXX:${PORT}/api/status`);
    
    // Log network interfaces to help find the right IP
    const os = require('os');
    const networkInterfaces = os.networkInterfaces();
    console.log('\nAvailable network interfaces:');
    Object.keys(networkInterfaces).forEach(interfaceName => {
        networkInterfaces[interfaceName].forEach(interface => {
            if (interface.family === 'IPv4' && !interface.internal) {
                console.log(`  ${interfaceName}: ${interface.address}`);
            }
        });
    });
});