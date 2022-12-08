# brazo_robotico_compdig
Codigos utilizados para el brazo robotico hecho en Computadores Digitales - II 2022

## Descripcion
El codigo consiste en enviar datos seriales a la ESP32 que controla el brazo robotico.
El brazo tiene 2 motores stepper, por ende 2 grados de libertad, uno para movimiento lineal (un riel), y otro para movimiento angular.
La ESP32 se comunica con la PC a traves de USB.

## Codigo ESP32 - C
El codigo fue realizado utilizando la ESPRESSIF-IDF, implementando las librerias: 
- stdio.h
- string.h
- driver/uart.h
- freertos/FreeRTOS.h
- freertos/task.h 
- heap_caps.h (creo que asi se escribe)


## Codigo GUI - Python
La GUI fue desarrollada utilizando Tkinter.
Reconoce automaticamente el puerto COM donde esta conectada la ESP32 y establece la comunicacion.
