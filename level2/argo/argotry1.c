#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h> // change this to <stdlib.h>

typedef struct	json {
	enum {
		MAP,
		INTEGER,
		STRING
	} type;
	union {
		struct {
			struct pair	*data;
			size_t		size;
		} map;
		int	integer;
		char	*string;
	};
}	json;

typedef struct	pair {
	char	*key;
	json	value;
}	pair;

int	parser(json *file, FILE *stream);

void	free_json(json j);
int	argo(json *dst, FILE *stream);

int	peek(FILE *stream)
{
	int	c = getc(stream);
	ungetc(c, stream);
	return c;
}

void	unexpected(FILE *stream)
{
	if (peek(stream) != EOF)
		printf("unexpected token '%c'\n", peek(stream));
	else
		printf("unexpected end of input\n");
}

int	accept(FILE *stream, char c)
{
	if (peek(stream) == c)
	{
		(void)getc(stream);
		return 1;
	}
	return 0;
}

int	expect(FILE *stream, char c)
{
	if (accept(stream, c))
		return 1;
	unexpected(stream);
	return 0;
}

void	free_json(json j)
{
	switch (j.type)
	{
		case MAP:
			for (size_t i = 0; i < j.map.size; i++)
			{
				free(j.map.data[i].key);
				free_json(j.map.data[i].value);
			}
			free(j.map.data);
			break ;
		case STRING:
			free(j.string);
			break ;
		default:
			break ;
	}
}

void	serialize(json j)
{
	switch (j.type)
	{
		case INTEGER:
			printf("%d", j.integer);
			break ;
		case STRING:
			putchar('"');
			for (int i = 0; j.string[i]; i++)
			{
				if (j.string[i] == '\\' || j.string[i] == '"')
					putchar('\\');
				putchar(j.string[i]);
			}
			putchar('"');
			break ;
		case MAP:
			putchar('{');
			for (size_t i = 0; i < j.map.size; i++)
			{
				if (i != 0)
					putchar(',');
				serialize((json){.type = STRING, .string = j.map.data[i].key});
				putchar(':');
				serialize(j.map.data[i].value);
			}
			putchar('}');
			break ;
	}
}

char	*ft_strdup(char *src)
{
	int	size = 0;
	while (src[size])
	{
		size++;
	}
	char *new;
	new = malloc(sizeof(char) * (size + 1));
	if (new == NULL)
        return (NULL);
	int i = 0;
	while (i < size)
	{
		new[i] = src[i];
		i++;
	}
	new[i] = '\0';
	return (new);
}



int	parse_string(json *file, FILE *stream)
{
	char	c;
	char	buffer[5000];
	int		i;

	i = 0;
	if (!expect(stream, '"'))
	{
		return (-1);
	}
	while (1)
	{
		c = getc(stream);
		if (c == EOF)
		{
			unexpected(stream);
			return (-1); //HERE RETURN AN ERROR!!!
		}
		else if (c == '\\')
		{
			c = getc(stream);
			if (c == EOF)
			{
				unexpected(stream);
				return (-1);
			}
		}
		else if (c == '"')
		{
			break;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';
	file->type = STRING;
	file->string = ft_strdup(buffer);
	return (1);
}

int	parse_int(json *file, FILE *stream)
{
	int	n;
	if (fscanf(stream, "%d", &n) == 1)
	{
		file->type = INTEGER;
		file->integer = n;
	}
	else
	{
		unexpected(stream);
		return (-1);
	}
	return (1);
}

int parse_map(json *file, FILE *stream)
{
	json	key;
	pair	*items;
	size_t	size;

	size = 0;
	items = NULL;
	if (!expect(stream, '{'))
		return (-1);
	while (!accept(stream, '}'))
	{
		items = realloc(items, sizeof(pair) * (size + 1));
		if (parse_string(&key, stream) == -1)
		{
			free(items);
			return (-1);
		}
		if (!expect(stream, ':'))
		{
			free(items);
			free(key.string); // USE . NOT -> HERE!!!
			return (-1);
		}
		if (parser(&items[size].value, stream) == -1)
		{
			free(key.string);
			free(items);
			return (-1);
		}
		items[size].key = key.string;
		size++;
		// APRES SIZE++ : IL MANQUE:
		// -> Vérifier s'il y a une virgule (pour une autre paire) ou une accolade (fin de l'objet)
		// ICI SOIT VIRGULE SOIT } SINON ERREUR
		if (!accept(stream, ',') && peek(stream) != '}')
		{
			unexpected(stream);
			free(items);
			return (-1);
		}
	}
	file->type = MAP;
	file->map.data = items;
	file->map.size = size;
	return (1);
}
int	parser(json *file, FILE *stream)
{
	char c;
	c = peek(stream);
	if (c == '"')
	{
		return (parse_string(file, stream));
	}
	else if (c == '-' || isdigit(c))
	{
		return (parse_int(file, stream));
	}
	else if (c == '{')
	{
		return (parse_map(file, stream));
	}
	else
	{
		unexpected(stream);
		return (-1);
	}
}

int	argo(json *file, FILE *stream)
{
	file->type = INTEGER;
	file->integer = 0;
	return(parser(file, stream));
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	char *filename = argv[1];
	FILE *stream = fopen(filename, "r");
	json	file;
	if (argo (&file, stream) != 1)
	{
		free_json(file);
		return 1;
	}
	serialize(file);
	printf("\n");
}
