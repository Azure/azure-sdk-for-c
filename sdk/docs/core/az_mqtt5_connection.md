# MQTT Connection Management

## Connection HFSM

The following diagram describes the hierarchical finite state machine implementing connection management with the following functionality:

1. Credential rotation
1. Retry-policy based back-off timer
1. Robust and reliable event processing:
    1. State resyncrhonization in case of dropped/missed events.
    1. Reliable execution for any type of event (unexpected/unknown events result in maintaining the Faulted state to avoid further state corruption).

![event_pipeline.puml](https://www.plantuml.com/plantuml/png/nLPVJyCs47_FfxWtcb4AsxGNO1LL6qIrjO4sJ6ph6fdaA1QfNTaE1KryzzaE6KbZoBezZ0SQd-_kT_UxFyc5L4IeV9t1a3E6YQASnLUNyudd7S10odozCI9vgeydFRXbJunl6U0pCYM128G7BhP4f9X2Xp9GZqW5S45Nb94CKYeAdrr0gky4K7RFSvPg5U2viiz-Jpl_jWUJcoYAX-5q6WoZU1RS09MmoeaWJA5shpW8b1kkZRVQL-cmRZM_ZiEVW3l4mMncSBxhmJBu9PX6nZi1-SY5Id9blECBwYSqEO3Ig9AQo858oHDA5BxZ5jOFWg-jPf8BWKo1L5e5Eem3ILHEie8ZWoBrhqdMvlbzpvpJ-9s7Rd4ggR2Cvj9cQs-0CatmcjX85BU8PBC1egyOgYqNJvOLm_8pPgyzpxE1LCev8OoiS6s2-DC3WFT8nsc6zcWUu1JEx_k3tsARN6p8-w5aRnnEu-7FuJmmDJa_lkyV0rqnBl1inznIFXhFAwpNrIleboJFDADLqLmHeKKsoEAldWbRbHSLKO6CJ8dN0aigljaIbJp60YMgZl-vqGjLkpOhLB6YQtIQjMIvr1qMpZntz9ZkEiIq4RjJPQ50O588ueJdJA7uyMGNPbQgH3fwdglokd7X17LhMcox7y1m8beq6r4U7hPOv9kKA8mJWIaoHKacEtwB2Ns0JWFe0Fem6TpzFUw3A0ecrz7SXwMjLgMHPq4q-pKExgu6j_Ce6CNZwQW5yQlhHXUr4xNFc2qTlRPR-16LIMD0wj96oIir1RT9UsukVSSgjkRr7TTfP1NS-6wt_sejVJ3q-pT_m56_PO4SVMXW-dKU2rRRtHs7dk-4w7_I2QwNb47wBCx_GCw2xL652Y7_7-m_txmBLYlBE2n8hKwyddF_p1sCAv9clHpnHED94Dv6xZpThj_iV_kcQ-v_vvwlwhSm7ru7qyOmDEmTJTk5sXkitKJhxTXuvRpfmkryMcZ1lWdimMZfkiJzkbMqqqQhD7fNApTybnyirhjDkGlL-jVTMWNVU_Ckj3HVPzvV)