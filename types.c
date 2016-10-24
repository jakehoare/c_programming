/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Description:  A program that parses a number of phrases input as command    *
 * line arguments. Each phrase can be a basic type of variable or a legitimate *
 * complex type (array, function or pointer).                                  *
 * Complex types may be compounded and may refer to other phrases by variable  *
 * names, each ending in a basic type.                                         *
 * A first pass through the phrases is made to establish the type, length and  *
 * any variable named in each phrase as well as to fully process basic         *
 * phrases. The second pass processes complex phrases including continuations  *
 * and references between phrase types.                                        *
 *                                                                             *
 * Written by Jake Hoare for COMP9021                                          *
 *                                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* type is one of the following ... */
#define BASIC 0
#define ARRAY 1
#define POINTER 2
#define FUNCTION 3

/* stage is one of the following ... */
#define START 1
#define CONTINUE 2
#define REFER 3
#define FINISHED 4
#define ERROR 5

/* basic type bit values. */
#define INT 1
#define CHAR 2
#define DOUBLE 4
#define FLOAT 8
#define SIGNED 16
#define UNSIGNED 32
#define SHORT 64
#define LONG 128
#define LONGLONG 256

#define NB_ILLEGAL_VARIABLES 25
#define MAX_OUTPUT 200
#define NO_DATA -1
#define EMPTY_CHAR_BEFORE 99
#define EMPTY_CHAR_AFTER 100
#define MIN_BASIC 2
#define MIN_COMPLEX 4

char *illegal_variables[NB_ILLEGAL_VARIABLES] = {"a", "an", "to", "array", "pointer", "function", "signed", "unsigned", "int", "char", "double", "float", "long", "short", "void", "datum", "data", "of", "type", "returning", "A", "An", "pointers", "functions", "arrays"};

int max_phrase_index = NO_DATA;  // The highest phrase index (i.e. number of phrases - 1).
int current; // The index in argv of the current word being processed.
int phrase_nb = 0;  // The index of the current phrase being processed after any reference to another phrase.
int original_phrase_nb = 0;  // The index of the current phrase being processed before any reference to another phrase.

int *phrase_end; // The argv index of the last word of each phrase.
int *phrase_start; // The argv index of the first word of each phrase.
char **var_name; // The variable names (if any).
char **reference; // The name of a referenced variable.
int *continuation; // The phrase index of the referenced variable.
int *stage; // The stage of processing, initially set to START.
int *type; // The type description of each phrase, initially set to BASIC.
int *chain; // Used to check for loops in variable name references.
char **phrase_output; // The text to be displayed.
int *output_start;  // The first empty character before the text of the output.
int *output_end;  // The first empty character after the text of the output.

/* Functions that process each type description.  They return the next processing stage. */
int process_basic(char **);
int process_array(char **);
int process_pointer(char **);
int process_function(char **);

/* Functions that handle reading the text. */
bool check_vowel(char);
bool check_preposition(char *, char *, bool);
bool ends_with_full_stop(char *);
bool permitted_variable_name(char *);
void output_at_front(int, char *);
void output_at_back(int, char *);
int first_phrase_type(char *);
int data_continuation(char **);
bool inc_current(void);
bool complex_variable(void);
void resize_arrays(void);

