/*
 * mensaje_get_pokemon.c
 *
 *  Created on: 24 may. 2020
 *      Author: utnso
 */

#include "mensaje_get_pokemon.h"

t_mensaje_get_pokemon* mensaje_get_pokemon_crear(char* especie) {

	t_mensaje_get_pokemon* get_pokemon = malloc(sizeof(t_mensaje_get_pokemon));

	mensaje_header_inicializar(&get_pokemon->mensaje_header);

	get_pokemon->especie_lenght = strlen(especie) + 1;
	get_pokemon->especie = malloc(get_pokemon->especie_lenght);
	strcpy(get_pokemon->especie, especie);

	return get_pokemon;

}

void mensaje_get_pokemon_destruir(t_mensaje_get_pokemon* get_pokemon) {
	free(get_pokemon->especie);
	free(get_pokemon);
}

t_buffer* mensaje_get_pokemon_serializar(t_mensaje_get_pokemon* get_pokemon) {

	int size = sizeof(get_pokemon->mensaje_header) 
	         + sizeof(get_pokemon->especie_lenght)
			 + get_pokemon->especie_lenght;

	t_buffer* bfr = buffer_crear(size);
	int desplazamiento = 0;

	// header
	memcpy(bfr->stream + desplazamiento, &(get_pokemon->mensaje_header),
			sizeof(get_pokemon->mensaje_header));
	desplazamiento += sizeof(get_pokemon->mensaje_header);

	//especie_lenght
	memcpy(bfr->stream + desplazamiento, &(get_pokemon->especie_lenght),
			sizeof(get_pokemon->especie_lenght));
	desplazamiento += sizeof(get_pokemon->especie_lenght);

	//especie
	memcpy(bfr->stream + desplazamiento, get_pokemon->especie,
			get_pokemon->especie_lenght);
	desplazamiento += get_pokemon->especie_lenght;

	return bfr;
}

t_mensaje_get_pokemon* mensaje_get_pokemon_deserializar(void* stream) {

	t_mensaje_get_pokemon* msj = malloc(sizeof(t_mensaje_get_pokemon));
	int desplazamiento = 0;

	// header
	memcpy(&msj->mensaje_header, stream + desplazamiento, sizeof(msj->mensaje_header));
	desplazamiento += sizeof(msj->mensaje_header);

	//especie_lenght
	memcpy(&msj->especie_lenght, stream + desplazamiento,
			sizeof(msj->especie_lenght));
	desplazamiento += sizeof(msj->especie_lenght);

	//especie
	char* especie = strdup(stream + desplazamiento);
	msj->especie = especie;
	desplazamiento += msj->especie_lenght;

	free(stream);

	return msj;

}

// Getters
uint32_t mensaje_get_pokemon_get_id(t_mensaje_get_pokemon* msj) {
	return msj->mensaje_header.id;
}

uint32_t mensaje_get_pokemon_get_id_correlativo(t_mensaje_get_pokemon* msj) {
	return msj->mensaje_header.id_correlativo;
}

//Setters
void mensaje_get_pokemon_set_id(t_mensaje_get_pokemon* msj, uint32_t id) {
	msj->mensaje_header.id = id;
}

void mensaje_get_pokemon_set_id_correlativo(t_mensaje_get_pokemon* msj,
		uint32_t id_correlativo) {
	msj->mensaje_header.id_correlativo = id_correlativo;
}