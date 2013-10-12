//
//  submarine.c
//  submarine
//
//  Created by Tiago Rezende on 3/13/13.
//  Copyright (c) 2013 Tiago Rezende. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include "m_pd.h"

static t_class *fsm_class = NULL, *state_class = NULL, *transition_class = NULL;

static t_symbol *switch_state = NULL, *exit_state = NULL, *enter_state = NULL;
static t_symbol *do_transition = NULL, *fsm_discover = NULL;

typedef struct _fsm_obj t_fsm_obj;
typedef struct _state_obj t_state_obj;
typedef struct _transition_obj t_transition_obj;

struct _fsm_obj {
    t_object x_obj;
    t_symbol *current_state;
    t_inlet *state_change;
    t_outlet *out_port;
};

struct _state_obj {
    t_object x_obj;
    t_fsm_obj *fsm;
    t_symbol *name;
    bool active;
    t_inlet *in_transition_port;
    t_outlet *out_port;
};

struct _transition_obj {
    t_object x_obj;
    t_symbol *name;
    t_outlet *out_port;
};


/* [fsm] object */
static void *fsm_new(t_symbol *first_state)
{
    t_fsm_obj *fsm = (t_fsm_obj *)pd_new(fsm_class);
    fsm->current_state = first_state;
    fsm->state_change = inlet_new(&fsm->x_obj, &fsm->x_obj.ob_pd, &s_anything, switch_state);
    fsm->out_port = outlet_new(&fsm->x_obj, &s_list);
    return (void *)fsm;
}

static void fsm_destroy(t_fsm_obj *fsm)
{
    outlet_free(fsm->out_port);
    inlet_free(fsm->state_change);
}

static void fsm_bang(t_fsm_obj *fsm) {
    // pass thru
    t_atom *params = getbytes(sizeof(t_atom)*2);
    SETSYMBOL(params, fsm->current_state);
    SETPOINTER(params+1, (void*)fsm);
    outlet_anything(fsm->out_port, fsm_discover, 2, params);
    freebytes(params, sizeof(t_atom)*2);
    outlet_bang(fsm->out_port);
}

static void fsm_switch_state(t_fsm_obj *fsm, t_symbol *state)
{
    // exiting current state
    t_atom * args_list = getbytes(sizeof(t_atom));
    SETSYMBOL(args_list, fsm->current_state);
    outlet_anything(fsm->out_port, exit_state, 1, args_list);
    fsm->current_state = state;
    SETSYMBOL(args_list, state);
    outlet_anything(fsm->out_port, enter_state, 1, args_list);
    freebytes(args_list, sizeof(t_atom));
}

/* [state] object */

static void * state_new(t_symbol *sym)
{
    t_state_obj *state = (t_state_obj *)pd_new(state_class);
    state->name = sym;
    state->active = false;
    state->out_port = outlet_new(&state->x_obj, &s_list);
    return (void *)state;
}

static void state_destroy(t_state_obj *state)
{
    outlet_free(state->out_port);
}

static void state_bang(t_state_obj *state) {
    if (state->active) {
        outlet_bang(state->out_port);
    }
}

static void state_do_transition(t_state_obj *state, t_symbol *transition) {
    if (state->active) {
        //t_atom *arglist = getbytes(sizeof(t_atom));
        //SETSYMBOL(arglist, transition);
        //outlet_anything(state->out_port, do_transition, 1, arglist);
        //freebytes(arglist, sizeof(t_atom));
        if (!state->fsm) {
            error("either no [fsm] object was connected or a transition was applied before first updating the [fsm] object.");
        } else {
            fsm_switch_state(state->fsm, state->name);
        }
    }
}

static void state_discover(t_state_obj *state, t_fsm_obj *fsm, t_symbol *current_state) {
    state->fsm = fsm;
    if ((current_state == state->name) && (!state->active)) {
        fsm_switch_state(fsm, current_state);
    }
}


static void state_enter(t_state_obj *state, t_symbol *new_state)
{
    if (state->name == new_state) {
        state->active = true;
        outlet_symbol(state->out_port, gensym("active"));
    } else {
        if (state->active) {
            outlet_symbol(state->out_port, gensym("inactive"));
        }
        state->active = false;
    }
}

static void state_exit(t_state_obj *state, t_symbol *name)
{
    if (state->name == name) {
        if (state->active) {
            state->active = false;
            outlet_symbol(state->out_port, gensym("inactive"));
        }
    }
}

/* [transition] object */

static void *transition_new(t_symbol *name)
{
    t_transition_obj *trans = (t_transition_obj *)pd_new(transition_class);
    trans->name = name;
    trans->out_port = outlet_new(&trans->x_obj, &s_list);
    return (void *)trans;
}

static void transition_destroy(t_transition_obj *trans)
{
    outlet_free(trans->out_port);
}

static void transition_bang(t_transition_obj *trans)
{
    outlet_symbol(trans->out_port, do_transition);
}


/* setup */

void pdfsm_setup(void)
{
    if (!fsm_class) {
        post("PD Finite State Machine objects\n(c) 2013 Tiago Rezende.");
        switch_state = gensym("switch-state");
        exit_state = gensym("exit-state");
        enter_state = gensym("enter-state");
        do_transition = gensym("transition");
        
        fsm_class = class_new(gensym("fsm"),
                              (t_newmethod)fsm_new,
                              (t_method)fsm_destroy,
                              sizeof(t_fsm_obj), CLASS_DEFAULT,
                              A_SYMBOL, 0);
        class_addbang(fsm_class, (t_method)fsm_bang);
        class_addmethod(fsm_class, (t_method)fsm_switch_state,
                        switch_state, A_SYMBOL, 0);
        
        state_class = class_new(gensym("state"),
                                (t_newmethod)state_new,
                                (t_method)state_destroy,
                                sizeof(t_state_obj), CLASS_DEFAULT,
                                A_SYMBOL, 0);
        class_addbang(state_class, (t_method)state_bang);
        class_addmethod(state_class, (t_method)state_enter,
                        enter_state, A_SYMBOL, 0);
        class_addmethod(state_class, (t_method)state_exit,
                        exit_state, A_SYMBOL, 0);
        class_addmethod(state_class, (t_method)state_do_transition,
                        do_transition, A_SYMBOL, 0);
        
        transition_class = class_new(gensym("transition"),
                                     (t_newmethod)transition_new,
                                     (t_method)transition_destroy,
                                     sizeof(t_transition_obj), CLASS_DEFAULT,
                                     A_SYMBOL, 0);
        class_addbang(transition_class, (t_method)transition_bang);
    }
}

void fsm_setup(void) { pdfsm_setup(); }
void state_setup(void) { pdfsm_setup(); }
void transition_setup(void) { pdfsm_setup(); }
