## GDB CH32V003

```
set architecture riscv:rv32
target extended-remote /dev/ttyACM0
load main.elf 0
```

## tasks V1->V2

- [x] C6 to 0603 or type-a tantal
- [x] c18 - 0402
- [x] vt3 (ap2301b) wrong case, sot23-6 instead of sot23-3
- [x] power switch latch change to ch32v003
	- [x] PD3 - enable (to base vt2)
	- [x] PD4 - button
	- [x] PC1 - I2C-SDA (25 pin hlk-7688)
	- [x] PC2 - I2C-SCL (24 pin hlk-7688)
	- [x] PA2 - voltage divider 470k/470k to battery (before tp4056)
	- [x] PC6 - WS2812
	- [x] PD1 - SWD (SWIO)
	- [x] wwan/wpan/wlan led - to one pin
	- [x] power tp4056 - one pin
	- [x] charge tp4056 - one pin
- [x] sim card set connector with loading from top, not from side
- [x] add 10k pull-up for sd-card (@see link-it 7688 schematic)
- [x] add zero resistor to avoid dw01


## Notes

```
socat /dev/ttyUSB2,b115200,raw,echo=0,crnl -

```

# Asterisks termination 
## quectel.conf

```
[general]  
  
  
[ec25_modem]  
device=/dev/ttyUSB1      ; or whichever your voice port is  
context=from-ec25        ; context for incoming calls from EC25  
audio=/dev/ttyUSB1       ; in some builds you might configure separate audio channels  
data=/dev/ttyUSB2        ; optional if used for data  
autostart=yes  
```

## PJSIP.conf

```
[transport-udp]  
type=transport  
protocol=udp  
bind=0.0.0.0  
  
  
[200]  
type=endpoint  
transport=transport-udp  
context=zoiper_in  
disallow=all  
allow=ulaw  
auth=200  
aors=200  
message_context=outgoing-sms  
  
[200]  
type=auth  
auth_type=userpass  
username=200  
password=200  
  
[200]  
type=aor  
max_contacts=1  
```

## extension.conf

```
[zoiper_in]  
; When Zoiper dials an external number, send it out through the EC25  
exten => _X.,1,NoOp("Outgoing call from Zoiper to EC25")  
same => n,Dial(Quectel/ec25_modem/${EXTEN},60)  
same => n,Hangup()  
  
[outgoing-sms]  
exten => _X!,1,NoOp(Outgoing SIP MESSAGE to send as SMS)  
same => n,Verbose(1,"Outbound SMS to: ${EXTEN} -> ${MESSAGE(body)}")  
same => n,QuectelSendSMS(ec25_modem,${EXTEN},${MESSAGE(body)},1440,no,"magicID")  
same => n,NoOp("SMS send status: ${QUECTELSENDSMS_STATUS}")  
same => n,Hangup()  
  
[from-ec25]  
; For calls that come in from the modem  
exten => s,1,NoOp("Incoming call from EC25")  
same => n,Verbose(2, "Incoming dial ${CALLERID(num)})  
same => n,Dial(PJSIP/200)  
same => n,Hangup()  
  
exten => sms,1,NoOp(Inbound SMS Received)  
same => n,Set(MESSAGE(body)=${SMS})  
same => n,Set(MESSAGE(from)=sip:${CALLERID(num)}@domain.invalid)  
same => n,Set(MESSAGE(to)=sip:200@domain.invalid)    
same => n,MessageSend(pjsip:200/200,${CALLERID(num)})  
same => n,Verbose(2, "Send status is ${SMS} ${CALLERID(num)})  
same => n,HangUp()
```

### modules.conf

root@OpenWrt:~# cat /etc/asterisk/modules.conf    
```
[modules]  
autoload=yes  
  
noload = chan_alsa.so  
noload = res_hep.so  
noload = res_hep_pjsip.so  
noload = res_hep_rtcp.so  
noload = chan_sip.so  
noload = app_voicemail_imap.so  
noload = app_voicemail_odbc.so  
  
load => chan_quectel.so
```


----

# PMIC register map

