#ifndef _TEMPLATES
#define _TEMPLATES

struct templ_var {
	char var_name[32];
	char var_val[32];
};

#define templ_vars_setup(vars) \
	_templ_vars_setup(vars, sizeof(vars) / sizeof(vars[0]));

#define templ_var_name_len (sizeof(((struct templ_var *)0)->var_name))
#define templ_var_val_len (sizeof(((struct templ_var *)0)->var_val))

void _templ_vars_setup(struct templ_var *vars, int count);
void dump_templ_vars(void);
struct templ_var *get_templ_var(char *var_name);
char *get_templ_var_val(char *var_name);
void set_templ_var_val(char *var_name, char *var_val);
int parse_templ_string(char *buf, int len);
void parse_templ(char *fname);
struct templ_var *parse_templ_var_str(char *buf, int len);
int load_templ_vars_vals_from_file(char *fname);

#endif /* _TEMPLATES */
