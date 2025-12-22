const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();

// CORS Configuration - Allow all origins for API access
const corsOptions = {
    origin: true,
    methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
    allowedHeaders: ['Content-Type', 'Authorization', 'x-auth-token'],
    credentials: true
};

// Middleware
app.use(cors(corsOptions));
app.use(bodyParser.json());

// Serve static files (CSS, JS, images)
app.use(express.static('public'));

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

// Clean URL routes (without .html)
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.get('/login', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'login.html'));
});

app.get('/dashboard', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'dashboard.html'));
});

app.get('/publish', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'publish.html'));
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
