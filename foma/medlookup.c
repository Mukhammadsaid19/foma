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

static char *usagestring = "Usage: flookup [-h] [-a] [-i] [-s \"separator\"] [-w \"wordseparator\"] [-v] [-x] [-b] [-I <#|#k|#m|f>] [-S] [-P] [-A] <binary foma file>\n";

static char *helpstring =
"Applies words from stdin to a foma transducer/automaton read from a file and prints results to stdout.\n"

"If the file contains several nets, inputs will be passed through all of them (simulating composition) or applied as alternates if the -a flag is specified (simulating priority union: the first net is tried first, if that fails to produce an output, then the second is tried, etc.).\n\n"
"Options:\n\n"
"-h\t\tprint help\n"
"-a\t\ttry alternatives (in order of nets loaded, default is to pass words through each)\n"
"-b\t\tunbuffered output (flushes output after each input word, for use in bidirectional piping)\n"
"-i\t\tinverse application (apply down instead of up)\n"
"-I indextype\tindex arcs with indextype (one of -I f -I #k -I #m or -I #)\n"
"\t\t(usually slower than the default except for states > 1,000 arcs)\n"
"\t\t  -I # will index all states containing # arcs or more\n"
"\t\t  -I NUMk will index states from densest to sparsest until reaching mem limit of # kB\n"
"\t\t  -I NUMM will index states from densest to sparsest until reaching mem limit of # MB\n"
"\t\t  -I f will index flag-containing states only\n"
"-q\t\tdon't sort arcs before applying (usually slower, except for really small, sparse automata)\n"
"-S\t\trun flookup as UDP server (default addr INADDR_ANY port 6062)\n"
"-A\t\t  specify address of server\n"
"-P\t\t  specify port of server (default 6062)\n"
"-s \"separator\"\tchange input/output separator symbol (default is TAB)\n"
"-w \"separator\"\tchange words separator symbol (default is LF)\n"
"-v\t\tprint version number\n"
"-x\t\tdon't echo input string";

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
static int  echo = 0, apply_alternates = 0, numnets = 0, direction = DIR_UP, results, buffered_output = 1, index_arcs = 0, index_flag_states = 0, index_cutoff = 0, index_mem_limit = INT_MAX, mode_server = 0, port_number = FLOOKUP_PORT, udpsize;
static char *separator = "\t", *wordseparator = "\n", *server_address = NULL, *line, *serverstring = NULL;
static FILE *INFILE;
static struct lookup_chain *chain_head, *chain_tail, *chain_new, *chain_pos;
static fsm_read_binary_handle fsrh;

struct fsm *med_net;
struct apply_med_handle *medh;

struct fsm *check_net;
struct apply_handle *checkh;

static char *(*applyer)() = &apply_up;  /* Default apply direction = up */
static void med_lookup(char *s);
static void handle_line(char *s);
static void app_print(char *result);
static char *get_next_line();
static void server_init();

void app_print(char *result) {

	if (result != NULL) {
    if (!mode_server) {
	if (echo == 1) {
	    fprintf(stdout, "%s%s",line, separator);
	}
	if (result == NULL) {
	    fprintf(stdout,"+?\n");
	} else {
	    fprintf(stdout, "%s\n", result);
	}
    } else {
	if (echo == 1) {
	    strncat(serverstring+udpsize, line, UDP_MAX-udpsize);
	    udpsize += strlen(line);
	    strncat(serverstring+udpsize, separator, UDP_MAX-udpsize);
	    udpsize += strlen(separator);
	}
	if (result == NULL) {
	    strncat(serverstring+udpsize, "?+\n", UDP_MAX-udpsize);
	    udpsize += 3;
	} else {
	    strncat(serverstring+udpsize, result, UDP_MAX-udpsize);
	    udpsize += strlen(result);
	    strncat(serverstring+udpsize, "\n", UDP_MAX-udpsize);
	    udpsize++;
	}
    }
	}
}

