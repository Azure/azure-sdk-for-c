# Action and Emitters

```c
// action (used in C#),
//   actor (drama?),
//   reactor (Nuclear?),
// accept (..hm... no),
//   acceptor (too long)
// callback (too long,
//           too scary,
//           too specific (a callback in C as a function passed to some special functions with events) and
//           too generic (it can be any function with different arguments and parameter structure, w/o self etc.),
// functor (C++ has slightly different meaning, has a different meaning in Theory of Category),
// ftor (chemistry?)
// observer, subscriber (Reactive, but Rx uses this for events)
// closure (has slightly different meaning as a syntax sugar)
struct az_span_action {
  az_result (*func)(az_span); // fn (less known)
  // https://en.wikipedia.org/wiki/This_%28computer_programming%29
  void *self; // data (too abstract),
              // this (bad for C++ compatibility),
              // self (used in some languages),
              // context (long and non-descriptive)
              // me (VB :-) )
};

az_result az_span_action_do(az_span_action const action, az_span const arg) {
  AZ_CONTRACT_NOT_NULL(action.func);

  return action.func(action.self, arg);
}

// (SVO) subject verb object // https://en.wikipedia.org/wiki/Subject–verb–object
az_result az_http_request__emit__spans(az_http_request, span_action);
           _emit_span_seq // good one because it avoids plural forms.
           _push_span_seq // an action is not necessary a container.
           _append_span_seq // an action is not necessary a container.
           _emitter // bad for C name constructions by conventions.
                    // However we may use it as a synonym for `..._emit_..._action`
           _observable // too long,
                       // bad for C name constructions by conventions, Rx uses events which are slightly different
```
