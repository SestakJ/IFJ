/**CFile****************************************************************

  FileName    [builtin_gen.c]

  SystemName  [IFJ - PROJECT]

  PackageName []

  Synopsis    []

  Author      [Jindrich Sestak, Lukas Valek]

  Affiliation []

  Date        [--/--/2018]

  Revision    []

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdbool.h>

#include "builtin_gen.h"
#include "symtable.h"
#include "dynamic_arr_param.h"
#include "stack_str.h"
#include "token.h"
#include "stack_tkn.h"
#include "list.h"

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

token_t *length(symtable_t *symtab, token_t *par)
{
	if (strcmp(par->name, "nil") == 0)
	{
		token_info_t info1;
		token_t *err = createToken("ERR_SEM", info1);
		return err;
	}

	static unsigned long long label_n = 0;
	char name[20];
	token_info_t info;
	sprintf(name, "LEN%lluSTR", label_n);
	info.ptr = symtab_elem_add(symtab, name);
	token_t *des = createToken("INT_ID", info);

	char frame_act [3] = "LF";
	char frame_par [7] = "LF";      //from which frame is variable ID
	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame_act, "GF");

	printf("DEFVAR %s@%s\n"
	       "MOVE %s@%s nil@nil\n"
	       "DEFVAR %s@$length$tmp%llu\n",
	       frame_act, name, frame_act, name,
	       frame_act, label_n);

	if (strcmp(par->name, "ID") == 0)
	{
		if (strcmp(symtab->name, "$GT" ) == 0)
		{
			strcpy(frame_par, "GF");
		}
		else
		{
			strcpy(frame_par, "LF");
		}

		printf("MOVE %s@$length$tmp%llu %s@%s\n "
		       "JUMPIFEQ $%s$%llu$string %s@$length$tmp%llu string@string\n"
		       "EXIT int@4\n"
		       "LABEL $%s$%llu$string\n",
		       frame_act, label_n, frame_par, par->info.ptr->var.key,
		       frame_act, label_n, frame_act, label_n, frame_act, label_n);
	}
	else if (strcmp(par->name, "STR") == 0)
	{
		strcpy(frame_par, "string");

		printf("MOVE %s@$length$tmp%llu %s@%s\n ",
		       frame_act, label_n, frame_par, par->info.string);
	}
	else {
		label_n++;
		destroyToken(par);
		destroyToken(des);  // ????
		token_info_t info1;
		token_t *error = createToken("ERR_SEM", info1);
		return error;
	}

	printf("STRLEN %s@%s %s@$length$tmp%llu\n",
	       frame_act, name, frame_act, label_n);

	label_n++;
	return des;
}

//RETURN ascii char of value par<0,255> "chr(42)"
token_t *chr(list_t *code_buffer, bool in_stat, symtable_t *symtab, token_t *par)
{
	static unsigned long long label_n = 0;
	char name[24];
	sprintf(name, "CHR%llu", label_n);
	token_info_t info;
	info.ptr = symtab_elem_add(symtab, name);
	token_t *des = createToken("STR_ID", info);

	char frame[3] = "LF";

	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame, "GF");

	if (! print_or_append(code_buffer, in_stat, "MOVE GF@$des string@\n"))
		goto err_internal;

	if (strcmp(par->name, "ID") == 0)
	{
		if (symtab_find(symtab, par->info.ptr->var.key) == NULL)	//ID has not been declareted
			goto err_sem;
			
		if (! print_or_append(code_buffer, in_stat, 
				"MOVE GF@$tmp %s@%s\n"
				"TYPE GF@$type %s@%s\n"
				"JUMPIFEQ $chr$tmp%llu$int$true GF@$type string@int\n"
				"EXIT int@4\n"
				"LABEL $chr$tmp%llu$int$true\n",
				frame, par->info.ptr->var.key,
				frame, par->info.ptr->var.key,
				label_n,
				label_n))
			goto err_internal;
	}
	else if (strcmp(par->name, "INT") == 0)
	{
		if (! print_or_append(code_buffer, in_stat,
				"MOVE GF@$tmp int@%s\n",
		      	par->info.string))
			goto err_internal;
	}
	else
		goto err_sem_type;

	if (! print_or_append(code_buffer, in_stat, "INT2CHAR GF@$des GF@$tmp\n"))        //INT2CHAR takes value <0,255>
		goto err_internal;

	destroyToken(par);
	label_n++;
	return des;

err_internal:
	label_n++;
	destroyToken(par);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_INTERNAL", info);
	return des;

err_sem:
	label_n++;
	destroyToken(par);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_SEM_UNDEF", info);
	return des;

err_sem_type:
	label_n++;
	destroyToken(par);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_SEM_TYPE", info);
	return des;
}

token_t *ord(list_t *code_buffer, bool in_stat, symtable_t *symtab, token_t *par1, token_t *par2)      //par1 == string, par2 == position(INT)
{
	static unsigned long long label_n = 0;
	char name[24];
	sprintf(name, "ORD%llu", label_n);
	token_info_t info;
	info.ptr = symtab_elem_add(symtab, name);
	token_t *des = createToken("INT_ID", info);

	char frame[3] = "LF";

	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame, "GF");

	if (! print_or_append(code_buffer, in_stat, "MOVE GF@$des int@0\n"))
		goto err_internal;
		
	/*
	GF@$tmp = par1
	GF@$not = par2
	GF@$des = return
	*/
