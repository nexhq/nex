const mongoose = require('mongoose');

const PackageSchema = new mongoose.Schema({
    id: { type: String, required: true, unique: true }, // e.g., author.package
    name: { type: String, required: true },
    version: { type: String, required: true },
    description: String,
    author: {
        name: String,
        github: String
    },
    license: String,
    repository: String,
    runtime: {
        type: { type: String }, // 'python', 'node', etc.
        version: String
    },
    entrypoint: String,
    commands: {
        default: String,
        install: String
    },
    keywords: [String],
    downloadUrl: String, // URL to the raw nex.json or zip if we host files
    manifest: Object, // Store the full original JSON
    workspace: { type: mongoose.Schema.Types.ObjectId, ref: 'User' }, // Owner

    // Analytics fields
    downloads: { type: Number, default: 0 }, // Total download count
    weeklyDownloads: { type: Number, default: 0 }, // Downloads in last 7 days
    monthlyDownloads: { type: Number, default: 0 }, // Downloads in last 30 days
    downloadHistory: [{ // Daily download counts for graphs
        date: { type: Date },
        count: { type: Number, default: 0 }
    }],
    lastDownloadedAt: { type: Date },

    createdAt: { type: Date, default: Date.now },
    updatedAt: { type: Date, default: Date.now }
});

// Index for faster analytics queries
PackageSchema.index({ downloads: -1 });
PackageSchema.index({ weeklyDownloads: -1 });
PackageSchema.index({ createdAt: -1 });

module.exports = mongoose.model('Package', PackageSchema);

