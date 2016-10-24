/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Description: The input is expected to consist of height + 1 lines of        *
 *              length + 1 numbers in {0, ..., 15}, where length is at least   *
 *              equal to MIN_LENGTH and at most equal to MAX_LENGTH and height *
 *              is at least equal to MIN_HEIGHT and at most equal to           *
 *              MAX_HEIGHT, with possibly lines consisting of spaces only      *
 *              that will be ignored and with possibly spaces anywhere on the  *
 *              lines with digits. The xth digit n of the yth line, with       *
 *              0 <= x <= length and 0 <= y <= height, is to be associated     *
 *              with a point situated x * 0.2 cm to the right and y * 0.2 cm   *
 *              below an origin, is to be connected to the point 0.2 cm above  *
 *              if the rightmost digit of n is 1, is to be connected to the    *
 *              point 0.2 cm above and 0.2 cm to the right if the second       *
 *              rightmost digit of n is 1, is to be connected to the point     *
 *              0.2 cm to the right if the third rightmost digit of n  is 1,   *
 *              and is to be connected to the point 0.2 cm to the right and    *
 *              0.2 cm below if the fourth rightmost digit of n is 1.          *
 *              The input is further constrained to represent a frieze,        *
 *              that is, a picture that fits in a rectangle of length          *
 *              length * 0.2 cm and of height heigth * 0.2 cm, with horizontal *
 *              lines of length length at the top and at the bottom, identical *
 *              vertical borders at both ends, no crossing segments connecting *
 *              pairs of neighbours inside the rectangle, and a pattern of     *
 *              integral period at least equal to 2  that is fully repeated    *
 *              at least twice in the horizontal dimension.                    *
 *                                                                             *
 *              Practically, the input will be stored in a file and its        *
 *              contents redirected to standard input. The program will be run *
 *              with either no command-line argument or with "print" as unique *
 *              command line argument; otherwise it will exit.                 *
 *                                                                             *
 *              When provided with no command-line argument, the program       *
 *              displays one of two error messages if the input is incorrect   *
 *              or if it fails to represent a frieze, and otherwise outputs    *
 *              the period of the pattern and the set of symmetries that       *
 *              keep it invariant under an isometry; a result on frieze        *
 *              classification lists 7 alternatives based on 5 symmetries:     *
 *              horizontal translation by period (which of course can be       *
 *              applied to any frieze), horizontal reflection about the line   *
 *              that goes through the middle of the frieze, horizontal         *
 *              reflection about the line that goes through the middle of      *
 *              the frieze and horizontal translation of the lower part of the *
 *              frieze by half the period, vertical reflection about some      *
 *              vertical line, and rotation around some point situated on the  *
 *              horizontal line that goes through the middle of the frieze.    *
 *                                                                             *
 *              When provided with "print" as unique command-line argument,    *
 *              the program outputs some .tex code to depict the frieze        *
 *              as a tiz picture. Segments are drawn in purple with a single   *
 *              draw command for each longest segment, starting with the       *
 *              vertical segments, from the topmost leftmost one to the        *
 *              bottommost rightmost one with the leftmost ones first,         *
 *              followed by the segments that go from north west to            *
 *              south east, from the topmost leftmost one to the bottommost    *
 *              rightmost one with the ones topmost first, followed by the     *
 *              segments that go from west to east, from the topmost leftmost  *
 *              one to the bottommost rightmost one with the topmost ones      *
 *              first, followed by the segments that go from the south west    *
 *              to the north east, from the topmost leftmost one to the        *
 *              bottommost rightmost one with the topmost ones first.          *
 *                                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MIN_LENGTH 4
#define MAX_LENGTH 50
#define MIN_HEIGHT 2
#define MAX_HEIGHT 16
#define MAX_INPUT 15

/* Initilaise global arrays to hold the frieze data. */
int frieze[MAX_HEIGHT + 1][MAX_LENGTH + 1] = {{0}};
int shifted_frieze[MAX_HEIGHT + 1][MAX_LENGTH + 1] = {{0}};
int rotated_frieze[MAX_HEIGHT + 1][MAX_LENGTH + 1] = {{0}};

int length = 0;
int height = 0;
int period = 0;
/* An integer whose bits encode which symmetries are true of the frieze. */
int symmetry = 0;

/* Returns true if the input is correctly formatted and loads the array frieze, or else returns false. */ 
bool load_and_check(void);
/* Returns true if the data represent a frieze or else returns false. */
bool test_if_frieze(void);
/* Generates the output for the tex file. */
void make_tex(void);

/* Frieze manipulation functions. */
/* Fills an array with zeros. Operates on shifted_frieze if the input is false or else operates on rotated_frieze. */
void clear_frieze(bool);
/* Updates shifted_frieze with the pattern in frieze moved and an integer number of steps to the left. */
void shift_left(int);
/* Compares shifted_frieze (if input is false) or rotated_frieze (if input is true) to frieze within the
 * specified region and returns true if they are identical. */
