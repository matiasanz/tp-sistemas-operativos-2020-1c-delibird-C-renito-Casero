/*
 * conexiones.c
 *
 *  Created on: 30 abr. 2020
 *      Author: utnso
 */

#include "conexiones.h"

/*
 * Subscribe a los distintos procesos con el Broker.
 */
int suscribir(t_conexion_server* server, t_conexion_cliente* cliente);

/*
 * Envía un paquete usando el socket indicado por parámetro
 * y se queda a la espera una respuesta de tipo entero
 * la cual retorna
 */
static int handshake(int socket, void* pqt, int size);

int enviar(t_conexion_server* server, t_paquete* pqt) {

	int socket = socket_crear_client(server->ip, server->puerto);

	if (error_conexion(socket)) {
		return ERROR_SOCKET;
	}

	int id_mensaje = enviar_paquete(pqt, socket);

	socket_cerrar(socket);

	return id_mensaje;
}

int enviar_paquete(t_paquete* pqt, int socket) {

	int bytes = 0;
	void* a_enviar = paquete_serializar(pqt, &bytes);

	int id_mensaje = handshake(socket, a_enviar, bytes);
	free(a_enviar);

	return id_mensaje;
}

//*****************************************************************************
void mantener_suscripcion(t_conexion* conexion, void (*procesar_fallo)(t_conexion*)){

	t_conexion_cliente* cliente = conexion->cliente;
	t_suscriptor* subscriptor = cliente->suscriptor;

	while (1) {

		int estadoSuscripcion = recibir(subscriptor->socket, cliente->callback);

		if (error_conexion(estadoSuscripcion)) {

			procesar_fallo(conexion);

			break;
		};

	}
}

//int conexion_get_tiempo_reintento(t_conexion*conexion){
//	return conexion->cliente->segundos_reconexion;
//}

//TODO
//void intentar_reconectar(t_conexion* conexion){
//	uint32_t TIEMPO_RECONEXION = conexion;
//	int estado_suscripcion;
//
//	do{
//
//		sleep(TIEMPO_RECONEXION);
//
//		log_info(logger, "Inicio reintento de conexión");
//
//		pthread_mutex_lock(&mutex_subscripcion);
//		estado_suscripcion = subscribir(conexion->server, conexion->cliente);
//		pthread_mutex_unlock(&mutex_subscripcion);
//
//		log_info(logger, "Resultado del reintento de conexión: %s", (error_conexion(estado_suscripcion)? "Fallido": "Exitoso"));
//
//	} while(error_conexion(estado_suscripcion));
//
//}


//**************************************************************************

void suscribir_y_escuchar_cola(t_conexion* args) {

	pthread_mutex_lock(&mutex_subscripcion);
	int estado_subscripcion = suscribir(args->server, args->cliente);
	pthread_mutex_unlock(&mutex_subscripcion);

	t_conexion_cliente* cliente = args->cliente;

	//TO-DO reconectar
	if (error_conexion(estado_subscripcion)) {
		log_warning_suscripcion(cliente->id_cola);
		return;
	}

	t_suscriptor* suscriptor = cliente->suscriptor;

	while (1) {

		if (error_conexion(recibir(suscriptor_get_socket(suscriptor), cliente->callback))) {
			log_warning_conexion_perdida(cliente->id_cola);
			break;
		};

	}
}

static int handshake(int socket, void* pqt, int size) {

	socket_send(socket, pqt, size);
	return socket_recibir_int(socket);
}

void conectar_y_escuchar_gameboy(t_conexion_host* gameboy) {

	int socket_server = socket_crear_listener(gameboy->ip, gameboy->puerto);

	while (1) {

		int socket_cliente = socket_aceptar_conexion(socket_server);
		recibir(socket_cliente, gameboy->callback);
	}

}

int suscribir(t_conexion_server* server, t_conexion_cliente* cliente) {

	int socket = socket_crear_client(server->ip, server->puerto);
	suscriptor_set_socket(cliente -> suscriptor, socket);

	//TO-DO reconectar
	if (error_conexion(suscriptor_get_socket(cliente->suscriptor))) {
		log_warning_broker_desconectado(cliente->id_cola);
		return ERROR_SOCKET;
	}

	t_paquete_header pqt = paquete_header_crear(SUSCRIPCION,
			server->tipo_proceso, cliente->id_cola, suscriptor_get_id_proceso(cliente ->suscriptor));

	return handshake(socket, &pqt, sizeof(t_paquete_header));
}

int recibir(int socket, void (*callback)(t_id_cola, void*)) {

	t_paquete_header header = socket_recibir_header(socket);

	if (error_conexion(header.codigo_operacion)) {
		return ERROR_SOCKET;
	}

	int size = 0;
	void* msj = socket_recibir_mensaje(socket, &size);

	if (error_conexion(size)) {
		return ERROR_SOCKET;
	}

	uint32_t ACK = 1;
	socket_send(socket, &ACK, sizeof(ACK));

	callback(header.id_cola, msj);

	return EXIT_SUCCESS;
}
