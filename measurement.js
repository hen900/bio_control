var temperature = weather.temperature;
var humidity = weather.humidity;
var CO2 = 0.0;
var temperatureF = 0.0;

//updates each measurement
function updateMeasurements(){
    temperature = obj.temperature;
    humidity = obj.humidity;
    CO2 = 0;
    temperatureF = Math.round(((temperature * 9/5) + 32) * 10) / 10;
}
setInterval(updateMeasurements, 10000); //updates every 10 seconds

//prints the temperature onto the website
function currentTemperature(){
    var temperatureText = temperature + " \u00B0C | " + temperatureF + " \u00B0F";
    return temperatureText;
}

function currentHumidity(){
    var humidityText = humidity + "%";
    return humidityText;
}

function currentCO2(){
    var CO2Text = CO2 + " ppm";
    return CO2Text;
}