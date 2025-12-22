const express = require('express');
const router = express.Router();
const Package = require('../models/Package');
const PackageVersion = require('../models/PackageVersion');
const Review = require('../models/Review');
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

// ============ PACKAGE LISTING ============

// GET /api/packages - List all packages
router.get('/', async (req, res) => {
    try {
        const { category, tag, deprecated, search, sort, limit = 50 } = req.query;

        let query = {};

        // Filters
        if (category) query.category = category;
        if (tag) query.tags = tag;
        if (deprecated === 'true') query.deprecated = true;
        else if (deprecated === 'false') query.deprecated = { $ne: true };

        // Search
        if (search) {
            query.$text = { $search: search };
        }

        // Sort options
        let sortOption = { createdAt: -1 };
        if (sort === 'downloads') sortOption = { downloads: -1 };
        else if (sort === 'rating') sortOption = { averageRating: -1 };
        else if (sort === 'updated') sortOption = { updatedAt: -1 };
        else if (sort === 'name') sortOption = { name: 1 };

        const packages = await Package.find(query, '-__v -manifest -downloadHistory')
            .sort(sortOption)
            .limit(parseInt(limit));

        res.json({
            timestamp: new Date().toISOString(),
            count: packages.length,
            packages: packages
        });
    } catch (err) {
        console.error(err);
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/categories - Get all categories with counts
router.get('/categories', async (req, res) => {
    try {
        const categories = await Package.aggregate([
            { $match: { deprecated: { $ne: true } } },
            { $group: { _id: '$category', count: { $sum: 1 } } },
            { $sort: { count: -1 } }
        ]);
        res.json(categories);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/tags - Get all tags with counts
router.get('/tags', async (req, res) => {
    try {
        const tags = await Package.aggregate([
            { $unwind: '$tags' },
            { $group: { _id: '$tags', count: { $sum: 1 } } },
            { $sort: { count: -1 } },
            { $limit: 50 }
        ]);
        res.json(tags);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/stats - Analytics (Admin only)
router.get('/stats', adminAuth, async (req, res) => {
    try {
        const totalPackages = await Package.countDocuments();
        const totalUsers = await User.countDocuments();
        const totalDownloads = await Package.aggregate([
            { $group: { _id: null, total: { $sum: '$downloads' } } }
        ]);
        const deprecatedCount = await Package.countDocuments({ deprecated: true });

        const topPackages = await Package.find({})
            .sort({ downloads: -1 })
            .limit(10)
            .select('id name version downloads weeklyDownloads author averageRating');

        const recentPackages = await Package.find({})
            .sort({ createdAt: -1 })
            .limit(5)
            .select('id name version createdAt author');

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

        const downloadsByCategory = await Package.aggregate([
            {
                $group: {
                    _id: '$category',
                    count: { $sum: 1 },
                    downloads: { $sum: '$downloads' }
                }
            },
            { $sort: { downloads: -1 } }
        ]);

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
            deprecatedCount,
            topPackages,
            recentPackages,
            downloadsByRuntime,
            downloadsByCategory,
            dailyDownloads
        });
    } catch (err) {
        console.error(err);
        res.status(500).send('Server Error');
    }
});

// ============ SINGLE PACKAGE ============

// GET /api/packages/:id - Get package (latest version)
router.get('/:id', async (req, res) => {
    try {
        if (['stats', 'categories', 'tags'].includes(req.params.id)) return;

        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Track download
        const userAgent = req.get('User-Agent') || '';
        if (userAgent.includes('nex/') || req.query.download === 'true') {
            await trackDownload(pkg);
        }

        res.json(pkg.manifest || pkg);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/:id/info - Get full package info with ratings
router.get('/:id/info', async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id }, '-downloadHistory');
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Get recent reviews
        const reviews = await Review.find({ packageId: req.params.id })
            .sort({ createdAt: -1 })
            .limit(5);

        res.json({
            package: pkg,
            reviews: reviews
        });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// ============ VERSION MANAGEMENT ============

// GET /api/packages/:id/versions - List all versions
router.get('/:id/versions', async (req, res) => {
    try {
        const versions = await PackageVersion.find({ packageId: req.params.id })
            .sort({ publishedAt: -1 })
            .select('version changelog publishedAt downloads deprecated');
        res.json(versions);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/:id/versions/:version - Get specific version
router.get('/:id/versions/:version', async (req, res) => {
    try {
        const version = await PackageVersion.findOne({
            packageId: req.params.id,
            version: req.params.version
        });
        if (!version) return res.status(404).json({ msg: 'Version not found' });
        res.json(version.manifest);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// ============ REVIEWS ============

// GET /api/packages/:id/reviews - Get reviews
router.get('/:id/reviews', async (req, res) => {
    try {
        const { sort = 'recent', limit = 20 } = req.query;

        let sortOption = { createdAt: -1 };
        if (sort === 'helpful') sortOption = { helpful: -1 };
        if (sort === 'rating-high') sortOption = { rating: -1 };
        if (sort === 'rating-low') sortOption = { rating: 1 };

        const reviews = await Review.find({ packageId: req.params.id })
            .sort(sortOption)
            .limit(parseInt(limit));

        res.json(reviews);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// POST /api/packages/:id/reviews - Add review
router.post('/:id/reviews', auth, async (req, res) => {
    try {
        const { rating, title, comment } = req.body;

        if (!rating || rating < 1 || rating > 5) {
            return res.status(400).json({ msg: 'Rating must be between 1 and 5' });
        }

        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Check for existing review
        let review = await Review.findOne({ packageId: req.params.id, user: req.user.id });
        const isUpdate = !!review;
        const oldRating = review?.rating;

        if (review) {
            // Update existing review
            review.rating = rating;
            review.title = title;
            review.comment = comment;
            review.updatedAt = Date.now();
        } else {
            // Get username
            const user = await User.findById(req.user.id);

            review = new Review({
                packageId: req.params.id,
                user: req.user.id,
                username: user.username,
                rating,
                title,
                comment
            });
        }

        await review.save();

        // Update package rating
        await updatePackageRating(req.params.id, rating, isUpdate ? oldRating : null);

        res.json({ msg: isUpdate ? 'Review updated' : 'Review added', review });
    } catch (err) {
        console.error(err);
        if (err.code === 11000) {
            return res.status(400).json({ msg: 'You have already reviewed this package' });
        }
        res.status(500).send('Server Error');
    }
});

// DELETE /api/packages/:id/reviews - Delete own review
router.delete('/:id/reviews', auth, async (req, res) => {
    try {
        const review = await Review.findOne({ packageId: req.params.id, user: req.user.id });
        if (!review) return res.status(404).json({ msg: 'Review not found' });

        const oldRating = review.rating;
        await review.deleteOne();

        // Update package rating
        await updatePackageRating(req.params.id, null, oldRating);

        res.json({ msg: 'Review deleted' });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// ============ DEPRECATION ============

// POST /api/packages/:id/deprecate - Deprecate package (Admin only)
router.post('/:id/deprecate', adminAuth, async (req, res) => {
    try {
        const { message, replacementPackage } = req.body;

        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        pkg.deprecated = true;
        pkg.deprecationMessage = message || 'This package has been deprecated';
        pkg.deprecatedAt = Date.now();
        if (replacementPackage) pkg.replacementPackage = replacementPackage;

        await pkg.save();
        res.json({ msg: 'Package deprecated', package: pkg.id });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// POST /api/packages/:id/undeprecate - Remove deprecation (Admin only)
router.post('/:id/undeprecate', adminAuth, async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        pkg.deprecated = false;
        pkg.deprecationMessage = undefined;
        pkg.deprecatedAt = undefined;
        pkg.replacementPackage = undefined;

        await pkg.save();
        res.json({ msg: 'Deprecation removed', package: pkg.id });
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// ============ DEPENDENCIES ============

// GET /api/packages/:id/dependencies - Get package dependencies
router.get('/:id/dependencies', async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Resolve dependencies to full package info
        const deps = [];
        for (const dep of pkg.dependencies || []) {
            const depPkg = await Package.findOne({ id: dep.packageId }, 'id name version description');
            deps.push({
                ...dep.toObject(),
                package: depPkg || null
            });
        }

        res.json(deps);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// GET /api/packages/:id/dependents - Get packages that depend on this
router.get('/:id/dependents', async (req, res) => {
    try {
        const dependents = await Package.find(
            { 'dependencies.packageId': req.params.id },
            'id name version description'
        ).limit(50);

        res.json(dependents);
    } catch (err) {
        res.status(500).send('Server Error');
    }
});

// ============ PUBLISH / UPDATE ============

// POST /api/packages - Publish a package (Admin only)
router.post('/', adminAuth, async (req, res) => {
    try {
        const manifest = req.body;

        if (!manifest.id || !manifest.name || !manifest.version) {
            return res.status(400).json({ msg: 'id, name, and version are required' });
        }

        const idPattern = /^[a-z0-9][a-z0-9-]*[a-z0-9]$|^[a-z0-9]$/;
        if (!idPattern.test(manifest.id)) {
            return res.status(400).json({ msg: 'Invalid package ID format' });
        }

        let pkg = await Package.findOne({ id: manifest.id });
        const isUpdate = !!pkg;

        if (pkg) {
            // Check if version already exists
            if (pkg.versions && pkg.versions.includes(manifest.version)) {
                return res.status(409).json({ msg: `Version ${manifest.version} already exists` });
            }

            // Update package
            pkg.name = manifest.name;
            pkg.version = manifest.version;
            pkg.description = manifest.description;
            pkg.author = manifest.author;
            pkg.license = manifest.license;
            pkg.repository = manifest.repository;
            pkg.homepage = manifest.homepage;
            pkg.runtime = manifest.runtime;
            pkg.entrypoint = manifest.entrypoint;
            pkg.commands = manifest.commands;
            pkg.keywords = manifest.keywords;
            pkg.category = manifest.category || pkg.category;
            pkg.tags = manifest.tags || pkg.tags;
            pkg.dependencies = manifest.dependencies || pkg.dependencies;
            pkg.manifest = manifest;
            pkg.updatedAt = Date.now();
            pkg.lastPublishedAt = Date.now();
            pkg.latestVersion = manifest.version;

            // Add to versions list
            if (!pkg.versions) pkg.versions = [];
            pkg.versions.push(manifest.version);

        } else {
            // Create new package
            pkg = new Package({
                id: manifest.id,
                name: manifest.name,
                version: manifest.version,
                description: manifest.description,
                author: manifest.author,
                license: manifest.license,
                repository: manifest.repository,
                homepage: manifest.homepage,
                runtime: manifest.runtime,
                entrypoint: manifest.entrypoint,
                commands: manifest.commands,
                keywords: manifest.keywords,
                category: manifest.category || 'other',
                tags: manifest.tags || [],
                dependencies: manifest.dependencies || [],
                manifest: manifest,
                workspace: req.user.id,
                versions: [manifest.version],
                latestVersion: manifest.version,
                downloads: 0,
                weeklyDownloads: 0,
                monthlyDownloads: 0,
                downloadHistory: []
            });
        }

        await pkg.save();

        // Save version history
        const versionEntry = new PackageVersion({
            packageId: manifest.id,
            version: manifest.version,
            changelog: manifest.changelog || req.body.changelog || '',
            manifest: manifest,
            publishedBy: req.user.id
        });

        try {
            await versionEntry.save();
        } catch (e) {
            // Version might already exist, that's ok
            if (e.code !== 11000) throw e;
        }

        res.json({
            msg: isUpdate ? 'Package updated' : 'Package published',
            id: pkg.id,
            version: manifest.version
        });

    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

// POST /api/packages/:id/download - Track download
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

// DELETE /api/packages/:id - Delete package (Admin only)
router.delete('/:id', adminAuth, async (req, res) => {
    try {
        const pkg = await Package.findOne({ id: req.params.id });
        if (!pkg) return res.status(404).json({ msg: 'Package not found' });

        // Delete versions
        await PackageVersion.deleteMany({ packageId: req.params.id });

        // Delete reviews
        await Review.deleteMany({ packageId: req.params.id });

        // Delete package
        await pkg.deleteOne();

        res.json({ msg: 'Package and all versions removed' });
    } catch (err) {
        console.error(err.message);
        res.status(500).send('Server Error');
    }
});

// ============ HELPER FUNCTIONS ============

async function trackDownload(pkg) {
    const today = new Date();
    today.setHours(0, 0, 0, 0);

    pkg.downloads = (pkg.downloads || 0) + 1;
    pkg.lastDownloadedAt = new Date();

    const todayEntry = pkg.downloadHistory.find(
        h => h.date && h.date.toDateString() === today.toDateString()
    );

    if (todayEntry) {
        todayEntry.count += 1;
    } else {
        pkg.downloadHistory.push({ date: today, count: 1 });
        if (pkg.downloadHistory.length > 90) {
            pkg.downloadHistory = pkg.downloadHistory.slice(-90);
        }
    }

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

async function updatePackageRating(packageId, newRating, oldRating = null) {
    const pkg = await Package.findOne({ id: packageId });
    if (!pkg) return;

    if (!pkg.ratingDistribution) {
        pkg.ratingDistribution = { 1: 0, 2: 0, 3: 0, 4: 0, 5: 0 };
    }

    // Remove old rating if updating
    if (oldRating) {
        pkg.ratingDistribution[oldRating] = Math.max(0, (pkg.ratingDistribution[oldRating] || 0) - 1);
        if (!newRating) pkg.totalRatings = Math.max(0, pkg.totalRatings - 1);
    }

    // Add new rating
    if (newRating) {
        pkg.ratingDistribution[newRating] = (pkg.ratingDistribution[newRating] || 0) + 1;
        if (!oldRating) pkg.totalRatings = (pkg.totalRatings || 0) + 1;
    }

    // Calculate average
    if (pkg.totalRatings > 0) {
        const sum = Object.entries(pkg.ratingDistribution)
            .reduce((acc, [rating, count]) => acc + (parseInt(rating) * count), 0);
        pkg.averageRating = Math.round((sum / pkg.totalRatings) * 10) / 10;
    } else {
        pkg.averageRating = 0;
    }

    await pkg.save();
}

module.exports = router;
