#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fomalib.h"

int g_print_space = 0;
int g_print_pairs = 0;
int g_show_flags = 0;
int g_obey_flags = 1;
int g_list_limit = 10;

// void tokenize() {
//     int i;
//     char *word = "bola"; 
//     char *result;
//     struct apply_handle *ah;
// 	struct fsm *net;

//     net = fsm_read_binary_file("tokenize.fst");

//     printf("Loaded tokenize.fst file...\n");

//     ah = apply_init(net);

//     apply_set_print_space(ah, g_print_space);
//     apply_set_print_pairs(ah, g_print_pairs);
//     apply_set_show_flags(ah, g_show_flags);
//     apply_set_obey_flags(ah, g_obey_flags);

//     printf("Initialized ah...\n");

//     result = apply_up(ah, word);

//     if (result == NULL) {
//         printf("???\n");
//         return;
//     } else {
//         printf("%s\n",result);
//     }
//     for (i = g_list_limit; i > 0; i--) {
//         result = apply_up(ah, NULL);
//         if (result == NULL)
//             break;
//         printf("%s\n",result);
//     }
// }

int main () {
    int i;
    char *word = "bola"; 
    char *result;
    struct apply_handle *ah;
	struct fsm *net;

    net = fsm_read_binary_file("tokenize.fst");

    printf("Loaded tokenize.fst file...\n");

    ah = apply_init(net);

    apply_set_print_space(ah, g_print_space);
    apply_set_print_pairs(ah, g_print_pairs);
    apply_set_show_flags(ah, g_show_flags);
    apply_set_obey_flags(ah, g_obey_flags);

    printf("Initialized ah...\n");

    result = apply_up(ah, word);

    if (result == NULL) {
        printf("???\n");
        return;
    } else {
        printf("%s\n",result);
    }
    for (i = g_list_limit; i > 0; i--) {
        result = apply_up(ah, NULL);
        if (result == NULL)
            break;
        printf("%s\n",result);
    }

	// char *mystring = "bolaa"; 
	// char *result;
	// struct apply_med_handle *medh;
	// struct fsm *net;

	// net = fsm_read_binary_file("nouns.fst"); 

	// printf("loaded this file\n");

    // medh = apply_med_init(net);
	// printf("initialized medh\n");

	// apply_med_set_heap_max(medh, 4194304);
    // apply_med_set_med_limit(medh, 10); 
    // apply_med_set_med_cutoff(medh, 5); 

	// printf("set up with parameters\n");


	// while (result = apply_med(medh, mystring)) {
	// 	printf("looping through...");
	// 	mystring = NULL;
	// 	printf("%s\n%s\nCost:%i\n\n", result, apply_med_get_instring(medh), apply_med_get_cost(medh));
	// }

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
    return 0;
}