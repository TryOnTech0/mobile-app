require('dotenv').config();
const express = require('express');
const path = require('path'); // ADD THIS LINE - Missing import
const connectDB = require('./config/db');
const authRoutes = require('./routes/auth');
const garmentRoutes = require('./routes/garments');
const userRoutes = require('./routes/users');
const scanRoutes = require('./routes/scans');
const modelProcessedRoutes = require('./routes/3d-models');
const cors = require('cors');
const fs = require('fs');

const app = express();

// Connect Database
connectDB();

const allowedOrigins = [
    'http://192.168.1.5:5000', // home wi-fi
    // 'http://10.1.247.79:5000', // Server IP eduroam
    'http://10.1.239.113:5000', // Server IP eduroam
    'capacitor://localhost',    // Android
    'http://localhost'          // Development
];

    app.use(cors({
    origin: allowedOrigins,
    methods: ['GET', 'POST', 'DELETE', 'OPTIONS'],
    allowedHeaders: ['Content-Type', 'Authorization']
}));

const uploadsDir = path.join(__dirname, 'uploads');

// Create uploads directory if it doesn't exist
if (!fs.existsSync(uploadsDir)) {
  fs.mkdirSync(uploadsDir, { recursive: true });
}

app.use(express.json({ limit: '200mb' }));
app.use(express.urlencoded({ 
  extended: true, 
  limit: '200mb',
  parameterLimit: 10000 // Handle large numbers of parameters
}));

// Then serve static files
app.use('/uploads', express.static(uploadsDir));

// Middleware
app.use(express.json());
app.use(cors({
    origin: '*', // Allow all origins for development
    methods: ['GET', 'POST', 'DELETE', 'OPTIONS'],
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
app.use('/api/scans', scanRoutes);
app.use('/api/3d-models', modelProcessedRoutes);

// Database connection test endpoint
app.get('/api/database/health', async (req, res) => {
    try {
      // Ping the database to verify connection
      await mongoose.connection.db.admin().ping();
      res.json({ 
        success: true,
        message: 'Database connection healthy',
        database: mongoose.connection.name 
      });
    } catch (err) {
      res.status(500).json({
        success: false,
        error: 'Database connection failed',
        details: err.message
      });
    }
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