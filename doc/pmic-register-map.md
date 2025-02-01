# PMIC register map

| #     | type   | name      | Description                                         |
| ----- | ------ | --------- | --------------------------------------------------- |
| 4..7  | uint32 | tm        | Internal time                                       |
| 8..11 | uint32 | led-color | Led Color RGB,  if data\[11\] > 0 then update_led() |
| 31    | uint8  | shutdown  | if write 0xff -> then shutdown                      |
