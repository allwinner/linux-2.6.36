#include "OSAL_Parser.h"

int OSAL_script_parser_fetch(char *main_name, char *sub_name, int value[], int count)
{
	return script_parser_fetch(main_name, sub_name, value, count);
}