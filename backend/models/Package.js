const mongoose = require('mongoose');

const PackageSchema = new mongoose.Schema({
    id: { type: String, required: true, unique: true }, // e.g., author.package
    name: { type: String, required: true },
    version: { type: String, required: true }, // Current/latest version
    description: String,
    author: {
        name: String,
        github: String,
        email: String
    },
    license: String,
    repository: String,
    homepage: String,

    // Runtime configuration
    runtime: {
        type: { type: String }, // 'python', 'node', 'bash', etc.
        version: String
    },
    entrypoint: String,
    commands: {
        default: String,
        install: String
    },

    // Categorization
    keywords: [String],
    category: {
        type: String,
        enum: ['cli', 'utility', 'development', 'automation', 'data', 'web', 'security', 'other'],
        default: 'other'
    },
    tags: [String], // Additional tags for filtering

    // Dependencies
    dependencies: [{
        packageId: String,
        version: String, // Semver range like "^1.0.0" or ">=2.0.0"
        optional: { type: Boolean, default: false }
    }],

    // Deprecation
    deprecated: { type: Boolean, default: false },
    deprecationMessage: String,
    deprecatedAt: Date,
    replacementPackage: String, // ID of replacement package if deprecated

    // Version tracking
    versions: [String], // List of all published versions
    latestVersion: String,
    latestStableVersion: String,

    // Reviews & Ratings
    averageRating: { type: Number, default: 0, min: 0, max: 5 },
    totalRatings: { type: Number, default: 0 },
    ratingDistribution: {
        1: { type: Number, default: 0 },
        2: { type: Number, default: 0 },
        3: { type: Number, default: 0 },
        4: { type: Number, default: 0 },
        5: { type: Number, default: 0 }
    },

    // URLs and resources
    downloadUrl: String,
    documentationUrl: String,
    bugTrackerUrl: String,

    // Full manifest storage
    manifest: Object,

    // Ownership
    workspace: { type: mongoose.Schema.Types.ObjectId, ref: 'User' },
    maintainers: [{ type: mongoose.Schema.Types.ObjectId, ref: 'User' }],

    // Analytics
    downloads: { type: Number, default: 0 },
    weeklyDownloads: { type: Number, default: 0 },
    monthlyDownloads: { type: Number, default: 0 },
    downloadHistory: [{
        date: { type: Date },
        count: { type: Number, default: 0 }
    }],
    lastDownloadedAt: { type: Date },

    // Timestamps
    createdAt: { type: Date, default: Date.now },
    updatedAt: { type: Date, default: Date.now },
    lastPublishedAt: { type: Date, default: Date.now }
});

// Indexes
PackageSchema.index({ downloads: -1 });
PackageSchema.index({ weeklyDownloads: -1 });
PackageSchema.index({ averageRating: -1 });
PackageSchema.index({ createdAt: -1 });
PackageSchema.index({ category: 1 });
PackageSchema.index({ tags: 1 });
PackageSchema.index({ keywords: 1 });
PackageSchema.index({ deprecated: 1 });

// Text index for search
PackageSchema.index({
    name: 'text',
    description: 'text',
    keywords: 'text',
    tags: 'text'
});

module.exports = mongoose.model('Package', PackageSchema);
