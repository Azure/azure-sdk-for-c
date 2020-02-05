


typedef AZ_NODISCARD az_result (*_az_set_scopes)(void * credential, az_span scopes);

typedef struct {
  _az_apply_credential apply_credential;
  _az_set_scopes set_scopes; // NULL if this credential doesn't support scopes.
} _az_credential_vtbl;