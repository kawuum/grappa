//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#include "defs.h"

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);


int main(int argc, char* argv[]) {
	parseOptions(argc, argv);
	setupParams(SCALE, 8);
	
	printf("[[ Graph Application Suite ]]\n"); 	
	
	printf("generating random edgelist data\n");
	edgelist ing;
	genScalData(&ing, A, B, C, D);
	
	
	free_edgelist(&ing);
	
	return 0;
}

static void printHelp(const char * exe) {
	printf("Usage: %s [options]\nOptions:\n", exe);
	printf("  --help,h    Prints this help message displaying command-line options\n");
	printf("  --scale,s  Scale of the graph: 2^SCALE vertices.\n");
	exit(0);
}

static void parseOptions(int argc, char ** argv) {
	struct option long_opts[] = {
		{"help", no_argument, 0, 'h'},
		{"scale", required_argument, 0, 's'},
		{"dot", required_argument, 0, 'd'}
	};
	
	SCALE = 1; //default value
	dotfile = NULL;
	
	int c = 0;
	while (c != -1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "hs", long_opts, &option_index);
		switch (c) {
			case 'h':
				printHelp(argv[0]);
				exit(0);
			case 's':
				SCALE = atoi(optarg);
				break;
			case 'd':
				dotfile = fopen(optarg,"w");
				break;
		}
	}
}

