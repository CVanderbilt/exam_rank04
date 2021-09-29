#include "microshell.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int g_stdout;
int g_stdin;
int g_is_child;

/*
*	-Description: ft_strlen returns the length of the given
*	string.
*	-Params: (const char *str) is the string to measure.
*	-Return: It returns the length of str.
*/
int ft_strlen(const char *str)
{
	int len = 0;
	while (str[len])
		len++;
	return (len);
}

/*
*	-Description: ft_strdup returns a duplicate of the given
*	string.
*	-Params: (const char *str) is the string to be duplicated.
*	-Return: It returns the address of the new string or null
*	if allocation failed.
*/
char *ft_strdup(const char *str)
{
	int len = ft_strlen(str);
	char *ret = malloc(len + 1);

	if (ret)
	{
		for (int i = 0; str[i]; i++)
			ret[i] = str[i];
		ret[len] = 0;
	}
	return (ret);
}

/*
*	-Description: clean_list loops through the list
*	freeing the str field on each node if not null and
*	freeing each node.
*	-Params: (t_list *l) is the list to be freed.
*	-Return: It returns a null pointer which is equivalent
*	to an empty list.
*/
t_list *clean_list(t_list *l)
{
	t_list *aux;

	while (l)
	{
		if (l->str)
			free(l->str);
		aux = l;
		l = l->next;
		free(aux);
	}
	return (l);
}

/*
*	-Description: argv_to_list creates a t_list, each node of the list
*	will store each word in the argv array.
*	-Params: (char *argv[]) is the argv of the main function, an array
*	of strings. (if we want to ignore the first word on the array it
*	must be passed as &argv[1]).
*	-Return: It returns the given t_list pointer or null if allocation failed.
*/
t_list *argv_to_list(char *argv[])
{
	t_list *ret = 0;
	t_list *last = 0;

	for (int i = 0; argv[i]; i++)
	{
		t_list *aux = (t_list*)malloc(sizeof(t_list));
		if (!aux)
		{
			ret = clean_list(ret);
			break ;
		}
		if (!last)
		{
			last = aux;
			ret = last;
		}
		else
			last->next = aux;
		aux->next = 0;
		aux->str = ft_strdup(argv[i]);
		if (!aux->str)
		{
			ret = clean_list(ret);
			break ;
		}
		last = aux;
	}
	return (ret);
}

/*
*	-Description: ft_putstr writes a string at a given file
*	descriptor.
*	-Params: (int fd) is the file descriptor where the string is
*	going to be written, (const char *str) is the string to be
*	written.
*/
void ft_putstr(int fd, const char *str)
{
	write(fd, str, ft_strlen(str));
}

/*
*	-Description: ft_error prints an error message depending
*	on the error passed.
*	-Params: (t_error e) is the code of the error the error
*	to be displayed, (const char *str) is an additional string
*	used only if the error message requires extra information.
*/
void ft_error(t_error e, const char *str)
{
	if (e == err_cmd_args)
		ft_putstr(2, "error: cd: bad arguments");
	if (e == err_cmd_file)
	{
		ft_putstr(2, "error: cd: cannot change directory to ");
		ft_putstr(2, str);
	}
	if (e == err_exec)
	{
		ft_putstr(2, "error: cannot execute ");
		ft_putstr(2, str);
	}
	if (e == err_fatal)
		ft_putstr(2, "error: fatal");
	write(2, "\n", 1);
}

/*
*	-Description: count_args counts the nodes(size) of the given list.
*	-Params: (t_list *list) is the address of the list to be measured.
*	-Return: It returns the size of the list.
*/
int count_args(t_list *list)
{
	int count = 0;
	for (; list; list = list->next)
	{
		if (!list || !strcmp(list->str, ";") || !strcmp(list->str, "|"))
			break ;
		count++;
	}
	return (count);
}

/*
*	-Description: clean_argv frees each (char *) in the array and then
*	frees the array itself.
*	-Params: (char **argv) is the argv array to be freed.
*	-Return: It returns a null pointer, which is equivalentto an empty
*	array.
*/
char **clean_argv(char **argv)
{
	for (int i = 0; argv[i]; i++)
		free(argv[i]);
	free(argv);
	return (0);
}

