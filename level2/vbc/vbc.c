#include <stdio.h>
#include <malloc.h> // change this to <stdlib.h>
#include <ctype.h>

typedef struct node {
    enum {
        ADD,
        MULTI,
        VAL
    }   type;
    int val;
    struct node *l;
    struct node *r;
}   node;

//node *n(**s);

node    *new_node(node n)
{
    node *ret = calloc(1, sizeof(n));
    if (!ret)
        return (NULL);
    *ret = n;
    return (ret);
}

void    destroy_tree(node *n)
{
    if (!n)
        return ;
    if (n->type != VAL)
    {
        destroy_tree(n->l);
        destroy_tree(n->r);
    }
    free(n);
}

void    unexpected(char c)
{
    if (c)
        printf("Unexpected token '%c'\n", c);
    else
        printf("Unexpected end of file\n");
}

int accept(char **s, char c)
{
    if (**s)
    {
        (*s)++;
        return (1);
    }
    return (0);
}

int expect(char **s, char c)
{
    if (accept(s, c))
        return (1);
    unexpected(**s);
    return (0);
}
node	*parse_addition(char **str);
node	*parse_mult(char **str);
node	*parse_par_or_val(char **str);

node	*parse_addition(char **str)
{
	node	*left;
	node	*right;
	node	tmp;

	left = parse_mult(str);
	while (**str == '+')
	{
		(*str)++;
		right = parse_mult(str);
		tmp.type = ADD;
		tmp.l = left;
		tmp.r = right;
		left = new_node(tmp);
	}
	return (left);
}

node	*parse_mult(char **str)
{
	node	*left;
	node	*right;
	node tmp;

	left = parse_par_or_val(str);
	while (**str == '*')
	{
		(*str)++;
		right = parse_par_or_val(str);
		tmp.type = MULTI;
		tmp.l = left;
		tmp.r = right;
		left = new_node(tmp);
	}
	return (left);
}

node	*parse_par_or_val(char **str)
{
	node	tmp;
	node	*left;
    
	if (**str == '(')
	{
		(*str)++;
		left = parse_addition(str);
		(*str)++;
		return (left);
		
	}
	//TODO!!! parenthesis

	if (isdigit(**str))
	{
		tmp.type = VAL;
		tmp.val = **str - '0';
		left = new_node(tmp);
		(*str)++;
	}
	return (left);
}

//...
/*
node    *parse_expr(char *s)
{
    //...

    if (*s) 
    {
        destroy_tree(ret);
        return (NULL);
    }
    return (ret);
}
*/
int eval_tree(node *tree)
{
    switch (tree->type)
    {
        case ADD:
            return (eval_tree(tree->l) + eval_tree(tree->r));
        case MULTI:
            return (eval_tree(tree->l) * eval_tree(tree->r));
        case VAL:
            return (tree->val);
    }
}

int	check_par(char *str)
{
	int i = 0;
	int count = 0;
	while (*str)
	{
		if (str[i] == '(')
		{
			count++;
		}
		else if (str[i] == ')')
		{
			count--;
		}
		if (count < 0)
		{
			return (')');
		}
		str++;
	}
	if (count > 0)
	{
		return ('(');
	} 
	return (0);
}

int main(int argc, char **argv)
{
	int returned_par_val;

    if (argc != 2)
		return (1);
	returned_par_val = check_par(argv[1]);
	if (returned_par_val != 0)
	{
		if (returned_par_val == ')')
			return (unexpected(')'), 1);
		else
			return (unexpected(0), 1);
	}
    node *tree = parse_addition(&argv[1]);
    if (!tree)
        return (1);
    printf("%d\n", eval_tree(tree));
    destroy_tree(tree);
}
