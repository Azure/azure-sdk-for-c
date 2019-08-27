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

- [ ] State machine by expected value.

  ```c
  BEFORE_VALUE   : EXPECT_WS_OR_VALUE = 0
  ERROR          :                    = 1
  AFTER_VALUE    : EXPECT_WS_OR_END
  END            : EXPECT_NOTHING
  VALUE_DONE     :
    NULL_
    FALSE_
    TRUE_
    ARRAY_OPEN     : EXPECT_WS_OR_ARRAY_CLOSE_OR_VALUE
    ARRAY_ITEM     : EXPECT_WS_OR_ARRAY_CLOSE_OR_COMMA
    OBJECT_OPEN    : EXPECT_WS_OR_OBJECT_CLOSE_OR_STRING
    OBJECT_PROPERTY: EXPECT_WS_OR_COLON
    OBJECT_ EXPECT_WS_OR_OBJECT_CLOSE_OR_COMMA

  NULL_N
  NULL_NU
  NULL_NUL

  FALSE_F
  FALSE_FA
  FALSE_FAL
  FALSE_FALS

  TRUE_T
  TRUE_TR
  TRUE_TRU

  STRING_CHAR
  STRING_ESC
  STRING_U

  NUMBER_MINUS
  NUMBER_ZERO
  NUMBER_INT
  NUMBER_DOT
  NUMBER_FRACTION
  NUMBER_E
  NUMBER_E_SIGN
  NUMBER_E_DIGIT
  ```
