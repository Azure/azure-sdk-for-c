# Actions and Emitters

Weinberg's Second Law: If builders built buildings the way programmers wrote programs, 
then the first woodpecker that came along would destroy civilization. 

## Actions

An action is similar to C# generic type `Action<T>`. It contains a pointer to a function `func` and a context `self`.

It's also known as 
- an actor,
- a reactor,
- a callback (in some C programms),
- a functor (in C++),
- an observer and a subscriber (in Reactive programming),
- a closure (in some high-level languages),
- a delegate and an event.

```c
struct az_span_action {
  az_result (*func)(az_span); // fn (less known)
  void * self;
};

az_result az_span_action_do(az_span_action const action, az_span const arg) {
  AZ_CONTRACT_ARG_NOT_NULL(action.func);

  return action.func(action.self, arg);
}
```

The [self](https://en.wikipedia.org/wiki/This_%28computer_programming%29) field points on actor data. 
This data is also known as 
- `this` in C++, C#,
- `context`,
- `me` in VB :-).

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
az_result az_http_request__emit__span_seq(az_http_request, span_action);
```

### One Way Street

Iterators (both 'pull' and 'push') allow to construct a program as an immutable [data flow](https://en.wikipedia.org/wiki/Dataflow) which 
usually doesn't require big intermidiate storages/buffers. Also, the amount of interfaces can be reduces, for example, an interface for 
a JSON parser should be compatable with a JSON builder. A JSON parser output is an input for a JSON builder. And a JSON builder output 
(usially it's a byte span iterator) is an input for a JSON parser.

### Disciminated Unions

C doesn't have discriminated unions. There are two main options to implement discriminated unions. The first one is to use conventions. For example,

```c
typedef enum {
  FOO = 0,
  BAR = 1,
} foo_or_bar_kind;

typedef struct {
  foo_or_bar_kind kind;
  union {
    char const * foo;
	bool bar;
  };
} foo_or_bar;
```

A user must access the `foo` field only if `kind` is `FOO` and access the `bar` field only if `kind` is `BAR`. 
Otherwise, we may have an undefined behaviour which can lead to all sort of very bad bugs (dangling pointer, 
memory leak, security issues etc).

The second option is to use a visitor pattern (simplified).

```c
typedef void (*foo_action)(int);
typedef void (*bar_action)(bool);
typedef void (*foo_or_bar)(foo_action, bar_action);
```

The second options looks a little bit more complicated but it eliminates a class of bugs when `kind` is not respected. 

The visitor pattern can be represented using actions. 

```c
struct {
  str_action foo;
  bool_action bar;
} foo_or_bar_visitor;

struct {
  az_result (* func)(void *, foo_or_bar_visitor const *);
  void * self;
} foo_or_bar;

az_result foo_or_bar_do(foo_or_bar const self, foo_or_bar_visitor const * p_visitor) {
	AZ_CONTRACT_ARG_NOT_NULL(self.func);

	return self.func(self.self, p_visitor);
}
```
