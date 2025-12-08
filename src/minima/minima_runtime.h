#ifndef MINIMA_RUNTIME_H
#define MINIMA_RUNTIME_H

#include <stdx_common.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdx_string.h>
#include "minima.h"

//
// Runtime Values / Environment
//

typedef struct MiRtValue MiRtValue;
typedef struct MiRuntime MiRuntime;
typedef struct MiRtList MiRtList;

typedef MiRtValue (*MiRtBuiltinFn) (
    MiRuntime       *rt,
    const XSlice    *head_name,
    int             argc,
    struct MiExprList *args);

typedef enum MiRtValueKind
{
  MI_RT_VAL_VOID = 0,
  MI_RT_VAL_INT,
  MI_RT_VAL_FLOAT,
  MI_RT_VAL_BOOL,
  MI_RT_VAL_STRING,
  MI_RT_VAL_LIST,
  MI_RT_VAL_BLOCK
} MiRtValueKind;

struct MiRtValue
{
  MiRtValueKind kind;

  union
  {
    long long        i;
    double           f;
    bool             b;
    XSlice           s;
    MiRtList *list;
    MiScript        *block;
  } as;
};

struct MiRtList
{
  MiRtValue *items;
  size_t     count;
  size_t     capacity;
};

typedef struct MiRtVar
{
  XSlice   name;
  MiRtValue value;
} MiRtVar;

typedef struct MiRtUserCommand
{
  XSlice        name;
  MiRtBuiltinFn fn;
} MiRtUserCommand;

struct MiRuntime
{
  MiRtVar *vars;
  size_t   var_count;
  size_t   var_capacity;
  MiRtUserCommand *commands;    // Command table
  size_t   command_count;
  size_t   command_capacity;
};


//
// Public API
//

void mi_rt_init(MiRuntime *rt);                            // Initializes Minima runtime
void mi_rt_shutdown(MiRuntime *rt);                        // Terminates Minima runtime and releases resources
MiRtValue mi_rt_eval_script(MiRuntime *rt, const MiScript *script); // Evaluate a whole script. Returns the value of the last command, or void if there are no commands.
MiRtValue mi_rt_eval_expr(MiRuntime *rt, const MiExpr *expr);       // Evaluates an expression
bool mi_rt_register_command(MiRuntime *rt, const char *name, MiRtBuiltinFn fn);
bool mi_rt_var_get(const MiRuntime *rt, XSlice name, MiRtValue *out_value);  // Variable access. Returns false if variable does not exist.  
MiRtList *mi_rt_list_create(void);                        // Make empy list literal
bool      mi_rt_list_push(MiRtList *list, MiRtValue v);   // Pushes a value into a list
MiRtValue mi_rt_make_list(MiRtList *list);                // Make list literal
MiRtValue mi_rt_make_void(void);                          // make void literal
MiRtValue mi_rt_make_int(long long v);                    // Make int literal
MiRtValue mi_rt_make_float(double v);                     // Make float lieteral
MiRtValue mi_rt_make_bool(bool v);                        // Make boolean literal
MiRtValue mi_rt_make_string_slice(XSlice s);              // Keeps stlice pointers to the original string.

void mi_fold_constants(const MiScript *script);

#endif // MINIMA_RUNTIME_H
