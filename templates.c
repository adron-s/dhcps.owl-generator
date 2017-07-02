#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "templates.h"
/* реализация простого геренатора конфигов из шаблона
	 имя переменной описывается так: %VAR_NAME%

	 так же умеет загружать значения для переменных
	 templ_vars из xml файла.
*/

struct templ_var *templ_vars = NULL;
int templ_vars_count = 0;

//**********************************************************************************
/* выполняет загрузку переменных и их значений в массив templ_vars */
void _templ_vars_setup(struct templ_var *vars, int count){
	templ_vars = vars;
	templ_vars_count = count;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* печатает текущее содержимое templ_vars */
void dump_templ_vars(void){
	int a;
	int var_max_len = 0;
	char templ[32];
	for(a = 0; a < templ_vars_count; a++){
		if(strlen(templ_vars[a].var_name) > var_max_len)
			var_max_len = strlen(templ_vars[a].var_name);
	}
	snprintf(templ, sizeof(templ), "%%-%ds := \"%%s\"\n", var_max_len);
	for(a = 0; a < templ_vars_count; a++){
		printf(templ, templ_vars[a].var_name, templ_vars[a].var_val);
	}
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* ищет шаблонную переменную по ее имени и возвращает указатель на ее
 	 структуру или NULL если такой переменной нет */
struct templ_var *get_templ_var(char *var_name){
	int a;
	for(a = 0; a < templ_vars_count; a++){
		if(!strcmp(templ_vars[a].var_name, var_name))
			return &templ_vars[a];
	}
	return NULL;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* ищет шаблонную переменную по ее имени и возвращает указаткль на ее
	 строку значения или NULL если такой переменной нет */
char *get_templ_var_val(char *var_name){
	struct templ_var *var = get_templ_var(var_name);
	if(!var)
		return NULL;
	return var->var_val;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* устанавливает значение для шаблонной переменной с заданным именем */
void set_templ_var_val(char *var_name, char *var_val){
	struct templ_var *var = get_templ_var(var_name);
	if(!var)
		return;
	memset(var->var_val, 0x0, sizeof(var->var_val));
	if(var_val)
		strncpy(var->var_val, var_val, sizeof(var->var_val) - 1);
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* провяет все переменные шаблона на заполненность значения */
int check_all_templ_vars_for_zerofill(void){
	int a;
	for(a = 0; a < templ_vars_count; a++){
		if(templ_vars[a].var_val[0] == '\0')
			return 1;
	}
	return 0;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* парсит строку шаблона находя начало и конец переменной */
int parse_templ_string(char *buf, int len){
	int ret = 0;
	int a;
	char *res;
	char var_name[templ_var_name_len];
	char *vp = NULL;
	char *rp = NULL;
	int rest = 0;
	int rrest = 0;
	char *start = NULL;
	res = malloc(len);
	if(!res)
		return -1;
	memset(res, 0x0, len);
	rp = res;
	rrest = len - 1;
	for(a = 0; a < len - 1; a++){
		if(buf[a] == '\0')
			break;
		if(buf[a] == '%'){
			if(start == NULL){
				//начало переменной. подготовка к ее парсингу.
				memset(var_name, 0x0, sizeof(var_name));
				rest = sizeof(var_name) - 1;
				vp = var_name;
				start = &buf[a + 1];
			}else{
				//достигнут конец переменной
				struct templ_var *var = get_templ_var(var_name);
				int len = 0;
				if(var == NULL){
					//неизвестная переменная!
					fprintf(stderr,
						"ERROR: Unknown var '%%%s%%' found in template!%s!\n",
						var_name, buf);
					ret = -1;
					goto end;
				}
				if(rrest > 0) //если есть место в приемном буфере
					len = strlen(strncpy(rp, var->var_val, rrest));
				rrest -= len;
				if(rrest > 0)
					rp += len;
				start = NULL;
				vp = NULL;
			}
			continue;
		}
		//копируем символ в текущий объект приема
		if(start && rest-- > 0)
			*(vp++) = buf[a];
		//копируем символ в буфер результата
		else if(rrest -- > 0)
			*(rp++) = buf[a];
	}
	//копируем буфер результата обратно в buf
	memcpy(buf, res, len);
end:
	free(res);
	return ret;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* парсит шаблон подставляя в него значения переменных */
void parse_templ(char *fname){
	FILE* fd;
	char buf[255];
	fd = fopen(fname, "r");
	if(!fd){
		perror("Can't open templ file");
		exit(-1);
	}
	//построчно читаем файл
	while(fgets(buf, sizeof(buf), (FILE*)fd)){
		if(parse_templ_string(buf, sizeof(buf))){
			fclose(fd);
			exit(-1);
		}
		printf("%s", buf);
	}
	fclose(fd);
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* парсит строку вида <VAR_NAME>VAR_VAL</VAR_NAME>\n
	 каждая переменная должна идти с новой строки! */
struct templ_var *parse_templ_var_str(char *buf, int len){
	int a;
	static struct templ_var templ_var;
	char verify_var_name[templ_var_name_len];
	int rest = 0;
	//указатель на текущий объект приема данных
	char *r = NULL; //(имя переменной, значение переменной)
	int stage = 0;
	memset(&templ_var, 0x0, sizeof(templ_var));
	for(a = 0; a < len - 1; a++){
		if(buf[a] == '\n' || buf[a] == '\0')
			break;
		if(buf[a] == '<' && buf[a + 1] != '/'){
			stage = 1; //началиный xml тег
			//готовимся принять имя переменной
			rest = sizeof(templ_var.var_name) - 1;
			r = templ_var.var_name;
			memset(&templ_var, 0x0, sizeof(templ_var));
			continue;
		}
		if(buf[a] == '<' && buf[a + 1] == '/' && stage == 1){
			stage = 2; //конечный '/' xml тег
			//проверим что имя этой переменной нам известно/интересно
			if(!get_templ_var(templ_var.var_name))
				return NULL; //если нет то и нет смысла дальше продолжать
			//готовимся принять повторно имя переменной для верфицикации
			rest = sizeof(verify_var_name) - 1;
			r = verify_var_name;
			memset(&verify_var_name, 0x0, sizeof(verify_var_name));
			a++; //пропускаем следующий символ '/'
			continue;
		}
		if(buf[a] == '>' && stage == 1){
			//закончился начальный xml тег
			//готовимся принять значение переменной
			rest = sizeof(templ_var.var_val) - 1;
			r = templ_var.var_val;
			continue;
		}
		if(buf[a] == '>' && stage == 2){
			//закончился конечный xml тег
			//проводим верификацию и возвращаем результат.
			if(strcmp(templ_var.var_name, verify_var_name)){
				fprintf(stderr, "XML content has errors: !%s!\n", buf);
				return NULL;
			}
			return &templ_var;
		}

		//копируем символ в текущий объект приема
		if(rest-- > 0) //защита от переполнения
			*(r++) = buf[a];
	}
	return NULL;
}//---------------------------------------------------------------------------------

//**********************************************************************************
/* загружает из XML файла значения для переменных шаблона */
int load_templ_vars_vals_from_file(char *fname){
	FILE* fd;
	char buf[255];
	fd = fopen(fname, "r");
	if(!fd) return -1;
	//построчно читаем файл
	while(fgets(buf, sizeof(buf), (FILE*)fd)){
		struct templ_var *var_defs, *var;
		var_defs = parse_templ_var_str(buf, sizeof(buf));
		if(!var_defs)
			continue;
		var = get_templ_var(var_defs->var_name);
		if(var)
			strcpy(var->var_val, var_defs->var_val);
	}
	fclose(fd);
	return 0;
}//---------------------------------------------------------------------------------
