/*
 ============================================================================
 Name        : Team.c
 Author      : C-renito Casero
 Version     : Checkpoint
 Description : Proceso Team
 ============================================================================
 */

#include "team.h"
#include "hilos-del-team/hilos_team.h"
//#include "tests/tests_team.o"

//funciones team_log_info para logs importantes TODO

int main(void) {

	team_inicializar();

	Get_pokemones(objetivosGlobales, inventariosGlobales);

	//Para pruebas sin los otros modulos//
//	pthread_create(&hiloReceptorDeMensajes, NULL, (void*) broker_simulator, NULL);

	inicializar_hilos();

	team_ejecutar_algoritmo_de_deteccion_de_deadlock();


	sem_wait(&FinDePlanificacion);

	team_loggear_resultados();

	log_info(logger, "\n\n                              Fin del proceso Team\n"
						      "****************************************************************************");

	return team_exit();
}
																/*Fin del main*/
//**********************************************************************************

						/*Implementacion de funciones*/

void team_inicializar(){

	inicializar_logs_y_config();

	inicializar_listas();

	cantidadDeEntrenadores = list_size(equipo);

	inicializar_estadisticas(cantidadDeEntrenadores);

	cargar_algoritmo_planificacion();

 	inicializar_semaforos();

	log_info(logger, "\n\n*************************************************************************\n"
         		     "                        Inicio del proceso Team\n\n");

	inicializar_conexiones();

//	inicializar_hilos();

	bool inicio_exitoso = config && event_logger ;
	if(!inicio_exitoso){
		error_show("algo malio sal en el inicio :(");
		exit(1);
	}
}

int team_exit(){

	finalizar_estadisticas();
	finalizar_logs_y_config();
	listas_destroy();
	finalizar_hilos();
	finalizar_semaforos();

	return EXIT_SUCCESS;
}

/***********************************Funciones auxiliares *************************************/

//Logs y config
void inicializar_logs_y_config(){
	config=config_create(CONFIG_PATH);
	char*TEAM_LOG_PATH = config_get_string_value(config,"LOG_FILE");
	logger=log_create(TEAM_LOG_PATH,"TEAM",true,LOG_LEVEL_INFO);
	event_logger = log_create("log/team_event.log", "TEAM_EVENT", true, LOG_LEVEL_INFO);
}

void team_loggear_resultados(){

	char*resultados = estadisticas_to_string(Estadisticas);

	pthread_mutex_lock(&Mutex_AndoLoggeando);
	log_info(logger, "\n*************** Resultados del Team ****************\n\n%s", resultados);
	pthread_mutex_unlock(&Mutex_AndoLoggeando);

	free(resultados);
}

void finalizar_logs_y_config(){
	config_destroy(config);
	log_destroy(logger);
	log_destroy(event_logger);
}

//Estadisticas
void inicializar_estadisticas(numero cantidadDeProcesos){

	numero*ciclosPorEntrenador = malloc(sizeof(numero)*cantidadDeProcesos);

	int i=0;
	for(i=0; i<cantidadDeProcesos; i++){
		ciclosPorEntrenador[i]=0;
	}

	Estadisticas = (estadisticas_team) {0,0,ciclosPorEntrenador, 0, 0};
}

void finalizar_estadisticas(){
	free(Estadisticas.ciclosDelEntrenador);
}

char*estadisticas_to_string(estadisticas_team resultados){
	char* string = string_new();

	char*ciclosDeCadaEntrenador = num_array_to_string(Estadisticas.ciclosDelEntrenador, cantidadDeEntrenadores);

	string_append_with_format(&string, ">> Total de Cambios de Contexto ..... %u\n", resultados.cambiosDeContexto);
	string_append_with_format(&string, ">> Ciclos de CPU por entrenador:\n%s", ciclosDeCadaEntrenador);
	string_append_with_format(&string, ">> Total de Ciclos de CPU ........... %u\n", resultados.ciclosCPU);
	string_append_with_format(&string, ">> Deadlocks producidos ............. %u\n", resultados.deadlocksProducidos);
	string_append_with_format(&string, ">> Deadlocks resueltos .............. %u\n", resultados.deadlocksResueltos);

	free(ciclosDeCadaEntrenador);

	return string;
}

char*num_array_to_string(numero*arreglo, int length){
	char* stringFromArray = string_new();

	numero i;
	for(i=0; i<cantidadDeEntrenadores; i++){
		string_append_with_format(&stringFromArray,"  . Entrenador N°%u .................. %u\n", i, arreglo[i]);
	}

	return stringFromArray;
}

//Listas
void inicializar_listas() {
	equipo = entrenadores_cargar();
	objetivosGlobales = entrenadores_objetivos_globales(equipo);
	inventariosGlobales = entrenadores_inventarios_globales(equipo);
	recursosEnMapa = recursos_create();
	pokemonesRecibidos = mapa_create();
	capturasPendientes = pendientes_create();
	potencialesDeadlock = candidatos_create();

	//Ver si vale la pena abstraer las listas
	historialDePokemones = list_create();
	pokemonesDeRepuesto = list_create();
	entrenadoresReady = cr_list_create();
	registroDePedidos = list_create();

	mensajesCAUGHT    = cr_list_create();
	mensajesLOCALIZED = cr_list_create();

	//***************
}