/* Functions that process basic type descriptions. */
int read_basic_phrase(int, int, char **);
int basic_word_type(char *);
int standardise_basic_phrase(int);
char *make_basic_output(int);

    
int main(int argc, char **argv) {
    
    /* Find the number of phrases. */
    for (int word_nb = 1; word_nb < argc; ++word_nb)
        if (ends_with_full_stop(*(argv + word_nb)))
            ++max_phrase_index;
    resize_arrays();
    
    /* Check that we have at least one phrase and that the final phrase ends at the end of argv. */
    if (max_phrase_index == NO_DATA || !ends_with_full_stop(*(argv + argc - 1))) {
        printf("Incorrect input\n");
        return EXIT_FAILURE;
    }
    
    /* Find the endpoints of each phrase and remove their final full stops. */
    phrase_start[0] = 1;
    for (int word_nb = 1; word_nb < argc; ++word_nb)
        if (ends_with_full_stop(*(argv + word_nb))) {
            *(*(argv + word_nb) + strlen(*(argv + word_nb)) - 1) = '\0';
            phrase_end[phrase_nb++] = word_nb;
            if (phrase_nb <= max_phrase_index)
                phrase_start[phrase_nb] = word_nb + 1;
        }
    
    /* First pass through phrases to find variable names, references and process basic phrases. */
    for (phrase_nb = 0; phrase_nb <= max_phrase_index; ++phrase_nb) {
        original_phrase_nb =  phrase_nb;
        stage[phrase_nb] = START;
        type[phrase_nb]= BASIC;
        continuation[phrase_nb] = NO_DATA;
        current = phrase_start[phrase_nb] + 1;
        
        /* Initialise the output array counters. */
        output_start[phrase_nb] = EMPTY_CHAR_BEFORE;
        output_end[phrase_nb] = EMPTY_CHAR_AFTER;
        
        /* Check that phrase has at least 2 words. */
        if (phrase_end[phrase_nb] - phrase_start[phrase_nb] < (MIN_BASIC - 1)) {
            printf("Incorrect input\n");
            return EXIT_FAILURE;
        }

        /* Check correct usage of preposition 'A' or 'An'. */
        if (!check_preposition(*(argv + current - 1), *(argv + current), true)) {
            printf("Incorrect input\n");
            return EXIT_FAILURE;
        }
    
        /* Identify complex phrases (array, pointer or function) and process basic phrases. */
        type[phrase_nb] = first_phrase_type(*(argv + current));
        if (type[phrase_nb] == BASIC) {
            stage[phrase_nb] = process_basic(argv);
            continue;
        }
        
        /* Check that each complex phrase has the minimum length. */
        if (phrase_end[phrase_nb] - phrase_start[phrase_nb] < (MIN_COMPLEX - 1)) {
            printf("Incorrect input\n");
            return EXIT_FAILURE;
        }
        
        /* If the 3rd word of an ARRAY phrase is not 'of', or POINTER is not 'to', or FUNCTION
         * is not 'returning' then check if the variable name is valid and store it. */
        ++current;
        if ((type[phrase_nb] == ARRAY && strcmp(*(argv + current), "of")) ||
            (type[phrase_nb] == POINTER && strcmp(*(argv + current), "to")) ||
            (type[phrase_nb] == FUNCTION && strcmp(*(argv + current), "returning"))) {
            if (!permitted_variable_name(*(argv + current)))
                stage[phrase_nb] = ERROR;
            else
                var_name[phrase_nb] = *(argv + current);
        }

        /* If the last word of the complex phrase is a permitted variable name then 
         * it is a referenced variable. */
        if (permitted_variable_name(*(argv + phrase_end[phrase_nb])))
            reference[phrase_nb] = *(argv + phrase_end[phrase_nb]);
    }
    
    /* For each phrase that references another variable ...*/
    for (phrase_nb = 0; phrase_nb <= max_phrase_index; ++phrase_nb) {
        int reference_count = 0;
        /* if the variable names are the same then we have a continuation. */
        for (int ref_phrase = 0; ref_phrase <= max_phrase_index; ++ref_phrase)
            if (reference[phrase_nb] && var_name[ref_phrase] && !strcmp(reference[phrase_nb], var_name[ref_phrase])) {
                ++reference_count;
                continuation[phrase_nb] = ref_phrase;
            }
        /* There can be only one referenced variable per phrase. */
        if (reference[phrase_nb] && reference_count != 1) {
            printf("Incorrect input\n");
            return EXIT_FAILURE;            
        }
    }
    
    /* Check that variable references do not create a loop. */
    for (phrase_nb = 0; phrase_nb <= max_phrase_index; ++phrase_nb) {
        chain[phrase_nb] = 1;
        int next_variable = continuation[phrase_nb];
        while (next_variable != NO_DATA) {
            if (chain[next_variable]) {
                printf("Incorrect input\n");
                return EXIT_FAILURE;
            }
            else {
                chain[next_variable] = 1;
                next_variable = continuation[next_variable];
            }
        }
        for (int ref_phrase = 0; ref_phrase <= max_phrase_index; ++ref_phrase)
            chain[ref_phrase] = 0;
    }      
    
    /* Second pass through to process the complex phrases. */
    for (original_phrase_nb = 0; original_phrase_nb <= max_phrase_index; ++original_phrase_nb) {
        /* phrase_nb is initially original_phrase_nb and will be changed if there is a REFER. */
        phrase_nb = original_phrase_nb;
        current = phrase_start[phrase_nb] + 1;
        
        while (stage[original_phrase_nb] != FINISHED && stage[original_phrase_nb] != ERROR) {
            if (type[phrase_nb] == ARRAY)
                stage[original_phrase_nb] = process_array(argv);
            else if (type[phrase_nb] == FUNCTION)
                stage[original_phrase_nb] = process_function(argv);
            else if (type[phrase_nb] == POINTER)
                stage[original_phrase_nb] = process_pointer(argv);
            else
                stage[original_phrase_nb] = process_basic(argv);
        }
        
        if (stage[original_phrase_nb] == ERROR) {
            printf("Incorrect input\n");
            return EXIT_FAILURE;
        }
        /* Reset all types to original - for use in continuation. */
        for (int ref_phrase = 0; ref_phrase <= max_phrase_index; ++ref_phrase)
            type[ref_phrase] = first_phrase_type(*(argv + phrase_start[ref_phrase] + 1));
    }
    
    /* Print out the phrases. */
    for (int phrase_nb = 0; phrase_nb <= max_phrase_index; ++phrase_nb) {
        for (int i = output_start[phrase_nb] + 1; i < output_end[phrase_nb]; ++i){
            putchar(phrase_output[i][phrase_nb]);
        }
        putchar('\n');
    }
    
    return EXIT_SUCCESS;
}

