const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();

// CORS Configuration - Allow all origins for API access
// The frontend (https://try-nex.vercel.app) and Vercel build servers need access
// Also allows localhost for development
const corsOptions = {
    origin: true, // Allow all origins (needed for SSR and build-time fetches)
    methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
    allowedHeaders: ['Content-Type', 'Authorization'],
    credentials: true
};

// Middleware
app.use(cors(corsOptions));
app.use(bodyParser.json());
app.use(express.static('public')); // Serve static files for simple frontend

// DB Config
const db = process.env.MONGO_URI || 'mongodb://localhost:27017/nex';

// Connect to MongoDB
mongoose
    .connect(db)
    .then(() => console.log('MongoDB Connected'))
    .catch(err => console.log(err));

// API Routes
app.use('/api/auth', require('./routes/auth'));
app.use('/api/packages', require('./routes/packages'));

// Route compatibility for CLI
// CLI expects /registry/index.json -> maps to /api/packages
app.get('/registry/index.json', (req, res) => {
    res.redirect('/api/packages');
});

// Redirect /packages to frontend (Vercel)
app.get('/packages', (req, res) => {
    res.redirect('https://try-nex.vercel.app/packages');
});

app.get('/packages/*', (req, res) => {
    res.redirect('https://try-nex.vercel.app' + req.path);
});

const PORT = process.env.PORT || 5000;

app.listen(PORT, () => console.log(`Server started on port ${PORT}`));
