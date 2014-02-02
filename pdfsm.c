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

static t_class *fsm_class = NULL, *state_class = NULL;

static t_symbol *switch_state = NULL, *exit_state = NULL, *enter_state = NULL;
static t_symbol *inactive = NULL, *active = NULL;
typedef struct _fsm_obj t_fsm_obj;
typedef struct _state_obj t_state_obj;

struct _fsm_obj {
    t_object x_obj;
    t_symbol *current_state;
    t_state_obj *current_state_obj;
    t_inlet *state_change;
    t_outlet *out_port;
};

struct _state_obj {
    t_object x_obj;
    t_symbol *name;
    bool active;
    t_outlet *out_port;
    t_outlet *msg_port;
};


/* [fsm] object */
static void *fsm_new(t_symbol *first_state)
{
    t_fsm_obj *fsm = (t_fsm_obj *)pd_new(fsm_class);
    fsm->current_state = first_state;
    fsm->current_state_obj = NULL;
    fsm->state_change = inlet_new(&fsm->x_obj, &fsm->x_obj.ob_pd, &s_symbol, switch_state);
    fsm->out_port = outlet_new(&fsm->x_obj, &s_list);
    return (void *)fsm;
}

static void fsm_destroy(t_fsm_obj *fsm)
{
    outlet_free(fsm->out_port);
    inlet_free(fsm->state_change);
}

static void fsm_switch_state(t_fsm_obj *fsm, t_symbol *state);

static void fsm_anything(t_fsm_obj *fsm, t_symbol *sym, int argc, t_atom *argv) {
    if (!fsm->current_state_obj) {
        fsm_switch_state(fsm, fsm->current_state);
        if (!fsm->current_state_obj) {
            error("FSM object couldn't find a state named %s.", fsm->current_state->s_name);
            return;
        }
    }
    // direct pass thru to current state
    outlet_anything(fsm->current_state_obj->out_port, sym, argc, argv);
}

static t_fsm_obj *current_fsm = NULL;

static void fsm_switch_state(t_fsm_obj *fsm, t_symbol *state)
{
    t_fsm_obj *bkp_fsm = current_fsm;
    current_fsm = fsm;
    // exiting current state
    t_atom * args_list = getbytes(sizeof(t_atom));
    SETSYMBOL(args_list, fsm->current_state);
    outlet_anything(fsm->out_port, exit_state, 1, args_list);
    fsm->current_state = state;
    SETSYMBOL(args_list, state);
    outlet_anything(fsm->out_port, enter_state, 1, args_list);
    freebytes(args_list, sizeof(t_atom));
    current_fsm = bkp_fsm;
}

/* [state] object */

static void * state_new(t_symbol *sym)
{
    t_state_obj *state = (t_state_obj *)pd_new(state_class);
    state->name = sym;
    //post("created state %s.", state->name);
    state->active = false;
    state->out_port = outlet_new(&state->x_obj, &s_anything);
    state->msg_port = outlet_new(&state->x_obj, &s_symbol);
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

static void state_enter(t_state_obj *state, t_symbol *new_state)
{
    //post("trying comparison between state %s and %s.", state->name, new_state->s_name);
    if (state->name == new_state) {
        //post("entering state %s.", new_state->s_name);
        current_fsm->current_state_obj = state;
        state->active = true;
        outlet_symbol(state->msg_port, active);
    } else {
        if (state->active) {
            outlet_symbol(state->msg_port, inactive);
        }
        state->active = false;
    }
}

static void state_exit(t_state_obj *state, t_symbol *name)
{
    if (state->name == name) {
        if (state->active) {
            state->active = false;
            outlet_symbol(state->msg_port, inactive);
        }
    }
}


/* setup */

void pdfsm_setup(void)
{
    if (!fsm_class) {
        post("PD Finite State Machine objects\n(c) 2013 Tiago Rezende.");
        switch_state = gensym("switch");
        exit_state = gensym("exit-state");
        enter_state = gensym("enter-state");
        inactive = gensym("inactive");
        active = gensym("active");
        
        fsm_class = class_new(gensym("fsm"),
                              (t_newmethod)fsm_new,
                              (t_method)fsm_destroy,
                              sizeof(t_fsm_obj), CLASS_DEFAULT,
                              A_SYMBOL, 0);
        class_addanything(fsm_class, (t_method)fsm_anything);
        class_addmethod(fsm_class, (t_method)fsm_switch_state,
                        switch_state, A_SYMBOL, 0);
        
        state_class = class_new(gensym("fstate"),
                                (t_newmethod)state_new,
                                (t_method)state_destroy,
                                sizeof(t_state_obj), CLASS_DEFAULT,
                                A_SYMBOL, 0);
        class_addbang(state_class, (t_method)state_bang);
        class_addmethod(state_class, (t_method)state_enter,
                        enter_state, A_SYMBOL, 0);
        class_addmethod(state_class, (t_method)state_exit,
                        exit_state, A_SYMBOL, 0);
    }
}

void fsm_setup(void) { pdfsm_setup(); }
void fstate_setup(void) { pdfsm_setup(); }
