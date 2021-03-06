/*
 * broker_utils.h
 *
 *  Created on: 16 jun. 2020
 *      Author: utnso
 */

#ifndef SRC_UTILS_BROKER_UTILS_H_
#define SRC_UTILS_BROKER_UTILS_H_

#include <crenito-commons/conexiones/conexiones.h>

pthread_mutex_t mutex_id_univoco;
uint32_t id_univoco;
uint32_t generar_id_univoco();

//timeval utils
long get_fecha_en_microsegundos(struct timeval fecha);
void string_append_timestamp(char** string);
t_mensaje_header mensaje_get_header(void* mensaje, t_id_cola id_cola);
void mensaje_destruir(void* deserializado, t_id_cola id_cola);
void mensaje_set_header(void* mensaje, t_mensaje_header header, t_id_cola id_cola);

//math utils
uint32_t siguiente_potencia_de_dos(uint32_t un_numero);
uint32_t logaritmo_base_dos(uint32_t un_numero);
uint32_t calcular_base_dos(uint32_t un_numero);

#endif /* SRC_UTILS_BROKER_UTILS_H_ */