int process_basic(char **argv) {
    int basic_phrase_type = 0;
    
    if (stage[original_phrase_nb] == START) {
        if (!basic_word_type(*(argv + phrase_end[phrase_nb]))) {
            if (!permitted_variable_name(*(argv + phrase_end[phrase_nb])))
                return ERROR;
            else {
                /* Add variable name to output string with a preceeding space and store in var_name array.
                 * Read the rest of basic phrase apart from the variable name and preposition. */
                output_at_back(phrase_nb, *(argv + phrase_end[phrase_nb]));
                output_at_front(phrase_nb, " ");
                var_name[phrase_nb] = *(argv + phrase_end[phrase_nb]);
                basic_phrase_type = read_basic_phrase(phrase_start[phrase_nb] + 1, phrase_end[phrase_nb] - 1, argv);
            }
        }
        else
            /* No named variable so read all of the basic phrase apart from the preposition. */
            basic_phrase_type = read_basic_phrase(phrase_start[phrase_nb] + 1, phrase_end[phrase_nb], argv);
    }
    
    if (stage[original_phrase_nb] == CONTINUE)
        /* Read all of the basic phrase.*/
        basic_phrase_type = read_basic_phrase(current, phrase_end[phrase_nb], argv);

    if (stage[original_phrase_nb] == REFER)
        /* Ignore the variable at the end of the basic phrase.*/
        basic_phrase_type = read_basic_phrase(current, phrase_end[phrase_nb] - 1, argv);
 
    if (!basic_phrase_type)
        return ERROR;

    output_at_front(original_phrase_nb, make_basic_output(basic_phrase_type));
    return FINISHED;
}