int main(int argc, char *argv[]) {
    med_net = fsm_read_binary_file("analyzer-spellcheck.fst"); 
    medh = apply_med_init(med_net);
    apply_med_set_heap_max(medh, 4194304);
    apply_med_set_med_limit(medh, 10); 
    apply_med_set_med_cutoff(medh, 5); 

    // check_net = fsm_read_binary_file("model.foma");
    // checkh = apply_init(check_net);

    // result = apply_up(ah, sentence);

    // if (result == NULL) {
    //     printf("???\n");
    //     return;
    // } else {
    //     printf("%s\n",result);
    // }
    // for (i = list_limit; i > 0; i--) {
    //     result = apply_up(ah, NULL);
    //     if (result == NULL)
    //         break;
    //     printf("%s\n",result);
    // }


    
    int opt, sortarcs = 1;
    char *infilename;
    struct fsm *net;

    setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

    while ((opt = getopt(argc, argv, "abhHiIM:qs:SA:P:w:vx")) != -1) {
        switch(opt) {
        case 'a':
	    apply_alternates = 1;
	    break;
        case 'b':
	    buffered_output = 0;
	    break;
        case 'h':
	    printf("%s%s\n", usagestring,helpstring);
            exit(0);
        case 'i':
	    direction = DIR_DOWN;
	    applyer = &apply_down;
	    break;
        case 'q':
	    sortarcs = 0;
	    break;
	case 'I':
	    if (strcmp(optarg, "f") == 0) {
		index_flag_states = 1;
		index_arcs = 1;
	    } else if (strstr(optarg, "k") != NULL && strstr(optarg,"K") != NULL) {
		/* k limit */
		index_mem_limit = 1024*atoi(optarg);
		index_arcs = 1;
	    } else if (strstr(optarg, "m") != NULL && strstr(optarg,"M") != NULL) {
		/* m limit */
		index_mem_limit = 1024*1024*atoi(optarg);
		index_arcs = 1;
	    } else if (isdigit(*optarg)) {
		index_arcs = 1;
		index_cutoff = atoi(optarg);
	    }
	    break;
	case 's':
	    separator = strdup(optarg);
	    break;
	case 'S':
	    mode_server = 1;
	    break;
	case 'A':
	    server_address = strdup(optarg);
	    break;
	case 'P':
	    port_number = atoi(optarg);
	    break;
	case 'w':
	    wordseparator = strdup(optarg);
	    break;
        case 'v':
	    printf("flookup 1.03 (foma library version %s)\n", fsm_get_library_version_string());
	    exit(0);
        case 'x':
	    echo = 0;
	    break;
	default:
            fprintf(stderr, "%s", usagestring);
            exit(EXIT_FAILURE);
	}
    }
    // if (optind == argc) {
	// fprintf(stderr, "%s", usagestring);
	// exit(EXIT_FAILURE);
    // }

    // infilename = argv[optind];

    // if ((fsrh = fsm_read_binary_file_multiple_init(infilename)) == NULL) {
    //     perror("File error");
	// exit(EXIT_FAILURE);
    // }
    // chain_head = chain_tail = NULL;

    // while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
	// numnets++;
	// chain_new = malloc(sizeof(struct lookup_chain));
	// if (direction == DIR_DOWN && net->arcs_sorted_in != 1 && sortarcs) {
	//     fsm_sort_arcs(net, 1);
	// }
	// if (direction == DIR_UP && net->arcs_sorted_out != 1 && sortarcs) {
	//     fsm_sort_arcs(net, 2);
	// }
	// chain_new->net = net;
	// chain_new->ah = apply_init(net);
	// if (direction == DIR_DOWN && index_arcs) {
	//     apply_index(chain_new->ah, APPLY_INDEX_INPUT, index_cutoff, index_mem_limit, index_flag_states);
	// }
	// if (direction == DIR_UP && index_arcs) {
	//     apply_index(chain_new->ah, APPLY_INDEX_OUTPUT, index_cutoff, index_mem_limit, index_flag_states);
	// }

	// chain_new->next = NULL;
	// chain_new->prev = NULL;
	// if (chain_tail == NULL) {
	//     chain_tail = chain_head = chain_new;
	// } else if (direction == DIR_DOWN || apply_alternates == 1) {
	//     chain_tail->next = chain_new;
	//     chain_new->prev = chain_tail;
	//     chain_tail = chain_new;
	// } else {
	//     chain_new->next = chain_head;
	//     chain_head->prev = chain_new;
	//     chain_head = chain_new;
	// }
    // }

    // if (numnets < 1) {
	// fprintf(stderr, "%s: %s\n", "File error", infilename);
	// exit(EXIT_FAILURE);
    // }

    if (mode_server) {
	server_init();
	serverstring = calloc(UDP_MAX+1, sizeof(char));
	line = calloc(UDP_MAX+1, sizeof(char));
	addrlen = sizeof(clientaddr);
	for (;;) {
	    numbytes = recvfrom(listen_sd, line, UDP_MAX, 0,(struct sockaddr *)&clientaddr, &addrlen);
	    if (numbytes == -1) {
		perror("recvfrom() failed, aborting");
		break;
	    }
	    line[numbytes] = '\0';
	    line[strcspn(line, "\n\r")] = '\0';
	    fflush(stdout);
	    results = 0;
	    udpsize = 0;
	    serverstring[0] = '\0';
        med_lookup(line);
	    if (results == 0) {
		app_print(NULL);
	    }
	    if (serverstring[0] != '\0') {
		numbytes = sendto(listen_sd, serverstring, strlen(serverstring), 0, (struct sockaddr *)&clientaddr, addrlen);
		if (numbytes < 0) {
		    perror("sendto() failed"); fflush(stdout);
		}
	    }
	}
    } else {
	/* Standard read from stdin */
	line = calloc(LINE_LIMIT, sizeof(char));
	INFILE = stdin;
	while (get_next_line() != NULL) {
	    results = 0;
	    // handle_line(line);
		med_lookup(line);
	    if (results == 0) {
		app_print(NULL);
	    }
	    fprintf(stdout, "%s", wordseparator);
	    if (!buffered_output) {
		fflush(stdout);
	    }
	}
    }
   /* Cleanup */
    for (chain_pos = chain_head; chain_pos != NULL; chain_pos = chain_head) {
	chain_head = chain_pos->next;
	if (chain_pos->ah != NULL) {
	    apply_clear(chain_pos->ah);
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

void med_lookup(char *s) {
	char *result;

	while (result = apply_med(medh, s)) {
		// printf("looping through...");
		s = NULL;
        app_print(result);

		// printf("%s\n%s\nCost:%i\n\n", result, apply_med_get_instring(medh), apply_med_get_cost(medh));
	}
}

void server_init(void) {
    unsigned int rcvsize = 262144;

    if ((listen_sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
	perror("socket() failed");
	exit(1);
    }
    if (setsockopt(listen_sd, SOL_SOCKET, SO_RCVBUF, (char *) &rcvsize, sizeof(rcvsize)) < 0) {
    	perror("setsockopt() failed");
    	exit(1);
    }
    if (setsockopt(listen_sd, SOL_SOCKET, SO_SNDBUF, (char *) &rcvsize, sizeof(rcvsize)) < 0) {
    	perror("setsockopt() failed");
    	exit(1);
    }

    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port_number);
    if (server_address != NULL) {
	serveraddr.sin_addr.s_addr = inet_addr(server_address);
    } else {
	serveraddr.sin_addr.s_addr = INADDR_ANY;
    }
    if (bind(listen_sd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
	perror("bind() failed");
	exit(1);
    }
    printf("Started flookup server on %s port %i\n", inet_ntoa(serveraddr.sin_addr), port_number); fflush(stdout);
}
