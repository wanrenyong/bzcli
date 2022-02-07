#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <cligen/cligen.h>

#include "bzcli_signal.h"
#include "bzcli.h"

static cg_var *bzcli_buildin_func_var_get(cligen_handle h, cvec *cvv, char *name)
{
	if(name != NULL && strlen(name) > 2){
		if(name[0] == '$' && name[1] == '_'){
			return cvec_find(cvv, name+2);
		}
	}
	return NULL;
}

/**
*  bzcli_buildin_cli_tree - show cli tree
*/
static int bzcli_buildin_cli_tree(cligen_handle h, cvec *cvv, cvec *argv)
{
	parse_tree     *pt;
	pt_head *ph = NULL;
	while ((ph = cligen_ph_each(h, ph)) != NULL) {
	    pt = cligen_ph_parsetree_get(ph);
	    printf("Syntax:\n");
	    pt_print(stdout, pt, 0);
	}
	fflush(stdout);
}

static void bzcli_cvv2env(cvec *cvv, char *env[], int num)
{
	cg_var *cv;
	char str[64];
	char buf[32];
	int len;
	int i;
	
	if(cvv == NULL)
		return;

	i = 0;
	cv = NULL;
	while ((cv = cvec_each(cvv, cv)) != NULL && i < num) {
		if(!cv_const_get(cv)){
			cv2str(cv, buf, sizeof(buf)-1);
			len = snprintf(str, sizeof(str), "_%s=%s", 
				cv_name_get(cv), buf);
			if(len > 0){
				env[i++] = strdup(str);
			}
		}
	}
	return;
}

#define MAX_NUM_ENV 32
static int bzcli_buildin_shell(cligen_handle h, cvec *cvv, cvec *argv)
{
	int pid = 0;
	int status;
	char *cmd[4] = {"/bin/bash", NULL, NULL, NULL};
	
	pid = vfork();
	if(pid < 0){
		cligen_output(stderr, "Could not fork process!");
		return 0;
	}else if(pid == 0){
		int i;
		char *env[MAX_NUM_ENV+1];
		memset(env, 0, sizeof(env));
		bzcli_cvv2env(cvv, env, MAX_NUM_ENV);
		if(argv != NULL){
			cmd[1] = "-c";
			cmd[2] = cv_string_get(cvec_i(argv, 0));
		}
		execvpe(cmd[0], cmd, env);	
		i = 0;
		while(env[i] != NULL){
			free(env[i++]);
		}
		exit(0);
	}
	waitpid(pid, &status, 0);
	return 0;	
}


/*! CLI generic callback printing the variable vector and argument
 */
static int bzcli_buildin_debug(cligen_handle h,
	 cvec         *cvv,
	 cvec         *argv)
{
    int     i = 0;
    cg_var *cv;
    char    buf[64];

    cligen_output(stderr, "function: %s\n", cligen_fn_str_get(h));
    cligen_output(stderr, "variables:\n");
    cv = NULL;
    while ((cv = cvec_each(cvv, cv)) != NULL) {
	cv2str(cv, buf, sizeof(buf)-1);
	cligen_output(stderr, "\t%d name:%s type:%s value:%s, is-key:%d\n", 
		i++, 
		cv_name_get(cv),
		cv_type2str(cv_type_get(cv)),
		buf, cv_const_get(cv)
	    );
    }
    if (argv){
	    cv = NULL;
	    i=0;
	    while ((cv = cvec_each(argv, cv)) != NULL) {
		cv2str(cv, buf, sizeof(buf)-1);
		cligen_output(stderr, "arg %d: %s\n", i++, buf);
	    }
	}
    return 0;
}


static int bzcli_buildin_logout(cligen_handle h, cvec *cvv, cvec *argv)
{
	cligen_exiting_set(h, 1);
    return 0;
}


static int bzcli_buildin_hostname(cligen_handle h, cvec *cvv, cvec *argv)
{
	cg_var  *arg;
	cg_var  *v = NULL;
	char    buf[32];
	char    prompt[64];
	
	arg = cvec_i(argv, 0);
	if(arg != NULL){
		v = bzcli_buildin_func_var_get(h, cvv, cv_string_get(arg));
		if(v == NULL)
			v = arg;
	}
	if(v != NULL){
		cv2str(v, buf, sizeof(buf)-1);
		snprintf(prompt, sizeof(prompt), "%s> ", buf);
		cligen_prompt_set(h, prompt);
		return 0;
	}
	return 0;
}

