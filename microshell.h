#ifndef MICROSHELL_H
# define MICROSHELL_H

	typedef struct s_list
	{
		char *str;
		void *next;
	}	t_list;

	typedef enum e_error
	{
		err_cmd_args,
		err_cmd_file,
		err_exec,
		err_fatal
	}	t_error;

#endif