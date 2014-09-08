/* $Id$ */

/*
 * SID Output.
 *
 * TODO: fprintf(fout), instead of stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../ast.h"
#include "../xalloc.h"

#include "io.h"

static void output_alt(const struct ast_alt *);

static void
output_section(const char *section)
{
	printf("\n%%%s%%\n\n", section);
}

static void
output_literal(const char *s)
{
	char c;

	assert(s != NULL);

	c = strchr(s, '\"') ? '\'' : '\"';
	printf("%c%s%c; ", c, s, c);
}

static void
output_basic(const struct ast_term *term)
{
	switch (term->type) {
	case TYPE_EMPTY:
		fputs("$$; ", stdout);
		break;

	case TYPE_RULE:
		printf("%s; ", term->u.rule->name);
		break;

	case TYPE_TERMINAL:
		output_literal(term->u.literal);
		break;

	case TYPE_GROUP:
		fputs("{ ", stdout);
		output_alt(term->u.group);
		fputs("}; ", stdout);
	}
}

static void
output_term(const struct ast_term *term)
{
	/* SID cannot express term repetition; TODO: semantic checks for this */
	/* TODO: can output repetition as [ ... ] local rules with a stub to call them X times? */
	assert(term->min <= 1);
	assert(term->max <= 1);

	if (term->min == 0) {
		fputs("{ $$; || ", stdout);
	}

	output_basic(term);

	if (term->min == 0) {
		fputs("}; ", stdout);
	}
}

static void
output_alt(const struct ast_alt *alt)
{
	const struct ast_term *term;

	for (term = alt->terms; term != NULL; term = term->next) {
		output_term(term);
	}
}

static void
output_rule(const struct ast_rule *rule)
{
	const struct ast_alt *alt;

	printf("\t%s = {\n\t\t", rule->name);

	for (alt = rule->alts; alt != NULL; alt = alt->next) {
		output_alt(alt);

		printf("\n");

		if (alt->next != NULL) {
			printf("\t||\n\t\t");
		}
	}

	printf("\t};\n\n");
}

static int
is_equal(const struct ast_term *a, const struct ast_term *b)
{
	if (a->type != b->type) {
		return 0;
	}

	switch (a->type) {
	case TYPE_RULE:     return 0 == strcmp(a->u.rule->name, b->u.rule->name);
	case TYPE_TERMINAL: return 0 == strcmp(a->u.literal,    b->u.literal);
	}
}

static void
output_terminals(const struct ast_rule *grammar)
{
	const struct ast_rule *p;
	struct ast_term *found = NULL;

	/* List terminals */
	for (p = grammar; p != NULL; p = p->next) {
		struct ast_alt *alt;

		for (alt = p->alts; alt != NULL; alt = alt->next) {
			const struct ast_term *term;
			struct ast_term *t;

			for (term = alt->terms; term != NULL; term = term->next) {
				switch (term->type) {
				case TYPE_EMPTY:
				case TYPE_GROUP:
					continue;

				case TYPE_RULE:
					continue;

				case TYPE_TERMINAL:
					break;
				}

				for (t = found; t != NULL; t = t->next) {
					if (is_equal(t, term)) {
						break;
					}
				}

				if (t != NULL) {
					continue;
				}

				t = xmalloc(sizeof *t);
				t->u.literal = term->u.literal;
				t->type = term->type;
				t->next = found;
				found = t;
			}
		}
	}

	/* Output list */
	{
		struct ast_term *next;
		struct ast_term *t;

		for (t = found; t != NULL; t = next) {
			next = t->next;
			printf("\t");
			output_basic(t);
			printf("\n");
			free(t);
		}
	}
}

void
sid_output(const struct ast_rule *grammar)
{
	const struct ast_rule *p;

	output_section("types");

	output_section("terminals");

	output_terminals(grammar);

	output_section("rules");

	/* TODO list rule declartations */

	for (p = grammar; p != NULL; p = p->next) {
		output_rule(p);
	}

	output_section("entry");

	printf("\t%s;\n\n", grammar->name);
}