bool compare_friezes(int, int, bool);
/* Returns true if the frieze is symmetrical under horizontal reflection about a central axis. */
bool horizontal_reflection(void);
/* Returns true if the frieze is symmetrical under horizontal reflection then translation by half a period. */
bool glided_horizontal_reflection(void);
/* Returns true if the frieze is symmetrical under reflection about a vertical axis. */
bool vertical_reflection(void);
/* Returns true if the frieze is symmetrical under vertical and horizontal reflections together. */
bool rotation(void);


int main(int argc, char **argv) {
    if (argc > 2 || argc == 2 && strcmp(argv[1], "print")) {
        printf("I expect no command line argument or \"print\" as unique command line argument.\n");
        return EXIT_FAILURE;
    }

    if (!load_and_check()) {
        printf("Incorrect input.\n");
        return EXIT_FAILURE;
    }
    
    if (!test_if_frieze()) {
        printf("Input does not represent a frieze.\n");
        return EXIT_FAILURE;
    }
    
    if (argc == 2) {
        make_tex();
        return EXIT_SUCCESS;
    }   
 
/* Bit 0 (the rightmost) of symmetry is set if the frieze has horizontal reflection symmetry.
 * Bit 1 is set for set for glided horizontal reflection.
 * Bit 2 is set for vertical reflection.
 * Bit 3 is set for rotation. */
    
    if (horizontal_reflection())
        symmetry += (1 << 0);
    if (glided_horizontal_reflection())
        symmetry += (1 << 1);
    if (vertical_reflection())
        symmetry += (1 << 2);
    if (rotation())
        symmetry += (1 << 3);
    
    printf("Pattern is a frieze of period %d that is invariant under translation", period);
    if (symmetry == 0)
        printf(" only.\n");
    if (symmetry == 1)
        printf("\n\tand horizontal reflection only.\n");
    if (symmetry == 2)
        printf("\n\tand glided horizontal reflection only.\n");
    if (symmetry == 4)
        printf("\n\tand vertical reflection only.\n");
    if (symmetry == 8)
        printf("\n\tand rotation only.\n");
    if (symmetry == 13)
        printf("\n\thorizontal and vertical reflections, and rotation only.\n");   
    if (symmetry == 14)
        printf("\n\tglided horizontal and vertical reflections, and rotation only.\n");      
      
    return EXIT_SUCCESS;
}



bool load_and_check(void) {

    /* c is the character being read, row and column are counters.
     * prev_digit is true if the previous character was a numerical digit. */
    int c;
    int row = 0;
    int column = 0;
    bool prev_digit = false;

    while ((c = getchar()) != EOF) {
        
        if (c == ' ') {
            prev_digit = false;
        }
        
        /* Transform the character to a digit, multiply the previous digit by 10 if we have 2 consecutive digits,
         * and check that the total number is not more than MAX_INPUT. */
        else if (isdigit(c)) {
            if (prev_digit) {   
                frieze[row][column - 1] = (frieze[row][column - 1] * 10) + (c - '0');
                if (frieze[row][column - 1] > MAX_INPUT)
                    return false;                
            }        
            else {
                frieze[row][column] = (c - '0');
                ++column;
                prev_digit = true;
            }
        }

        /* Ignore lines that end without any data. At the end of the first line of data set the length and
         * check that all future lines are the same length. */
        else if (c == '\n' && column == 0)
            ;
        else if (c == '\n' && column != 0) {
            if (length == 0)
                length = column;
            else if (length != column)
                return false;
            column = 0;
            ++row;
            prev_digit = false;
        }
        else
            return false;
    }
    
    /* Length and height are with respect to a first column and row of zero so we subtract 1.
     * Check that the frieze is within the permitted dimensions. Note that we assume a new line before EOF. */
    length -= 1;
    height = row - 1;
    if ((length > MAX_LENGTH) || (length < MIN_LENGTH))
        return false;
    if ((height > MAX_HEIGHT) || (height < MIN_HEIGHT))
        return false;
    
    return true;
}


