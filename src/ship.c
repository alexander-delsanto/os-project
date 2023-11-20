#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#include "include/const.h"
#include "include/shm_config.h"
#include "include/shm_port.h"
#include "include/shm_ship.h"
#include "include/utils.h"

#define GET_DISTANCE(dest)\
	(sqrt(pow(dest.x - get_ship_coords(_this_id).x, 2) + pow(dest.y - get_ship_coords(_this_id).y, 2)))

void signal_handler(int signal);
struct sigaction *signal_handler_init(void);

void init_location(void);
void move(struct coordinates destination_port);

void close_all(void);
void loop(void);
void find_new_destination(int *port_id, struct coordinates *coords);
void trade(int id_port);

struct state {
	int id;
	shm_config_t *config;
	shm_port_t *port;
	shm_ship_t *ship;
};

struct state state;

int main(int argc, char *argv[])
{
	struct sigaction *sa;

	sa = signal_handler_init();

	state.id = (int)strtol(argv[1], NULL, 10);
	state.config = config_shm_attach();
	state.port = port_shm_attach(state.config);
	state.ship = ship_shm_attach(state.config);
	init_location();

	while (1) {
		sleep(1);
	}

	return EXIT_SUCCESS;
}

struct sigaction *signal_handler_init(void)
{
	static struct sigaction sa;

	sigset_t mask;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;

	sigfillset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	sigemptyset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGDAY, &sa, NULL);
	sigaction(SIGSTORM, &sa, NULL);
	sigaction(SIGMAELSTROM, &sa, NULL);

	return &sa;
}

void loop(void) {
	int id_dest_port;
	struct coordinates destination_coords;
	while (1) {
		find_new_destination(&id_dest_port, &destination_coords);
		move(destination_coords);
		trade(id_dest_port);
	}
}

/**
 * @brief initializes ship's location.
 */
void init_location(void)
{
	struct coord coords;
	/* generate a random location on the map */
	srand(time(NULL) * getpid());
	coords.x = RANDOM_DOUBLE(0, get_lato(state.config));
	coords.y = RANDOM_DOUBLE(0, get_lato(state.config));

	ship_shm_set_coordinates(state.ship, state.id, coords);
	ship_shm_set_is_moving(state.ship, state.id, TRUE);
}

/**
 * @brief simulates the movement of the ship and updates the location.
 */
void move(struct coordinates destination_port)
{
	double distance;
	double time_required;
	set_ship_is_moving(_this_id, TRUE);
	/* calculate distance between actual position and destination */
	distance = GET_DISTANCE(dest);
	/* calculate time required to arrive (in days) */
	time_required = distance / SO_SPEED;
	convert_and_sleep(time_required);
	/* set new location */
	set_ship_coords(_this_id, destination_port);
	set_ship_is_moving(_this_id, FALSE);
}

void find_new_destination(int *port_id, struct coordinates *coords)
{
	/* TODO */
}

void trade(int id_port)
{
	/* TODO */
}

void signal_handler(int signal)
{
	switch (signal) {
	case SIGDAY:
		dprintf(1, "Ship %d: Received SIGDAY signal. Current day: %d\n",
			state.id, get_current_day(state.config));
		/* TODO */
		break;
	case SIGSTORM:
		dprintf(1, "Ship %d: Received SIGSTORM signal.\n", state.id);
		/* TODO */
		break;
	case SIGMAELSTROM:
		dprintf(1, "Ship %d: Received SIGMAELSTROM signal.\n",
			state.id);
		ship_shm_set_dead(state.ship, state.id);
		close_all();
	case SIGSEGV:
		dprintf(1, "Received SIGSEGV signal.\n");
		dprintf(2, "ship.c: id: %d: Segmentation fault. Terminating.\n",
			state.id);
	case SIGINT:
		close_all();
	}
}

void close_all(void)
{
	port_shm_detach(state.port);
	ship_shm_detach(state.ship);
	config_shm_detach(state.config);
	exit(0);
}
