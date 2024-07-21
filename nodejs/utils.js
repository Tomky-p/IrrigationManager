function genRandom(){
    return Math.floor(Math.random() * 100) + 1;
}

function celciusToFahrenheit(celsius){
    return (celsius * 9)/5 +32;
}
module.exports = {
    genRandom,
    celciusToFahrenheit
}