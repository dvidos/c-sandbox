#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "../defs.h"
#include "../token.h"
#include "../ast_node.h"
#include "../ast.h"
#include "iterator.h"
#include "shunting_yard.h"


// light & simple list implementation for functions
#define declare_list(type)   type *list = NULL, *list_tail = NULL
#define list_append(var)     if (list_tail == NULL) { list = var; list_tail = var; } \
                             else { list_tail->next = var; list_tail = var; }


/**
 * How to parse an expression into a tree????
 * Sounds like the shunting-yard algorithm is in order
 * It uses a stack to push things in natural (source code) order, 
 * but pop in RPN (reverse polish notation)
 * Invented by Dijkstra of course!
 * Oh... there's a lot to learn here, actally...
 * Parsers, LL, LR, recursive, etc.
 * I think I'll need a Recursive descent parser in general,
 * but still want to get to the bottom of parsing an expression with precedence.
 * It seems Recursive parser is ok for some levels of precedence, but,
 * for many levels, coding becomes tedius, as the sequence of calling the functions
 * is what determines precedence.
 * For a true multi-precedence parser, I should look into 
 * 
 * See 
 * - https://rosettacode.org/wiki/Parsing/Shunting-yard_algorithm
 * - https://en.wikipedia.org/wiki/Recursive_descent_parser
 * - https://www.lysator.liu.se/c/ANSI-C-grammar-y.html
 * - https://www.tutorialspoint.com/cprogramming/c_operators.htm
 */

// let's try a Recursive Descend Parser
// if operator precedence is too high, we can switch to other parsers.
// we need a ton of tests, to verify things!!!

// three main AST types:
// - declaration (variable, function)
// - statement (take action: loop, jump, return)
// - expression (something to be evaluate and produce a value, includes function calls)

static ast_statement_node *parse_statement();
static ast_statement_node *parse_block(); // parses statements in a loop


static bool is_type_declaration() {
    // storage_class_specifiers: typedef, extern, static, auto, register.
    // type_qualifiers: const, volatile.
    // type specifiers: void, char, short, int, long, float, double, 
    //                  signed, unsigned, <struct/union/enum>, <type-name>
    // keeping it simple for now
    return (next_is(TOK_INT)
         || next_is(TOK_CHAR)
         || next_is(TOK_VOID));
}

static ast_data_type_node *accept_type_declaration() {
    if (!is_type_declaration())
        return NULL;

    consume();
    ast_data_type_node *p = create_ast_data_type_node(accepted(), NULL);
    return p;
}

static ast_data_type_node *expect_type_declaration() {
    if (!is_type_declaration()) {
        parsing_error("was expecting type declaration, got \"%s\"", token_type_name(next()->type));
        return NULL;
    }
    return accept_type_declaration();
}

static char *expect_identifier() {
    if (!expect(TOK_IDENTIFIER))
        return NULL;

    return accepted()->value;
}

