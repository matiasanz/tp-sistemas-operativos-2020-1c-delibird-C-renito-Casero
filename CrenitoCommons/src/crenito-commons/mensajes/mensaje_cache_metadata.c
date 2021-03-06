/*
 * mensaje_metadata.c
 *
 *  Created on: 13 jun. 2020
 *      Author: utnso
 */

#include "mensaje_cache_metadata.h"

t_mensaje_cache_metadata* mensaje_metadata_crear() {

	t_mensaje_cache_metadata* metadata = malloc(sizeof(t_mensaje_cache_metadata));

	metadata->id_correlativo = 0;
	metadata->suscriptores_confirmados = list_create();
	metadata->suscriptores_enviados = list_create();
	metadata->suscriptores_fallidos = list_create();
	metadata->tamanio_contenido = 0;

	return metadata;
}

void mensaje_metadata_destruir(t_mensaje_cache_metadata* msj) {

	list_destroy_and_destroy_elements(msj -> suscriptores_confirmados, (void*)suscriptor_destruir);
	list_destroy_and_destroy_elements(msj -> suscriptores_enviados, (void*)suscriptor_destruir);
	list_destroy_and_destroy_elements(msj -> suscriptores_fallidos, (void*)suscriptor_destruir);
	free(msj);
}

void mensaje_cache_metadata_set_id_correlativo(t_mensaje_cache_metadata* metadata, uint32_t id_correlativo) {
	metadata ->id_correlativo = id_correlativo;
}
