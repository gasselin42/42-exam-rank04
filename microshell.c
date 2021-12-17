/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gasselin <gasselin@student.42quebec.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/15 14:08:54 by gasselin          #+#    #+#             */
/*   Updated: 2021/12/17 15:10:57 by gasselin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int	print_error(const char *s)
{
	int i = 0;

	while (s[i])
		i++;
	write(STDERR_FILENO, s, i);
	return (EXIT_FAILURE);
}

int fatal_error(void)
{
	exit (print_error("error: fatal\n"));
}

char	**next_pipe(char **cmd)
{
	int i = 0;

	if (!cmd)
		return (NULL);
	while (cmd[i])
	{
		if (!strcmp(cmd[i], "|"))
			return (cmd + i + 1);
		i++;
	}
	return (NULL);
}

int cmd_size(char **cmd)
{
	int i = 0;
	
	if (!cmd)
		return (0);
	while (cmd[i])
	{
		if (!strcmp(cmd[i], "|"))
			return (i);
		i++;
	}
	return (i);
}

int exec_cd(char **cmd)
{
	if (!cmd[1] || cmd[2])
		return (print_error("error: cd: bad arguments\n"));
	if (chdir(cmd[1]) == -1)
	{
		print_error("error: cd: cannot change directory to ");
		print_error(cmd[1]);
		return (print_error("\n"));
	}
	return (0);
}

int exec_cmd(char **cmd, char **envp)
{
	pid_t pid;
	
	if ((pid = fork()) < 0)
		fatal_error();
	if (pid == 0)
	{
		if (execve(cmd[0], cmd, envp) < 0)
		{
			print_error("error: cannot execute ");
			print_error(cmd[0]);
			exit(print_error("\n"));
		}
	}
	waitpid(0, NULL, 0);
	return (0);
}

int exec_son(char **cmd, char **envp, int in, int fd[2])
{
	if (dup2(in, STDIN_FILENO) < 0)
		fatal_error();
	if (next_pipe(cmd) && dup2(fd[1], STDOUT_FILENO) < 0)
		fatal_error();
	close(in);
	close(fd[0]);
	close(fd[1]);
	cmd[cmd_size(cmd)] = NULL;
	exec_cmd(cmd, envp);
	exit (0);
}

int	execute(char **cmd, char **envp)
{
	pid_t	pid;
	int in;
	int fd[2];
	int nb_wait = 0;
	char	**tmp = cmd;
	
	if (!cmd[0])
		return (0);
    if (!strcmp(cmd[0], "cd"))
		return (exec_cd(cmd));
	if (!next_pipe(cmd))
		return (exec_cmd(cmd, envp));
	if ((in = dup(STDIN_FILENO)) < 0)
		return (fatal_error());
	while (tmp)
	{
		if (pipe(fd) < 0 || (pid = fork()) < 0)
			fatal_error();
		if (pid == 0)
			exec_son(tmp, envp, in, fd);
		else
		{
			if (dup2(fd[0], in) < 0)
				fatal_error();
			close(fd[0]);
			close(fd[1]);
			tmp = next_pipe(tmp);
			nb_wait++;
		}
	}
	close(in);
	while (nb_wait-- > 0)
		waitpid(0, NULL, 0);
	return (0);
}

int main(int ac, char **av, char **envp)
{
	int i = 1;
	int start = 1;

	if (ac < 2)
		return (0);
	while (av[i])
	{
		if (!strcmp(av[i], ";"))
		{
			av[i] = 0;
			execute(av + start, envp);
			i++;
			while (av[i] && !strcmp(av[i], ";"))
				i++;
			start = i;
		}
		else
			i++;
	}
	execute(av + start, envp);
	return (0);
}