//////////////////////**FIRST PARAMETR**////////////////////////////////
	if (strcmp(par1->name, "ID") == 0)
	{
		if (symtab_find(symtab, par1->info.ptr->var.key) == NULL)
			goto err_sem;

		if (! print_or_append(code_buffer, in_stat, 
				"MOVE GF@$tmp %s@%s\n"
				"TYPE GF@$type %s@%s\n"
				"JUMPIFEQ $ord$string%llu$string$true GF@$type string@string\n"
				"EXIT int@4\n"
				"LABEL $ord$string%llu$string$true\n",
				frame, par1->info.ptr->var.key,
				frame, par1->info.ptr->var.key,
				label_n,
				label_n))
			goto err_internal;
	}
	else if (strcmp(par1->name, "STR") == 0)
	{
		if (! print_or_append(code_buffer, in_stat, "MOVE GF@$tmp string@%s\n", par1->info.string))
			goto err_internal;
	}
	else
		goto err_sem_type;
//////////////////////**SECOND PARAMETR**////////////////////////////////
	if (strcmp(par2->name, "ID") == 0)
	{
		if (symtab_find(symtab, par2->info.ptr->var.key) == NULL)
			goto err_sem;

		if (! print_or_append(code_buffer, in_stat,
				"MOVE GF@$not %s@%s\n"
				"TYPE GF@$type %s@%s\n"
				"JUMPIFEQ $ord$position%llu$int$true GF@$type string@int\n"
				"EXIT int@4\n"
				"LABEL $ord$string%llu$string$true\n",
				frame, par2->info.ptr->var.key,
				frame, par2->info.ptr->var.key,
				label_n,
				label_n))
			goto err_internal;
	}
	else if (strcmp(par2->name, "INT") == 0)
	{
		if (! print_or_append(code_buffer, in_stat, 
				"MOVE GF@$not int@%s\n", par2->info.string))
			goto err_internal;
	}
	else
		goto err_sem_type;

	if (! print_or_append(code_buffer, in_stat,
			"STRLEN GF@$des GF@$tmp\n"
			"LT GF@$type GF@$not GF@$des\n"      //position < strlen(string)
			"JUMPIFEQ $ord%llu GF@$type bool@true\n"
			"MOVE GF@$des nil@nil\n"
			"JUMP $ord$end%llu\n"
			"LABEL $ord%llu\n"
			"STRI2INT GF@$des GF@$tmp GF@$not\n"
			"LABEL $ord$end%llu\n",
			label_n,
			label_n,
			label_n,
			label_n))
		goto err_internal;

	destroyToken(par1);
	destroyToken(par2);
	label_n++;
	return des;

err_internal:
	label_n++;
	destroyToken(par1);
	destroyToken(par2);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_INTERNAL", info);
	return des;

