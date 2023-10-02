

const config = require('./common/config/env.config.js');
const express = require('express');
const mongodb = require('mongodb');
const app = express();
const path = require('path'); // Required for serving static files

const MeasRouter = require('./meas/routes.config');

app.use(function (req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Credentials', 'true');
    res.header('Access-Control-Allow-Methods', 'GET,HEAD,PUT,PATCH,POST,DELETE');
    res.header('Access-Control-Expose-Headers', 'Content-Length');
    res.header('Access-Control-Allow-Headers', 'Accept, Authorization, Content-Type, X-Requested-With, Range');
    if (req.method === 'OPTIONS') {
        return res.sendStatus(200);
    } else {
        return next();
    }
});

app.use(express.json());
MeasRouter.routesConfig(app);

// Serve static files (HTML, CSS, JS, etc.) from a directory
app.use(express.static(path.join(__dirname, 'public')));

// Define a route for the root URL ('/') to serve your HTML file
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(config.port, function () {
    console.log('app listening at port %s', config.port);
});


const { MongoClient } = require('mongodb');
const mongoURL = 'mongodb://localhost:27017/rest-tutorial'; // Replace with your MongoDB connection URL and database name

app.get('/update', async (req, res) => {
    try {
        const client = new MongoClient(mongoURL, { useUnifiedTopology: true });

        await client.connect(); // Connect to the MongoDB server
        const db = client.db(); // Get the database instance
        const collection = db.collection('measurements'); // Replace with your collection name

        // Fetch data from MongoDB
        const data = await collection.find({}).toArray(); // Fetch all documents in the collection

        // Close the MongoDB client
        await client.close();

        // Return the data as JSON
        res.json(data);
    } catch (error) {
        console.error(error);
        res.status(500).json({ error: 'Internal server error' });
    }
});
