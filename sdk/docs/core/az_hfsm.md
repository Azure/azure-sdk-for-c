# Hierarchical Finite State Machines

This design document describes an implementation for Hierarchical (Finite) State Machines or Statechart execution engine. Statecharts have been introduced by [D. Harel in A visual formalism for complex systems](https://www.wisdom.weizmann.ac.il/~dharel/SCANNED.PAPERS/Statecharts.pdf) and is documented in detail within the [Practical UML Statecharts in C/C++, 2nd Ed by M. Samek](https://www.state-machine.com/psicc2) book.

Hierarchical Finite State Machines (HFSM) are extending the traditional state machine concept by adding the notion of hierarchy (composite) states. This document also introduces a coding technique that promotes cleaner and easier to understand source code and reduces the risk of overlooking exceptional situations.

The main goal of using a type of state machine is to allow support of both synchronous and asynchronous (event based) handling of I/O events on any embedded system.

## Requirements

1. Support resource-constrained devices (such as a microcontroller) in terms of both CPU cycles, RAM and ROM:
    a. HFSM code does not allocate memory
    b. HFSM stack usage remains constant regardless of state machine size
2. State description and hierarchy are hardcoded and verifiable at compile-time.
3. Support a limited subset of explicit and implicit HFSM transitions:
  a. Explicit transition to peer-state
  b. Explicit transition to sub-state
  c. Explicit transition to super-state
  d. Implicit transition from a super-state handling the event to a peer-state.
4. Entering and exiting a state is handled by the framework.
5. Reliable and self-healing: detecting failures and always required to react due to the hierarchical aspect, logging is built in.
6. The programming model makes it mandatory for the developer to think of and implement all possible state-machine states.

HFSMs support a limited subset of transitions covered by the following example:

![core_hfsm_example.puml](https://www.plantuml.com/plantuml/png/ZL9HIyCm47xFhxZ7JLZxFaQPT2aKfTMqp0ULqBL10bsgsmqKyR_RD8RUN0NpqlHottVtVNVh8rO7FErRrbzGWUWwX4y6_N61j6cL_Hqki8JLBb863n_XAjn5i7z3bDfedCqP0HMML3K_H6CyxenHDMhpClbW8So_U9ABchTy8_DiPZyPBnzoSIElupBTylaVq7UD19aa_uTkqYBEiup7fUHRdfLIy7iYlD77sZKk1nkz2VOyNZt38ffGJ4Am63E6ah-A5NS3w8jQcrIdja-s4N-VdagZo5nyEsVJjEM9tWkb43gcSur14CmPSSgo-EKWVX49D3VoJI_1PN0XSa7LK3xVIi5BrhjWbzE99TX1CdaQn-s3JmYlbT5rxyLQcSOk-pS0)

## Design Details

The Azure HFSM implementation consists of a single compilation unit: `az_hfsm.c` (and corresponding include file). The HFSM implementation relies on constructs that already exist in the Embedded C SDK Azure Core component such as AZ_RESULT and is C99 compliant.

Developers creating applications which use the new HFSMs framework must be familiar with the following concepts:

- _State Handler_: a function able to (reactively) handle events. A State Handler is guaranteed to run-to-completion with respect to any other handlers within the same state machine.
- _Event_: dynamically typed structured data.
- _Hierarchy description_: a `get_parent` function able to return the parent (State Handler) of a given State Handler. get_parent will return `NULL` for the `root` state.
- `Enter` / `Exit` events are events that are guaranteed to be executed by the framework.

The `az_hfsm` class diagram (note that class in this context would translate to a struct):

![core_hfsm_architecture.puml](https://www.plantuml.com/plantuml/png/fLFDQlCm4BphAGIvEFa-vm5C3-aXfJca0JSj_Q48LTuq0aSsAocGjddtIfQL7-Fsq0Q2sPdPdM7NdcZ3qTXDIOWekTQKlxFrIcye-I3K_L9X4K4PvdG6Ca_3rjTrLKgnOskkYX8mQD_0uRoHhwmI6MNjD7PaWl90I2LDWbNL6loddHd3ZjuWLreQMIbM0s2YAui2OdC1saZ5FHsW7zgrVMqaVnfH0_vgY0PLX4KcSQEj94sRvu1zi-daH3pesMyYrh8iWcli6P8z8Q3ivdW-iwl1TIb0ATfJNpwnwwlREPlUJs-MpFtpnpxyORRiW_DyaVVWozbykhWB7S_ZyVY5xEER5hEqzlzyfEmyOI0ARdX6jvKiHhwRkxiX5uQUZs5UHaWFo02nmcZJ5EnWXg1jQGBbhm8OhKwWCEmOpiSWiQ2ZDEt4xgT2mBEINdns534j8HDdxJ_CBm00)

- Each HFSM must have at least one root state capable of handling any type of event. (If an event is unknown, a panic function should be called.)
- Each HFSM must have a `get_parent` function that will return the parent State Handler of a given State Handler (`get_parent` will return NULL for `root`.)

### HFSM Object

The HFSM Object describes both the current state as well as the hierarchy.

#### Internal API

```C
AZ_NODISCARD az_result _az_hfsm_init(
    _az_hfsm* h, 
    az_event_policy_handler root_state, 
    _az_hfsm_get_parent get_parent_func);

// The source_state is always a required parameter to allow internal computation of the Least Common
// Ancestor (LCA) state, used to produce the correct sequence of Exit and Enter events.

AZ_NODISCARD az_result _az_hfsm_transition_peer(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

AZ_NODISCARD az_result _az_hfsm_transition_substate(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

AZ_NODISCARD az_result _az_hfsm_transition_superstate(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

AZ_NODISCARD az_result _az_hfsm_send_event(az_hfsm* h, az_hfsm_event event);
```

## Implementing a new hierarchical state machine (HFSM Development Model)

A new state machine implementation must provide 3 components:

1. A description of the hierarchy.
2. State handlers for each state (at least one root state).
3. (Optional) Event types and associated data format.

### Graphical Representation

One of the major advantages of HFSMs is that they can easily be described in a graphical form. It is recommended that the graphical representation is maintained together with the code. 
Using PlantUML, the source-code for the above example is as follows:

```text
@startuml
state Root {
    state Idle
    Idle : <b>entry/</b> mqtt_init()

    state Started {
        state Connecting
        state Connected 

        Started : <b>entry/</b> LED_ON(LED_NETWORK_ACTIVE)
        Started : <b>exit/</b> LED_OFF(LED_NETWORK_ACTIVE)
        Started : <b>DISCONNECT_EVENT_REQ/</b> mqtt_disconnect_start()

        [*] -> Connecting
        Connecting --> Connected : CONNECT_EVENT_RSP
    }

    state Faulted
    Faulted : <b>entry/</b> mqtt_deinit()

    [*] -> Idle
    Idle --> Started : CONNECT_EVENT_REQ / mqtt_connect_start(...)
    Started --> Idle : DISCONNECT_EVENT_RSP
    Started -> Faulted: EVENT_ERROR
    Faulted --> Idle : EVENT_RESET
}

Root : <b>EVENT_ERROR/</b>
Root : <b>exit/</b> 
Root : \t panic()
@enduml
```

### Describing the hierarchy

The hierarchy is described using a function that returns the parent of any given state handler:

```C
static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started)
  {
    parent_state = root;
  }

// [. . .]

  return parent_state;
}
```

### State Handler

A state-handler is a C function with a pre-defined signature. The recommended function structure is defined below.

All HFSMs must define at least a root state handler which must never return `AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE`.

```C
static az_result connected(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_HUB_TELEMETRY_REQ:
      ret = _hub_telemetry_send(
                this_policy, (az_hfsm_iot_hub_telemetry_data*)event.data);
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      ret = az_hfsm_pipeline_send_indbound_event((az_hfsm_policy*)this_policy, event);
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      ret = _hub_message_parse(this_policy, (az_hfsm_mqtt_recv_data*)event.data);
      break;

    case AZ_IOT_HUB_METHODS_RSP:
      ret = _hub_methods_response_send(
          this_policy, (az_hfsm_iot_hub_method_response_data*)event.data);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
```

### Handling errors

HFSMs allow errors on both the synchronous (i.e. I/O setup phase) as well as asynchronous errors (i.e. AZ_HFSM_EVENT_ERROR).

Processing an event (triggered by `_az_hfsm_send`) may result in either successful execution or synchronous failure. These errors usually occur due to argument (i.e. event data) errors such as incorrect format, insufficient buffer space, etc.

In certain cases, especially events triggered by network responses, errors must trigger changes in state. These error cases are handled through the standard HFSM event AZ_HFSM_EVENT_ERROR. It is advised that the root state is always handling the error event type (like the try/catch behavior of OOP).

### State Transitions

A limited sub-set of all HFSM transitions are supported. The following limitations apply:

1. The application must make explicit transitions through the hierarchy. E.g. in the above example, the application must explicitly transition first to Started then to Connecting to ensure all `Enter` / `Exit` events are executed.
2. Peer transitions between two states with the same parent are allowed. The source state executes an `Exit` event followed by the destination state executing an `Enter` event.
3. Substate transitions between a parent and child are allowed. The destination state executes an `Enter` event.
4. Superstate transitions between a child and a parent are allowed. The source state executes an `Exit` event.
5. Implicit superstate to peer transitions between any ancestor state and its peer are allowed. The current state as well as all ancestors including the one performing the transition will execute an `Exit` event followed by the ancestor’s peer executing an `Enter` event. E.g. `DISCONNECT_EVENT_RSP` received when Connected will cause Exit events on Connected and Started followed by Idle executing the `Enter` event.
6. Transitions in `Enter` / `Exit` handlers are **not** supported.

## Events

Events are dynamically typed objects, optionally containing additional data required by the state machine.

### Event Types

The following public-API events are always supported. Additionally, HFSM-implementation specific events can be defined.

```C
enum az_event_type_generic
{
  /// Entry event: must not set or use the data field, must be handled by each state.
  AZ_HFSM_EVENT_ENTRY = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 1),

  /// Exit event: must not set or use the data field, must be handled by each state.
  AZ_HFSM_EVENT_EXIT = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 2),

  /// Generic error event: must use a data field containing a structure derived from
  /// #az_hfsm_error_data
  AZ_HFSM_EVENT_ERROR = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 3),

  /**
   * @brief Generic timeout event: if multiple timers are necessary it's recommended to create
   * separate timeout events.
   *
   */
  AZ_HFSM_EVENT_TIMEOUT = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 4),
};
```

### Event Data

The HFSM framework defines the event-data corresponding to the `AZ_HFSM_EVENT_ERROR` only. Enter and Exit events do not have attached event-data (i.e. the parameter is set to NULL). Additionally, HFSM-implementation specific events can be defined.

### Event Object Lifetime

Events are always passed by value (on the stack). It is recommended that event data are modeled as C structs that avoid double-buffering of large data items (i.e. by relying on `az_span`).

The application is responsible for maintaining run-to-completion semantics while the HFSM handler is running: any event-data must be available and remain unchanged throughout handler execution.

## State Machine Composition

While the hierarchical aspect already substantially decreases the number of states required, two different state composition patterns are also supported: bi-directional pipeline of HFSMs and orthogonal region.

### Pipeline

The pipeline is described in the [az_event_pipeline.md](az_event_pipeline.md) document. Within the SDK, the [Pipeline pattern](https://en.wikipedia.org/wiki/Pipeline_%28software%29) allows bi-directional message (event) passing within a chain of HFSM objects.

### Orthogonal Region

We implement the [Orthogonal region pattern](https://en.wikipedia.org/wiki/UML_state_machine#Orthogonal_regions) within the `az_event_policy_collection`. This works by multicasting the events to a subset of all the HFSMs in the collection based on an event-filter.
