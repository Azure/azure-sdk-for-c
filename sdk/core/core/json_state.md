```c
//                          | prefix |
// - info_tokens    - 0..7  | 000    |
//   - space_before_value   |        |  000
//   - space_before_name    |        |  001
//   - space_after_value    |        |  010
//   - space_after_name     |        |  011
//   - space_open           |        |  100
//   - comma                |        |  101
//   - colon                |        |  110
//                          |        |  111 x 1
// - number_tokens  - 0..7  | 001    |
//   - minus                |        |  000
//   - zero                 |        |  001
//   - digit                |        |  010
//   - dot                  |        |  011
//   - fraction             |        |  100
//   - e                    |        |  101
//   - e_sign               |        |  110
//   - e_digit              |        |  111
//
// - name_tokens    - 0..7  | 010    |
//   - ...                  |        |  ...
//
// - string_tokens  - 0..7  | 011    |
//   - char                 |        |  000
//   - esc                  |        |  001
//   - esc_char             |        |  010
//   - esc_u                |        |  011
//   - esc_u0               |        |  100
//   - esc_u1               |        |  101
//   - esc_u2               |        |  110
//   - esc_u3               |        |  111
//
// - keyword_tokens - 0..15 | 10     |
//   - keyword_bool         | 100
//     - false              |        |  0XX
//     - true               |        |  1XX
//   - other keywords
//     - null               | 101    |  0XX
//                          |        |  1XX x 4
//
// - value tokens   - 0..15 | 11     |
//
//   - name_open            |        | 0000
//   - string_open          |        | 0001
//
//   - object_open          |        | 0010
//   - array_open           |        | 0011
//                          |        |
//   - object               |        | 0100
//   - array                |        | 0101
//
//   - name                 |        | 0110
//   - string               |        | 0111
//
//   - number               |        | 10XX
//
//   - false                |        | 1100
//   - true                 |        | 1101
//
//   - null                 |        | 1110
//   - error                |        | 1111
```
