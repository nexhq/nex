const mongoose = require('mongoose');

const PackageVersionSchema = new mongoose.Schema({
    packageId: { type: String, required: true, index: true }, // Reference to package ID
    version: { type: String, required: true },
    changelog: { type: String, default: '' }, // What changed in this version
    manifest: { type: Object, required: true }, // Full manifest for this version

    // Metadata
    downloads: { type: Number, default: 0 },
    publishedAt: { type: Date, default: Date.now },
    publishedBy: { type: mongoose.Schema.Types.ObjectId, ref: 'User' },

    // Deprecation for specific versions
    deprecated: { type: Boolean, default: false },
    deprecationMessage: { type: String }
});

// Compound index for unique version per package
PackageVersionSchema.index({ packageId: 1, version: 1 }, { unique: true });

module.exports = mongoose.model('PackageVersion', PackageVersionSchema);