int process_array(char **argv) {        
    /* Handle the variable name (if any) according to the stage. */
    if (!complex_variable())
        return ERROR;
            
    /* current must now be 'of'.*/
    if (strcmp(*(argv + current), "of"))
        return ERROR;

    if (!inc_current())
        return ERROR;
        
    /* current must now be an integer to be output in square brackets. */
    int elements = atoi(*(argv + current));
    if (!elements)
        return ERROR;
    /* current can only consist of digits. */
    for (int i = 0; i < strlen(*(argv + current)); ++i)
        if (!isdigit(*(*(argv + current) + i)))
            return ERROR;
    output_at_back(original_phrase_nb, "[");
    output_at_back(original_phrase_nb, *(argv + current));
    output_at_back(original_phrase_nb, "]");
    
    if (!inc_current())
        return ERROR;
        
    /* If singular and 'datum' or plural and 'data'. */
    if ((elements == 1  && !strcmp(*(argv + current), "datum")) ||
        (elements > 1 && !strcmp(*(argv + current), "data")))
        return data_continuation(argv);
    
    if ((elements == 1  && !strcmp(*(argv + current), "array")) ||
        (elements > 1 && !strcmp(*(argv + current), "arrays"))) {
        return CONTINUE;
    }
    
    if ((elements == 1  && !strcmp(*(argv + current), "pointer")) ||
        (elements > 1 && !strcmp(*(argv + current), "pointers"))) {
        type[phrase_nb] = POINTER;
        return CONTINUE;
    }
    return ERROR;
}

int process_pointer(char **argv) {
    /* Check if we have singular or plural pointer. */
    bool singular = true;
    if (*(*(argv + current) + strlen(*(argv + current)) - 1) == 's')
        singular = false;
    
    /* Handle the variable name (if any) according to the stage. */
    if (!complex_variable())
        return ERROR;
    
    /* current must now be 'to'.*/
    if (strcmp(*(argv + current), "to"))
        return ERROR;
    
    if (!inc_current())
        return ERROR;
    
    /* output the star and move to the next word. */
    output_at_front(original_phrase_nb, "*");

    /* output and end if void. */
    if (!strcmp(*(argv + current), "void")) {
        output_at_front(original_phrase_nb, "void ");
        return FINISHED;
    }
    
    if (singular) {
        /* Check the proposition if pointer is singular and move to the next word. */
        if (!inc_current())
            return ERROR;        
        if (!check_preposition(*(argv + current - 1), *(argv + current), false))
            return ERROR;
    }

    /* One of 4 possibilities - pointer, array, function or datum (or their plurals).
     * Paranetheses go outside pointer(s) to array(s) or function(s). */
    if ((singular && !strcmp(*(argv + current), "pointer")) || (!singular && !strcmp(*(argv + current), "pointers")))
        return CONTINUE;
    if ((singular && !strcmp(*(argv + current), "array")) || (!singular && !strcmp(*(argv + current), "arrays"))) {
        type[phrase_nb] = ARRAY;
        output_at_front(original_phrase_nb, "(");
        output_at_back(original_phrase_nb, ")");
        return CONTINUE;
    }
    if ((singular && !strcmp(*(argv + current), "function")) || (!singular && !strcmp(*(argv + current), "functions"))) {
        type[phrase_nb] = FUNCTION;
        output_at_front(original_phrase_nb, "(");
        output_at_back(original_phrase_nb, ")");        
        return CONTINUE;
    }
    if ((singular && !strcmp(*(argv + current), "datum")) || (!singular && !strcmp(*(argv + current), "data")))
        return data_continuation(argv);
        
    return ERROR;
}

int process_function(char **argv) {
    /* Handle the variable name (if any) according to the stage. */
    if (!complex_variable())
        return ERROR;
    
    /* current must now be 'returning'.*/
    if (strcmp(*(argv + current), "returning"))
        return ERROR;
     
    if (!inc_current())
        return ERROR;
        
    /* output the parentheses and move to the next word. */
    output_at_back(original_phrase_nb, "()");

    /* output and end if void. */
    if (!strcmp(*(argv + current), "void")) {
        output_at_front(original_phrase_nb, "void ");
        return FINISHED;
    }
    
    /* else we continue with a preposition. */
    stage[original_phrase_nb] = CONTINUE;
    if (!inc_current())
        return ERROR;    
    if (!check_preposition(*(argv + current - 1), *(argv + current), false))
        return ERROR;
    
    /* then one of 2 possibilities - pointer or datum */
    if (!strcmp(*(argv + current), "pointer")) {
        type[phrase_nb] = POINTER;
        return CONTINUE; 
    }
    if (!strcmp(*(argv + current), "datum"))
        return data_continuation(argv);
   
    return ERROR;
}