err_sem:
	label_n++;
	destroyToken(par1);
	destroyToken(par2);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_SEM", info);
	return des;

err_sem_type:
	label_n++;
	destroyToken(par1);
	destroyToken(par2);
	destroyToken(des);
	info.ptr = NULL;
	des = createToken("ERR_SEM_TYPE", info);
	return des;
}

token_t *substr(symtable_t *symtab, token_t *string, token_t *begin, token_t *end)
{
	static unsigned long long label_n = 0;
	char name[24];
	sprintf(name, "SUBS%llu", label_n);
	token_info_t info;
	info.ptr = symtab_elem_add(symtab, name);
	token_t *des = createToken("STR_ID", info);

	char frame[3] = "LF";

	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame, "GF");

	printf("DEFVAR %s@%s\n"
	       "MOVE %s@%s string@\n"
	       "DEFVAR %s@$substr$string%llu\n"
	       "DEFVAR %s@$substr$begin%llu\n"
	       "DEFVAR %s@$substr$end%llu\n",
	       frame, name,
	       frame, name,
	       frame, label_n,
	       frame, label_n,
	       frame, label_n);

	//////////////////////**FIRST PARAMETR**////////////////////////////////
	if (strcmp(string->name, "ID") == 0)
	{
		if (symtab_find(symtab, string->info.ptr->var.key) == NULL)
		{
			//Variable doesn't exist
			printf("EXIT int@4\n");
			label_n++;
			destroyToken(string);
			destroyToken(begin);
			destroyToken(end);
			destroyToken(des);
			token_info_t info1;
			token_t *error = createToken("ERR_SEM", info1);
			return error;
		}

		printf("MOVE %s@$substr$string%llu %s@%s\n"
		       "DEFVAR %s@$substr$string%llu$type\n"
		       "TYPE %s@$substr$string%llu$type %s@%s\n"
		       "JUMPIFEQ $substr$string%llu$string$true %s@$substr$string%llu$type string@string\n"
		       "EXIT int@4\n"
		       "LABEL $substr$string%llu$string$true\n",
		       frame, label_n, frame, string->info.ptr->var.key,
		       frame, label_n,
		       frame, label_n, frame, string->info.ptr->var.key,
		       label_n, frame, label_n,
		       label_n);
	}
	else if (strcmp(string->name, "STR") == 0)
	{
		printf("MOVE %s@$substr$string%llu string@%s\n",
		       frame, label_n, string->info.string);
	}
	else
	{
		printf("EXIT int@4\n");
		label_n++;
		destroyToken(string);
		destroyToken(begin);
		destroyToken(end);
		destroyToken(des);
		token_info_t info1;
		token_t *error = createToken("ERR_SEM", info1);
		return error;
	}

	//////////////////////**SECOND PARAMETR**////////////////////////////////
	if (strcmp(begin->name, "ID") == 0)
	{
		if (symtab_find(symtab, begin->info.ptr->var.key) == NULL)
		{
			//Variable doesn't exist
			printf("EXIT int@4\n");
			label_n++;
			destroyToken(string);
			destroyToken(begin);
			destroyToken(end);
			destroyToken(des);
			token_info_t info1;
			token_t *error = createToken("ERR_SEM", info1);
			return error;
		}

		printf("MOVE %s@$substr$begin%llu %s@%s\n"
		       "DEFVAR %s@$substr$begin%llu$type\n"
		       "TYPE %s@$substr$begin%llu$type %s@%s\n"
		       "JUMPIFEQ $substr$begin%llu$int$true %s@$substr$begin%llu$type string@int\n"
		       "EXIT int@4\n"
		       "LABEL $substr$begin%llu$int$true\n",
		       frame, label_n, frame, begin->info.ptr->var.key,
		       frame, label_n,
		       frame, label_n, frame, begin->info.ptr->var.key,
		       label_n, frame, label_n,
		       label_n);
	}
	else if (strcmp(begin->name, "INT") == 0)
	{
		printf("MOVE %s@$substr$begin%llu int@%s\n",
		       frame, label_n, begin->info.string);
	}
	else
	{
		printf("EXIT int@4\n");
		label_n++;
		destroyToken(string);
		destroyToken(begin);
		destroyToken(end);
		destroyToken(des);
		token_info_t info1;
		token_t *error = createToken("ERR_SEM", info1);
		return error;
	}

	//////////////////////**THIRD PARAMETR**////////////////////////////////
	if (strcmp(end->name, "ID") == 0)
	{
		if (symtab_find(symtab, end->info.ptr->var.key) == NULL)
		{
			//Variable doesn't exist
			printf("EXIT int@4\n");
			label_n++;
			destroyToken(string);
			destroyToken(begin);
			destroyToken(end);
			destroyToken(des);
			token_info_t info1;
			token_t *error = createToken("ERR_SEM", info1);
			return error;
		}

		printf("MOVE %s@$substr$end%llu %s@%s\n"
		       "DEFVAR %s@$substr$end%llu$type\n"
		       "TYPE %s@$substr$end%llu$type %s@%s\n"
		       "JUMPIFEQ $substr$end%llu$int$true %s@$substr$end%llu$type string@int\n"
		       "EXIT int@4\n"
		       "LABEL $substr$end%llu$int$true\n",
		       frame, label_n, frame, end->info.ptr->var.key,
		       frame, label_n,
		       frame, label_n, frame, end->info.ptr->var.key,
		       label_n, frame, label_n,
		       label_n);
	}
	else if (strcmp(end->name, "INT") == 0)
	{
		printf("MOVE %s@$substr$end%llu int@%s\n",
		       frame, label_n, end->info.string);
	}
	else
	{
		printf("EXIT int@4\n");
		label_n++;
		destroyToken(string);
		destroyToken(begin);
		destroyToken(end);
		destroyToken(des);
		token_info_t info1;
		token_t *error = createToken("ERR_SEM", info1);
		return error;
	}
