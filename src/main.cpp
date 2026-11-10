#include <Arduino.h>
#include "App.h"

void setup() 
{ 
    App::begin(); 
}

void loop() 
{ 
    App::update(); 
}
