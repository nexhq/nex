const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();

// Middleware
app.use(cors());
app.use(bodyParser.json());
app.use(express.static('public')); // Serve static files for simple frontend

// DB Config
const db = process.env.MONGO_URI || 'mongodb://localhost:27017/nex';

// Connect to MongoDB
mongoose
    .connect(db)
    .then(() => console.log('MongoDB Connected'))
    .catch(err => console.log(err));

// Routes
app.use('/api/auth', require('./routes/auth'));
app.use('/api/packages', require('./routes/packages'));

// Serve package detail page for any /packages/:id URL
app.get('/packages/:id', (req, res, next) => {
    // If it looks like a file extension (css, js, maps), skip
    if (req.params.id.match(/\.(css|js|map|png|jpg|ico)$/)) {
        return next();
    }
    res.sendFile(path.join(__dirname, 'public/packages/view/index.html'));
});

// Route compatibility for CLI
// CLI expects /registry/index.json -> maps to /api/packages
app.get('/registry/index.json', (req, res) => {
    res.redirect('/api/packages');
});
// CLI expects /registry/packages/... -> we need to handle this query param style or path style if possible
// Ideally we update CLI to use /api/packages/:id, but for now:

const PORT = process.env.PORT || 5000;

app.listen(PORT, () => console.log(`Server started on port ${PORT}`));