/////////////////////////////**FINAL**////////////////////////////////////////

	printf("ADD %s@$substr$end%llu %s@$substr$begin%llu %s@$substr$end%llu\n"                   //%s@$substr$end%llu = begin + end == the last index
	       "DEFVAR %s@$substr$lenstr%llu\n"                                        //LENGTH OF STRING
	       "DEFVAR %s@$substr$tmp%llu\n"                                       //save one char at time to %s@$substr$tmp%llu
	       "STRLEN %s@$substr$lenstr%llu %s@$substr$string%llu\n"
	       "DEFVAR %s@$substr$cmp%llu\n"

	       "LT %s@$substr$cmp%llu %s@$substr$begin%llu int@0\n"                     //I < 0
	       "JUMPIFEQ $substr$nil%llu %s@$substr$cmp%llu bool@true\n"
	       "GT %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$lenstr%llu\n"     //I > lens(STR)
	       "JUMPIFEQ $substr$nil%llu %s@$substr$cmp%llu bool@true\n"
	       "EQ %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$lenstr%llu\n"     //I = lens(STR)
	       "JUMPIFEQ $substr$nil%llu %s@$substr$cmp%llu bool@true\n"

	       "LT %s@$substr$cmp%llu %s@$substr$end%llu int@0\n"                       //N < 0
	       "JUMPIFEQ $substr$nil%llu %s@$substr$cmp%llu bool@true\n"
	       "LT %s@$substr$cmp%llu %s@$substr$lenstr%llu %s@$substr$end%llu\n"      // LEN(STR) < IDX-max ==> do till end of STR
	       "JUMPIFEQ $substr$wholestr%llu %s@$substr$cmp%llu bool@true\n"
/////////////////////////**ONLY REQUIRED SIZE**////////////////////////////////
	       "LABEL $substr$reqsize%llu\n"
	       "LT %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$end%llu\n"       //BEGIN < IDX-max ==> again
	       "JUMPIFEQ $substr$nextchar%llu %s@$substr$cmp%llu bool@true\n"
	       //    "EQ %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$end%llu\n"    //BEGIN == IDX-max ==> once more
	       //   "JUMPIFEQ $substr$nextchar%llu %s@$substr$cmp%llu bool@true\n"
	       "JUMP $substr$end%llu\n"

	       "LABEL $substr$nextchar%llu\n"
	       "GETCHAR %s@$substr$tmp%llu %s@$substr$string%llu %s@$substr$begin%llu\n"
	       "CONCAT %s@%s %s@%s %s@$substr$tmp%llu\n"                                //frame,name,frame,name,frame,label_n
	       "ADD %s@$substr$begin%llu %s@$substr$begin%llu int@1\n"
	       "JUMP $substr$reqsize%llu\n"

/////////////////////////**TILL THE END OF STRING**////////////////////////////////
	       "LABEL $substr$wholestr%llu\n"
	       "LT %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$lenstr%llu\n"    //BEGIN < LEN(STR) ==> again
	       "JUMPIFEQ $substr$nextcharwhole%llu %s@$substr$cmp%llu bool@true\n"
	       //    "EQ %s@$substr$cmp%llu %s@$substr$begin%llu %s@$substr$lenstr%llu\n"    //BEGIN == LEN(STR) ==> once more
	       //    "JUMPIFEQ $substr$nextcharwhole%llu %s@$substr$cmp%llu bool@true\n"
	       "JUMP $substr$end%llu\n"

	       "LABEL $substr$nextcharwhole%llu\n"
	       "GETCHAR %s@$substr$tmp%llu %s@$substr$string%llu %s@$substr$begin%llu\n"
	       "CONCAT %s@%s %s@%s %s@$substr$tmp%llu\n"                            //frame,name,frame,name,frame,label_n
	       "ADD %s@$substr$begin%llu %s@$substr$begin%llu int@1\n"
	       "JUMP $substr$wholestr%llu\n"

	       "LABEL $substr$nil%llu\n"
	       "MOVE %s@%s nil@nil\n"
	       "LABEL $substr$end%llu\n",

	       frame, label_n, frame, label_n, frame, label_n,
	       frame, label_n,
	       frame, label_n,
	       frame, label_n, frame, label_n,
	       frame, label_n,

	       frame, label_n, frame, label_n,
	       label_n, frame, label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       label_n, frame, label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       label_n, frame, label_n,

	       frame, label_n, frame, label_n,
	       label_n, frame, label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       label_n, frame, label_n,

	       label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       label_n, frame, label_n,
	       label_n,

	       label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       frame, name, frame, name, frame, label_n,
	       frame, label_n, frame, label_n,
	       label_n,

	       label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       label_n, frame, label_n,
	       label_n,

	       label_n,
	       frame, label_n, frame, label_n, frame, label_n,
	       frame, name, frame, name, frame, label_n,
	       frame, label_n, frame, label_n,
	       label_n,

	       label_n,
	       frame, name,
	       label_n);

	destroyToken(string);
	destroyToken(begin);
	destroyToken(end);
	label_n++;
	return des;
}

