# White box DIY LTE router

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/dev.jpg)

## 1. Project Description

This project provides a detailed guide to building a DIY battery-powered LTE router, designed with portability and flexibility in mind. The router features two Ethernet ports—one dedicated WAN port for external network connectivity and one LAN port to connect local devices. With built-in LTE connectivity, it ensures reliable internet access even in remote or mobile environments.

The DIY LTE router supports multiple functionalities tailored to diverse applications, such as LTE voice calls and SMS termination through VoIP, secured VPN connectivity, and integrated GPS tracking capabilities. These use cases and their configurations are explained in detail in the subsequent sections of this guide.

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/inside.jpg)

## 2. Use cases

### 2.1 LTE Voice Calls and SMS Termination via VOIP

The DIY LTE router integrates seamlessly with LTE modems (e.g., Quectel EC25) using the Asterisk VoIP server to enable voice calls and SMS functionality. Calls and SMS messages can be managed using SIP clients (e.g., Zoiper) directly over an IP network.

#### Voice Calls

- **Outgoing Calls:**
    
    - Calls initiated from the SIP client are routed through the LTE modem. The call handling logic is provided in `openwrt/extensions.conf` under the `[zoiper_in]` context.
        
    - Example:
        
        ```
        exten => _X.,1,NoOp("Outgoing call from Zoiper to EC25")
        same => n,Dial(Quectel/ec25_modem/${EXTEN},60)
        same => n,Hangup()
        ```
        
- **Incoming Calls:**
    
    - Calls received by the LTE modem are routed to the configured SIP client.
        
    - Defined in `openwrt/quectel.conf` under `[ec25_modem]` and routed through context `[from-ec25]`:
        
        ```
        exten => s,1,NoOp("Incoming call from EC25")
        same => n,Dial(PJSIP/200)
        same => n,Hangup()
        ```
        

#### SMS Handling

- **Outgoing SMS:**
    
    - SMS messages can be sent directly from the SIP client using SIP MESSAGE requests.
        
    - Defined under context `[outgoing-sms]`:
        
        ```
        exten => _X!,1,NoOp(Outgoing SIP MESSAGE to send as SMS)
        same => n,QuectelSendSMS(ec25_modem,${EXTEN},${MESSAGE(body)},1440,no,"magicID")
        same => n,Hangup()
        ```
        
- **Incoming SMS:**
    
    - Incoming SMS messages received by the LTE modem are forwarded to the SIP client.
        
    - Managed under `[from-ec25]`:
        
        ```
        exten => sms,1,NoOp(Inbound SMS Received)
        same => n,Set(MESSAGE(body)=${SMS})
        same => n,MessageSend(pjsip:200/200,${CALLERID(num)})
        same => n,HangUp()
        ```
        

#### Configuration and Permissions

- Configuration files (`extensions.conf`, `PJSIP.conf`, `quectel.conf`, and `modules.conf`) are available in the project's `openwrt` folder.
    
- A SIP client (e.g., Zoiper) should connect to the endpoint defined in `openwrt/PJSIP.conf`.
    
- Basic Asterisk configuration (`/etc/config/asterisk`):
    

```conf
config asterisk 'general'  
       option enabled '1'  
       option log_stderr '1'  
       option log_stdout '0'  
       option options ''
```

- **Permissions Issues:** 

If there are permission errors accessing `ttyUSB` ports, modify `/etc/init.d/asterisk` as follows: (remove flag -U from command)