static int bzcli_buildin_history(cligen_handle h, cvec *cvv, cvec *argv)
{
	
	if(argv != NULL){
		int line;
		cg_var  *cv;
		line = CLIGEN_HISTSIZE_DEFAULT;
		cv = bzcli_buildin_func_var_get(h, cvv, cv_string_get(cvec_i(argv, 0)));
		if(cv != NULL && cv_type_get(cv) == CGV_UINT32){
			line = cv_uint32_get(cv);
			cligen_output(stderr, "Reset max number of history line to %d\n", line);
		}
		cligen_hist_init(h, line);
	}
	else
		cligen_hist_file_save(h, stdout);
	return 0;
}

static int bzcli_buildin_changetree(cligen_handle h,
	   cvec         *cvv,
	   cvec         *argv)
{
    cg_var *cv;
    char *treename;
	pt_head *ph;

	ph = cligen_ph_active_get(h);

    cv = cvec_i(argv, 0);
    treename = cv_string_get(cv);
	cligen_ph_active_set_byname(h, treename);
	cv = cvec_i(argv, 1);
	if(cv != NULL && cv_string_get(cv) != NULL)
		cligen_prompt_set(h, cv_string_get(cv));
	
    return 0;
}

static int bzcli_buildin_exit(cligen_handle h,
	   cvec         *cvv,
	   cvec         *argv)
{
  pt_head *active, *head;
  cg_var *cv;

  active = cligen_ph_active_get(h);
  head = cligen_pt_head_get(h);
  if(active == head){
  	cligen_exiting_set(h, 1);
	return 0;
  }
  cligen_ph_active_set_byname(h, cligen_ph_name_get(head));
  cv = cvec_i(argv, 0);
  if(cv != NULL && cv_string_get(cv) != NULL)
	cligen_prompt_set(h, cv_string_get(cv));
  return 0;
}


static int bzcli_buildin_prompt(cligen_handle h,
	  cvec		   *cvv,
	  cvec		   *argv)
{
 	if(argv != NULL){
		cg_var  *cv;
		char    prompt[64];
		cv = bzcli_buildin_func_var_get(h, cvv, cv_string_get(cvec_i(argv, 0)));
		if(cv != NULL){
			cv2str(cv, prompt, sizeof(prompt)-1);
			cligen_prompt_set(h, prompt);
		}else
			cligen_prompt_set(h, cv_string_get(cvec_i(argv, 0)));
	}
 	return 0;
}


static int bzcli_buildin_loadcmd(cligen_handle h,
	  cvec		   *cvv,
	  cvec		   *argv)
{
	if(argv != NULL){
		cg_var  *cv;
		char    filename[64];
		cv = bzcli_buildin_func_var_get(h, cvv, cv_string_get(cvec_i(argv, 0)));
		if(cv != NULL){
			cv2str(cv, filename, sizeof(filename)-1);
			fprintf(stderr, "Loading command from %s\n", filename);
			bzcli_load_cmd(h, filename);
		}else{
			fprintf(stderr, "Loading command from %s\n", cv_string_get(cvec_i(argv, 0)));
			bzcli_load_cmd(h, cv_string_get(cvec_i(argv, 0)));
		}
	}
 	return 0;
}


cgv_fnstype_t *bzcli_buildin_func(char *name, void *arg, char **error)
{
	*error = NULL;
	if (strcmp(name, "_logout") == 0)
		return bzcli_buildin_logout;
	else if(strcmp(name, "_debug") == 0)
		return bzcli_buildin_debug;
	else if(strcmp(name, "_hostname") == 0)
		return bzcli_buildin_hostname;
	else if(strcmp(name, "_shell") == 0) 
		return bzcli_buildin_shell;
	else if(strcmp(name, "_tree") == 0) 
		return bzcli_buildin_cli_tree;
	else if(strcmp(name, "_history") == 0) 
		return bzcli_buildin_history;
	else if(strcmp(name, "_changetree") == 0) 
		return bzcli_buildin_changetree;
	else if(strcmp(name, "_exit") == 0) 
		return bzcli_buildin_exit;
	else if(strcmp(name, "_prompt") == 0)
		return bzcli_buildin_prompt;
	else if(strcmp(name, "_loadcmd") == 0)
		return bzcli_buildin_loadcmd;

    return NULL;
}




