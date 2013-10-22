Finite State Machine for PD
=========
(c) 2013 Tiago Rezende

Objects for dealing with states and state transitions within a patch.
------------------------------------------------------------------------

[fsm statename]

Creates a state machine that will control the current state, sending signals to the states on transitions.
Connect to the desired states through the first outlet. The only parameter available is the name of the initial state.
Any data sent to the first inlet is sent straight to the currently active state's first outlet.

[fstate statename]

Creates a state with the given name (required). First inlet must be connected to a [start]/[fsm] object.
Right outlet is the data sent to the state machine, if the state is active.
Left outlet emits state management messages:
- When state is entered, "active" is emitted
- when state is exited, "inactive" is emitted
Specifying multiple state objects with the same names will lead to undefined results, so keep your states organized.