```bash
#!/bin/sh /etc/rc.common  
# Copyright (C) 2014 OpenWrt.org  
  
START=99  
  
USE_PROCD=1  
  
#PROCD_DEBUG=1  
  
NAME=asterisk  
COMMAND=/usr/sbin/$NAME  
  
LOGGER="/usr/bin/logger -p daemon.err -s -t $NAME --"  
  
start_service() {  
  
 dbdir=/var/lib/asterisk/astdb  
 logdir=/var/log/asterisk  
 cdrcsvdir=$logdir/cdr-csv  
 rundir=/var/run/asterisk  
 spooldir=/var/spool/asterisk  
 varlibdir=/var/lib/asterisk  
  
 config_load $NAME  
  
 config_get_bool enabled general enabled 0  
 if [ $enabled -eq 0 ]; then  
   $LOGGER service not enabled in /etc/config/$NAME  
   return 1  
 fi  
  
 config_get_bool log_stderr general log_stderr 1  
 config_get_bool log_stdout general log_stdout 0  
  
 config_get options general options  
  
 for i in \  
   "$logdir" \  
   "$cdrcsvdir" \  
   "$rundir" \  
   "$spooldir" \  
   "$varlibdir" \  
   "$dbdir"  
 do  
   if ! [ -e "$i" ]; then  
     mkdir -m 0750 -p "$i"  
     [ -d "$i" ] && chown $NAME:$NAME "$i"  
   fi  
 done  
  
 procd_open_instance  
 procd_set_param command $COMMAND  
 procd_append_param command \  
   $options \  
   -f  
 # forward stderr to logd  
 procd_set_param stderr $log_stderr  
 # same for stdout  
 procd_set_param stdout $log_stdout  
 procd_close_instance  
  
}  
  
reload_service() {  
 procd_send_signal $NAME  
}
```

Configuration files (`extensions.conf`, `PJSIP.conf`, `quectel.conf`, `modules.conf`) are located in the project's `openwrt` folder and should copied to the `/etc/asterisk` folder on the device. 

### 2.2 Secured VPN Gateway

The DIY LTE router supports secure VPN connections using WireGuard and OpenVPN, configured via OpenWrt's command-line interface (CLI). These VPN configurations provide secure, encrypted tunnels for safe internet browsing and remote network access.

#### 2.2.1 WireGuard Setup via UCI

- WireGuard configuration (`/etc/config/network`):
    
    ```
    config interface 'wg0'
      option proto 'wireguard'
      option private_key '<private_key_here>'
      option listen_port '51820'
      list addresses '<VPN-IP>'
    
    config wireguard_wg0
      option public_key '<peer-public-key>'
      option allowed_ips '<VPN_SUBNET>'
      option endpoint '<VPN_SERVER_IP:PORT>'
    ```
    
- Firewall rules in `/etc/config/firewall`:
    
    ```
    config zone
      option name 'wg'
      list network 'wg0'
      option input 'ACCEPT'
      option output 'ACCEPT'
      option forward 'REJECT'
    ```
    

#### 2.2.2 OpenVPN Setup via CLI

- Place your `.ovpn` file into `/etc/openvpn/`.
    
- Edit `/etc/config/openvpn`:
    
    ```
    config openvpn 'vpnclient'
      option enabled '1'
      option config '/etc/openvpn/<your-config>.ovpn'
    ```
    
- Adjust firewall settings in `/etc/config/firewall`:
    
    ```
    config zone
      option name 'vpn'
      option input 'REJECT'
      option output 'ACCEPT'
      option forward 'REJECT'
      list network 'vpnclient'
    
    config forwarding
      option src 'lan'
      option dest 'vpn'
    ```
    

#### 2.2.3 LTE Modem MBIM Configuration and Routing

- Reconfigure Quectel EC25 modem to use MBIM mode:
    
    ```
    echo 'at+qcfg="usbnet",1' | socat - /dev/ttyUSB2,crlf
    ```
    
- Configure network interface in `/etc/config/network`:
    ```
    config interface 'wwan'
      option proto 'dhcp'
      option ifname 'eth1'
    ```
- Firewall configuration in `/etc/config/firewall`:
    ```
    config zone
      option name 'wwan'
      list network 'wwan'
      option input 'REJECT'
      option output 'ACCEPT'
      option forward 'REJECT'
      option masq '1'
    
    config forwarding
      option src 'lan'
      option dest 'wwan'
    ```

#### 2.2.4 Routing All Traffic through VPN via WWAN (`eth1`)

- Configure default routing to forward all traffic through VPN interface linked with the WWAN (`eth1`):
    ```
    uci set network.lan.gateway='<VPN_GATEWAY_IP>'
    uci set network.lan.dns='<VPN_DNS_IP>'
    uci commit network
    /etc/init.d/network restart
    ```
- Ensure firewall rules allow forwarding from LAN to VPN:
    ```
    config forwarding
      option src 'lan'
      option dest 'vpn'
    ```
This setup ensures that all network traffic from your local devices is securely tunneled through your VPN connection using the WWAN (`eth1`) interface.

