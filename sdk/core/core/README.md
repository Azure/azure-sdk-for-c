# Azure SDK Core Library

## Issues

- [ ] JSON number reader/writer should produce the same numbers as C library.
- [ ] Token enum order should be:

  ```c
  typedef enum {
    AZ_JSON_..._NONE  = 0,
    AZ_JSON_..._ERROR = 1,
    AZ_JSON_..._DONE  = 2,
    AZ_JSON_..._...
  } az_json_..._tag;
  ```

- [ ] Serializer driven parser. The main problem is that the JSON object properties are not ordered.
