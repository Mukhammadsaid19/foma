#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fomalib.h"

static char *mystring = "bolaa"; 
static char *result;
struct apply_med_handle *medh;
struct fsm *net;


int main ()
{
	net = fsm_read_binary_file("nouns.fst"); 

    medh = apply_med_init(net);

    apply_med_set_heap_max(medh, 4194304);
    apply_med_set_med_limit(medh, 10); 
    apply_med_set_med_cutoff(medh, 5); 

	apply_med_set_align_symbol(medh, "-");

	while (result = apply_med(medh, mystring)) {
		mystring = NULL;
		printf("%s\n%s\nCost:%i\n\n", result, apply_med_get_instring(medh), apply_med_get_cost(medh));
	}

    // int aflag = 0;
    // int bflag = 0;
    // char *cvalue = NULL;
    // int index;
    // int c;

    // opterr = 0;

    // while ((c = getopt (argc, argv, "abc:")) != -1)
    //     switch (c)
    //     {
    //     case 'a':
    //         aflag = 1;
    //         break;
    //     case 'b':
    //         bflag = 1;
    //         break;
    //     case 'c':
    //         cvalue = optarg;
    //         break;
    //     case '?':
    //         if (optopt == 'c')
    //         fprintf (stderr, "Option -%c requires an argument.\n", optopt);
    //         else if (isprint (optopt))
    //         fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    //         else
    //         fprintf (stderr,
    //                 "Unknown option character `\\x%x'.\n",
    //                 optopt);
    //         return 1;
    //     default:
    //         abort ();
    //     }

    // printf ("aflag = %d, bflag = %d, cvalue = %s\n",
    //         aflag, bflag, cvalue);

    // for (index = optind; index < argc; index++)
    //     printf ("Non-option argument %s\n", argv[index]);
    // return 0;
}