bool test_if_frieze(void) {

    /* Bit zero (rightmost) encodes a vertical line |
     * Bit 1 encodes /
     * Bit 2 encodes -
     * Bit 3 encodes \   */
    
    /* The top border must have bit 2 set and cannot have bits zero or 1 set.
     * The lower border must have bit 2 set and cannot have bit 3 set. */
    for (int j = 0; j < length; ++j) {
        if (!((1 << 2) & frieze[0][j]) || ((1 << 0) & frieze[0][j]) || ((1 << 1) & frieze[0][j]))
            return false;
        if (!((1 << 2) & frieze[height][j]) || ((1 << 3) & frieze[height][j]))
            return false;
    }

    /* The RHS border can only have zero bit set and no others i.e. is 1 or 0 only.
     * The zeroth bits of the RHS and LHS right must be equal to have identical vertical borders. */
    for (int i = 0; i <= height; ++i) {
        if (frieze[i][length] > 1)
            return false;
        if (((1 << 0) & frieze[i][0]) != ((1 << 0) & frieze[i][length]))
            return false;
    }

    /* Segments cross if a point with bit 3 set is above a point with bit 1 set. */
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j <= length; ++j) {
            if (((1 << 3) & frieze[i][j]) && ((1 << 1) & frieze[i + 1][j]))
                return false;
        }
    }

    /* Check that the pattern repeats horizontally at least twice and find the period (>= 2). */
    for (int k = 1; k <= (length / 2); ++k) {
        shift_left(k);
        if (compare_friezes(height, length - k, false)) {
            period = k;
            break;
        }       
    }
    if (period <= 1)
        return false;

    return true;
}


void make_tex() {
    printf("\\documentclass[10pt]{article}\n"
           "\\usepackage{tikz}\n"
           "\\usepackage[margin=0cm]{geometry}\n"
           "\\pagestyle{empty}\n"
           "\n"
           "\\begin{document}\n"
           "\n"
           "\\vspace*{\\fill}\n"
           "\\begin{center}\n"
           "\\begin{tikzpicture}[x=0.2cm, y=-0.2cm, thick, purple]\n"
           "% North to South lines\n");
    
    for (int j = 0; j <= length; ++j) { 
        for (int i = 0; i <= height; ++i) {
            /* If bit zero is set and the point to the North is out of frieze or has bit zero not set
             * then a line starts at the point to the North. */
            if (((1 << 0) & frieze[i][j]) && ((i == 0) || !((1 << 0) & frieze[i - 1][j]))) {
                printf("    \\draw (%d,%d) -- ", j, i - 1);
                /* Continue South along the line as long as bit zero is set and we are within the frieze height. */
                while (((1 << 0) & frieze[i][j]) && (i++ < height))
                    ;
                printf("(%d,%d);\n", j, i - 1);
            }
        }
    }
 
    printf("% North-West to South-East lines\n");
    for (int i = 0; i <= height; ++i) {       
        for (int j = 0; j <= length; ++j) {
            
            /* If bit 3 is set and the point to the North West is out of the frieze or has bit 3 not set then a line starts here. */
            if (((1 << 3) & frieze[i][j]) && ((i == 0) || (j == 0) || !((1 << 3) & frieze[i - 1][j - 1]))) {
                printf("    \\draw (%d,%d) -- ", j, i);
                /* Continue South East along the line as long as bit 3 is set and we are within the frieze. */
                int k = 0;
                while (((1 << 3) & frieze[i + k][j + k]) && (((i + k) < height) || ((j + k) < length)))
                    ++k;
                printf("(%d,%d);\n", j + k, i + k);
            }  
        }
    }
    
    printf("% West to East lines\n");
    for (int i = 0; i <= height; ++i) {       
        for (int j = 0; j <= length; ++j) {
            /* If bit 2 is set and the point to the West is out of the frieze or has bit 2 not set then a line starts here. */
            if (((1 << 2) & frieze[i][j]) && ((j == 0) || !((1 << 2) & frieze[i][j - 1]))) {
                printf("    \\draw (%d,%d) -- ", j, i);
                /* Continue East along the line as long as bit 2 is set and we are within the frieze length. */
                while (((1 << 2) & frieze[i][j]) && (j++ < length))
                    ;
                printf("(%d,%d);\n", j, i);
            }
        }
    }
    
    putchar('%');
    printf(" South-West to North-East lines\n");
    for (int i = 0; i <= height; ++i) {       
        for (int j = 0; j <= length; ++j) {
            
            /* If bit 1 is set and the point to the South West point is out of the frieze or has bit 1 not set then a line starts here. */
            if (((1 << 1) & frieze[i][j]) && ((i == height) || (j == 0) || !((1 << 1) & frieze[i + 1][j - 1]))) {
                printf("    \\draw (%d,%d) -- ", j, i);
                /* Continue North East along the line as long as bit 1 is set and we are within the frieze. */
                int k = 0;
                while (((1 << 1) & frieze[i - k][j + k]) && (((i - k) > 0) || ((j + k) < length)))
                    ++k;
                printf("(%d,%d);\n", j + k, i - k);
            }  
        }
    }     
    printf("\\end{tikzpicture}\n"
           "\\end{center}\n"
           "\\vspace*{\\fill}\n"
           "\n"
           "\\end{document}\n");
}