token_t *input(symtable_t *symtab, int type)
{
	static unsigned long long label_n = 0;
	char name[20];
	token_info_t info;
	token_t *des;
	char param[7];
	char frame[3] = "LF";
	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame, "GF");

	if (type == 0)
	{
		sprintf(name, "IN%lluINT", label_n);
		info.ptr = symtab_elem_add(symtab, name);
		des = createToken("INT_ID", info);
		strcpy(param, "int");

	}
	else if (type == 1)
	{
		sprintf(name, "IN%lluDBL", label_n);
		info.ptr = symtab_elem_add(symtab, name);
		des = createToken("DBL_ID", info);
		strcpy(param, "float");
	}
	else if (type == 2)
	{
		sprintf(name, "IN%lluSTR", label_n);
		info.ptr = symtab_elem_add(symtab, name);
		des = createToken("STR_ID", info);
		strcpy(param, "string");
	}
	else
	{
		return NULL;
	}

	printf("DEFVAR %s@%s\n"
	       "READ %s@%s %s\n",
	       frame, name, frame, name, param);

	label_n++;
	return des;
}

token_t *print(symtable_t *symtab, stack_tkn_t *stack)
{
	static unsigned long long label_n = 0;
	token_info_t info;
	info.string = "nil";
	token_t *des = createToken("nil", info);
	char param[7];      //from which frame is variable ID
	char frame[3] = "LF";
	char *print;
	if (strcmp(symtab->name, "$GT" ) == 0)
		strcpy(frame, "GF");

	token_t *tmp;
	int iterator = stack->top + 1;

	/*printf("DEFVAR %s@%s\n"
	       "MOVE %s@%s nil@nil\n",
	       frame, des->info.ptr->var.key, frame, des->info.ptr->var.key);*/

	for (int i = 0; i < iterator; ++i)
	{
		tmp = stcTkn_pop(stack);
		if (strcmp(tmp->name, "ID") == 0)
		{
			if (strcmp(symtab->name, "$GT" ) == 0)
			{
				strcpy(param, "GF");
			}
			else
			{
				strcpy(param, "LF");
			}

			print = malloc(sizeof(char) * (strlen(tmp->info.ptr->var.key) + 1));
			if (print == NULL)
			{
				free(des->name);
				free(des);
				token_info_t info1;
				token_t *error = createToken("ERR_INT", info1);
				return error;
			}
			strcpy(print, tmp->info.ptr->var.key);
		}
		else
		{
			print = malloc(sizeof(char) * (strlen(tmp->info.string) + 1));
			if (print == NULL)
			{
				free(des->name);
				free(des);
				token_info_t info1;
				token_t *error = createToken("ERR_INT", info1);
				return error;
			}
			strcpy(print, tmp->info.string);
			if (strcmp(tmp->name, "INT") == 0)
			{
				strcpy(param, "int");
			}
			else if (strcmp(tmp->name, "DBL") == 0)
			{
				strcpy(param, "float");
			}
			else if (strcmp(tmp->name, "STR") == 0)
			{
				strcpy(param, "string");
			}
			else if (strcmp(tmp->name, "nil") == 0)
			{
				strcpy(param, "string");
				strncpy(print, "", 3);
			}
		}

		printf("WRITE %s@%s\n", param, print);
		destroyToken(tmp); // ????
	}

	label_n++;
	free(print);
	return des;
}

