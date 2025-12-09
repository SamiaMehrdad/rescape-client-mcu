# Re‑Escape Room Server OS Baseline (Ubuntu) – v0.1

## 0. Goals

- Same **pattern** everywhere, easy to automate.
- Only **central tech team** has OS access.
- Franchisees only see the **Operator Web UI**.
- Safe base for **remote SSH, updates, and debugging**.

---

## 1. Machine Identity & Naming

**Hostname pattern:**

```
re-<country>-<cityCode><venueIndex>-r<roomIndex>
```

**Examples:**

```
re-us-sj01-r01
re-us-sj01-r02
re-us-la02-r01
```

Set hostname:

```
sudo hostnamectl set-hostname re-us-sj01-r01
```

Optional config:

```
echo "ROOM_ID=re-us-sj01-r01" | sudo tee -a /etc/reescape.env
```

---

## 2. User Accounts

### 2.1 `roomadmin` – human admin

```
sudo adduser roomadmin
sudo usermod -aG sudo roomadmin
```

### 2.2 `roomsvc` – service account (no SSH)

```
sudo adduser --system --home /opt/reescape --shell /usr/sbin/nologin roomsvc
```

---

## 3. SSH Setup & Hardening

### Install SSH

```
sudo apt update
sudo apt install -y openssh-server
```

### Add admin SSH key

```
sudo mkdir -p /home/roomadmin/.ssh
sudo nano /home/roomadmin/.ssh/authorized_keys
sudo chown -R roomadmin:roomadmin /home/roomadmin/.ssh
sudo chmod 700 /home/roomadmin/.ssh
sudo chmod 600 /home/roomadmin/.ssh/authorized_keys
```

### Harden SSH

Create:

```
sudo nano /etc/ssh/sshd_config.d/reescape.conf
```

Add:

```
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
AllowUsers roomadmin
X11Forwarding no
```

Restart:

```
sudo systemctl restart ssh
```

---

## 4. Firewall (`ufw`)

```
sudo apt install -y ufw
sudo ufw allow from 192.168.1.0/24 to any port 22 proto tcp
sudo ufw allow from 192.168.1.0/24 to any port 8080 proto tcp
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw enable
```

---

## 5. Systemd Service for Room Engine

Example service file:

```
sudo nano /etc/systemd/system/reescape-room.service
```

```
[Unit]
Description=Re-Escape Room Engine
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=roomsvc
Group=roomsvc
WorkingDirectory=/opt/reescape
EnvironmentFile=/etc/reescape.env
ExecStart=/opt/reescape/bin/room-engine
Restart=on-failure
RestartSec=5
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

[Install]
WantedBy=multi-user.target
```

Enable:

```
sudo systemctl daemon-reload
sudo systemctl enable reescape-room.service
sudo systemctl start reescape-room.service
```

---

## 6. Remote Update Pattern (simple)

```
ssh roomadmin@re-us-sj01-r01
cd /opt/reescape/app
sudo -u roomsvc git pull
sudo systemctl restart reescape-room.service
```

---

## 7. Operator Web UI Exposure

- Expose UI on LAN port (e.g., 8080)
- Accessible at:

```
http://re-us-sj01-r01:8080/operator
```

---

## 8. New Room Server Checklist

1. Install Ubuntu Server.
2. Set hostname.
3. Create `roomadmin` + `roomsvc`.
4. Add SSH keys.
5. Harden SSH.
6. Configure firewall.
7. Install app to `/opt/reescape`.
8. Add `/etc/reescape.env`.
9. Install systemd service.
10. Verify SSH, UI, and service auto-start.

---

**End of document**