int data_continuation(char **argv) {    
    int previous_type = type[phrase_nb];
    type[phrase_nb] = BASIC;  
    
    /* Must have 'of type'. */
    if (!inc_current())
        return ERROR;    
    if (strcmp(*(argv + current), "of"))
        return ERROR;
    if (!inc_current())
        return ERROR;    
    if (strcmp(*(argv + current), "type"))
        return ERROR;        
    if (!inc_current())
        return ERROR;    
                
    /* If we next have 'the type of' then REFER. */
    if (!strcmp(*(argv + current), "the")) {
        if (!inc_current())
            return ERROR;    
        if (strcmp(*(argv + current), "type"))
            return ERROR;
        if (!inc_current())
            return ERROR;    
        if (strcmp(*(argv + current), "of"))        
            return ERROR;        
      
        phrase_nb = continuation[phrase_nb];
        current = phrase_start[phrase_nb] + 1;
        if (type[phrase_nb] == BASIC)
            output_at_front(original_phrase_nb, " ");
        /* Parentheses when refering from a pointer to and array or function. */
        if (previous_type == POINTER && (type[phrase_nb] == FUNCTION || type[phrase_nb] == ARRAY)) {
            output_at_front(original_phrase_nb, "(");
            output_at_back(original_phrase_nb, ")");               
        }   
        return REFER;
    }
    else {
        output_at_front(original_phrase_nb, " ");
        return CONTINUE;        
    }
}

/* If the next word is within the current phrase then increment current, else return false for error. */
bool inc_current(void) {
    if (current == phrase_end[phrase_nb]) 
        return false;
    ++current;
    return true;
}

/* Handle named variables at the start of complex phrases. */
bool complex_variable(void) {
    /* Move the argv index past the variable name if there is one. */
    if (!inc_current())
        return false;
        
    if (stage[original_phrase_nb] == START && var_name[phrase_nb] != 0) {
        if (!inc_current())
            return false;
        /* Add variable name to the output if we are at the START stage. */
        output_at_front(original_phrase_nb, var_name[phrase_nb]);
    }
    /* REFER must have a variable name, move on without output. */
    if (stage[original_phrase_nb] == REFER)
        if (!inc_current())
            return false;
    return true;    
}

bool check_vowel(char letter) {
    if (letter == 'a' || letter == 'e' || letter == 'i' || letter == 'o' || letter == 'u')
        return true;
    return false;
}

/* Checks usage of 'an' or 'a'. If we are at the start of a phrase then there should be a capital letter. */
bool check_preposition(char *word_1, char *word_2, bool start) {
    if (start) {
        if (!strcmp(word_1, "A") && !check_vowel(*word_2))
            return true;
        if (!strcmp(word_1, "An") && check_vowel(*word_2))
            return true;
    }
    else if (!strcmp(word_1, "a") && !check_vowel(*word_2))
        return true;
    else if (!strcmp(word_1, "an") && check_vowel(*word_2))
        return true; 
    return false;
}

int first_phrase_type(char *word) {
    if (!strcmp(word, "array"))
        return ARRAY;
    else if (!strcmp(word, "pointer"))
        return POINTER;
    else if (!strcmp(word, "function"))
        return FUNCTION;
    else 
        return BASIC;
}

bool ends_with_full_stop(char *word) {
    if (*(word + strlen(word) - 1) == '.')
        return true;
    return false;
}
             
