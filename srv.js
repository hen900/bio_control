const express = require("express");
const app = express();
const path = require("path");
const fs = require("fs");
// constants
const DB_PATH = path.resolve("db.json");
const PORT = process.env.PORT || 8080; // port server is to listen on 
// middlewares
app.use(express.json());
// routes
app.get("/", async (req, res) => {
  fs.readFile(DB_PATH, "utf-8", (err, jsonString) => {
    if (err) return console.log("Error in reading from db");
    let values = JSON.parse(jsonString);
    res.status(200).json({
      totalValues: values.length,
      values,
    });
  });
});
app.post("/", async (req, res) => {
  fs.readFile(DB_PATH, "utf-8", (err, jsonString) => {
    if (err) return console.log("Error in reading from db");
    let body = req.body;
    let valuesArr = JSON.parse(jsonString);
    let obj = {
      temperature: body.temperature,
      humidity: body.humidity,
      timestamp: new Date(),
    };
    valuesArr.push(obj);
    fs.writeFile(DB_PATH, JSON.stringify(valuesArr), (err) => {
      if (err) return console.log("Error in updating db");
      res.status(200).json({
        message: "Values saved",
        value: valuesArr[valuesArr.length - 1],
      });
    });
  });
});
app.listen(PORT, () => console.log("Listening on port", PORT));