### 2.3 Network Storage with MicroSD and NFS Server

The DIY LTE router includes onboard storage using a MicroSD card and features an integrated NFS-kernel-server for network file sharing.

#### Mounting the MicroSD Card

Use the following commands to detect and mount the MicroSD card automatically:

```
block detect | uci import fstab
uci commit
/etc/init.d/fstab enable
/etc/init.d/fstab start
```

Example UCI configuration (`/etc/config/fstab`):

```
config global
        option anon_swap '0'
        option anon_mount '0'
        option auto_swap '1'
        option auto_mount '1'
        option delay_root '5'
        option check_fs '0'

config mount
        option target '/mnt/mmcblk0p1'
        option uuid '<UUID_generated_by_block_detect>'
        option enabled '1'
```

Verify mounted storage with:

```
df -h
```

#### Configuring the NFS-kernel-server

- Configure exports in `/etc/exports`:
    

```
/mnt/mmcblk0p1  *(rw,sync,no_root_squash,no_subtree_check)
```

- Enable and start NFS server:
    

```
/etc/init.d/nfsd enable
/etc/init.d/nfsd start
```

- Adjust firewall rules to allow NFS:
    

```
uci add firewall rule
uci set firewall.@rule[-1].src='lan'
uci set firewall.@rule[-1].dest_port='2049'
uci set firewall.@rule[-1].proto='tcpudp'
uci set firewall.@rule[-1].target='ACCEPT'
uci commit firewall
/etc/init.d/firewall restart
```

This setup allows your router to function as a robust network-attached storage solution accessible to clients on your local network.

## 3. Assembly

### 3.1 Hardware

#### 3.1.1 STL files and box

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/case.png)

use files located in `3d/stl`. To modify it, use 'Freecad'

#### 3.1.2 PCB

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/pcb-top.png)

Use altium project `pcb/lte-router_V2_ARCHIVE/lte-router_V2_DIV/lte-router_V2.PrjPcb`

Schematic and PCB documentation available in PDF file here: [`pcb/lte-router_V2_ARCHIVE/lte-router_V2_GENERATED/DOC/lte-router_V2_DOC.PDF`](https://github.com/intx82/lte-router/blob/master/pcb/lte-router_V2_ARCHIVE/lte-router_V2_GENERATED/DOC/lte-router_V2_DOC.PDF)

In near future will need to fix:

**PCB V2->V3**

- [ ] Ethernet led connect to VCC instead of GND
- [ ] Connect modem thru the diode to the VBAT (after Mosfet)
- [ ] Change bc847 to bss138 and add to the Gate-GND resistor 100k (or change it together to the AP2003 double mosfet)
- [ ] separate resistors to CC1 / CC2  (for each pin own - 5.1k)


**Additional thanks to Michael who done this work**


### 3.2 Software

#### 3.2.1 Power Managment IC (pmic) firmware

For `PMIC` has been used 'CH32V003F4P6' Risc-V CPU

To build it, be sure that you have installed `riscv64-gnu-linux-gcc` and just run `make -C pmic/fw`.

#### 3.2.2 Openwrt 

- Use official 'Openwrt' repository and checkout to the '09e32bf62a7dc0f252605720910b02ed26994263' commit, follow the openwrt build instructions
- Then put `openwrt/config` file to the openwrt folder as .config
- After copy contents of `pmic/tool` to the `package/utils/pmicctrl/`
- Apply all patches from `openwrt` folder
- `make`
- Take a firmware from `bin/targets/ramips/mt76x8` 
- Upload it to the device using `scp` - `scp openwrt-snapshot-r28739+2-69890e16b3-ramips-mt76x8-hilink_hlk-7688a-squashfs-sysupgrade.bin root@192.168.1.1:/tmp` or if this is stock HLK-7688 - `scp openwrt-snapshot-r28466-6881b48dc6-ramips-mt76x8-hilink_hlk-7688a-squashfs-sysupgrade.bin root@192.168.16.254:/tmp`. Password to stock module Wi-Fi is '12345678'
- Flash it with `sysupgrade -n /tmp/openwrt-snapshot-r28739+2-69890e16b3-ramips-mt76x8-hilink_hlk-7688a-squashfs-sysupgrade.bin`

