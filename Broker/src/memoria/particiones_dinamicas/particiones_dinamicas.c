/*
 * particiones_dinamicas.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */

#include "particiones_dinamicas.h"

//Funciones Privadas
static void consolidar(t_particion* victima, int index_victima);
static void liberar_victima(bool (*algoritmo_victima)(t_particion*, t_particion*));
static uint32_t eliminar_particiones_libres(bool* hay_fragmentacion_externa);

t_particion* pd_obtener_particion_libre_best_fit(int tamanio_contenido, int* offset) {

	/*Inicializa el minimo de fragnmentación con el tamaño total de la memoria
	 * Luego este valor se actualiza con el minimo real */
	uint32_t min_fragmentacion = tamanio_total_memoria;

	t_particion* particion = NULL;
	uint32_t index_candidato = 0;

	// Busca el mejor fit, que será el que genere una fragmentación mínima
	for (int i = 0; i < list_size(particiones); ++i) {

		t_particion* candidato = list_get(particiones, i);

		if (particion_es_best_fit(candidato, tamanio_contenido, min_fragmentacion)) {
			particion = candidato;
			index_candidato = i;
			min_fragmentacion = particion_get_tamanio(particion) - tamanio_contenido;
		}

		*offset += particion_get_tamanio(candidato);
	}

	if (!particion_encontrada(particion))
		return NULL;

	return particion_con_tamanio_exacto(particion, tamanio_contenido, index_candidato);
}

t_particion* pd_obtener_particion_libre_first_fit(int tamanio_contenido, int* offset) {

	t_particion* particion = NULL;
	uint32_t index_candidato = 0;

	/*Busca la primer particion libre con tamaño suficiente dado el
	 / tamaño del contenido que recibe por parámetro*/
	for (int i = 0; i < list_size(particiones); ++i) {

		t_particion* candidato = list_get(particiones, i);
		if (particion_tiene_tamanio_suficiente(candidato, tamanio_contenido)) {
			particion = candidato;
			index_candidato = i;
			break;
		}

		*offset += particion_get_tamanio(candidato);
	}

	if (!particion_encontrada(particion))
		return NULL;

	return particion_con_tamanio_exacto(particion, tamanio_contenido, index_candidato);
}

void pd_liberar_victima_fifo() {

	liberar_victima((void*) fecha_creacion_anterior);
}

void pd_liberar_victima_lru() {

	liberar_victima((void*) lru_anterior);
}

void compactar_memoria() {

	//Reset contador de busquedas fallidas para futuras compactaciones
	cantidad_busquedas_fallidas = 0;
	/*Elimina todas las particiones libres y devuelve el tamaño total ocupado
	 *En caso de detectar fragmetnación pone en true el bool hay_fragmentacion_externa*/
	bool hay_fragmentacion_externa = false;
	int tamanio_ocupado = eliminar_particiones_libres(&hay_fragmentacion_externa);

	//Si la memoria ya está compactada no tiene sentido volver a compactar
	if (!hay_fragmentacion_externa)
		return;

	/* Copia el contenido de las particiones ocupadas a un buffer y actualiza las direcciones de memoria
	 * de forma tal que queden todas al inicio. */
	reubicar_particiones_y_compactar_contenido(tamanio_ocupado);
	//Log obligatorio pedido en el enunciado
	log_ejecucion_compactacion();
}

bool hay_particiones_ocupadas() {

	t_particion* p = list_find(particiones, (void*) particion_esta_ocupada);
	return particion_encontrada(p);
}

static uint32_t eliminar_particiones_libres(bool* hay_fragmentacion_externa) {

	int tamanio_ocupado = 0;

	for (int i = 0; i < list_size(particiones); ++i) {

		t_particion* p = list_get(particiones, i);

		if (particion_esta_libre(p)) {
			list_remove_and_destroy_element(particiones, i, (void*) particion_destruir);
			i--;
			*hay_fragmentacion_externa = true;
		} else {
			tamanio_ocupado += particion_get_tamanio(p);
		}
	}

	/* Incrementa el tamaño disponible para generar nuevas particiones
	 * dado que se fueron borrando particiones libre */
	memoria_actualizar_tamanio_disponible(tamanio_ocupado - tamanio_total_memoria);
	return tamanio_ocupado;
}

void reubicar_particiones_y_compactar_contenido(uint32_t tamanio_ocupado) {

	void* buffer = malloc(tamanio_ocupado);
	int offset = 0;

	/* Llena el buffer con el contenido de las particiones ocupadas
	 * El contenido se coloca de forma contigua*/
	for (int i = 0; i < list_size(particiones); ++i) {

		t_particion* p = list_get(particiones, i);

		int tamanio = particion_get_tamanio(p);
		memcpy(buffer + offset, particion_get_direccion_contenido(p), tamanio);

		particion_recalcular_direcciones(p, offset);

		offset += tamanio;
	}

	//pisa la memoria con el buffer, luego destruye el buffer
	memcpy(memoria_principal, buffer, tamanio_ocupado);
	free(buffer);
}

static void liberar_victima(bool (*algoritmo_victima)(t_particion*, t_particion*)) {

	int index_victima = 0;
	t_link_element* item = particiones->head;

	//Busca indice de la primer particion ocupada
	while (item != NULL && particion_esta_libre(item->data)) {
		item = item->next;
		index_victima += 1;
	}

	//Si todas las particiones están vacias, no tiene sentido buscar víctima
	if (item == NULL) return;

	//Se inicializa la victima con la primer particion ocupada encontrada
	t_particion* victima = list_get(particiones, index_victima);

	/*Recorre la lista y si aparece un mejor candidato a liberar
	 *segun el algoritmo de reemplazo, pisa la victima */
	for (int i = index_victima; i < list_size(particiones); ++i) {

		t_particion* candidato = list_get(particiones, i);

		if (algoritmo_victima(victima, candidato)) {
			victima = candidato;
			index_victima = i;
		}
	}

	cola_eliminar_mensaje(particion_get_id_mensaje(victima), particion_get_id_cola(victima));
	consolidar(victima, index_victima);
}

static void consolidar(t_particion* victima, int index_victima) {

	// Completa las variables anterior/sgte solo si existen particiones anteriores o siguientes
	t_particion* anterior = index_victima > 0 ? list_get(particiones, index_victima - 1) : NULL;
	t_particion* sgte = index_victima <= list_size(particiones) ? list_get(particiones, index_victima + 1) : NULL;

	/* Si la partición anterior está libre, la consolida con la victima
	 * Luego elimina la victima de la lista */
	if (anterior != NULL && particion_esta_libre(anterior)) {
		list_replace(particiones, index_victima - 1, particion_fusionar(anterior, victima));
		list_remove_and_destroy_element(particiones, index_victima, (void*) particion_destruir);
		/* Los siguientes cambios de indice y reasignación son necesarios en el caso de que
		 se requiera volver a consolidar con el sgte*/
		index_victima -= 1;
		victima = anterior;
	}

	/* Si la partición siguiente está libre, la consolida con la victima
	 * Luego elimina la sgte de la lista */
	if (sgte != NULL && particion_esta_libre(sgte)) {
		list_replace(particiones, index_victima, particion_fusionar(victima, sgte));
		list_remove_and_destroy_element(particiones, index_victima + 1, (void*) particion_destruir);
	}
}