#if 0

//gcc builtin_gen.c symtable.c dynamicArrParam.c stackStr.c token.c stackTkn.c -o gen


int main()
{
	elem_t elem;
	elem.var.key = "jsemID1";
	token_info_t info;
	info.ptr = &elem;
	token_t *token = createToken("ID", info);

	elem_t elem5;
	elem5.var.key = "jsemID2";
	token_info_t info6;
	info6.ptr = &elem5;
	token_t *token5 = createToken("ID", info6);


	elem_t elem1;
	elem1.var.key = "jsem0INT_ID";
	token_info_t info2;
	info2.ptr = &elem1;
	token_t *token1 = createToken("INT_ID", info2);

	elem_t elem2;
	elem2.var.key = "jsem0STR_ID";
	token_info_t info5;
	info5.ptr = &elem2;
	token_t *token4 = createToken("STR_ID", info5);


	symtable_t *symtable = symtab_init("foo", VARIABLES);

	token_info_t tmp;
	tmp.ptr = NULL;


	token_t *op = createToken("+", tmp);
	token_info_t info3;
	info3.string = "ahoj";
	token_info_t info4;
	info4.string = "7.1";
	token_t *token2 = createToken("STR", info3);
	token_t *token3 = createToken("DBL", info4);

	stack_tkn_t *stack = stcTkn_create();
	stcTkn_push(stack, token2);
	stcTkn_push(stack, token3);
	stcTkn_push(stack, token5);
	stcTkn_push(stack, token);


	token_t *tmp12 = print(symtable, stack);
	//token_t *tmp12 = gen_expr(NULL,  token3, NULL, symtable);
	/*printf("---%s---\n", tmp12->info.ptr->var.key);
	printf("---%s---\n", tmp12->name);*/
	//token_t *tmp13 = gen_expr(op, tmp12, token2, symtable);



	/*free(info);
	free(info6);
	free(tmp12);
	//free(token);
	free(token1);
	//free(op);
	free(token2);
	free(token3);
	free(token4);*/
	//free(token5);
	symtab_free(symtable);
	return 0;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

