/**CFile****************************************************************

  FileName    [if_while.c]

  SystemName  [IFJ - PROJECT]

  PackageName []

  Synopsis    []

  Author      [Jindrich Sestak]

  Affiliation []

  Date        [20/11/2018]

  Revision    []

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdbool.h>

#include "parser_gen.h"

#include "symtable.h"
#include "dynamicArrParam.h"
#include "stackStr.h"
#include "token.h"

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

unsigned long long int count = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

bool generate_if(symtable_t *var_tab, stack_str_t *stack, char *cond)
{
    char frame[3] = "LF";
    if (strcmp(var_tab->name, "$GT") == 0)
        strcpy(frame, "GF");
    
    /*********Jump to ELSE if cond == false*********/ 
    printf("\nJUMPIFEQ $else$%llu %s@%s int@0\n\n", count, frame, cond);
    /********* DOING IF statement *************/

    //Must go from bottom to top (LABEL endif -> JMP endif) viz. assembler
    //"LABEL $endif$%llu" has max length 34 bits
    char b[34];
    sprintf(b, "\nLABEL $endif$%llu\n\n", count);
    if (stcStr_push(stack, b) == false)
        return false; 

    /*********END IF, ELSE branch **************/
    //"LABEL $else$%llu\nJMP $endif$%llu" has max length 66 bits
    char c[66];
    sprintf(c,  "\n"
                "JMP $endif$%llu\n"
                "LABEL $else$%llu\n\n",
                count, count);
    if (stcStr_push(stack,c) == false)
        return false;

    count++;
    return true;
}


bool generate_while_ending(stack_str_t *stack)
{
    /******** END of While***********/
    //"LABEL $end_while$%llu\nJUMP $while$%llu\n" has max length 72 bits
    char b[72];
    sprintf(b,  "\n"
                "JUMP $while$%llu\n"
                "LABEL $end_while$%llu\n\n",
                 count,count);
    if (stcStr_push(stack, b) == false)
        return false;

  /*  //"JUMPIFEQ $end_while$%llu COND int@0\n" has max length 53 bits
    char *d =malloc(sizeof(char) * 53);
    sprintf(d, "JUMPIFEQ $end_while$%llu COND int@0\n", count);
    if (stcStr_push(stack, d) == false)
        return false;
 */  
    count++;
    return true;
}


void generate_LABEL_while()
{
    /******* LABEL WHILE*************/
    printf("\nLABEL $while$%llu\n", count);
}


void generate_while_false(symtable_t *var_tab, char *cond)
{
    char frame[3] = "LF";
    if (strcmp(var_tab->name, "$GT") == 0)
        strcpy(frame, "GF");

    printf("JUMPIFEQ $end_while$%llu %s@%s int@0\n\n", count, frame, cond);
}


bool generate_function(stack_str_t *stack_str, elem_t *fun, dynamicArrParam_t *param_arr)
{
    printf( "\n"
            "LABEL %s\n" 
            "PUSHFRAME\n"
            "DEFVAR LF@%%retval\n"
            "MOVE LF@%%retval nil@nil\n", 
            fun->func.key
    );
    
    for (int i = 0; i < fun->func.n_params; i++)
    {
        printf( "DEFVAR LF@%s\n"
                "MOVE LF@%s LF@%%%d\n", 
                param_arr->param_arr[i]->var.key,
                param_arr->param_arr[i]->var.key,
                i+1
        );
    }
    printf("\n");

    if (! stcStr_push(stack_str, "\nPOPFRAME\nRETURN\n\n"))
        return false;

    return true;
}



void generate_var(symtable_t *var_tab, char *var_name, char *right_val)
{
    char frame[3] = "LF";
    if (strcmp(var_tab->name, "$GT") == 0)
        strcpy(frame, "GF");
    
    char frame_right_val[3];
    if (strcmp(right_val, "%retval") == 0) // TODO
        strcpy(frame_right_val, "TF");
    else
        strcpy(frame_right_val, frame);
    
    if (symtab_find(var_tab, var_name) == NULL)
        printf("DEFVAR %s@%s\n", frame, var_name);

    printf("MOVE %s@%s %s@%s\n", frame, var_name, frame_right_val, right_val);
}

void length(symtable_t *symtab, token_t *par)
{
    char frame_act [3] = "LF";
    char frame_var [3] = "LF";      //from which frame is variable ID
    if (strcmp(symtab->name, "$GT" ) == 0)
        strcpy(frame_act, "GF");

    printf("DEFVAR LF@%%retval\n"
           "MOVE LF@%%retval nil@nil\n"
           "DEFVAR %s@$length$tmp%llu\n",
           frame_act, count);

    if (strcmp(par->name, "ID") == 0)
    {
        if (symtab_find(symtab, par->info.string) == NULL)
            strcpy(frame_var, "GF");

        printf("MOVE %s@$length$tmp%llu %s@%s\n ", 
                frame_act, count, frame_var, par->info.string);
    }
    else if ((strcmp(par->name, "STR") == 0))
       printf("MOVE %s@$length$tmp%llu string@%s\n ", 
                frame_act, count, par->info.string);
    
    else{
        count++;
        return;
        //return error 4 
    }
    
    printf("STRLEN LF@%%retval %s@$length$tmp%llu\n",
            frame_act, count);

    count++;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
