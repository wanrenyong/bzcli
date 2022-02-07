#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#include <cligen/cligen.h>

#include "bzcli_buildin.h"
#include "bzcli_signal.h"


typedef cgv_fnstype_t *bzcli_str2fn_t(char *name, void *arg, char **error);
typedef translate_cb_t *bzcli_str2fn_trans_t(char  *name,void  *arg, char **error);
typedef expandv_cb *bzcli_str2fn_exp_t(char  *name, void  *arg, char **error);

static bzcli_str2fn_t *bzcliStr2fn;
static bzcli_str2fn_trans_t *bzcliStr2fnTrans;
static bzcli_str2fn_exp_t *bzcliStr2fnExp;


static int bzcli_loop(cligen_handle h)
{
	int           retval = -1;
    char         *line;
    int           callback_ret = 0;
    char         *reason = NULL;
    cligen_result result;
    
    /* Run the CLI command interpreter */
    while (!cligen_exiting(h)){
	if (cliread_eval(h, &line, &callback_ret, &result, &reason) < 0)
	    goto done;
	switch (result){
	case CG_EOF: /* eof */
	    cligen_exiting_set(h, 1);
	    break;
	case CG_ERROR: /* cligen match errors */
	    printf("CLI read error\n");
	    goto done;
	case CG_NOMATCH: /* no match */
	    printf("CLI syntax error in: \"%s\": %s\n", line, reason);
	    break;
	case CG_MATCH: /* unique match */
	    if (callback_ret < 0)
		printf("Failed to execute command, return code=%d\n", callback_ret);
	    break;
	default: /* multiple matches */
	    printf("Ambiguous command\n");
	    break;
	}
	if (reason){
	    free(reason);
	    reason = NULL;
	}
    }
    retval = 0;
 done:
    if (reason)
	free(reason);
    return retval;
}


int bzcli_exec_cmd(cligen_handle      h, char *cmd)
{
	int           callback_ret = 0;
	int           parse_ret = 0;
    char         *reason = NULL;
    cligen_result result;
    cg_obj      *matchobj = NULL;    /* matching syntax node */
    cvec        *cvv = NULL;
    parse_tree  *pt = NULL;     /* Orig */
    cg_callback *callbacks = NULL;
	int ret = -1;

	if ((pt = cligen_pt_active_get(h)) == NULL){
		fprintf(stderr, "No active parse-tree found\n");
		return -1;
    }
	parse_ret = cliread_parse(h, cmd, pt, &matchobj, &cvv, &callbacks, &result, &reason);
	if (parse_ret < 0)
		goto done;
	
	if (result == CG_MATCH){
		callback_ret = cligen_eval(h, matchobj, cvv, callbacks);
		if(callback_ret){
			goto done;
		}
	}
	else if(result != CG_MULTIPLE){
		goto done;
	}

	ret = 0;

done:
	if(ret){
		switch (result){
		case CG_EOF: /* eof */
		    cligen_exiting_set(h, 1);
		    break;
		case CG_ERROR: /* cligen match errors */
		    printf("CLI read error\n");
		    break;
		case CG_NOMATCH: /* no match */
		    printf("CLI syntax error in: \"%s\": %s\n", cmd, reason);
		    break;
		case CG_MATCH: /* unique match */
		    if (callback_ret < 0)
				printf("CLI callback error\n");
		    break;
		default: /* multiple matches */
		    printf("Ambiguous command\n");
		    break;
		}
	}
	if (callbacks)
		co_callbacks_free(&callbacks);
	if (matchobj)
		co_free(matchobj, 0);
	if (cvv)
		cvec_free(cvv); 
	if(reason != NULL)
		free(reason);
	
	return ret;
}


int bzcli_load_cmd(cligen_handle h, char *cmdfile)
{

	FILE          *f = stdin;
	char *buf = NULL;
	int line;
	int ret = 0;

	if(cmdfile != NULL){
		f = fopen(cmdfile, "r");
		if(f == NULL){
			fprintf(stderr, "Could not open cmd file:%s\n", cmdfile);
			goto done;
		}
	}

	buf = malloc(1024);
	if(buf == NULL){
		fprintf(stderr, "Could not alloc load buf\n");
		goto done;
	}
	line = 0;
	while(fgets(buf, 1023, f) != NULL){
		ret = bzcli_exec_cmd(h, buf);
		if(ret){
			fprintf(stderr, "Loading cmd error, line:%d!\n", line);
			goto done;
		}
		line++;
	}
	ret = 0;
done:
	if(buf != NULL)
		free(buf);
	if(cmdfile != NULL && f != NULL)
		fclose(f);
	return ret;
}

static void bzcli_print_banner(cligen_handle h, char *bannerfile)
{
	FILE *f;
	char c;

	if(bannerfile == NULL)
		return;
	f = fopen(bannerfile, "r");
	if(f == NULL){
		fprintf(stderr, "Could not open banner file:%s\n", bannerfile);
		return;
	}

	while (1){ /* read the whole file */
		if ((c = fgetc(f)) == EOF)
			break;
		fputc(c, stderr);
	}
	fclose(f);
	return;
}



