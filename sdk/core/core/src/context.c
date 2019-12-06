#include <stddef.h>

const long _az_context_max_expiration = 0x7FFFFFFFFFFFFFFF;

// An az_context value in a node in a tree that represents expiration times and key/value pairs
// The root node in the tree (ultimate parent) is az_context_app which is a context for the entire
// application. Each new node is a child of some parent.
typedef struct az_context {
  az_context * parent; // Pointer to parent context (or NULL); immutable after creation
  long expiration; // Time in milliseconds from Jan 1st, 1970 when context expires
  void *key, *value; // Pointers to the key & value (usually NULL)
} az_context;

// Creates a new child az_context node by specifying its parent and the absolute time (in ms since
// 1/1/1970) when this node expires.
az_context az_context_with_expiration(az_context * parent, long expiration) {
  return (az_context){ .parent = parent, .expiration = expiration };
}

// Creates a new child az_context node by specifying its parent and its key/value pairs.
az_context az_context_with_value(az_context * parent, void * key, void * value) {
  return (az_context){
    .parent = parent, .expiration = _az_context_max_expiration, .key = key, .value = value
  };
}

// Cancels an az_context node in the tree; this effectively cancels all the child nodes as well.
void az_context_cancel(az_context * context) {
  context->expiration = 0; // Jan 1st, 1970; the beginning of time
}

// Returns non-0 if this az_context node or any of its parent nodes' expiration is before the
// current time.
int az_context_has_expired(az_context * context) {
  return az_context_get_expiration(context) < /* TODO: get current time in ms from 1/1/1970 */ 0;
}

// Returns the soonest expiration time of this az_context node or any of its parent nodes.
long az_context_get_expiration(az_context * context) {
  long expiration = _az_context_max_expiration;
  for (; context != NULL; context = context->parent) {
    if (context->expiration < expiration)
      expiration = context->expiration;
  }
  return expiration;
}

// Walks up this az_context node's parent until it find a node whose key matches the specified key
// and return the corresponding value. Returns AZ_NOT_FOUND is there are no nodes matching the
// specified key.
az_result az_context_get_value(az_context * context, void * key, void ** value) {
  for (; context != NULL; context = context->parent) {
    if (context->key == key) {
      *value = context->value;
      return AZ_OK;
    }
    value = NULL;
    return AZ_NOT_FOUND;
  }

  // This is a global az_context node representing the entire application. By default, this node
  // never expires. Call az_context_cancel passing a pointer to this node to cancel the entire
  // application (which cancels all the child nodes).
  az_context az_context_app
      = {.parent = NULL,
         .expiration = _az_context_max_expiration,
         .key = NULL,
         .value = NULL }

  void
  func() {
    void *key = "k", *value = "v";
    az_context ctx1 = az_context_with_expiration(&az_context_app, 100);
    az_context ctx2 = az_context_with_value(&ctx1, key, value);
    az_context ctx3 = az_context_with_expiration(&ctx2, 250);

    long expiration = az_context_get_expiration(&ctx3);
    void * value2;
    az_result r = az_context_get_value(&ctx3, key, &value2);
    if (r == AZ_OK) {
      // Use value2
    } else {
      // key not found; do NOT use value 2
    }

    az_context_cancel(&ctx1);
    expiration = az_context_get_expiration(&ctx3); // Should be 0
  }
