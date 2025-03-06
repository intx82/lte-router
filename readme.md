# White box DIY LTE router

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/dev.jpg)

## 1. Project Description

This project provides a detailed guide to building a DIY battery-powered LTE router, designed with portability and flexibility in mind. The router features two Ethernet ports—one dedicated WAN port for external network connectivity and one LAN port to connect local devices. With built-in LTE connectivity, it ensures reliable internet access even in remote or mobile environments.

The DIY LTE router supports multiple functionalities tailored to diverse applications, such as LTE voice calls and SMS termination through VoIP, secured VPN connectivity, and integrated GPS tracking capabilities. These use cases and their configurations are explained in detail in the subsequent sections of this guide.

![Image](https://raw.githubusercontent.com/intx82/lte-router/master/img/inside.jpg)

## 2. Use cases
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

