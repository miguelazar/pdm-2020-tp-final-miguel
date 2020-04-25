//
//  tp1_v1.c
//  
//
//  Created by Miguel Azar on 22/4/20.
//
//

/*
 * Date: 2020-04-22
 */

/*==================[inclusions]=============================================*/

//#include "tp1_v1.h"
#include "sapi.h"
#include "string.h"


// Tipo de datos enumerado llamado "estado"
typedef enum{
    ESTADO_EMA, // Espera confirmacion en modo automatico
    ESTADO_EMM, // Espera confirmacion en modo manual
    ESTADO_CHS, // Chequeo de sensores
    ESTADO_MAN, // Manual
    ESTADO_AUT  // Automatico
} estado;


// Variable de estado (global)
estado estadoActual;
estado estadoAnterior;

// Variables para almacenar humedad y temperatura
float humidity = 0, temperature = 0;


// Prototipos de funciones
void InicializarMEF(void);
void ActualizarMEF(void);


// Programa principal
int main (void){
    /* Inicializar la placa */
    boardConfig();
    
    // Inicializar periferico UART_USB
    uartConfig( UART_USB, 115200 );
    printf( "Placa EDU-CIAA-NXP inicializada...\r\n\r\n");
    delay(500);
    
    // Inicializo el sensor DHT11
    dht11Init( GPIO1 );

    InicializarMEF();
    while(1){
        ActualizarMEF();
    }
    return 0;
}

// Función Inicializar MEF
void InicializarMEF(void){
//void InicializarMEF(delay_t){
    estadoActual = ESTADO_EMA;
    // el estado anterior lo uso para determinar que deberia hacer el nuevo estado
    estadoAnterior = ESTADO_EMA;
}

// Función Actualizar MEF
void ActualizarMEF(void){

    /* variable para medir el tiempo que permanece presionada cada tecla */
    uint64_t tiempoTec1 = 0; uint64_t tiempoTec2 = 0; uint64_t tiempoTec3 = 0;
    uint8_t fallaLectura = 1;
    uint8_t contadorFallas = 0;
    uint8_t espera = 1;
    
    switch (estadoActual) {
        case ESTADO_EMA:
            // Actualizar salida del estado
        	gpioWrite( LEDR, OFF );
        	gpioWrite( LEDG, ON );
        	printf( "\r\n\r\n\r\n\r\n\r\n\r\nConfirma proceso en modo AUTOMATICO?\r\n\r\n");
        	printf( "Presione TEC3 para confirmar\r\n");
        	printf( "Presione TEC2 para cambiar a modo MANUAL\r\n");

            // Calculo el tiempo presionado del pulsador TEC2
            // La idea aqui era que el usuario (si presionaba mas de x segundos) accedia
            // a una configuracion avanzada, pero asi como está funciona como
            // un pseudo debounce
        	while(espera){
            while( !gpioRead(TEC2) ){
                tiempoTec2++;
                delay(1);
                espera = 0;
            }
            // Calculo el tiempo presionado del pulsador TEC3
            while( !gpioRead(TEC3) ){
                tiempoTec3++;
                delay(1);
                espera = 0;
            }
        	}
            // Chequear condiciones de transición de estado
            // Si se presiona por mas de 40 ms entonces cambia de estado
            if(tiempoTec2 > 40){ // Cambio de estado
                estadoActual = ESTADO_EMM;
                tiempoTec2 = 0;
                espera = 1;
                printf( "\r\n\r\n\r\n>>>   Se presionó el pulsador TEC2\r\n");

            }
            if(tiempoTec3 > 40){ // Cambio de estado
                estadoActual = ESTADO_CHS;
            	}
            estadoAnterior = ESTADO_EMA;
        break;
        case ESTADO_EMM:

            // Actualizar salida del estado
        	gpioWrite( LEDG, OFF );
        	gpioWrite( LEDR, ON );
        	printf( "Confirma proceso en modo MANUAL?\r\n\r\n");
        	printf( "Presione TEC3 para confirmar\r\n");
        	printf( "Presione TEC1 para cambiar a modo AUTOMATICO\r\n");
        	
            // Calculo el tiempo presionado del pulsador TEC1
        	while(espera){
            while( !gpioRead(TEC1) ){
                tiempoTec1++;
                delay(1);
                espera = 0;
            }
            // Calculo el tiempo presionado del pulsador TEC3
            while( !gpioRead(TEC3) ){
                tiempoTec3++;
                delay(1);
                espera = 0;
            }
        	}
            
            // Chequear condiciones de transición de estado
            if(tiempoTec1 > 40){ // Cambio de estado
                estadoActual = ESTADO_EMA;
                tiempoTec1 = 0;
                printf( "\r\n\r\n\r\n>>>   Se presionó el pulsador TEC1\r\n");
            }
            if(tiempoTec3 > 40){ // Cambio de estado
                estadoActual = ESTADO_CHS; }
            estadoAnterior = ESTADO_EMM;
        break;
        case ESTADO_CHS:
            
            // Funcion evaluacion sensores
        	printf( "\r\n\r\n\r\n\r\n>>>   Se presionó el pulsador TEC3\r\n");
            while (fallaLectura == 1 && contadorFallas < 3) {
                printf( "Checking sensors...\r\n\r\n\r\n");
                // Lectura del sensor DHT11, devuelve true si pudo leer correctamente
                if( dht11Read(&humidity, &temperature) ) {
                    // Si leyo bien prendo el LEDG y enciendo el LEDR
                    gpioWrite( LED3, ON );
                    gpioWrite( LED2, OFF );
                    // Informo los valores de los sensores
                    printf( "Temperatura: %.2f grados C.\r\n\r\n", temperature );
                    printf( "Humedad: %.2f  %.\r\n\r\n", humidity );
                    delay(1000);
                    fallaLectura = 0;
                    estadoActual = ESTADO_AUT;
                } else {
                    // Si leyo mal apago el LEDG y enciendo el LEDR
                    gpioWrite( LED3, OFF );
                    gpioWrite( LED2, ON );
                    // Informo el error de lectura
                    printf( "Error al leer DHT11.\r\n\r\n");
                    contadorFallas++;
                    estadoActual = ESTADO_MAN;
                }
            }
            estadoAnterior = ESTADO_CHS;
        break;
        case ESTADO_MAN:
            // Enciende led verde en forma permanente
            printf( "Desde aqui continua el algoritmo en Modo Manual...\r\n\r\n");
            while(1){}
        
        break;
        case ESTADO_AUT:
            // Enciende led rojo en forma permanente
            printf( "Desde aqui continua el algoritmo en Modo Automatico...\r\n\r\n");
            while(1){}
        break;
    }
}

////// Fin del programa