bool compare_friezes(int compare_height, int compare_length, bool rotated) {
    int testcase;
    for (int i = 0; i <= compare_height; ++i) {
        for (int j = 0; j <= compare_length; ++j) {
            if (rotated)
                testcase = rotated_frieze[i][j];
            else
                testcase = shifted_frieze[i][j];

            if ((j == compare_length) && (testcase != (frieze[i][j] & (1 << 0))))
                return false;
            if ((j != compare_length) && (testcase != frieze[i][j]))
                return false;
                
        }
    }
    return true;
}


void clear_frieze(bool rotated) {
    for (int i = 0; i <= height; ++i) {
        for (int j = 0; j <= length; ++j) {
            if (rotated)
                rotated_frieze[i][j] = 0;
            else
                shifted_frieze[i][j] = 0;
        }
    }    
}


void shift_left(int shift) {
    clear_frieze(false);
    for (int i = 0; i <= height; ++i) {
        for (int j = 0; j + shift <= length; ++j) {
            shifted_frieze[i][j] = frieze[i][j + shift];
        }
    }    
    
}


bool horizontal_reflection(void) {
    clear_frieze(false);
    for (int i = 0; i <= height; ++i) {
        for (int j = 0; j <= length; ++j) {
            /* We have already checked that for i = 0 bit zero is not set so do not need to do so here.
             * Transform each point to its reflection accounting for which bits are set and the location of the reflection. */
            if (((1 << 0) & frieze[i][j]))
                shifted_frieze[height - i + 1][j] += (1 << 0);
            if (((1 << 1) & frieze[i][j]))
                shifted_frieze[height - i][j] += (1 << 3);
            if (((1 << 2) & frieze[i][j]))
                shifted_frieze[height - i][j] += (1 << 2);
            if (((1 << 3) & frieze[i][j]))
                shifted_frieze[height - i][j] += (1 << 1);
        }
    }
    if (compare_friezes(height, length, false))
        return true;
    else
        return false;
}


bool glided_horizontal_reflection(void) {
    /* Operates on shifted_frieze that has already undergone horizontal reflection.
     * Period must be even or else we do not have symmetry when translating by half a period. */
    if (period % 2)
        return false;
    for (int i = 0; i <= height; ++i) {
        for (int j = 0; j <= length - period / 2; ++j) {
            shifted_frieze[i][j] = shifted_frieze[i][j + period / 2];
        }
    }

    if (compare_friezes(height, length - (period / 2), false))
        return true;
    else
        return false;
}


bool vertical_reflection(void) {
    /* reflection_region (which we effectively fold in half and is double the axis column) must not be more
     * than the length and must contain at least one period of the frieze. */
    for (int reflection_region = period; reflection_region <= length; ++reflection_region) {
        clear_frieze(false);
        for (int i = 0; i <= height; ++i) {
            for (int j = 0; j <= reflection_region; ++j) {
                /* Transform each point to its reflection accounting for the bits set and location. */
                if (((1 << 0) & frieze[i][j]))
                    shifted_frieze[i][j + reflection_region - (2 * j)] += (1 << 0);
                if (((1 << 1) & frieze[i][j]) && (j != reflection_region) && (i != 0))
                    shifted_frieze[i - 1][j + reflection_region - (2 * j) - 1] += (1 << 3);
                if (((1 << 2) & frieze[i][j]) && (j != reflection_region))
                    shifted_frieze[i][j + reflection_region - (2 * j) - 1] += (1 << 2);
                if (((1 << 3) & frieze[i][j]) && (j != reflection_region) && (i != height))
                    shifted_frieze[i + 1][j + reflection_region - (2 * j) - 1] += (1 << 1);
            }
        }
        if (compare_friezes(height, reflection_region, false))
            return true;
    }
    return false;
}


bool rotation(void) {
    /* Perform the horizontal reflection on the original frieze then loop over vertical reflections to create
     * rotated_friezes. */
    horizontal_reflection();
    for (int reflection_region = period; reflection_region <= length; ++reflection_region) {

        clear_frieze(true);        
        for (int i = 0; i <= height; ++i) {
            for (int j = 0; j <= reflection_region; ++j) {
                if (((1 << 0) & shifted_frieze[i][j]))
                    rotated_frieze[i][j + reflection_region - (2 * j)] += (1 << 0);
                if (((1 << 1) & shifted_frieze[i][j]) && (j != reflection_region) && (i != 0))
                    rotated_frieze[i - 1][j + reflection_region - (2 * j) - 1] += (1 << 3);
                if (((1 << 2) & shifted_frieze[i][j]) && (j != reflection_region))
                    rotated_frieze[i][j + reflection_region - (2 * j) - 1] += (1 << 2);
                if (((1 << 3) & shifted_frieze[i][j]) && (j != reflection_region) && (i != height))
                    rotated_frieze[i + 1][j + reflection_region - (2 * j) - 1] += (1 << 1);
            }
        }
        
        if (compare_friezes(height, reflection_region, true)) {
            return true;
        }
    }
    return false;
}