bool permitted_variable_name(char *variable_name) {
    /* Check list of reserved words. */
    for (int illegal_var_nb = 0; illegal_var_nb < NB_ILLEGAL_VARIABLES; ++illegal_var_nb) {
        if (!strcmp(variable_name, illegal_variables[illegal_var_nb]))
            return false;
    }
    /* Cannot begin with a digit or be empty. */
    if (isdigit(*variable_name) || *variable_name == '\0')
        return false;
    /* Can only have alphanumeric characters or underscores. */
    while (*variable_name != '\0') {
        if (!isalnum(*variable_name) && *variable_name != '_')
            return false;
        ++variable_name;
    }
    return true;
}

int read_basic_phrase(int basic_start, int basic_end, char **argv) {
    /* Set basic phrase type to zero. */
    int basic_phrase_type = 0;

    /* Loop over the words. */
    for (int word_nb = basic_start; word_nb <= basic_end; ++word_nb) {

        /* If its not a basic word type and not the last word of the phrase then exit. */
        int word_type = basic_word_type(*(argv + word_nb));
        if (!word_type && word_nb != basic_end)
            return false;
        
        /* If is long ... */
        if (word_type == LONG) {
            if (basic_phrase_type & LONGLONG)
                return false;
            else if (basic_phrase_type & LONG) {
                basic_phrase_type = basic_phrase_type ^ LONG;
                basic_phrase_type = basic_phrase_type | LONGLONG;
            }
            else
                basic_phrase_type = basic_phrase_type | LONG;            
        }
        /* If it's not 'long' then exit if we have see it before otherwise set that bit. */
        else if (basic_phrase_type & word_type)
            return false;
        else
            basic_phrase_type = basic_phrase_type | word_type;
    }
    
    basic_phrase_type = standardise_basic_phrase(basic_phrase_type);
    if (!basic_phrase_type)
        return false;
    
    return basic_phrase_type;
}

int basic_word_type(char *word) {
    if (!strcmp(word, "int"))
        return INT;
    if (!strcmp(word, "char"))
        return CHAR;
    if (!strcmp(word, "double"))
        return DOUBLE;
    if (!strcmp(word, "float"))
        return FLOAT;    
    if (!strcmp(word, "signed"))
        return SIGNED;
    if (!strcmp(word, "unsigned"))
        return UNSIGNED;
    if (!strcmp(word, "short"))
        return SHORT;
    if (!strcmp(word, "long"))
        return LONG;
    return 0;
}

int standardise_basic_phrase(int basic_phrase_type) {
    
    /* There must be some input. */
    if (!basic_phrase_type)
        return false;
    
    /* If it's none of char, float or double then it is an int. */
    if (!(basic_phrase_type & (CHAR + DOUBLE + FLOAT)))
        basic_phrase_type = basic_phrase_type | INT;

    /* If it's an int and not unsigned then it's signed. */
    if ((basic_phrase_type & INT) && !(basic_phrase_type & UNSIGNED))
        basic_phrase_type |= SIGNED;    

    /* Fail for combinations that are not allowed. */
    if (basic_phrase_type & INT) {
        if (basic_phrase_type & (CHAR + DOUBLE + FLOAT))
            return false;
        if ((basic_phrase_type & SIGNED) && (basic_phrase_type & UNSIGNED))
            return false;
        /* Can't have more than one of long, short, longlong. */
        if ((basic_phrase_type & SHORT) && ((basic_phrase_type & LONG) || (basic_phrase_type & LONGLONG)))
            return false;
        if ((basic_phrase_type & LONG) && ((basic_phrase_type & SHORT) || (basic_phrase_type & LONGLONG)))
            return false;
        if ((basic_phrase_type & LONGLONG) && ((basic_phrase_type & SHORT) || (basic_phrase_type & LONG)))
            return false;        
    }
    
    if (basic_phrase_type & CHAR) {
        if (basic_phrase_type & (INT + DOUBLE + FLOAT + SHORT + LONG + LONGLONG))
            return false;
        if ((basic_phrase_type & SIGNED) && (basic_phrase_type & UNSIGNED))
            return false;
    }
    
    if (basic_phrase_type & FLOAT)
        if (basic_phrase_type & (INT + CHAR + DOUBLE + SHORT + LONG + LONGLONG + SIGNED + UNSIGNED))
            return false;
    
    if (basic_phrase_type & DOUBLE)
        if (basic_phrase_type & (INT + CHAR + FLOAT + SHORT + LONGLONG + SIGNED + UNSIGNED))
            return false;
    
    return basic_phrase_type;
}

