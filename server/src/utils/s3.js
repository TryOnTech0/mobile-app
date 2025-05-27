const { S3Client, PutObjectCommand } = require('@aws-sdk/client-s3');
const { v4: uuidv4 } = require('uuid');

const s3 = new S3Client({
    region: process.env.AWS_REGION,
    credentials: {
      accessKeyId: process.env.AWS_ACCESS_KEY_ID,
      secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY
    }
});

// Add validation before upload
const uploadFileToS3 = async (file) => {
if (!process.env.AWS_S3_BUCKET_NAME) {
    throw new Error('AWS_S3_BUCKET_NAME environment variable not set');
}

const key = `${Date.now()}-${file.originalname}`;

const uploadParams = {
    Bucket: process.env.AWS_S3_BUCKET_NAME,
    Key: key,
    Body: file.buffer,
    ContentType: file.mimetype,
};

try {
    const command = new PutObjectCommand(uploadParams);
    await s3.send(command);
    
    // Return both URL and key with proper formatting
    return {
    url: `https://${process.env.AWS_S3_BUCKET_NAME}.s3.${process.env.AWS_REGION}.amazonaws.com/${key}`,
    key: key
    };
} catch (error) {
    console.error('S3 Upload Error:', error);
    throw new Error(`File upload failed: ${error.message}`);
}
};

module.exports = { uploadFileToS3 };