const express = require('express');
const router = express.Router();
const Package = require('../models/Package');
const jwt = require('jsonwebtoken');

const SECRET = process.env.JWT_SECRET || 'nex-secret-key-change-me';

// Middleware to verify token
const auth = (req, res, next) => {
    const token = req.header('x-auth-token');
    if (!token) return res.status(401).json({ msg: 'No token, authorization denied' });

    try {
        const decoded = jwt.verify(token, SECRET);
        req.user = decoded.user;
        next();
    } catch (err) {
        res.status(401).json({ msg: 'Token is not valid' });
    }
};

// GET /api/packages - List all packages (Registry Index format)
router.get('/', async (req, res) => {
    try {
        const packages = await Package.find({}, '-_id -__v -manifest'); // Exclude heavy fields
        res.json({
            timestamp: new Date().toISOString(),
            packages: packages
        });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/:id - Get specific package manifest
router.get('/:id', async (req, res) => {
    try {
        // Find package by ID (e.g., author.name)
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Return the stored manifest directly for CLI compatibility
        res.json(pkg.manifest || pkg);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// POST /api/packages - Publish a package
router.post('/', auth, async (req, res) => {
    try {
        const manifest = req.body;

        // Basic validation
        if (!manifest.id || !manifest.name || !manifest.version) {
            return res.status(400).json({ msg: 'Invalid manifest: id, name, and version are required' });
        }

        // Validate package ID format (lowercase, alphanumeric, hyphens only)
        const idPattern = /^[a-z0-9][a-z0-9-]*[a-z0-9]$|^[a-z0-9]$/;
        if (!idPattern.test(manifest.id)) {
            return res.status(400).json({
                msg: 'Invalid package ID format. Use lowercase letters, numbers, and hyphens only. Cannot start or end with hyphen.'
            });
        }

        // Check if package exists
        let pkg = await Package.findOne({ id: manifest.id });

        if (pkg) {
            // Package exists - check if current user is the owner
            if (pkg.workspace.toString() !== req.user.id) {
                return res.status(409).json({
                    msg: `Package ID '${manifest.id}' is already taken. Please choose a different ID.`
                });
            }

            // Owner is updating their package
            pkg.name = manifest.name;
            pkg.version = manifest.version;
            pkg.description = manifest.description;
            pkg.author = manifest.author;
            pkg.license = manifest.license;
            pkg.repository = manifest.repository;
            pkg.runtime = manifest.runtime;
            pkg.entrypoint = manifest.entrypoint;
            pkg.commands = manifest.commands;
            pkg.keywords = manifest.keywords;
            pkg.manifest = manifest;
            pkg.updatedAt = Date.now();
            await pkg.save();
            return res.json({ msg: 'Package updated', id: pkg.id });
        }

        // Create new package
        pkg = new Package({
            id: manifest.id,
            name: manifest.name,
            version: manifest.version,
            description: manifest.description,
            author: manifest.author,
            license: manifest.license,
            repository: manifest.repository,
            runtime: manifest.runtime,
            entrypoint: manifest.entrypoint,
            commands: manifest.commands,
            keywords: manifest.keywords,
            manifest: manifest,
            workspace: req.user.id
        });

        await pkg.save();
        res.json({ msg: 'Package published', id: pkg.id });

    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

// DELETE /api/packages/:id - Delete a package
router.delete('/:id', auth, async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Check ownership or admin
        if (pkg.workspace.toString() !== req.user.id && req.user.role !== 'admin') {
            return res.status(401).json({ msg: 'Not authorized' });
        }

        await pkg.deleteOne();
        res.json({ msg: 'Package removed' });
    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

module.exports = router;