static cgv_fnstype_t *bzcli_str2fn(char *name, void *arg, char **error)
{
	*error = NULL;
	cgv_fnstype_t *f;

	f = bzcli_buildin_func(name, arg, error);
	if(f != NULL)
		return f;
	if(bzcliStr2fn != NULL)
		return bzcliStr2fn(name, arg, error);
    return NULL;
}


static expandv_cb *bzcli_str2fn_exp(char  *name,
	   void  *arg,
	   char **error)
{
	if(bzcliStr2fnExp != NULL)
		return bzcliStr2fnExp(name, arg, error);
    return NULL;
}

static translate_cb_t *bzcli_str2fn_trans(char  *name,
		void  *arg,
		char **error)
{
	if(bzcliStr2fnTrans != NULL)
		return bzcliStr2fnTrans(name, arg, error);
   return NULL;
}

static void usage(char *argv)
{
	fprintf(stderr, "Usage:%s [-h][-f <filename>][-q] [-l <cmdfile>] [-b <bannerfile>], where the options have the following meaning:\n"
		"\t-h \t\tHelp\n"
		"\t-1 \t\tOnce only. Do not enter interactive mode\n"
		"\t-f <file> \tConfig-file (or stdin) Example use: tutorial.cli for \n"
		"\t-p \t\tPrint cli syntax\n"
		"\t-l <file> \t\tLoad command file\n"
		"\t-b <file> \t\tLoad banner from file\n"
		"\t-C \t\tDont copy treeref mode\n"
		,
		argv);
	exit(0);
}





/* Main */
int
main(int   argc,
     char *argv[])
{
    cligen_handle   h;
    int             retval = -1;
    parse_tree     *pt;
    pt_head        *ph;
    FILE           *f = stdin;
    char           *argv0 = argv[0];
    char           *filename=NULL;
    cvec           *globals;   /* global variables from syntax */
    char           *str;
    int             quiet = 1;
	int once = 0;
	char *cmdfile = NULL;
	char *banner_file = NULL;

    if ((h = cligen_init()) == NULL)
        goto done;    
    argv++;argc--;
    for (;(argc>0)&& *argv; argc--, argv++){
        if (**argv != '-')
            break;
        (*argv)++;
        if (strlen(*argv)==0)
            usage(argv0);
        switch(**argv) {
        case 'h': /* help */
            usage(argv0); /* usage exits */
            break;
        case 'p': /* quiet */
	    	quiet = 0;
            break;
		case '1': /* quiet */
	    	once = 1;
            break;
		case 'l': /*load cmd file*/
			 argc--;argv++;
			 cmdfile = *argv;
			 break;
		case 'b': /*load banner file*/
			 argc--;argv++;
			 banner_file = *argv;
			 break;
        case 'f' : 
            argc--;argv++;
            filename = *argv;
            if ((f = fopen(filename, "r")) == NULL){
				fprintf(stderr, "fopen %s: %s\n", filename, strerror(errno));
                exit(1);
            }
            break;
		case 'C': /* Dont copy reftree mode */
	    	cligen_reftree_copy_set(h, 0);
	    	break;
       default:
            usage(argv0);
            break;
        }
    }
    if ((globals = cvec_new(0)) == NULL)
		goto done;
    if (cligen_parse_file(h, f, filename?filename:"stdin", NULL, globals) < 0)
        goto done;
    ph = NULL;
    while ((ph = cligen_ph_each(h, ph)) != NULL) {
		pt = cligen_ph_parsetree_get(ph);
		if (cligen_callbackv_str2fn(pt, bzcli_str2fn, NULL) < 0) /* map functions */
		    goto done;
		if (cligen_expandv_str2fn(pt, bzcli_str2fn_exp, NULL) < 0)
		    goto done;
		if (cligen_translate_str2fn(pt, bzcli_str2fn_trans, NULL) < 0)     
		    goto done;
    }
    if ((str = cvec_find_str(globals, "prompt")) != NULL)
        cligen_prompt_set(h, str);
    if ((str = cvec_find_str(globals, "comment")) != NULL)
        cligen_comment_set(h, *str);
    if ((str = cvec_find_str(globals, "tabmode")) != NULL){
		if (strcmp(str,"long") == 0)
		    cligen_tabmode_set(h, CLIGEN_TABMODE_COLUMNS);
    }
	cvec_free(globals);
    if (!quiet){
		ph = NULL;
		while ((ph = cligen_ph_each(h, ph)) != NULL) {
		    pt = cligen_ph_parsetree_get(ph);
		    printf("Syntax:\n");
		    pt_print(stdout, pt, 0);
		}
		fflush(stdout);
    }
	if(cmdfile){
		if(bzcli_load_cmd(h, cmdfile))
			goto done;
	}
	bzcli_signal_init();
	if(once){
		retval = 0;
		goto done;
	}
	bzcli_print_banner(h, banner_file);
    if (bzcli_loop(h) < 0)
		goto done;
    retval = 0;
 done:
    fclose(f);
    if (h)
		cligen_exit(h);
    return retval;
}