// cannot parse a function, but can parse a block and anything in it.
static ast_statement_node *parse_statement() {

    if (accept(TOK_BLOCK_START)) {
        // we need to parse the nested block
        ast_statement_node *bl = parse_block();
        if (!expect(TOK_BLOCK_END)) return NULL;
        return bl;
    }

    if (is_type_declaration()) {
        // it's a declaration of a variable or function declaration or definition
        ast_data_type_node *dt = accept_type_declaration();
        if (dt == NULL) return NULL;

        char *name = expect_identifier();
        if (name == NULL) return NULL;

        ast_var_decl_node *vd = create_ast_var_decl_node(dt, name);
        ast_expression_node *init = NULL;

        if (accept(TOK_ASSIGNMENT)) {
            init = parse_expression_using_shunting_yard();
        }

        if (!expect(TOK_END_OF_STATEMENT)) return NULL;
        return create_ast_decl_statement(vd, init);
    }

    if (accept(TOK_IF)) {
        if (!expect(TOK_LPAREN)) return NULL;
        ast_expression_node *cond = parse_expression_using_shunting_yard();
        if (!expect(TOK_RPAREN)) return NULL;
        ast_statement_node *if_body = parse_statement();
        if (if_body == NULL) return NULL;
        ast_statement_node *else_body = NULL;
        if (accept(TOK_ELSE)) {
            else_body = parse_statement();
            if (else_body == NULL) return NULL;
        }
        return create_ast_if_statement(cond, if_body, else_body);
    }

    if (accept(TOK_WHILE)) {
        if (!expect(TOK_LPAREN)) return NULL;
        ast_expression_node *cond = parse_expression_using_shunting_yard();
        if (!expect(TOK_RPAREN)) return NULL;
        ast_statement_node *body = parse_statement();
        if (body == NULL) return NULL;
        return create_ast_while_statement(cond, body);
    }

    if (accept(TOK_CONTINUE)) {
        if (!expect(TOK_END_OF_STATEMENT)) return NULL;
        return create_ast_continue_statement();
    }

    if (accept(TOK_BREAK)) {
        if (!expect(TOK_END_OF_STATEMENT)) return NULL;
        return create_ast_break_statement();
    }

    if (accept(TOK_RETURN)) {
        ast_expression_node *value = NULL;
        if (!accept(TOK_END_OF_STATEMENT)) {
            value = parse_expression_using_shunting_yard();
            if (!expect(TOK_END_OF_STATEMENT)) return NULL;
        }
        return create_ast_return_statement(value);
    }
    
    // what is left?
    ast_expression_node *expr = parse_expression_using_shunting_yard();
    if (!expect(TOK_END_OF_STATEMENT)) return NULL;
    return create_ast_expr_statement(expr);
}

static ast_statement_node *parse_block() {
    declare_list(ast_statement_node);

    while (!parsing_failed() && !next_is(TOK_BLOCK_END) && !next_is(TOK_EOF)) {
        ast_statement_node *n = parse_statement();
        if (n == NULL) // error?
            return NULL;
        list_append(n);
    }

    return create_ast_block_node(list);
}

static ast_var_decl_node *parse_function_arguments_list() {
    declare_list(ast_var_decl_node);

    while (!next_is(TOK_RPAREN)) {
        ast_data_type_node *dt = accept_type_declaration();
        char *name = expect_identifier();
        if (dt == NULL || name == NULL)
            return NULL;
        ast_var_decl_node *n = create_ast_var_decl_node(dt, name);
        list_append(n);

        if (!accept(TOK_COMMA))
            break;
    }

    return list;
}

int parse_file_using_recursive_descend() {
    while (!parsing_failed() && !next_is(TOK_EOF)) {
        ast_data_type_node *dt = expect_type_declaration();
        char *name = expect_identifier();
        if (dt == NULL || name == NULL)
            break;

        if (accept(TOK_END_OF_STATEMENT)) {
            // variable declaration without initial value
            ast_var_decl_node *var = create_ast_var_decl_node(dt, name);
            ast_add_var(var);

        } else if (accept(TOK_ASSIGNMENT)) {
            // variable with initial value
            parse_expression_using_shunting_yard();
            expect(TOK_END_OF_STATEMENT);

        } else if (accept(TOK_LPAREN)) {
            // function declaration or definition
            ast_var_decl_node *args_list = parse_function_arguments_list();
            expect(TOK_RPAREN);

            if (accept(TOK_END_OF_STATEMENT)) {
                // just declaration
                ast_func_decl_node *func = create_ast_func_decl_node(dt, name, args_list, NULL);
                ast_add_func(func);

            } else if (accept(TOK_BLOCK_START)) {
                parse_block();
                expect(TOK_BLOCK_END);
                ast_func_decl_node *func = create_ast_func_decl_node(dt, name, args_list, NULL);
                ast_add_func(func);

            }
        } else {
            parsing_error("unexpected token type \"%s\"\n", token_type_name((next())->type));
        }
    }

    return parsing_failed() ? ERROR : SUCCESS;
}