char *make_basic_output(int basic_phrase_type) {
    if (basic_phrase_type == (CHAR + SIGNED))
        return "signed char";
    if (basic_phrase_type == (CHAR + UNSIGNED))
        return "unsigned char";
    if (basic_phrase_type == CHAR)
        return "char";
    if (basic_phrase_type == DOUBLE)
        return "double";
    if (basic_phrase_type == (DOUBLE + LONG))
        return "long double";
    if (basic_phrase_type == FLOAT)
        return "float";
    if (basic_phrase_type == (INT + LONGLONG + SIGNED))
        return "long long";
    if (basic_phrase_type == (INT + LONGLONG + UNSIGNED))
        return "unsigned long long";
    if (basic_phrase_type == (INT + LONG + SIGNED))
        return "long";
    if (basic_phrase_type == (INT + LONG + UNSIGNED))
        return "unsigned long";
    if (basic_phrase_type == (INT + SHORT + SIGNED))
        return "short";
    if (basic_phrase_type == (INT + SHORT + UNSIGNED))
        return "unsigned short";
    if (basic_phrase_type == (INT + SIGNED))
        return "int";
    if (basic_phrase_type == (INT + UNSIGNED))
        return "unsigned";
    printf("ERROR - UNKNOWN BASIC TYPE\n");
}

void resize_arrays(void) {
    /* Allocate arrray sizes according to the number of phrases. */
    phrase_end = (int *) realloc(phrase_end, (max_phrase_index + 1) * sizeof(int));
    phrase_start = (int *) realloc(phrase_start, (max_phrase_index + 1) * sizeof(int));
    var_name = (char **) realloc(var_name, (max_phrase_index + 1) * sizeof(char*));
    reference = (char **) realloc(reference, (max_phrase_index + 1) * sizeof(char*));
    continuation = (int *) realloc(continuation, (max_phrase_index + 1) * sizeof(int));
    stage = (int *) realloc(stage, (max_phrase_index + 1) * sizeof(int));
    type = (int *) realloc(type, (max_phrase_index + 1) * sizeof(int));
    chain = (int *) realloc(chain, (max_phrase_index + 1) * sizeof(int));
    output_start = (int *) realloc(output_start, (max_phrase_index + 1) * sizeof(int));
    output_end = (int *) realloc(output_end, (max_phrase_index + 1) * sizeof(int));
    phrase_output = (char **) calloc(MAX_OUTPUT, sizeof(char*));
    for (int i = 0; i <= MAX_OUTPUT; ++i)
        phrase_output[i] = (char *) calloc(max_phrase_index + 1, sizeof(char));
            
    for (int i = 0; i <= max_phrase_index; ++i) {
        phrase_end[i] = phrase_start[i] = 0;
        var_name[i] = reference[i] = continuation[i] = 0; 
        stage[i] = type[i] = chain[i] = 0;
        output_start[i] = output_end[i] = 0;        
    }
}

void output_at_front(int phrase_nb, char *string_to_add) {
    /* Add text to the front of the output. */
    output_start[phrase_nb] -= strlen(string_to_add);
    for (int i = 0; i < strlen(string_to_add); ++i) {
        phrase_output[i + output_start[phrase_nb] + 1][phrase_nb] = string_to_add[i];
    }
}

void output_at_back(int phrase_nb, char *string_to_add) {
    /* Add text to the front of the output. */
    for (int i = 0; i < strlen(string_to_add); ++i) {
        phrase_output[i + output_end[phrase_nb]][phrase_nb] = string_to_add[i];
    }
    output_end[phrase_nb] += strlen(string_to_add);    
}

