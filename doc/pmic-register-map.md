# PMIC register map

| #      | type   | name      | Description                                                                                                                                                 |
| ------ | ------ | --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 4..7   | uint32 | tm        | Internal time in ms                                                                                                                                         |
| 8..11  | uint32 | led-color | Led Color RGB,  if data\[11\] > 0 then update_led()                                                                                                         |
| 12..13 | uint16 | adc-val   | Battery value                                                                                                                                               |
| 14     | uint8  | in-state  | Bits:<br>0 - TP4056 - Charge<br>1 - TP4056 - Standby<br>2 - LTE leds state (wwan/wpan/wlan)<br>3 - Power button state<br>4 - Battery low indication (~3.5v) |
| 31     | uint8  | shutdown  | if write 0xff -> then shutdown                                                                                                                              |
