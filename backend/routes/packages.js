const express = require('express');
const router = express.Router();
const Package = require('../models/Package');
const User = require('../models/User');
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

// Middleware to verify admin
const adminAuth = (req, res, next) => {
    const token = req.header('x-auth-token');
    if (!token) return res.status(401).json({ msg: 'No token, authorization denied' });

    try {
        const decoded = jwt.verify(token, SECRET);
        req.user = decoded.user;
        if (req.user.role !== 'admin') {
            return res.status(403).json({ msg: 'Admin access required' });
        }
        next();
    } catch (err) {
        res.status(401).json({ msg: 'Token is not valid' });
    }
};

// GET /api/packages - List all packages (Registry Index format)
router.get('/', async (req, res) => {
    try {
        const packages = await Package.find({}, '-_id -__v -manifest -downloadHistory');
        res.json({
            timestamp: new Date().toISOString(),
            packages: packages
        });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/stats - Get analytics (Admin only)
router.get('/stats', adminAuth, async (req, res) => {
    try {
        const totalPackages = await Package.countDocuments();
        const totalUsers = await User.countDocuments();
        const totalDownloads = await Package.aggregate([
            { $group: { _id: null, total: { $sum: '$downloads' } } }
        ]);

        // Get top packages by downloads
        const topPackages = await Package.find({})
            .sort({ downloads: -1 })
            .limit(10)
            .select('id name version downloads weeklyDownloads author');

        // Get recent packages
        const recentPackages = await Package.find({})
            .sort({ createdAt: -1 })
            .limit(5)
            .select('id name version createdAt author');

        // Downloads by runtime
        const downloadsByRuntime = await Package.aggregate([
            {
                $group: {
                    _id: '$runtime.type',
                    count: { $sum: 1 },
                    downloads: { $sum: '$downloads' }
                }
            },
            { $sort: { downloads: -1 } }
        ]);

        // Daily downloads for last 30 days
        const thirtyDaysAgo = new Date();
        thirtyDaysAgo.setDate(thirtyDaysAgo.getDate() - 30);

        const dailyDownloads = await Package.aggregate([
            { $unwind: { path: '$downloadHistory', preserveNullAndEmptyArrays: false } },
            { $match: { 'downloadHistory.date': { $gte: thirtyDaysAgo } } },
            {
                $group: {
                    _id: { $dateToString: { format: '%Y-%m-%d', date: '$downloadHistory.date' } },
                    count: { $sum: '$downloadHistory.count' }
                }
            },
            { $sort: { _id: 1 } }
        ]);

        res.json({
            totalPackages,
            totalUsers,
            totalDownloads: totalDownloads[0]?.total || 0,
            topPackages,
            recentPackages,
            downloadsByRuntime,
            dailyDownloads
        });
    } catch (err) {
        console.error(err);
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/:id - Get specific package manifest (tracks download)
router.get('/:id', async (req, res) => {
    try {
        // Skip stats endpoint
        if (req.params.id === 'stats') return;

        // Find package by ID (e.g., author.name)
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Track download if requested from CLI (User-Agent check)
        const userAgent = req.get('User-Agent') || '';
        if (userAgent.includes('nex/') || req.query.download === 'true') {
            await trackDownload(pkg);
        }

        // Return the stored manifest directly for CLI compatibility
        res.json(pkg.manifest || pkg);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// POST /api/packages/:id/download - Explicit download tracking
router.post('/:id/download', async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        await trackDownload(pkg);
        res.json({ msg: 'Download tracked', downloads: pkg.downloads + 1 });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// Helper function to track downloads
async function trackDownload(pkg) {
    const today = new Date();
    today.setHours(0, 0, 0, 0);

    // Update total downloads
    pkg.downloads = (pkg.downloads || 0) + 1;
    pkg.lastDownloadedAt = new Date();

    // Update download history for today
    const todayEntry = pkg.downloadHistory.find(
        h => h.date && h.date.toDateString() === today.toDateString()
    );

    if (todayEntry) {
        todayEntry.count += 1;
    } else {
        pkg.downloadHistory.push({ date: today, count: 1 });
        // Keep only last 90 days of history
        if (pkg.downloadHistory.length > 90) {
            pkg.downloadHistory = pkg.downloadHistory.slice(-90);
        }
    }

    // Calculate weekly and monthly downloads
    const weekAgo = new Date();
    weekAgo.setDate(weekAgo.getDate() - 7);
    const monthAgo = new Date();
    monthAgo.setDate(monthAgo.getDate() - 30);

    pkg.weeklyDownloads = pkg.downloadHistory
        .filter(h => h.date >= weekAgo)
        .reduce((sum, h) => sum + h.count, 0);

    pkg.monthlyDownloads = pkg.downloadHistory
        .filter(h => h.date >= monthAgo)
        .reduce((sum, h) => sum + h.count, 0);

    await pkg.save();
}

// POST /api/packages - Publish a package (Admin only now)
router.post('/', adminAuth, async (req, res) => {
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
            // Update existing package
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
            workspace: req.user.id,
            downloads: 0,
            weeklyDownloads: 0,
            monthlyDownloads: 0,
            downloadHistory: []
        });

        await pkg.save();
        res.json({ msg: 'Package published', id: pkg.id });

    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

// DELETE /api/packages/:id - Delete a package (Admin only)
router.delete('/:id', adminAuth, async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        await pkg.deleteOne();
        res.json({ msg: 'Package removed' });
    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

module.exports = router;
