# Actions and Emitters

## Actions

```c
// action (used in C#, eg. `Action<T>` is `AZ_ACTION(T)`),
//   actor (drama?),
//   reactor (Nuclear?),
// accept (..hm... no),
//   acceptor (too long)
// callback (too long,
//           too scary,
//           too specific (a callback in C as a function passed to some special functions with events) and
//           too generic (it can be any function with different arguments and parameter structure, eg, without self etc.),
// functor (C++ has slightly different meaning, has a different meaning in Theory of Category),
// ftor (chemistry?)
// observer, subscriber (Reactive, but Rx uses this for events)
// closure (has slightly different meaning as a syntax sugar)
// event, delegate (it's even more confusing)
struct az_span_action {
  az_result (*func)(az_span); // fn (less known)
  // https://en.wikipedia.org/wiki/This_%28computer_programming%29
  void *self; // data (too generic, it doesn't describe the meaning of the field),
              // this (bad for C++ compatibility),
              // self (used in some languages),
              // context (long and non-descriptive)
              // me (VB :-) )
};

az_result az_span_action_do(az_span_action const action, az_span const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(action.func);

  return action.func(action.self, arg);
}
```

## Emitters (push iterators)

An emitter is a `push` iterator. Usually an iterator means a `pull` iterator (it has a function `get`, `next` etc).
A `push` iterator is a function which accepts an action and calls this action for every value in an iterator sequence.

See also https://blogs.msdn.microsoft.com/ericlippert/2009/06/26/iterators-at-the-summer-games/ and
https://blogs.msdn.microsoft.com/ericlippert/2009/07/23/iterator-blocks-part-five-push-vs-pull/.

C has neither GC nor lifetime checks (like Rust https://doc.rust-lang.org/nomicon/lifetimes.html). It also has no
RAII (https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization). So creating a `pull` iterator is complicated and could be unsafe. One of the way to solve the problem is to use a `push` iterator. It gives some scoped lifetime (a lifetime of a function execution) and it's very easy to create a complicated `push` iterator by using code flow (a code state machine) instead of a data state machine.

A `push` iterator can have a big set of map/reduce operations similar to a `pull` iterator. Including concatenation, a flat map (SelectMany), filters, reduce, groupBy etc.

In the same time, a `push` iterator has some limitations that a `pull` iterator doesn't. It's hard (or even impossible) to run two `push` iterators in the same time. Sometimes it's required for such operations as `zip merge`, `equal`, `compare` etc. However serialization/deserialization and reading/writing to IO (file, network, HTTP etc) don't require comparisons of emitters, usually.

It's possible to convert from an iterator to an emitter. It's a function which accepts an iterator and an action. and applies this action for all items in the iterator. Usually, this function is called `for_each`.

An emitter is a similar to a cold observable collection in reactive programming (Rx). The main difference is that an emitter is using an action instead of a subscriber.

```c
// (SVO) subject verb object // https://en.wikipedia.org/wiki/Subject–verb–object
az_result az_http_request__emit__spans(az_http_request, span_action);
           _emit_span_seq // good one because it avoids plural forms.
                          // it describes what the function does.
           _emit_span_list // list is more specific than a sequence (seq).
           _push_span_seq // an action arg is not necessary a container.
           _append_span_seq // an action arg is not necessary a container.
           _emitter // bad for C name constructions by conventions.
                    // However we may use `_something_emitter` as a synonym for `..._emit_something_seq_action`
           _observable // too long,
                       // bad for C name constructions by conventions, Rx uses events which are slightly different
           _to_span_emitter // doesn't describe what the function does. It doesn't create an emitter, it emits.
```

### One Way Street

Iterators (both 'pull' and 'push') allow to construct a program as an immutable [data flow](https://en.wikipedia.org/wiki/Dataflow) which 
usually doesn't require big intermidiate storages/buffers. Also, the amount of interfaces can be reduces, for example, an interface for 
a JSON parser should be compatable with a JSON builder. A JSON parser output is an input for a JSON builder. And a JSON builder output 
(usially it's a byte span iterator) is an input for a JSON parser.