void listas_destroy(){
	cr_list_destroy(entrenadoresReady);

	cr_list_clean_and_destroy_elements(pokemonesRecibidos, (void*) pokemon_destroy); //TODO, ver donde hago post(hayEntrenadores) porque no deberia romper
	pendientes_destroy(capturasPendientes);
	recursos_destroy(objetivosGlobales);
	recursos_destroy(inventariosGlobales);
	recursos_destroy(recursosEnMapa);
	candidatos_destroy(potencialesDeadlock);
	entrenadores_destroy(equipo);

	pthread_mutex_lock(&mutexPedidos);
	list_clean_and_destroy_elements(registroDePedidos, free);
	pthread_mutex_unlock(&mutexPedidos);

	pthread_mutex_lock(&mutexHistorialEspecies);
	list_clean_and_destroy_elements(historialDePokemones, free);
	pthread_mutex_unlock(&mutexHistorialEspecies);

	list_destroy_and_destroy_elements(pokemonesDeRepuesto, (void*) pokemon_destroy_hard);
	cr_list_clean_and_destroy_elements(pokemonesRecibidos, (void*) pokemon_destroy);
	cr_list_clean_and_destroy_elements(mensajesCAUGHT    , (void*) mensaje_caught_pokemon_destruir);
	cr_list_clean_and_destroy_elements(mensajesLOCALIZED , (void*) mensaje_localized_pokemon_destruir);
}

//Hilos
void inicializar_hilos(){
	inicializar_hilos_entrenadores();

	pthread_create(&hiloMensajesCAUGHT   , NULL, (void*)team_procesador_cola_CAUGHT   , mensajesCAUGHT);
	pthread_create(&hiloMensajesLOCALIZED, NULL, (void*)team_procesador_cola_LOCALIZED, mensajesLOCALIZED);

	pthread_create(&hiloPlanificador, NULL, (void*) team_planificar, NULL);

	pthread_create(&hiloProcesadorDePokemones, NULL, (void*) team_procesar_pokemones, NULL);
}

void finalizar_hilos(){
	finalizar_hilos_entrenadores();

	pthread_join(hiloPlanificador	   , NULL);
	pthread_join(hiloMensajesAppeard   , NULL);
	pthread_join(hiloMensajesCAUGHT	   , NULL);
	pthread_join(hiloMensajesLOCALIZED , NULL);
}

//Semaforos

void inicializar_semaforos(){
	sem_init(&HayTareasPendientes       , 0, 0);
	sem_init(&HayEntrenadoresDisponibles, 0, 0);
	sem_init(&EquipoNoPuedaCazarMas     , 0, 0);
	sem_init(&FinDeCiclo_CPU            , 0, 0);
	sem_init(&finDeIntercambio          , 0, 0);
	sem_init(&FinDePlanificacion        , 0, 0);


	pthread_mutex_init(&Mutex_AndoLoggeando       , NULL);
	pthread_mutex_init(&Mutex_AndoLoggeandoEventos, NULL);

	pthread_mutex_init(&mutexEntrenadores         , NULL);
	pthread_mutex_init(&mutexHistorialEspecies    , NULL);
	pthread_mutex_init(&mutexRepuestos            , NULL);
//	pthread_mutex_init(&mutexInventariosGlobales  , NULL);

	pthread_mutex_init(&mutexRecursosDisponibles  , NULL);
	pthread_mutex_init(&mutexPedidos              , NULL);
}

void finalizar_semaforos(){
	sem_destroy(&HayTareasPendientes);
	sem_destroy(&HayEntrenadoresDisponibles);
	sem_destroy(&FinDeCiclo_CPU);
	sem_destroy(&finDeIntercambio);
	sem_destroy(&FinDePlanificacion);

	pthread_mutex_destroy(&mutexEntrenadores);
//	pthread_mutex_destroy(&mutexInventariosGlobales);
	pthread_mutex_destroy(&mutexHistorialEspecies);
	pthread_mutex_destroy(&Mutex_AndoLoggeando);
	pthread_mutex_destroy(&Mutex_AndoLoggeandoEventos);
}

//Conexiones
void inicializar_conexiones() {

	id_proceso = config_get_int_value(config, "ID_PROCESO");

	conexion_broker = conexion_server_crear(
			config_get_string_value(config, "IP_BROKER"),
			config_get_string_value(config, "PUERTO_BROKER"), TEAM);

	pthread_mutex_init(&mutex_subscripcion, NULL);

	suscribir_y_escuchar_cola_appeared_pokemon((void*) mensaje_recibido);
	suscribir_y_escuchar_cola_caught_pokemon((void*) mensaje_recibido);
	suscribir_y_escuchar_cola_localized_pokemon((void*) mensaje_recibido);

	conectar_gameboy((void*) mensaje_recibido);
}
