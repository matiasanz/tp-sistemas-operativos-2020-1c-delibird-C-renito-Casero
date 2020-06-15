/*
 * gamecard.h
 *
 *  Created on: 30 abr. 2020
 *      Author: utnso
 */

#ifndef SRC_GAMECARD_H_
#define SRC_GAMECARD_H_

#include <crenito-commons/conexiones/conexiones.h>
#include "utils/mensajes-utils.h"


#include <commons/string.h>
#include <commons/bitarray.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define tope(x,y) (((x)+(y)-1)/(y))

typedef enum{METADATA,FILES,BLOCKS} e_key_path_estructura;

//---variables Globales-----------

t_config* config_metadata;
t_config* config_metadata_directorio_files;

int tiempo_de_reintento_operacion;
int tiempo_retardo_operacion;
char* punto_montaje_tallgrass;

t_bitarray* bitmap;
FILE* f_bitmap;

char* paths_estructuras[3]; //e_key_path_estructura

pthread_mutex_t mutBitarray;
t_dictionary* semaforosDePokemons; //key:char* especiePokemon, value: pthread_mutex_t* mutex
//-------------------------

void inicializar();
void inicializar_config();
void inicializar_logs();
void inicializar_conexiones();
void subscribir_y_escuchar_cola_catch_pokemon();
void subscribir_y_escuchar_cola_get_pokemon();
void subscribir_y_escuchar_cola_new_pokemon();


void crearEstructuras();
void gamecard_New_Pokemon(t_mensaje_new_pokemon* unMsjNewPoke);
void gamecard_Catch_Pokemon(t_mensaje_appeared_catch_pokemon* unMsjCatchPoke);
void gamecard_Get_Pokemon(t_mensaje_get_pokemon* unMsjGetPoke);

void gamecard_New_Pokemon_ReIntento(t_mensaje_new_pokemon* unMsjNewPoke);


int cant_elemetos_array(char** array);
void split_liberar(char** split);
int bloque_disponible(t_bitarray* bitmap,int totalBloques);
void guardarLinea(char* path,char* nuevalinea,int len);
char* crearLinea(t_mensaje_new_pokemon* unMsjNewPoke );
int size_bloque(char* path);
bool contiene_string_en_bloques(char* string, char**bloques);
int espacioDisponibleEnBloque(char* path);
int bloquesNecesarios(char* lineaNueva,int maxSizeBloque);

#endif /* SRC_GAMECARD_H_ */