| #      | type   | name      | Description                                                                                                                                                 |
| ------ | ------ | --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 4..7   | uint32 | tm        | Internal time in ms                                                                                                                                         |
| 8..11  | uint32 | led-color | Led Color GRB,  if data\[11\] > 0 then update_led()                                                                                                         |
| 12..13 | uint16 | adc-val   | Battery value                                                                                                                                               |
| 14     | uint8  | in-state  | Bits:<br>0 - TP4056 - Charge<br>1 - TP4056 - Standby<br>2 - LTE leds state (wwan/wpan/wlan)<br>3 - Power button state<br>4 - Battery low indication (~3.5v) |
| 31     | uint8  | shutdown  | if write 0xff -> then shutdown                                                                                                                              |

----------

# I2C shortcuts

```
i2cdetect -y 0 # PMIC should be visible as device with address 9
i2cget -y 0 9 0 i 32 # get all pmic registers

i2cset -y 0 9 8 0xff b # set red led to 0xff
i2cset -y 0 9 11 0xff b # apply led color

i2cset -y 0 9 8 0x10 0x20 0x30 0x01 i  # update the color of the led in one command
i2cset -y 0 9 8 0x0 0x0 0x0 0x01 i  # update the color of the led in one command

i2cset -y 0 9 31 0xff b  # shutdown the device
```


-----

# Wireless config

```
config wifi-device 'radio0'      
       option type 'mac80211'  
       option path 'platform/10300000.wmac'  
       option band '2g'     
       option channel '1'  
       option htmode 'HT20'  
       option disabled '0'  
  
config wifi-iface 'default_radio0'  
       option device 'radio0'  
       option network 'lan'  
       option mode 'ap'  
       option ssid 'OpenWrt'  
       option encryption 'none'
```

# Control LTE module power

```
echo 'out' > /sys/class/gpio/lte_sw/direction
echo 1 > /sys/class/gpio/lte_sw/value # enable
echo 0 > /sys/class/gpio/lte_sw/value # disable
```

## EC200 workaround

To set right modem mode to use it as WWAN:


```
echo "at+qnetdevctl=3,1,1" | socat - /dev/ttyUSB2,crlf
```

# PMICCTRL UBUS  shortcuts

```bash
ubus listen
ubus call pmic set_led '{"r":128, "g":0, "b": 16}'
ubus call pmic shutdown
```

## BlockD

To mount flash use next thing:

```bash
block detect | uci import fstab
uci commit
```

## Enable wifi

```
uci set wireless.radio0.disabled=0
```

## Change EC25 USB mode

```
at+qcfg="usbnet",1
```

/etc/config/network

```
config interface 'wwan'  
       option proto 'dhcp'  
       option ifname 'eth1'
```

## PCB V2->V3

- [ ] Ethernet led connect to VCC instead of GND
- [ ] Connect modem thru the diode to the VBAT (after Mosfet)
- [ ] Change bc847 to bss138 and add to the Gate-GND resistor 100k
- [ ] separate resistors to CC1 / CC2  (for each pin own - 5.1k)

## Wireguard config

/etc/config/firewall:

```
  
config zone  
       option name 'wg'  
       list network 'wg0'  
       option input 'ACCEPT'  
       option output 'ACCEPT'  
       option forward 'REJECT'  
  
config rule  
       option name 'Allow-SSH-from-WG'  
       option src 'wg'  
       option target 'ACCEPT'  
       option dest_port '22'  
       option proto 'tcp'  
  
config rule  
       option name             Allow-asterisk  
       option src              'wg'  
       option proto            udp  
       option dest_port        5060  
       option target           ACCEPT  
       option family           ipv4
```


/etc/config/network

```
config interface 'wg0'  
       option proto 'wireguard'  
       option private_key 'BASE64='  
       option listen_port '51820'  
       list addresses 'VPN-IP'  
  
config wireguard_wg0  
       option public_key 'BASE64='  
       option allowed_ips 'VPN-SUBNET/24'  
       option endpoint_host 'SRV-IP'  
       option endpoint_port 'SRV-PORT'  
       option persistent_keepalive '25'
```