/*
*	-Description: execute_external executes the argv array with
*	the execve function, the command is executed in a child process
*	and the parent waits for its child to terminate(when not piped),
*	if it is piped the parent wont wait and will go on to execute
*	the next command, also a pipe will be created conecting the
*	output of the child with the input of its parent. 
*	-Params: (char** argv) is the array to be executed, command +
*	args, (char** envp) is the enviroment variables array to be
*	passed on the execve call, (int piped) is a flag that states
*	if the execution will be blocking or will be piping its
*	output to the next command to be executed.
*	-Return: It returns 1 or 0 if failed or the process is a child
*	of the main process.
*/
int execute_external(char **argv, char **envp, int piped)
{
	int p[2];

	if (piped)
	{
		while(!pipe(p));
	}
	pid_t pid = fork();
	if (pid < 0)
		return (0);
	if (pid)
	{
		if (!piped)
			waitpid(pid, 0, 0);
		else
		{
			dup2(p[0], 0);
			close(p[0]);
			close(p[1]);	
		}
	}
	else
	{
		g_is_child = 1;
		if (piped)
		{
			dup2(p[1], 1);
			close(p[0]);
			close(p[1]);
		}
		execve(argv[0], argv, envp);		
		ft_error(err_exec, argv[0]);
		return (0);
	}
	return (1);
}

/*
*	-Description: execute_argv executes the argv array as if it were a
*	command on shell, if it is {cd, ...} it will attempt chdir, if not
*	it will execute the command on execute_external.
*	-Params: (int argc) is the size of the argv array, (char** argv) is
*	the null terminated array of strings to be executed, (char** envp)
*	is the enviroment variables array to be passed on the execve call,
*	(int piped) is a flag that states if the execution will be blocking
*	or will be piping its output to the next command to be executed.
*	-Return: It returns 1 or 0 if failed or the process is a child
*	of the main process.
*/
int execute_argv(int argc, char **argv, char **envp, int piped)
{
	int ret = 1;

	if (argc <= 0)
	{
		clean_argv(argv);
		return (0);
	}
	if (!strcmp(*argv, "cd"))
	{
		if (argc == 1)
			ft_error(err_cmd_args, 0);
		else
		{
			if (chdir(argv[1]))
				ft_error(err_cmd_file, argv[1]);
		}
	}
	else
	{
		if (!execute_external(argv, envp, piped))
			ret = 0;
	}
	clean_argv(argv);
	return (ret);
}


/*
*	-Description: exe_loop loops the list creating custom argv
*	arrays and use them to execute each command one after another.
*	If the command is not piped at the end of each iteration the
*	stdout and stdin file descriptors are reset to its default
*	status calling dup2.
*	-Params: (t_list *list) is the list to execute, (char **envp)
*	is the enviroment variables array to be passed on the execve
*	call.
*	-Return: It returns 1 on success and 0 on fatal fail.
*/
int exe_loop(t_list *list, char **envp)
{
	char **argv;
	int argc;
	int piped = 0;

	while (list)
	{
		argc = count_args(list);
		if (argc > 0)
		{
			argv = (char **)malloc(sizeof(char *) * (argc + 1));
			if (!argv)
				return (0);
			argv[argc] = 0;
			for (int i = 0; i < argc; i++)
			{
				argv[i] = list->str;
				list->str = 0;
				list = list->next;
			}
			piped = list && !strcmp(list->str, "|");
			if (!execute_argv(argc, argv, envp, piped))
				return (0);
			if (!piped)
			{
				dup2(g_stdin, 0);
				dup2(g_stdout, 1);
			}
		}
		if (list)
			list = list->next;
	}
	return (1);
}

int main(int argc, char *argv[], char **envp)
{
	t_list *list;
	int ret = 0;

	g_stdin = dup(0);
	g_stdout = dup(1);
	g_is_child = 0;

	if (argc <= 1)
		return (1);
	
	list = argv_to_list(&argv[1]);
	if (!list)
	{
		ft_error(err_fatal, 0);
		return (1);
	}

	if (!exe_loop(list, envp))
	{
		if (!g_is_child)
			ft_error(err_fatal, 0);
		ret = 1;
	}

	clean_list(list);
	close(g_stdin);
	close(g_stdout);
	close(1);
	close(0);
	close(2);

	if (!g_is_child)
	{
		system("leaks a.out");
		system("lsof -c a.out");
	}
	return (0);
}