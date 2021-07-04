/*   Foma: a finite-state toolkit and library.                                 */
/*   Copyright © 2008-2021 Mans Hulden                                         */

/*   This file is part of foma.                                                */

/*   Licensed under the Apache License, Version 2.0 (the "License");           */
/*   you may not use this file except in compliance with the License.          */
/*   You may obtain a copy of the License at                                   */

/*      http://www.apache.org/licenses/LICENSE-2.0                             */

/*   Unless required by applicable law or agreed to in writing, software       */
/*   distributed under the License is distributed on an "AS IS" BASIS,         */
/*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/*   See the License for the specific language governing permissions and       */
/*   limitations under the License.                                            */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "fomalib.h"

#define LINE_LIMIT 262144
#define UDP_MAX 65535
#define FLOOKUP_PORT 6062

static char *usagestring = "Usage: flookup <binary foma file>\n";

static char *helpstring =
"Applies words from stdin to a foma transducer/automaton read from a file and prints results to stdout.\n";

struct lookup_chain {
    struct fsm *net;
    struct apply_med_handle *ah;
    struct lookup_chain *next;
    struct lookup_chain *prev;
};

#define DIR_DOWN 0
#define DIR_UP 1

static struct sockaddr_in serveraddr, clientaddr;
static int                listen_sd, numbytes;
static socklen_t          addrlen;

static char buffer[2048];
static int  echo = 1, apply_alternates = 0, numnets = 0, direction = DIR_UP, results, buffered_output = 1, index_arcs = 0, index_flag_states = 0, index_cutoff = 0, index_mem_limit = INT_MAX, mode_server = 0, port_number = FLOOKUP_PORT, udpsize;
static char *separator = "\t", *wordseparator = "\n", *server_address = NULL, *line, *serverstring = NULL;
static FILE *INFILE;
static struct lookup_chain *chain_head, *chain_tail, *chain_new, *chain_pos;
static fsm_read_binary_handle fsrh;

static char *(*applyer)() = &apply_med;  /* Default apply direction = up */
static void handle_line(char *s);
static void app_print(char *result);
static char *get_next_line();

void app_print(char *result) {

    if (!mode_server) {
	if (echo == 1) {
	    fprintf(stdout, "%s%s",line, separator);
	}
	if (result == NULL) {
	    fprintf(stdout,"+?\n");
	} else {
	    fprintf(stdout, "%s\n", result);
	}
    }
}

int main(int argc, char *argv[]) {
    int opt, sortarcs = 1;
    char *infilename;
    struct fsm *net;

    setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

  while ((opt = getopt(argc, argv, "abhHimI:qs:SA:P:w:vx")) != -1) {
   switch(opt) {
	case 'h':
	printf("%s%s\n", usagestring,helpstring);
		exit(0);
	case 'w':
	    wordseparator = strdup(optarg);
	    break;
    case 'v':
	    printf("flookup 1.03 (foma library version %s)\n", fsm_get_library_version_string());
	    exit(0);
	default:
            fprintf(stderr, "%s", usagestring);
            exit(EXIT_FAILURE);
		}
    }



    if (optind == argc) {
		fprintf(stderr, "%s", usagestring);
		exit(EXIT_FAILURE);
    }

    infilename = argv[optind];

    if ((fsrh = fsm_read_binary_file_multiple_init(infilename)) == NULL) {
        perror("File error");
		exit(EXIT_FAILURE);
    }

    chain_head = chain_tail = NULL;

    while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
		numnets++;
		chain_new = malloc(sizeof(struct lookup_chain));
		// if (direction == DIR_DOWN && net->arcs_sorted_in != 1 && sortarcs) {
		// 	fsm_sort_arcs(net, 1);
		// }
		// if (direction == DIR_UP && net->arcs_sorted_out != 1 && sortarcs) {
		// 	fsm_sort_arcs(net, 2);
		// }
		chain_new->net = net;
		chain_new->ah = apply_med_init(net);
		// apply_med_set_heap_max(chain_new->ah, 4194304);    /* Don't grow heap more than this            */
		// apply_med_set_med_limit(chain_new->ah, 10);       /* Don't search for matches with cost > 10   */
		// apply_med_set_med_cutoff(chain_new->ah, 5);       /* Don't return more than 5 matches          */

		// if (direction == DIR_DOWN && index_arcs) {
		// 	apply_index(chain_new->ah, APPLY_INDEX_INPUT, index_cutoff, index_mem_limit, index_flag_states);
		// }
		// if (direction == DIR_UP && index_arcs) {
		// 	apply_index(chain_new->ah, APPLY_INDEX_OUTPUT, index_cutoff, index_mem_limit, index_flag_states);
		// }

		chain_new->next = NULL;
		chain_new->prev = NULL;
		if (chain_tail == NULL) {
			chain_tail = chain_head = chain_new;
		} else {
			chain_new->next = chain_head;
			chain_head->prev = chain_new;
			chain_head = chain_new;
		}
    }

    if (numnets < 1) {
		fprintf(stderr, "%s: %s\n", "File error", infilename);
		exit(EXIT_FAILURE);
    }

	/* Standard read from stdin */
	line = calloc(LINE_LIMIT, sizeof(char));
	INFILE = stdin;
	while (get_next_line() != NULL) {
	    results = 0;
	    handle_line(line);
	    if (results == 0) {
		app_print(NULL);
	    }
	    fprintf(stdout, "%s", wordseparator);
	    if (!buffered_output) {
		fflush(stdout);
	    }
	  }
	
   /* Cleanup */
    for (chain_pos = chain_head; chain_pos != NULL; chain_pos = chain_head) {
	chain_head = chain_pos->next;
	if (chain_pos->ah != NULL) {
	    apply_med_clear(chain_pos->ah);
	}
	if (chain_pos->net != NULL) {
	    fsm_destroy(chain_pos->net);
	}
	free(chain_pos);
    }
    if (serverstring != NULL)
	free(serverstring);
    if (line != NULL)
    	free(line);
    exit(0);
}

char *get_next_line() {
    char *r;
    if ((r = fgets(line, LINE_LIMIT, INFILE)) != NULL) {
	line[strcspn(line, "\n\r")] = '\0';
    }
    return r;
}

void handle_line(char *s) {
    char *result, *tempstr;

	/* Get result from chain */
	for (chain_pos = chain_head, tempstr = s;  ; chain_pos = chain_pos->next) {
	    result = applyer(chain_pos->ah, tempstr);
	    if (result != NULL && chain_pos != chain_tail) {
			tempstr = result;
			continue;
	    }
	    if (result != NULL && chain_pos == chain_tail) {
		do {
		    results++;
		    app_print(result);
		} while ((result = applyer(chain_pos->ah, NULL)) != NULL);
	    }
	    if (result == NULL) {
		/* Move up */
		for (chain_pos = chain_pos->prev; chain_pos != NULL; chain_pos = chain_pos->prev) {
		    result = applyer(chain_pos->ah, NULL);
		    if (result != NULL) {
			tempstr = result;
			break;
		    }
		}
	    }
	    if (chain_pos == NULL) {
		break;
	    }
	}
}