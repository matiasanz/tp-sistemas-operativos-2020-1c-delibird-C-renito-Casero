#include "broker.h"

static void validar_header(t_paquete_header header);
static void validar_socket(int socket_servidor);

int main(void) {

	inicializar();

	while (true) {

		int *socket_cliente = malloc(sizeof(int));
		*socket_cliente = socket_aceptar_conexion(socket_servidor);

		//Debug
		//atender_cliente(socket_cliente);
		pthread_create(&hilo_receptor_mensajes, NULL, (void*) atender_cliente, socket_cliente);
		pthread_detach(hilo_receptor_mensajes);
	}

	return EXIT_SUCCESS;
}

void inicializar_colas() {

	id_univoco = 0;
	pthread_mutex_init(&mutex_id_univoco, NULL);

	id_colas = list_create();

	cola_new_pokemon = cola_crear(NEW_POKEMON);
	cola_appeared_pokemon = cola_crear(APPEARED_POKEMON);
	cola_catch_pokemon = cola_crear(CATCH_POKEMON);
	cola_caught_pokemon = cola_crear(CAUGHT_POKEMON);
	cola_get_pokemon = cola_crear(GET_POKEMON);
	cola_localized_pokemon = cola_crear(LOCALIZED_POKEMON);
}

void inicializar_hilos() {
	inicializar_hilo_consolidacion_colas();
}

void inicializar(void) {

	inicializar_config();
	inicializar_logs();
	inicializar_colas();
	inicializar_memoria();
	inicializar_servidor();
	inicializar_hilos();
	inicializar_senial();
	proceso_inicializar(BROKER);
}

void inicializar_config() {

	config = config_create("./config/broker.config");
}

t_log* log_crear(char*proposito, char*TOKEN);

void inicializar_logs() {
	logger = get_log_oficial(BROKER_STRING);
	event_logger = get_log_event("BROKER_EVENT");
	dump_logger = log_crear("DUMP_CACHE_LOGGER", "DUMP_CACHE_LOGGER");
}

void atender_cliente(int* socket) {

	t_paquete_header header = socket_recibir_header(*socket);
	validar_header(header);

	switch (header.codigo_operacion) {
	case MENSAJE: {
		procesar_mensaje(socket, header);
		break;
	}
	case SUSCRIPCION: {
		procesar_suscripcion(socket, header);
		break;
	}
	default:
		log_error_atender_cliente(socket, header);
	}
}

void inicializar_servidor() {

	socket_servidor = socket_crear_listener(config_get_string_value(config, "IP_BROKER"),
			config_get_string_value(config, "PUERTO_BROKER"));

	validar_socket(socket_servidor);
}

void inicializar_senial() {
	signal(SIGUSR1, log_dump_cache);
}

static void validar_header(t_paquete_header header) {

	if (error_conexion(header.codigo_operacion)) {
		log_error_conexion_proceso();
		pthread_exit(NULL);
	} else {
		log_conexion_proceso(header.proceso);
	}
}

static void validar_socket(int socket_servidor) {

	if (error_conexion(socket_servidor)) {
		log_error_inicio_proceso();
		exit(EXIT_FAILURE);
	}

	log_event_inicio_proceso();
}
