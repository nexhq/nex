const mongoose = require('mongoose');

const ReviewSchema = new mongoose.Schema({
    packageId: { type: String, required: true, index: true },
    user: { type: mongoose.Schema.Types.ObjectId, ref: 'User', required: true },
    username: { type: String, required: true }, // Cached for display

    rating: {
        type: Number,
        required: true,
        min: 1,
        max: 5
    },
    title: { type: String, maxlength: 100 },
    comment: { type: String, maxlength: 1000 },

    // Helpful votes
    helpful: { type: Number, default: 0 },
    notHelpful: { type: Number, default: 0 },

    createdAt: { type: Date, default: Date.now },
    updatedAt: { type: Date, default: Date.now }
});

// One review per user per package
ReviewSchema.index({ packageId: 1, user: 1 }, { unique: true });

module.exports = mongoose.model('Review', ReviewSchema);
